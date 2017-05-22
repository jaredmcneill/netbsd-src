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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ioconf.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/alc56xx.h>

#include <dev/fdt/fdtvar.h>

#define	FDTPROP_LDO1_EN_GPIOS	"realtek,ldo1-en-gpios"

static const char * alc56xx_compats[] = {
	"realtek,rt5639",
	"realtek,rt5640",
	NULL
};

struct alc56xx_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_phandle;

	struct fdtbus_gpio_pin *sc_pin_ldo1_en;
};

enum {
	RT_OUTPUT_CLASS,
	RT_MASTER_VOL,
	RT_HEADPHONES_VOL,
	RT_HEADPHONES_MUTE,
#if notyet
	RT_LINE_VOL,
	RT_LINE_MUTE,
#endif

	RT_INPUT_CLASS,
	RT_DAC_VOL,

	RT_NDEVS
};

static int	alc56xx_match(device_t, cfdata_t, void *);
static void	alc56xx_attach(device_t, device_t, void *);

static int	alc56xx_read(struct alc56xx_softc *, uint8_t, uint16_t *);
static int	alc56xx_write(struct alc56xx_softc *, uint8_t, uint16_t);
static int	alc56xx_set_clear(struct alc56xx_softc *, uint8_t, uint16_t,
				  uint16_t);

#if 0
static int	alc56xx_pr_read(struct alc56xx_softc *, uint8_t, uint16_t *);
#endif
static int	alc56xx_pr_write(struct alc56xx_softc *, uint8_t, uint16_t);

CFATTACH_DECL_NEW(alc56xx, sizeof(struct alc56xx_softc),
    alc56xx_match, alc56xx_attach, NULL, NULL);

static int
alc56xx_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_name == NULL)
		return 0;

	return iic_compat_match(ia, alc56xx_compats);
}

static void
alc56xx_attach(device_t parent, device_t self, void *aux)
{
	struct alc56xx_softc * const sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	const int phandle = ia->ia_cookie;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = phandle;

	aprint_naive("\n");
	aprint_normal(": ALC5639/ALC5640 audio codec\n");

	if (of_hasprop(phandle, FDTPROP_LDO1_EN_GPIOS)) {
		sc->sc_pin_ldo1_en = fdtbus_gpio_acquire(phandle,
		    FDTPROP_LDO1_EN_GPIOS, GPIO_PIN_OUTPUT);
		if (sc->sc_pin_ldo1_en == NULL) {
			aprint_error_dev(self, "couldn't acquire gpio\n");
			return;
		}
	}
}

static int
alc56xx_read(struct alc56xx_softc *sc, uint8_t reg, uint16_t *val)
{
	int error;
	uint8_t buf[2];

	error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &reg, sizeof(reg), buf, sizeof(buf), I2C_F_POLL);
	if (error)
		return error;

	*val = be16dec(buf);
	return 0;
}

static int
alc56xx_write(struct alc56xx_softc *sc, uint8_t reg, uint16_t val)
{
	uint8_t buf[3];

	buf[0] = reg;
	be16enc(&buf[1], val);

	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    NULL, 0, buf, sizeof(buf), I2C_F_POLL);
}

static int
alc56xx_set_clear(struct alc56xx_softc *sc, uint8_t reg, uint16_t set,
    uint16_t clr)
{
	uint16_t old, new;
	int error;

	error = alc56xx_read(sc, reg, &old);
	if (error)
		return error;
	new = set | (old & ~clr);

	return alc56xx_write(sc, reg, new);
}

#if 0
static int
alc56xx_pr_read(struct alc56xx_softc *sc, uint8_t reg, uint16_t *val)
{
	int error;

	error = alc56xx_write(sc, MX_PR_INDEX, reg);
	if (error)
		return error;

	return alc56xx_read(sc, MX_PR_DATA, val);
}
#endif

static int
alc56xx_pr_write(struct alc56xx_softc *sc, uint8_t reg, uint16_t val)
{
	int error;

	error = alc56xx_write(sc, MX_PR_INDEX, reg);
	if (error)
		return error;

	return alc56xx_write(sc, MX_PR_DATA, val);
}

