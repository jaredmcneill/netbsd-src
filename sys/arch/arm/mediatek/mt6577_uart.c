/* $NetBSD$ */

/*-
 * Copyright (c) 2018, 2019 Jason R. Thorpe
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <arm/mediatek/mt6577_uartreg.h>

#include <dev/fdt/fdtvar.h>

#include <dev/ic/comvar.h>

/*
 * TODO:
 * - High-speed modes (> 115200)
 * - Power management (disable clocks when port not in use)
 * - DMA mode
 */

static int	mt6577_uart_match(device_t, cfdata_t, void *);
static void	mt6577_uart_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mt6577com, sizeof(struct com_softc),
    mt6577_uart_match, mt6577_uart_attach, NULL, NULL);

static const char *const compatible[] = {
	"mediatek,mt6577-uart",
	NULL
};

static const bus_size_t mt6577_uart_regmap[COM_REGMAP_NENTRIES] = {
	[COM_REG_RXDATA]	=	MTK_UART_RBR,
	[COM_REG_TXDATA]	=	MTK_UART_THR,
	[COM_REG_DLBL]		=	MTK_UART_DLL,
	[COM_REG_DLBH]		=	MTK_UART_DLM,
	[COM_REG_IER]		=	MTK_UART_IER,
	[COM_REG_IIR]		=	MTK_UART_IIR,
	[COM_REG_FIFO]		=	MTK_UART_FCR,
	[COM_REG_TCR]		=	MTK_UART_FCR,
	[COM_REG_LCR]		=	MTK_UART_LCR,
	[COM_REG_MCR]		=	MTK_UART_MCR,
	[COM_REG_LSR]		=	MTK_UART_LSR,
	[COM_REG_MSR]		=	MTK_UART_MSR,
};

static void
mt6577_uart_init_regs(struct com_regs *regsp, bus_space_tag_t bst,
    bus_space_handle_t bsh, bus_addr_t addr, bus_size_t size)
{

	com_init_regs(regsp, bst, bsh, addr);
	memcpy(regsp->cr_map, mt6577_uart_regmap, sizeof(regsp->cr_map));

	regsp->cr_nports = size;
}

static int
mt6577_uart_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mt6577_uart_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;
	void *ih;

	sc->sc_dev = self;
	sc->sc_type = COM_TYPE_MEDIATEK;

	int error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": unable to get device address\n");
		return;
	}

	if (com_is_console(bst, addr, &bsh) == 0 &&
	    bus_space_map(bst, addr, size, 0, &bsh) != 0) {
		aprint_error(": unable to map device\n");
		return;
	}

	/* Enable clocks. */
	struct clk *clk;
	for (int i = 0; (clk = fdtbus_clock_get_index(phandle, i)); i++) {
		if (clk_enable(clk) != 0) {
			aprint_error(": failed to enable clock #%d\n", i);
			return;
		}
		/* First clock is the baud clock. */
		/* XXX Use clock-names property instead? */
		if (i == 0)
			sc->sc_frequency = clk_get_rate(clk);
	}

	mt6577_uart_init_regs(&sc->sc_regs, bst, bsh, addr, size);

	/* Disable Fix Rate. */
	bus_space_write_4(bst, bsh, MTK_UART_RATE_FIX, 0);

	com_attach_subr(sc);
	aprint_naive("\n");

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	ih = fdtbus_intr_establish(phandle, 0, IPL_SERIAL, FDT_INTR_MPSAFE,
	    comintr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

static int
mt6577_uart_console_match(int phandle)
{

	return of_match_compatible(phandle, compatible);
}

static void
mt6577_uart_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	struct com_regs regs;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;
	tcflag_t flags;
	int error, speed;

	fdtbus_get_reg(phandle, 0, &addr, &size);
	speed = fdtbus_get_stdout_speed();
	if (speed < 0)
		speed = 115200; /* default */
	flags = fdtbus_get_stdout_flags();

	if ((error = bus_space_map(bst, addr, size, 0, &bsh)) != 0) {
		panic("cannot map MT6577 UART console");
	}

	mt6577_uart_init_regs(&regs, bst, bsh, addr, size);

	if (comcnattach1(&regs, speed, uart_freq, COM_TYPE_MEDIATEK, flags)) {
		panic("cannot attach MT6577 UART console");
	}

	cn_set_magic("+++++");
}

static const struct fdt_console mt6577_uart_console = {
	.match = mt6577_uart_console_match,
	.consinit = mt6577_uart_console_consinit,
};

FDT_CONSOLE(mt6577_uart, &mt6577_uart_console);
