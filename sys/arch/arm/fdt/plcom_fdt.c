/* $NetBSD$ */

/*-
 * Copyright (c) 2016 Jared McNeill <jmcneill@invisible.ca>
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
#include <sys/kmem.h>
#include <sys/termios.h>

#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

#include <dev/fdt/fdtvar.h>

struct plcom_fdt_softc {
	struct plcom_softc	sc;
	void			*sc_ih;
	struct clk		*sc_uartclk, *sc_apbpclk;
};

static int	plcom_fdt_match(device_t, cfdata_t, void *);
static void	plcom_fdt_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(plcom_fdt, sizeof(struct plcom_softc),
	plcom_fdt_match, plcom_fdt_attach, NULL, NULL);

static int
plcom_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "arm,pl011", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_compatible(faa->faa_phandle, compatible) >= 0;
}

static void
plcom_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct plcom_fdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	int phandle = faa->faa_phandle;
	int error;

	sc->sc.sc_dev = self;
	sc->sc.sc_hwflags = PLCOM_HW_TXFIFO_DISABLE;
	sc->sc.sc_swflags = 0;

	sc->sc_uartclk = fdtbus_clock_get(phandle, "uartclk");
	if (sc->sc_uartclk == NULL) {
		aprint_error(": couldn't get uart clk\n");
		return;
	}
	sc->sc_apbpclk = fdtbus_clock_get(phandle, "apb_pclk");
	if (sc->sc_apbpclk == NULL) {
		aprint_error(": couldn't get apb pclk\n");
		return;
	}

	sc->sc.sc_pi.pi_type = PLCOM_TYPE_PL011;
	sc->sc.sc_pi.pi_flags = PLC_FLAG_32BIT_ACCESS;
	sc->sc.sc_pi.pi_iot = faa->faa_bst;
	error = fdtbus_get_reg(phandle, 0, &sc->sc.sc_pi.pi_iobase,
	    &sc->sc.sc_pi.pi_size);
	if (error != 0) {
		aprint_error(": failed to get registers\n");
		return;
	}
	error = bus_space_map(faa->faa_bst, sc->sc.sc_pi.pi_iobase,
	    sc->sc.sc_pi.pi_size, 0, &sc->sc.sc_pi.pi_ioh);
	if (error != 0) {
		aprint_error(": failed to map registers\n");
		return;
	}

	error = clk_enable(sc->sc_uartclk);
	if (error != 0) {
		aprint_error(": couldn't enable uart clk\n");
		return;
	}
	error = clk_enable(sc->sc_apbpclk);
	if (error != 0) {
		aprint_error(": couldn't enable apb pclk\n");
		return;
	}
	sc->sc.sc_frequency = clk_get_rate(sc->sc_uartclk);

	plcom_attach_subr(&sc->sc);

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_SERIAL,
	    FDT_INTR_MPSAFE, plcomintr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		return;
	}
}
