/* $NetBSD$ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_soc.h"
#include "opt_multiprocessor.h"
#include "opt_console.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: meson_platform.c,v 1.34 2019/01/03 14:44:21 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>
#include <arm/cpufunc.h>

#include <arm/cortex/a9tmr_var.h>

#include <arm/amlogic/meson_uart.h>

#include <arch/evbarm/fdt/platform.h>

#include <libfdt.h>

#define	MESON_CORE_VBASE	KERNEL_IO_VBASE
#define	MESON_CORE_PBASE	0xc0000000
#define	MESON_CORE_SIZE		0x10000000

#define	MESON_WATCHDOG_BASE	0xc1109900
#define	MESON_WATCHDOG_SIZE	0x8
#define	 MESON_WATCHDOG_TC	0x00
#define	  WATCHDOG_TC_CPUS	__BITS(27,24)
#define	  WATCHDOG_TC_ENABLE	__BIT(19)
#define	  WATCHDOG_TC_TCNT	__BITS(15,0)
#define	 MESON_WATCHDOG_RESET	0x04
#define	  WATCHDOG_RESET_COUNT	__BITS(15,0)

extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern struct bus_space arm_generic_bs_tag;
extern struct bus_space arm_generic_a4x_bs_tag;

#define	meson_dma_tag		arm_generic_dma_tag
#define	meson_bs_tag		arm_generic_bs_tag
#define	meson_a4x_bs_tag	arm_generic_a4x_bs_tag

static const struct pmap_devmap *
meson_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(MESON_CORE_VBASE,
			     MESON_CORE_PBASE,
			     MESON_CORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
meson_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &meson_bs_tag;
	faa->faa_a4x_bst = &meson_a4x_bs_tag;
	faa->faa_dmat = &meson_dma_tag;
}

void meson_platform_early_putchar(char);

void
meson_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	((CONSADDR - MESON_CORE_PBASE) + MESON_CORE_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;
	int timeo = 150000;

	while ((uartaddr[UART_STATUS_REG/4] & UART_STATUS_TX_EMPTY) == 0) {
		if (--timo == 0)
			break;
	}

	uartaddr[UART_WFIFO_REG/4] = c;

	while ((uartaddr[UART_STATUS_REG/4] & UART_STATUS_TX_EMPTY) == 0) {
		if (--timo == 0)
			break;
	}
#endif
}

static void
meson_platform_device_register(device_t self, void *aux)
{
}

static u_int
meson_platform_uart_freq(void)
{
	return 0;
}

static void
meson_platform_bootstrap(void)
{
	arm_fdt_cpu_bootstrap();

	void *fdt_data = __UNCONST(fdtbus_get_data());
	const int chosen_off = fdt_path_offset(fdt_data, "/chosen");
	if (chosen_off < 0)
		return;

	if (match_bootconf_option(boot_args, "console", "fb")) {
		const int framebuffer_off =
		    fdt_path_offset(fdt_data, "/chosen/framebuffer");
		if (framebuffer_off >= 0) {
			const char *status = fdt_getprop(fdt_data,
			    framebuffer_off, "status", NULL);
			if (status == NULL || strncmp(status, "ok", 2) == 0) {
				fdt_setprop_string(fdt_data, chosen_off,
				    "stdout-path", "/chosen/framebuffer");
			}
		}
	} else if (match_bootconf_option(boot_args, "console", "serial")) {
		fdt_setprop_string(fdt_data, chosen_off,
		    "stdout-path", "serial0:115200n8");
	}
}

static void
meson_platform_reset(void)
{
	bus_space_tag_t bst = &meson_bs_tag;
	bus_space_handle_t bsh;

	bus_space_map(bst, MESON_WATCHDOG_BASE, MESON_WATCHDOG_SIZE, 0, &bsh);

	bus_space_write_4(bst, bsh, MESON_WATCHDOG_TC, 
	    WATCHDOG_TC_CPUS | WATCHDOG_TC_ENABLE | __SHIFTIN(0xfff, WATCHDOG_TC_TCNT));
	bus_space_write_4(bst, bsh, MESON_WATCHDOG_RESET, 0);

	for (;;) {
		__asm("wfi");
	}
}

#if defined(SOC_MESON8B)
static const struct arm_platform meson8b_platform = {
	.ap_devmap = meson_platform_devmap,
	.ap_bootstrap = meson_platform_bootstrap,
	.ap_init_attach_args = meson_platform_init_attach_args,
	.ap_device_register = meson_platform_device_register,
	.ap_reset = meson_platform_reset,
	.ap_delay = a9tmr_delay,
	.ap_uart_freq = meson_platform_uart_freq,
};

ARM_PLATFORM(meson8b, "amlogic,meson8b", &meson8b_platform);
#endif
