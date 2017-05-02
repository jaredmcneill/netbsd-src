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

#include "ioconf.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_i2sreg.h>
#include <arm/nvidia/tegra_ahubreg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

static int	tegra_i2s_match(device_t, cfdata_t, void *);
static void	tegra_i2s_attach(device_t, device_t, void *);

static void	tegra_i2s_intr(void *);

struct tegra_i2s_softc;

struct tegra_i2s_stream {
	struct tegra_i2s_softc	*st_sc;
	int			st_dir;
	struct fdtbus_dma	*st_dma;
	void			(*st_intr)(void *);
	void			*st_intrarg;

	bus_addr_t		st_fifo;
	bus_addr_t		st_start, st_end;
	bus_addr_t		st_cur;
	int			st_blksize;
};

struct tegra_i2s_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	struct clk		*sc_clk;
	struct fdtbus_reset	*sc_rst;

	device_t		sc_ahub;
	u_int			sc_ahub_chan;
	u_int			sc_i2s_chan;

	u_int			sc_cif_ids[2];

	struct tegra_i2s_stream	sc_stream[2];
};

#define	I2S_READ(sc, reg)						\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	I2S_WRITE(sc, reg, val)						\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(tegra_i2s, sizeof(struct tegra_i2s_softc),
	tegra_i2s_match, tegra_i2s_attach, NULL, NULL);

static int
tegra_i2s_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "nvidia,tegra124-i2s", NULL };
	struct fdt_attach_args * const faa = aux;

	/* i2s node should be a child of audio hub */
	if (!device_is_a(parent, "tegraahub"))
		return 0;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_i2s_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_i2s_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error, len;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}
	sc->sc_rst = fdtbus_reset_get(phandle, "i2s");
	if (sc->sc_rst == NULL) {
		aprint_error(": couldn't get reset i2s\n");
		return;
	}

	error = clk_enable(sc->sc_clk);
	if (error) {
		aprint_error(": couldn't enable clock (%d)\n", error);
		return;
	}
	fdtbus_reset_deassert(sc->sc_rst);

	aprint_naive("\n");
	aprint_normal(": I2S\n");

	sc->sc_ahub = parent;
	if (tegra_ahub_chan_alloc(sc->sc_ahub, &sc->sc_ahub_chan) != 0) {
		aprint_error_dev(self, "failed to get audio hub channel\n");
		return;
	}

	sc->sc_stream[0].st_sc = sc;
	sc->sc_stream[0].st_dir = AUMODE_PLAY;
	sc->sc_stream[0].st_dma = tegra_ahub_chan_setup_dma(sc->sc_ahub,
	    sc->sc_ahub_chan, AUMODE_PLAY, tegra_i2s_intr, &sc->sc_stream[0]);

	sc->sc_stream[1].st_sc = sc;
	sc->sc_stream[1].st_dir = AUMODE_RECORD;
#if 0
	sc->sc_stream[1].st_dma = tegra_ahub_chan_setup_dma(sc->sc_ahub,
	    sc->sc_ahub_chan, AUMODE_RECORD, tegra_i2s_intr, &sc->sc_stream[1]);
#endif

	len = OF_getprop(phandle, "nvidia,ahub-cif-ids", sc->sc_cif_ids,
	    sizeof(sc->sc_cif_ids));
	if (len != sizeof(sc->sc_cif_ids)) {
		aprint_error_dev(self,
		    "failed to get property nvidia,ahub-cif-ids\n");
		return;
	}
	sc->sc_cif_ids[0] = be32toh(sc->sc_cif_ids[0]);
	sc->sc_cif_ids[1] = be32toh(sc->sc_cif_ids[1]);

	/* Derive I2S unit number from CIF */
	sc->sc_i2s_chan = sc->sc_cif_ids[0] - 4;

	/* Setup bi-directional route between APBIF and this I2S instance */
	tegra_ahub_route_i2s(sc->sc_ahub, sc->sc_ahub_chan, sc->sc_i2s_chan,
	    sc->sc_cif_ids[0], sc->sc_cif_ids[1], true);
}

static int
tegra_i2s_set_audiocif(struct tegra_i2s_softc *sc, int dir,
    const audio_params_t *params)
{
	bus_size_t reg = dir == AUMODE_PLAY ?
	    I2S_AUDIOCIF_I2STX_CTRL_REG : I2S_AUDIOCIF_I2SRX_CTRL_REG;
	int cifdir = dir == AUMODE_PLAY ? AUDIOCIF_DIR_TX : AUDIOCIF_DIR_RX;
	uint32_t cif;
	int error;

	error = tegra_ahub_audiocif_encode(params, cifdir, &cif);
	if (error)
		return error;

	I2S_WRITE(sc, reg, cif);

	return 0;
}

