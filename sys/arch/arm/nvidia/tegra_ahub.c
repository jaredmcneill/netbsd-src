/* $NetBSD$ */

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/kmem.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_ahubreg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

/* Hardware can do 10 channels, this driver currently only uses 4 */
#define	AHUB_NCHAN	4

/* Hardware requires addresses to be aligned to 32-bits */
#define AHUB_ALIGN	4

/* Bus space resources */
enum {
	AHUB_RES_APBIF,
	AHUB_RES_XBAR,
	AHUB_RES_APBIF2,
	AHUB_NRES
};

struct tegra_ahub_dma {
	LIST_ENTRY(tegra_ahub_dma) dma_list;
	bus_dmamap_t		dma_map;
	void			*dma_addr;
	size_t			dma_size;
	bus_dma_segment_t	dma_segs[1];
	int			dma_nsegs;
};

struct tegra_ahub_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh[AHUB_NRES];
	bus_dma_tag_t		sc_dmat;
	int			sc_phandle;
	kmutex_t		sc_lock;

	void			*sc_ih;

	struct clk		*sc_clk_d_audio;
	struct clk		*sc_clk_apbif;

	uint32_t		sc_chan_mask;

	LIST_HEAD(, tegra_ahub_dma) sc_dmalist[AHUB_NCHAN];
};

static int	tegra_ahub_match(device_t, cfdata_t, void *);
static void	tegra_ahub_attach(device_t, device_t, void *);

static int	tegra_ahub_init_clocks(struct tegra_ahub_softc *);
static int	tegra_ahub_intr(void *);

CFATTACH_DECL_NEW(tegra_ahub, sizeof(struct tegra_ahub_softc),
	tegra_ahub_match, tegra_ahub_attach, NULL, NULL);

#define	AHUB_READ(sc, res, reg)						\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh[(res)], (reg))
#define	AHUB_WRITE(sc, res, reg, val)					\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh[(res)], (reg), (val))

#define	APBIF_READ(sc, reg)						\
	AHUB_READ((sc), AHUB_RES_APBIF, (reg))
#define	APBIF_WRITE(sc, reg, val)					\
	AHUB_WRITE((sc), AHUB_RES_APBIF, (reg), (val))

#define	APBIF2_READ(sc, reg)						\
	AHUB_READ((sc), AHUB_RES_APBIF2, (reg))
#define	APBIF2_WRITE(sc, reg, val)					\
	AHUB_WRITE((sc), AHUB_RES_APBIF2, (reg), (val))

#define	XBAR_READ(sc, reg)						\
	AHUB_READ((sc), AHUB_RES_XBAR, (reg))
#define	XBAR_WRITE(sc, reg, val)					\
	AHUB_WRITE((sc), AHUB_RES_XBAR, (reg), (val))

static int
tegra_ahub_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "nvidia,tegra124-ahub", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_ahub_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_ahub_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr[AHUB_NRES];
	bus_size_t size[AHUB_NRES];
	char intrstr[128];
	int child, error;
	char *name;
	int len;
	u_int n;

	for (n = 0; n < AHUB_NRES; n++) {
		if (fdtbus_get_reg(phandle, n, &addr[n], &size[n]) != 0) {
			aprint_error(": couldn't get registers (+%d)\n", n);
			return;
		}
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_phandle = phandle;
	for (n = 0; n < AHUB_NRES; n++) {
		error = bus_space_map(sc->sc_bst, addr[n], size[n], 0,
		    &sc->sc_bsh[n]);
		if (error) {
			aprint_error(": couldn't map %#llx: %d",
			    (uint64_t)addr[n], error);
			return;
		}
	}
	for (n = 0; n < AHUB_NCHAN; n++)
		LIST_INIT(&sc->sc_dmalist[n]);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_chan_mask = 0;

	aprint_naive("\n");
	aprint_normal(": Audio Hub\n");

	error = tegra_ahub_init_clocks(sc);
	if (error) {
		aprint_error_dev(self, "failed to init clocks (%d)\n", error);
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_AUDIO,
	    FDT_INTR_MPSAFE, tegra_ahub_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;

		len = OF_getproplen(child, "name");
		if (len <= 0)
			continue;

		name = kmem_zalloc(len, KM_SLEEP);
		if (OF_getprop(child, "name", name, len) != len) {
			kmem_free(name, len);
			continue;
		}

		struct fdt_attach_args cfaa = *faa;
		cfaa.faa_name = name;
		cfaa.faa_phandle = child;
		cfaa.faa_quiet = 0;

		config_found(self, &cfaa, fdtbus_print);

		kmem_free(name, len);
	}
}

