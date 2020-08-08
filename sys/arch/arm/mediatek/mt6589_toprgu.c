/* $NetBSD$ */

/*-
 * Copyright (c) 2019 Jason R. Thorpe
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
#include <sys/systm.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/mediatek/mt6589_toprgureg.h>

/*
 * The input of the TOPRGU's watchdog timer is the 32KHz oscillator.
 * The units of the LENGTH register are "512 ticks", which works out
 * to:
 *
 *	(1 / 32000) * 512 = .016 -> 16ms
 *
 * ...which is roughly 1/64th of a second (15.6ms).
 *
 * The minimum round number of seconds we can represent is 1, and the
 * maximum is 31, given the bits available.
 */
#define	TOPRGU_WDT_PERIOD_MIN		1
#define	TOPRGU_WDT_PERIOD_MAX		31
#define	TOPRGU_WDT_PERIOD_DEFAULT	TOPRGU_WDT_PERIOD_MAX

static int	mt6589_toprgu_match(device_t, cfdata_t, void *);
static void	mt6589_toprgu_attach(device_t, device_t, void *);

static const char * compatible[] = {
	"mediatek,mt6589-wdt",
	NULL
};

struct mt6589_toprgu_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct sysmon_wdog sc_smw;
};

#define	TOPRGU_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

#define	TOPRGU_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(mt6589_toprgu, sizeof(struct mt6589_toprgu_softc),
    mt6589_toprgu_match, mt6589_toprgu_attach, NULL, NULL);

static int	mt6589_toprgu_setmode(struct sysmon_wdog *);
static int	mt6589_toprgu_tickle(struct sysmon_wdog *);

static int
mt6589_toprgu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mt6589_toprgu_attach(device_t parent, device_t self, void *aux)
{
	struct mt6589_toprgu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	uint32_t val;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Top Reset Generate Unit\n");

	val = TOPRGU_READ(sc, WDT_MODE);
	val &= ~(WDT_MODE_WDT_EN | WDT_MODE_WDT_IRQ | WDT_MODE_DUAL_MODE);
	TOPRGU_WRITE(sc, WDT_MODE, val | WDT_MODE_UNLOCK_KEY);

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_period = TOPRGU_WDT_PERIOD_DEFAULT;

	sc->sc_smw.smw_setmode = mt6589_toprgu_setmode;
	sc->sc_smw.smw_tickle = mt6589_toprgu_tickle;

	aprint_normal_dev(self,
	    "default watchdog period is %u seconds\n",
	    sc->sc_smw.smw_period);
	
	if (sysmon_wdog_register(&sc->sc_smw) != 0) {
		aprint_error_dev(self,
		    "couldn't register with sysmon\n");
	}
}

static int
mt6589_toprgu_setmode(struct sysmon_wdog *smw)
{
	struct mt6589_toprgu_softc * const sc = smw->smw_cookie;
	uint32_t val;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		val = TOPRGU_READ(sc, WDT_MODE);
		val &= ~WDT_MODE_WDT_EN;
		TOPRGU_WRITE(sc, WDT_MODE, val | WDT_MODE_UNLOCK_KEY);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			smw->smw_period = TOPRGU_WDT_PERIOD_DEFAULT;
		if (smw->smw_period < TOPRGU_WDT_PERIOD_MIN ||
		    smw->smw_period > TOPRGU_WDT_PERIOD_MAX)
		    	return EINVAL;
		
		val = TOPRGU_READ(sc, WDT_MODE);
		val &= ~WDT_MODE_WDT_EN;
		TOPRGU_WRITE(sc, WDT_MODE, val | WDT_MODE_UNLOCK_KEY);

		TOPRGU_WRITE(sc, WDT_LENGTH,
		    __SHIFTIN((smw->smw_period << 6), WDT_LENGTH_WDT_LENGTH) |
		    WDT_LENGTH_UNLOCK_KEY);
		TOPRGU_WRITE(sc, WDT_RESTART, WDT_RESTART_WDT_RESTART);

		val |= WDT_MODE_WDT_EN;
		TOPRGU_WRITE(sc, WDT_MODE, val | WDT_MODE_UNLOCK_KEY);
	}

	return 0;
}

static int
mt6589_toprgu_tickle(struct sysmon_wdog *smw)
{
	struct mt6589_toprgu_softc * const sc = smw->smw_cookie;

	TOPRGU_WRITE(sc, WDT_RESTART, WDT_RESTART_WDT_RESTART);
	return 0;
}
