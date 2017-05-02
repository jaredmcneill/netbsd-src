/* $NetBSD$ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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

#include "alc56xx.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

#if NALC56XX > 0
#include <dev/i2c/alc56xx.h>
#endif

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/auconv.h>

#include <dev/fdt/fdtvar.h>

#define	TEGRA_CODEC_ALIGN	4

static int	tegra_codec_match(device_t, cfdata_t, void *);
static void	tegra_codec_attach(device_t, device_t, void *);

static int	tegra_codec_open(void *, int);
static void	tegra_codec_close(void *);
static int	tegra_codec_query_encoding(void *, struct audio_encoding *);
static int	tegra_codec_set_params(void *, int, int, audio_params_t *,
		    audio_params_t *, stream_filter_list_t *,
		    stream_filter_list_t *);
static int	tegra_codec_trigger_output(void *, void *, void *, int,
		    void (*)(void *), void *, const audio_params_t *);
static int	tegra_codec_trigger_input(void *, void *, void *, int,
		    void (*)(void *), void *, const audio_params_t *);
static int	tegra_codec_halt_output(void *);
static int	tegra_codec_halt_input(void *);
static int	tegra_codec_set_port(void *, mixer_ctrl_t *);
static int	tegra_codec_get_port(void *, mixer_ctrl_t *);
static int	tegra_codec_query_devinfo(void *, mixer_devinfo_t *);
static void *	tegra_codec_allocm(void *, int, size_t);
static void	tegra_codec_freem(void *, void *, size_t);
static paddr_t	tegra_codec_mappage(void *, void *, off_t, int);
static int	tegra_codec_getdev(void *, struct audio_device *);
static int	tegra_codec_get_props(void *);
static int	tegra_codec_round_blocksize(void *, int, int,
		    const audio_params_t *);
static void	tegra_codec_get_locks(void *, kmutex_t **, kmutex_t **);

static const struct audio_hw_if tegra_codec_hw_if = {
	.open = tegra_codec_open,
	.close = tegra_codec_close,
	.query_encoding = tegra_codec_query_encoding,
	.set_params = tegra_codec_set_params,
	.trigger_output = tegra_codec_trigger_output,
	.trigger_input = tegra_codec_trigger_input,
	.halt_output = tegra_codec_halt_output,
	.halt_input = tegra_codec_halt_input,
	.set_port = tegra_codec_set_port,
	.get_port = tegra_codec_get_port,
	.query_devinfo = tegra_codec_query_devinfo,
	.allocm = tegra_codec_allocm,
	.freem = tegra_codec_freem,
	.mappage = tegra_codec_mappage,
	.getdev = tegra_codec_getdev,
	.get_props = tegra_codec_get_props,
	.round_blocksize = tegra_codec_round_blocksize,
	.get_locks = tegra_codec_get_locks,
};

struct tegra_codec_softc;

struct tegra_codec_ops {
	const char		*id;

	device_t		(*codec_lookup)(int);
	int			(*init)(struct tegra_codec_softc *);

	int			(*open)(struct tegra_codec_softc *, int);
	void			(*close)(struct tegra_codec_softc *);
	int			(*set_port)(struct tegra_codec_softc *,
				    mixer_ctrl_t *);
	int			(*get_port)(struct tegra_codec_softc *,
				    mixer_ctrl_t *);
	int			(*query_devinfo)(struct tegra_codec_softc *,
				    mixer_devinfo_t *);
};

struct tegra_codec_softc {
	device_t		sc_dev;
	int			sc_phandle;

	kmutex_t		sc_lock;
	kmutex_t		sc_intr_lock;

	const struct tegra_codec_ops *sc_ops;

	struct clk		*sc_clk_pll_a;
	struct clk		*sc_clk_pll_a_out0;
	struct clk		*sc_clk_mclk;

	struct fdtbus_gpio_pin	*sc_gpio_hpdet;

	device_t		sc_codec;
	device_t		sc_i2s;

	struct audio_format	sc_format;
	struct audio_encoding_set *sc_encodings;

	device_t		sc_audiodev;
};

#if NALC56XX > 0
static device_t
tegra_codec_rt5640_codec_lookup(int phandle)
{
	return alc56xx_lookup(phandle);
}

static int
tegra_codec_rt5640_init(struct tegra_codec_softc *sc)
{
	int error;

	if (clk_set_rate_enable(sc->sc_clk_pll_a, 368640000) != 0 ||
	    clk_set_rate_enable(sc->sc_clk_pll_a_out0, 256 * 48000) != 0 ||
	    clk_enable(sc->sc_clk_mclk) != 0) {
		aprint_error_dev(sc->sc_dev, "failed to setup clocks\n");
		return ENXIO;
	}

	error = alc56xx_start(sc->sc_codec);
	if (error)
		return error;

	sc->sc_format.mode = AUMODE_PLAY | AUMODE_RECORD;
	sc->sc_format.encoding = AUDIO_ENCODING_SLINEAR_LE;
	sc->sc_format.validbits = 16;
	sc->sc_format.precision = 16;
	sc->sc_format.channels = 2;
	sc->sc_format.channel_mask = AUFMT_STEREO;
	sc->sc_format.frequency_type = 0;
	sc->sc_format.frequency[0] = sc->sc_format.frequency[1] = 48000;

	return 0;
}

static const struct tegra_codec_ops tegra_codec_rt5640_ops = {
	.id = "Realtek ALC5640",
	.codec_lookup = tegra_codec_rt5640_codec_lookup,
	.init = tegra_codec_rt5640_init,
};
#endif

CFATTACH_DECL_NEW(tegra_codec, sizeof(struct tegra_codec_softc),
	tegra_codec_match, tegra_codec_attach, NULL, NULL);

static const struct of_compat_data tegra_codec_compat[] = {
#if NALC56XX > 0
	{ "nvidia,tegra-audio-rt5640",	(uintptr_t)&tegra_codec_rt5640_ops },
#endif
	{ NULL, 0 }
};

static int
tegra_codec_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	return of_search_compatible(phandle, tegra_codec_compat)->data != 0;
}

static void
tegra_codec_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_codec_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int error;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_ops = (const struct tegra_codec_ops *)
	    of_search_compatible(phandle, tegra_codec_compat)->data;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_AUDIO);

	sc->sc_clk_pll_a = fdtbus_clock_get(phandle, "pll_a");
	sc->sc_clk_pll_a_out0 = fdtbus_clock_get(phandle, "pll_a_out0");
	sc->sc_clk_mclk = fdtbus_clock_get(phandle, "mclk");
	if (!sc->sc_clk_pll_a || !sc->sc_clk_pll_a_out0 || !sc->sc_clk_mclk) {
		aprint_error(": couldn't get clocks\n");
		return;
	}

	const int codec_phandle = fdtbus_get_phandle(phandle,
	    "nvidia,audio-codec");
	sc->sc_codec = sc->sc_ops->codec_lookup(codec_phandle);
	if (sc->sc_codec == NULL) {
		aprint_error(": couldn't find codec device instance\n");
		return;
	}

	const int i2s_phandle = fdtbus_get_phandle(phandle,
	    "nvidia,i2s-controller");
	sc->sc_i2s = tegra_i2s_lookup(i2s_phandle);
	if (sc->sc_i2s == NULL) {
		aprint_error(": couldn't find i2s device instance\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": %s\n", fdtbus_get_string(phandle, "nvidia,model"));
	aprint_verbose_dev(self, "codec %s, i2s %s\n",
	    device_xname(sc->sc_codec), device_xname(sc->sc_i2s));

	error = sc->sc_ops->init(sc);
	if (error) {
		aprint_error_dev(self, "audio codec failed to start (%d)\n",
		    error);
		return;
	}	

	error = auconv_create_encodings(&sc->sc_format, 1, &sc->sc_encodings);
	if (error) {
		aprint_error_dev(self, "couldn't create encodings (%d)\n",
		    error);
		return;
	}

	sc->sc_audiodev = audio_attach_mi(&tegra_codec_hw_if, sc, self);
}

static int
tegra_codec_open(void *priv, int flags)
{
	struct tegra_codec_softc * const sc = priv;
	int error = 0;

	if (sc->sc_ops->open)
		error = sc->sc_ops->open(sc, flags);

	return error;
}

static void
tegra_codec_close(void *priv)
{
	struct tegra_codec_softc * const sc = priv;

	if (sc->sc_ops->close)
		sc->sc_ops->close(sc);
}

static int
tegra_codec_query_encoding(void *priv, struct audio_encoding *ae)
{
	struct tegra_codec_softc * const sc = priv;

	return auconv_query_encoding(sc->sc_encodings, ae);
}

static int
tegra_codec_set_params(void *priv, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	struct tegra_codec_softc * const sc = priv;
	int index;

	if (play && (setmode & AUMODE_PLAY) != 0) {
		index = auconv_set_converter(&sc->sc_format, 1,
		    AUMODE_PLAY, play, true, pfil);
		if (index < 0)
			return EINVAL;
	}
	if (rec && (setmode & AUMODE_RECORD) != 0) {
		index = auconv_set_converter(&sc->sc_format, 1,
		    AUMODE_RECORD, rec, true, rfil);
		if (index < 0)
			return EINVAL;
	}

	return 0;
}

static int
tegra_codec_trigger_output(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct tegra_codec_softc * const sc = priv;

	return tegra_i2s_trigger(sc->sc_i2s, AUMODE_PLAY, start, end,
	    blksize, intr, intrarg, params);
}

static int
tegra_codec_trigger_input(void *priv, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *params)
{
	struct tegra_codec_softc * const sc = priv;

	return tegra_i2s_trigger(sc->sc_i2s, AUMODE_RECORD, start, end,
	    blksize, intr, intrarg, params);
}

static int
tegra_codec_halt_output(void *priv)
{
	struct tegra_codec_softc * const sc = priv;

	return tegra_i2s_halt(sc->sc_i2s, AUMODE_PLAY);
}

static int
tegra_codec_halt_input(void *priv)
{
	struct tegra_codec_softc * const sc = priv;

	return tegra_i2s_halt(sc->sc_i2s, AUMODE_RECORD);
}

static int
tegra_codec_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct tegra_codec_softc * const sc = priv;

	if (sc->sc_ops->set_port)
		return sc->sc_ops->set_port(sc, mc);

	return ENXIO;
}

static int
tegra_codec_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct tegra_codec_softc * const sc = priv;

	if (sc->sc_ops->get_port)
		return sc->sc_ops->get_port(sc, mc);

	return ENXIO;
}

static int
tegra_codec_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	struct tegra_codec_softc * const sc = priv;

	if (sc->sc_ops->query_devinfo)
		return sc->sc_ops->query_devinfo(sc, di);

	return ENXIO;
}

static void *
tegra_codec_allocm(void *priv, int dir, size_t size)
{
	struct tegra_codec_softc * const sc = priv;

	return tegra_i2s_allocm(sc->sc_i2s, size);
}

static void
tegra_codec_freem(void *priv, void *addr, size_t size)
{
	struct tegra_codec_softc * const sc = priv;

	return tegra_i2s_freem(sc->sc_i2s, addr, size);
}

static paddr_t
tegra_codec_mappage(void *priv, void *addr, off_t off, int prot)
{
	struct tegra_codec_softc * const sc = priv;

	return tegra_i2s_mappage(sc->sc_i2s, addr, off, prot);
}

static int
tegra_codec_getdev(void *priv, struct audio_device *audiodev)
{
	struct tegra_codec_softc * const sc = priv;

	snprintf(audiodev->name, sizeof(audiodev->name), "NVIDIA Tegra");
	snprintf(audiodev->version, sizeof(audiodev->version), sc->sc_ops->id);
	snprintf(audiodev->config, sizeof(audiodev->config),
	    tegracodec_cd.cd_name);

	return 0;
}

static int
tegra_codec_get_props(void *priv)
{
	return AUDIO_PROP_PLAYBACK | AUDIO_PROP_CAPTURE |
	       AUDIO_PROP_INDEPENDENT | AUDIO_PROP_MMAP |
	       AUDIO_PROP_FULLDUPLEX;
}

static int
tegra_codec_round_blocksize(void *priv, int bs, int mode,
    const audio_params_t *params)
{
	return roundup(bs, TEGRA_CODEC_ALIGN);
}

static void
tegra_codec_get_locks(void *priv, kmutex_t **intr, kmutex_t **thread)
{
	struct tegra_codec_softc * const sc = priv;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}