static int
tegra_ahub_init_clocks(struct tegra_ahub_softc *sc)
{
	const int phandle = sc->sc_phandle;
	struct fdtbus_reset *rst;
	int error;
	u_int n;

	/* Get resources */
	sc->sc_clk_d_audio = fdtbus_clock_get(phandle, "d_audio");
	if (sc->sc_clk_d_audio == NULL) {
		aprint_error_dev(sc->sc_dev, "couldn't get clock d_audio\n");
		return ENXIO;
	}
	sc->sc_clk_apbif = fdtbus_clock_get(phandle, "apbif");
	if (sc->sc_clk_apbif == NULL) {
		aprint_error_dev(sc->sc_dev, "couldn't get clock apbif\n");
		return ENXIO;
	}

	/* Enable audio crossbar and APBIF */
	error = clk_enable(sc->sc_clk_d_audio);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable clock d_audio\n");
		return error;
	}
	error = clk_enable(sc->sc_clk_apbif);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable clock apbif\n");
		return error;
	}

	/* De-assert all resets. */
	for (n = 0; (rst = fdtbus_reset_get_index(phandle, n)) != NULL; n++) {
		fdtbus_reset_deassert(rst);
		fdtbus_reset_put(rst);
	}

	return 0;
}

static int
tegra_ahub_intr(void *priv)
{
	return 1;
}

static int
tegra_ahub_allocdma(struct tegra_ahub_softc *sc, size_t size, size_t align,
    struct tegra_ahub_dma *dma)
{
	int error;

	dma->dma_size = size;
	error = bus_dmamem_alloc(sc->sc_dmat, dma->dma_size, align, 0,
	    dma->dma_segs, 1, &dma->dma_nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;

	error = bus_dmamem_map(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs,
	    dma->dma_size, &dma->dma_addr, BUS_DMA_WAITOK | BUS_DMA_COHERENT);
	if (error)
		goto free;

	error = bus_dmamap_create(sc->sc_dmat, dma->dma_size, dma->dma_nsegs,
	    dma->dma_size, 0, BUS_DMA_WAITOK, &dma->dma_map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(sc->sc_dmat, dma->dma_map, dma->dma_addr,
	    dma->dma_size, NULL, BUS_DMA_WAITOK);
	if (error)
		goto destroy;

	return 0;

destroy:
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
unmap:
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_addr, dma->dma_size);
free:
	bus_dmamem_free(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs);

	return error;
}

static void
tegra_ahub_freedma(struct tegra_ahub_softc *sc, struct tegra_ahub_dma *dma)
{
	bus_dmamap_unload(sc->sc_dmat, dma->dma_map);
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_addr, dma->dma_size);
	bus_dmamem_free(sc->sc_dmat, dma->dma_segs, dma->dma_nsegs);
}

static int
tegra_ahub_set_audiocif(struct tegra_ahub_softc *sc, u_int chan, int dir,
    const audio_params_t *params)
{
	bus_size_t reg = dir == AUMODE_PLAY ?
	    APBIF_AUDIOCIF_TXn_CTRL_REG(chan) :
	    APBIF_AUDIOCIF_RXn_CTRL_REG(chan);
	int cifdir = dir == AUMODE_PLAY ? AUDIOCIF_DIR_TX : AUDIOCIF_DIR_RX;
	uint32_t cif;
	int error;

	error = tegra_ahub_audiocif_encode(params, cifdir, &cif);
	if (error)
		return error;

	APBIF_WRITE(sc, reg, cif);

	return 0;
}