static void
alc56xx_route_dac1_to_hp(struct alc56xx_softc *sc)
{
	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);

	/* Power on OUTMIXL/OUTMIXR */
	alc56xx_set_clear(sc, MX_POWER_CTRL5_REG,
	    MX_POWER_CTRL5_OUTMIXL | MX_POWER_CTRL5_OUTMIXR, 0);

	/* Unmute DACL1 to OUTMIXL */
	alc56xx_set_clear(sc, MX_OUTMIXL_CTRL3_REG,
	    0, MX_OUTMIXL_CTRL3_DACL1_MUTE);
	/* Unmute DACR1 to OUTMIXR */
	alc56xx_set_clear(sc, MX_OUTMIXR_CTRL3_REG,
	    0, MX_OUTMIXR_CTRL3_DACR1_MUTE);

	/* Power on HPOVOLL/HPOVOLR */
	alc56xx_set_clear(sc, MX_POWER_CTRL6_REG,
	    MX_POWER_CTRL6_HPOVOLL | MX_POWER_CTRL6_HPOVOLR, 0);

	/* Unmute OUTMIX to HPOVOL */
	alc56xx_set_clear(sc, MX_HPOUT_REG,
	    0, MX_HPOUT_HPOVOLL_MUTE | MX_HPOUT_HPOVOLR_MUTE);

	/* Unmute HPOVOL to HPOMIX */
	alc56xx_set_clear(sc, MX_HPOMIX_REG,
	    0, MX_HPOMIX_HPOVOL_MUTE);

	/* Enable HP output */
	alc56xx_write(sc, MX_HP_AMP_CTRL1_REG,
	    MX_HP_AMP_CTRL1_POW_CAPLESS | MX_HP_AMP_CTRL1_EN_SOFTGEN_HP);

	iic_release_bus(sc->sc_i2c, I2C_F_POLL);
}

device_t
alc56xx_lookup(int phandle)
{
	device_t dev;
	deviter_t di;
	bool found = false;

	for (dev = deviter_first(&di, DEVITER_F_LEAVES_FIRST);
	     dev != NULL;
	     dev = deviter_next(&di)) {
		if (!device_is_a(dev, alc56xx_cd.cd_name))
			continue;
		struct alc56xx_softc *const sc = device_private(dev);
		if (sc->sc_phandle == phandle) {
			found = true;
			break;
		}
	}
	deviter_release(&di);

	return found ? dev : NULL;
}

int
alc56xx_start(device_t dev)
{
	struct alc56xx_softc * const sc = device_private(dev);
	uint16_t did, vid;
	int error;

	if (sc->sc_pin_ldo1_en) {
		fdtbus_gpio_write(sc->sc_pin_ldo1_en, 1);
		delay(400000);
	}

	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);
	error = alc56xx_read(sc, MX_DEVICE_ID_REG, &did);
	if (error == 0)
		error = alc56xx_read(sc, MX_VENDOR_ID_REG, &vid);
	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	if (error) {
		aprint_error_dev(dev, "couldn't read device ID register (%d)\n",
		    error);
		return error;
	}

	aprint_debug_dev(dev, "DEVICE_ID %04x, VENDOR_ID %04x\n", did, vid);

	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);

	/* S/W reset */
	alc56xx_write(sc, MX_DEVICE_ID_REG, 0);