static int
tegra_i2s_transfer(struct tegra_i2s_stream *st)
{
	struct tegra_i2s_softc * const sc = st->st_sc;
	int error;

	bus_dma_segment_t seg = {
		.ds_addr = st->st_cur,
		.ds_len = st->st_blksize
	};

	struct fdtbus_dma_req dreq = {
		.dreq_segs = &seg,
		.dreq_nsegs = 1,
		.dreq_dev_phys = st->st_cur,
		.dreq_dir = st->st_dir == AUMODE_PLAY ?
		    FDT_DMA_WRITE : FDT_DMA_READ,
		.dreq_block_irq = 1,
		.dreq_block_multi = 0,
		.dreq_flow = 1,
		.dreq_mem_opt = {
			.opt_bus_width = 4,
			.opt_burst_len = 4,
		},
		.dreq_dev_opt = {
			.opt_bus_width = 4,
			.opt_burst_len = 4,
		},
	};

	error = fdtbus_dma_transfer(st->st_dma, &dreq);
	if (error) {
		device_printf(sc->sc_dev, "failed to transfer DMA (%d)\n",
		    error);
		return error;
	}

	st->st_cur += st->st_blksize;
	if (st->st_cur >= st->st_end)
		st->st_cur = st->st_start;

	return 0;
}

static void
tegra_i2s_intr(void *priv)
{
	struct tegra_i2s_stream * const st = priv;

	if (st->st_intr) {
		st->st_intr(st->st_intrarg);
		tegra_i2s_transfer(st);
	}
}

device_t
tegra_i2s_lookup(int phandle)
{
	device_t dev;
	deviter_t di;
	bool found = false;

	for (dev = deviter_first(&di, DEVITER_F_LEAVES_FIRST);
	     dev != NULL;
	     dev = deviter_next(&di)) {
		if (!device_is_a(dev, tegrai2s_cd.cd_name))
			continue;
		struct tegra_i2s_softc * const sc = device_private(dev);
		if (sc->sc_phandle == phandle) {
			found = true;
			break;
		}
	}
	deviter_release(&di);

	return found ? dev : NULL;
}

int
tegra_i2s_trigger(device_t dev, int dir, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct tegra_i2s_softc * const sc = device_private(dev);
	struct tegra_i2s_stream * const st = dir == AUMODE_PLAY ?
	    &sc->sc_stream[0] : &sc->sc_stream[1];
	bus_addr_t pstart;
	bus_size_t psize;
	int error;

	if (st->st_dma == NULL)
		return ENXIO;

	pstart = tegra_ahub_chan_vtophys(sc->sc_ahub, sc->sc_ahub_chan, start);
	if (pstart == -1) {
		device_printf(sc->sc_dev, "bad addr %p\n", start);
		return EINVAL;
	}
	psize = (uintptr_t)end - (uintptr_t)start;

	error = tegra_i2s_set_audiocif(sc, dir, params);
	if (error)
		return error;

	st->st_intr = intr;
	st->st_intrarg = intrarg;
	st->st_start = pstart;
	st->st_end = pstart + psize;
	st->st_cur = pstart;
	st->st_blksize = blksize;

	error = tegra_ahub_chan_trigger(sc->sc_ahub, sc->sc_ahub_chan, dir,
	    start, end, blksize, intr, intrarg, params);
	if (error)
		return error;

	return tegra_i2s_transfer(st);
}

int
tegra_i2s_halt(device_t dev, int dir)
{
	struct tegra_i2s_softc * const sc = device_private(dev);
	struct tegra_i2s_stream * const st = dir == AUMODE_PLAY ?
	    &sc->sc_stream[0] : &sc->sc_stream[1];

	if (st->st_dma)
		fdtbus_dma_halt(st->st_dma);

	return tegra_ahub_chan_halt(sc->sc_ahub, sc->sc_ahub_chan, dir);
}

void *
tegra_i2s_allocm(device_t dev, size_t size)
{
	struct tegra_i2s_softc * const sc = device_private(dev);

	return tegra_ahub_chan_allocm(sc->sc_ahub, sc->sc_ahub_chan, size);
}

void
tegra_i2s_freem(device_t dev, void *addr, size_t size)
{
	struct tegra_i2s_softc * const sc = device_private(dev);

	return tegra_ahub_chan_freem(sc->sc_ahub, sc->sc_ahub_chan, addr, size);
}

paddr_t
tegra_i2s_mappage(device_t dev, void *addr, off_t off, int prot)
{
	struct tegra_i2s_softc * const sc = device_private(dev);

	return tegra_ahub_chan_mappage(sc->sc_ahub, sc->sc_ahub_chan, addr,
	    off, prot);
}