void * 
tegra_ahub_chan_allocm(device_t dev, u_int chan, size_t size)
{
	struct tegra_ahub_softc * const sc = device_private(dev);
	struct tegra_ahub_dma *dma;
	int error;

	dma = kmem_alloc(sizeof(*dma), KM_SLEEP);
	if (dma == NULL)
		return NULL;

	error = tegra_ahub_allocdma(sc, size, AHUB_ALIGN, dma);
	if (error) {
		kmem_free(dma, sizeof(*dma));
		device_printf(sc->sc_dev,
		    "couldn't allocate DMA memory for channel %u (%d)\n",
		    chan, error);
		return NULL;
	}

	LIST_INSERT_HEAD(&sc->sc_dmalist[chan], dma, dma_list);

	return dma->dma_addr;
}

void   
tegra_ahub_chan_freem(device_t dev, u_int chan, void *addr, size_t size)
{
	struct tegra_ahub_softc * const sc = device_private(dev);
	struct tegra_ahub_dma *dma;

	LIST_FOREACH(dma, &sc->sc_dmalist[chan], dma_list) {
		if (dma->dma_addr == addr) {
			tegra_ahub_freedma(sc, dma);
			LIST_REMOVE(dma, dma_list);
			kmem_free(dma, sizeof(*dma));
			break;
		}
	}
}

paddr_t
tegra_ahub_chan_mappage(device_t dev, u_int chan, void *addr, off_t off,
    int prot)
{
	struct tegra_ahub_softc * const sc = device_private(dev);
	struct tegra_ahub_dma *dma;

	if (off < 0)
		return -1;

	LIST_FOREACH(dma, &sc->sc_dmalist[chan], dma_list) {
		if (dma->dma_addr == addr)
			return bus_dmamem_mmap(sc->sc_dmat, dma->dma_segs,
			    dma->dma_nsegs, off, prot, BUS_DMA_WAITOK);
	}

	return -1;
}

bus_addr_t
tegra_ahub_chan_vtophys(device_t dev, u_int chan, void *addr)
{
	struct tegra_ahub_softc * const sc = device_private(dev);
	struct tegra_ahub_dma *dma;

	LIST_FOREACH(dma, &sc->sc_dmalist[chan], dma_list) {
		if (dma->dma_addr == addr)
			return dma->dma_map->dm_segs[0].ds_addr;
	}

	return -1;
}

int
tegra_ahub_chan_alloc(device_t dev, u_int *chan)
{
	struct tegra_ahub_softc * const sc = device_private(dev);
	u_int n;

	mutex_enter(&sc->sc_lock);
	for (n = 0; n < AHUB_NCHAN; n++) {
		const uint32_t mask = 1 << n;
		if ((sc->sc_chan_mask & mask) == 0) {
			*chan = n;
			sc->sc_chan_mask |= mask;

			APBIF_WRITE(sc, APBIF_CHANNELn_CTRL_REG(n),
			    APBIF_CHANNEL_CTRL_TX_PACK_EN |
			    APBIF_CHANNEL_CTRL_RX_PACK_EN |
			    __SHIFTIN(APBIF_CHANNEL_CTRL_PACK_16,
				      APBIF_CHANNEL_CTRL_TX_PACK) |
			    __SHIFTIN(APBIF_CHANNEL_CTRL_PACK_16,
				      APBIF_CHANNEL_CTRL_RX_PACK) |
			    __SHIFTIN(7, APBIF_CHANNEL_CTRL_TX_THRESHOLD) |
			    __SHIFTIN(7, APBIF_CHANNEL_CTRL_RX_THRESHOLD));

			break;
		}
	}
	mutex_exit(&sc->sc_lock);

	return n == AHUB_NCHAN ? EBUSY : 0;
}

void
tegra_ahub_chan_free(device_t dev, u_int chan)
{
	struct tegra_ahub_softc * const sc = device_private(dev);
	const uint32_t mask = 1 << chan;

	mutex_enter(&sc->sc_lock);
	KASSERT((sc->sc_chan_mask & mask) != 0);

	sc->sc_chan_mask &= ~mask;

	mutex_exit(&sc->sc_lock);
}

