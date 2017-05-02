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

static int	alc56xx_match(device_t, cfdata_t, void *);
static void	alc56xx_attach(device_t, device_t, void *);

static int	alc56xx_read(struct alc56xx_softc *, uint8_t, uint16_t *);
static int	alc56xx_write(struct alc56xx_softc *, uint8_t, uint16_t);

#if 0
static int	alc56xx_set_clear(struct alc56xx_softc *, uint8_t, uint16_t,
				  uint16_t);
#endif

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

#if 0
static int
alc56xx_set_clear(struct alc56xx_softc *sc, uint8_t reg, uint16_t set,
    uint16_t clr)
{
	uint16_t old, new;
	int error;

	error = alc56xx_read(sc, reg, &old);
	if (error) {
		return error;
	}
	new = set | (old & ~clr);

	return alc56xx_write(sc, reg, new);
}
#endif

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

	/* S/W reset */
	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);
	alc56xx_write(sc, MX_DEVICE_ID_REG, 0);
	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	return 0;
}