if (0) {
	/* Init private registers */
	alc56xx_pr_write(sc, 0x3d, 0x3600);
	alc56xx_pr_write(sc, 0x12, 0x0aa8);
	alc56xx_pr_write(sc, 0x14, 0x0aaa);
	alc56xx_pr_write(sc, 0x20, 0x6110);
	alc56xx_pr_write(sc, 0x21, 0xe0e0);
	alc56xx_pr_write(sc, 0x23, 0x1804);
}

	/* Enable MCLK detection */
	alc56xx_set_clear(sc, MX_GEN_CTRL1_REG, MX_GEN_CTRL1_MCLK_DET, 0);

	/* Set system clock source to MCLK */
	alc56xx_set_clear(sc, MX_GLOBAL_CLOCK_CTRL_REG,
	    __SHIFTIN(MX_GLOBAL_CLOCK_CTRL_MUX_MCLK, MX_GLOBAL_CLOCK_CTRL_MUX),
	    MX_GLOBAL_CLOCK_CTRL_MUX);

	/* Power on */
	alc56xx_set_clear(sc, MX_POWER_CTRL1_REG,
	    MX_POWER_CTRL1_I2S1 | MX_POWER_CTRL1_I2S2 |
	    MX_POWER_CTRL1_DACL1 | MX_POWER_CTRL1_DACR1 |
	    MX_POWER_CTRL1_CLASSD, 0);
	alc56xx_set_clear(sc, MX_POWER_CTRL3_REG,
	    MX_POWER_CTRL3_VREF1 | MX_POWER_CTRL3_VREF2 |
	    MX_POWER_CTRL3_MAIN_BIAS | MX_POWER_CTRL3_MBIAS_BG |
	    MX_POWER_CTRL3_HP_LEFT | MX_POWER_CTRL3_HP_RIGHT |
	    MX_POWER_CTRL3_HP_AMP, 0);
	delay(15000);
	alc56xx_set_clear(sc, MX_POWER_CTRL3_REG,
	    MX_POWER_CTRL3_VREF1_FASTMODE | MX_POWER_CTRL3_VREF2_FASTMODE, 0);

	/* Enable input clock */
	alc56xx_set_clear(sc, MX_GEN_CTRL1_REG,
	    MX_GEN_CTRL1_CLK_GATE |
	    MX_GEN_CTRL1_EN_IN1_SE | MX_GEN_CTRL1_EN_IN2_SE, 0);

	/* Configure I2S1 digital interface (I2S format, 16-bit, no compress */
	alc56xx_write(sc, MX_I2S1_CTRL_REG, MX_I2S1_CTRL_MODE);

	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	alc56xx_route_dac1_to_hp(sc);

	return 0;
}

int
alc56xx_query_devinfo(device_t dev, mixer_devinfo_t *di)
{
	switch (di->index) {
	case RT_OUTPUT_CLASS:
		di->mixer_class = RT_OUTPUT_CLASS;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case RT_MASTER_VOL:
		di->mixer_class = RT_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNmaster);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	case RT_HEADPHONES_VOL:
		di->mixer_class = RT_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNheadphone);
		di->type = AUDIO_MIXER_VALUE;
		di->prev = AUDIO_MIXER_LAST;
		di->next = RT_HEADPHONES_MUTE;
		di->un.v.num_channels = 2;
		di->un.v.delta = 1;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	case RT_HEADPHONES_MUTE:
		di->mixer_class = RT_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNmute);
		di->type = AUDIO_MIXER_ENUM;
		di->prev = RT_HEADPHONES_VOL;
		di->next = AUDIO_MIXER_LAST;
		di->un.e.num_mem = 2;
		strcpy(di->un.e.member[0].label.name, AudioNoff);
		di->un.e.member[0].ord = 0;
		strcpy(di->un.e.member[1].label.name, AudioNon);
		di->un.e.member[1].ord = 1;
		return 0;
#if notyet
	case RT_LINE_VOL:
		di->mixer_class = RT_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNline);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		di->un.v.delta = 1;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	case RT_LINE_MUTE:
		di->mixer_class = RT_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNmute);
		di->type = AUDIO_MIXER_ENUM;
		di->prev = RT_LINE_VOL;
		di->next = AUDIO_MIXER_LAST;
		di->un.e.num_mem = 2;
		strcpy(di->un.e.member[0].label.name, AudioNoff);
		di->un.e.member[0].ord = 0;
		strcpy(di->un.e.member[1].label.name, AudioNon);
		di->un.e.member[1].ord = 1;
		return 0;