int
tegra_ahub_chan_trigger(device_t dev, u_int chan, int dir, void *start,
    void *end, int blksize, void (*intr)(void *), void *intrarg,
    const audio_params_t *params)
{
	struct tegra_ahub_softc * const sc = device_private(dev);
	const uint32_t enable_bit = dir == AUMODE_PLAY ?
	    APBIF_CHANNEL_CTRL_TX_ENABLE : APBIF_CHANNEL_CTRL_RX_ENABLE;
	uint32_t val;
	int error;

	error = tegra_ahub_set_audiocif(sc, chan, dir, params);
	if (error)
		return error;

	val = APBIF_READ(sc, APBIF_CHANNELn_CTRL_REG(chan));
	val |= enable_bit;
	APBIF_WRITE(sc, APBIF_CHANNELn_CTRL_REG(chan), val);

	return 0;
}

int
tegra_ahub_chan_halt(device_t dev, u_int chan, int dir)
{
	struct tegra_ahub_softc * const sc = device_private(dev);
	const uint32_t enable_bit = dir == AUMODE_PLAY ?
	    APBIF_CHANNEL_CTRL_TX_ENABLE : APBIF_CHANNEL_CTRL_RX_ENABLE;
	uint32_t val;

	val = APBIF_READ(sc, APBIF_CHANNELn_CTRL_REG(chan));
	val &= ~enable_bit;
	APBIF_WRITE(sc, APBIF_CHANNELn_CTRL_REG(chan), val);

	return 0;
}

void
tegra_ahub_route_i2s(device_t dev, u_int apbif_chan, u_int i2s_chan,
    u_int play_cif, u_int rec_cif, bool enable)
{
	struct tegra_ahub_softc * const sc = device_private(dev);
	const uint32_t play_mask = 1 << play_cif;
	const uint32_t rec_mask = 1 << rec_cif;
	uint32_t val;

	mutex_enter(&sc->sc_lock);

	/* Playback: APBIF TX to I2S RX */
	val = XBAR_READ(sc, AUDIO_I2S_RX0_REG(i2s_chan));
	if (enable)
		val |= play_mask;
	else
		val &= ~play_mask;
	XBAR_WRITE(sc, AUDIO_I2S_RX0_REG(i2s_chan), val);

	/* Capture: I2S TX to APBIF RX */
	val = XBAR_READ(sc, AUDIO_APBIF_RXn_REG(apbif_chan));
	if (enable)
		val |= rec_mask;
	else
		val &= ~rec_mask;
	XBAR_WRITE(sc, AUDIO_APBIF_RXn_REG(apbif_chan), val);

	mutex_exit(&sc->sc_lock);
}

int
tegra_ahub_audiocif_encode(const audio_params_t *params, int dir, uint32_t *val)
{
	uint32_t cif = 0;
	u_int channels, bits;

	if (params->channels > 16)
		return EINVAL;
	if (params->precision != params->validbits)
		return EINVAL;

	channels = params->channels - 1;

	switch (params->validbits) {
	case 8:
		bits = ACIF_BITS_BIT8;
		break;
	case 16:
		bits = ACIF_BITS_BIT16;
		break;
	case 32:
		bits = ACIF_BITS_BIT32;
		break;
	default:
		return EINVAL;
	}

	cif |= __SHIFTIN(channels, ACIF_AUDIO_CHANNELS);
	cif |= __SHIFTIN(channels, ACIF_CLIENT_CHANNELS);
	cif |= __SHIFTIN(bits, ACIF_AUDIO_BITS);
	cif |= __SHIFTIN(bits, ACIF_CLIENT_BITS);
	cif |= __SHIFTIN(dir, ACIF_DIRECTION);

	*val = cif;
	return 0;
}

struct fdtbus_dma *
tegra_ahub_chan_setup_dma(device_t dev, u_int chan, int dir,
    void (*cb)(void *), void *cbarg)
{
	struct tegra_ahub_softc * const sc = device_private(dev);
	const int phandle = sc->sc_phandle;
	char devname[4];

	devname[0] = dir == AUMODE_PLAY ? 't' : 'r';
	devname[1] = 'x';
	devname[2] = '0' + chan;
	devname[3] = '\0';

	return fdtbus_dma_get(phandle, devname, cb, cbarg);
}