#endif

	case RT_INPUT_CLASS:
		di->mixer_class = RT_INPUT_CLASS;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case RT_DAC_VOL:
		di->mixer_class = RT_INPUT_CLASS;
		strcpy(di->label.name, AudioNdac);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 2;
		di->un.v.delta = 1;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;

	default:
		return ENXIO;
	}
}

int
alc56xx_set_port(device_t dev, mixer_ctrl_t *mc)
{
	struct alc56xx_softc * const sc = device_private(dev);
	uint16_t val;
	int error;

	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);

	switch (mc->dev) {
	case RT_MASTER_VOL:
	case RT_DAC_VOL:
		error = alc56xx_read(sc, MX_DAC1_DVOL_REG, &val);
		if (error)
			break;
		val &= ~MX_DAC1_LEFT;
		val |= __SHIFTIN(mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT],
				 MX_DAC1_LEFT);
		val &= ~MX_DAC1_RIGHT;
		val |= __SHIFTIN(mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT],
				 MX_DAC1_RIGHT);
		error = alc56xx_write(sc, MX_DAC1_DVOL_REG, val);
		break;

	case RT_HEADPHONES_VOL:
		error = alc56xx_read(sc, MX_HPOUT_REG, &val);
		if (error)
			break;
		val &= ~MX_HPOUT_HPOVOLL;
		val |= __SHIFTIN(mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT],
				 MX_HPOUT_HPOVOLL);
		val &= ~MX_HPOUT_HPOVOLR;
		val |= __SHIFTIN(mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT],
				 MX_HPOUT_HPOVOLR);
		error = alc56xx_write(sc, MX_HPOUT_REG, val);
		break;

	case RT_HEADPHONES_MUTE:
		error = alc56xx_read(sc, MX_HPOUT_REG, &val);
		if (error)
			break;
		if (mc->un.ord)
			val |= MX_HPOUT_HPO_MUTE;
		else
			val &= ~MX_HPOUT_HPO_MUTE;
		error = alc56xx_write(sc, MX_HPOUT_REG, val);
		if (error)
			break;

		error = alc56xx_read(sc, MX_HPOMIX_REG, &val);
		if (error)
			break;
		if (mc->un.ord)
			val |= MX_HPOMIX_DAC1_MUTE;
		else
			val &= ~MX_HPOMIX_DAC1_MUTE;
		error = alc56xx_write(sc, MX_HPOMIX_REG, val);
		if (error)
			break;
		break;

	default:
		error = ENXIO;
		break;
	}

	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	return error;
}

int
alc56xx_get_port(device_t dev, mixer_ctrl_t *mc)
{
	struct alc56xx_softc * const sc = device_private(dev);
	uint16_t val;
	int error;

	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);

	switch (mc->dev) {
	case RT_MASTER_VOL:
	case RT_DAC_VOL:
		error = alc56xx_read(sc, MX_DAC1_DVOL_REG, &val);
		if (!error) {
			mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
			    __SHIFTOUT(val, MX_DAC1_LEFT);
			mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
			    __SHIFTOUT(val, MX_DAC1_RIGHT);
		}
		break;

	case RT_HEADPHONES_VOL:
		error = alc56xx_read(sc, MX_HPOUT_REG, &val);
		if (!error) {
			mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
			    __SHIFTOUT(val, MX_HPOUT_HPOVOLL);
			mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
			    __SHIFTOUT(val, MX_HPOUT_HPOVOLR);
		}
		break;

	case RT_HEADPHONES_MUTE:
		error = alc56xx_read(sc, MX_HPOUT_REG, &val);
		if (error)
			break;
		const int hpout_mute = (val & MX_HPOUT_HPO_MUTE) != 0;
		error = alc56xx_read(sc, MX_HPOMIX_REG, &val);
		if (error)
			break;
		const int hpomix_mute = (val & MX_HPOMIX_DAC1_MUTE) != 0;
		mc->un.ord = hpout_mute && hpomix_mute;
		break;

	default:
		error = ENXIO;
		break;
	}

	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	return error;
}
