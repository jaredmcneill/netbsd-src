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

#include "opt_soc.h"
#include "opt_multiprocessor.h"
#include "opt_console.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>
#include <arm/cortex/mpcore_var.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>
#include <arm/cpufunc.h>

#include <arm/cortex/gtmr_var.h>
#include <arm/cortex/gic_reg.h>

#include <dev/ic/comreg.h>

#include <arm/mediatek/mediatek_platform.h>
#include <arm/mediatek/mtk_mc_smp.h>
#include <arm/mediatek/mt6589_toprgureg.h>
#include <arm/mediatek/mt6577_uartreg.h>

#include <libfdt.h>

#define	MEDIATEK_UART_FREQ	26000000	/* XXX */

extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern struct bus_space arm_generic_bs_tag;
extern struct bus_space arm_generic_a4x_bs_tag;

#define	mediatek_dma_tag	arm_generic_dma_tag
#define	mediatek_bs_tag		arm_generic_bs_tag
#define	mediatek_a4x_bs_tag	arm_generic_a4x_bs_tag

static const struct pmap_devmap *
mediatek_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(MEDIATEK_CORE_VBASE,
			     MEDIATEK_CORE_PBASE,
			     MEDIATEK_CORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
mediatek_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &mediatek_bs_tag;
	faa->faa_a4x_bst = &mediatek_a4x_bs_tag;
	faa->faa_dmat = &mediatek_dma_tag;
}

void mediatek_platform_early_putchar(char);

void
mediatek_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	((CONSADDR - MEDIATEK_CORE_PBASE) + MEDIATEK_CORE_VBASE)
	uintptr_t uart_base = cpu_earlydevice_va_p() ?
	    CONSADDR_VA : CONSADDR;

	volatile uint8_t *uart_lsr =
	    (volatile uint8_t *)(uart_base + MTK_UART_LSR);
	volatile uint8_t *uart_thr =
	    (volatile uint8_t *)(uart_base + MTK_UART_THR);

	while ((*uart_lsr & LSR_TXRDY) == 0)
		;

	*uart_thr = c;
#endif
}

static void
mediatek_platform_device_register(device_t self, void *aux)
{
	prop_dictionary_t prop = device_properties(self);

	/*
	 * XXX These are generic DT properties, not SoC quirks.  Should
	 * XXX have a generic arm_fdt_device_register() to call.
	 */
	if (device_is_a(self, "armgtmr")) {
		const struct mpcore_attach_args *mpcaa = aux;

		uint32_t frequency;
		if (of_getprop_uint32((int)mpcaa->mpcaa_handle,
		    "clock-frequency", &frequency) == 0) {
			prop_dictionary_set_uint32(prop, "frequency",
			    frequency);
		}

		if (of_hasprop((int)mpcaa->mpcaa_handle,
		    "arm,cpu-registers-not-fw-configured")) {
			prop_dictionary_set_bool(prop, "force-phys-secure-ppi",
			    true);
		}
	}
}

static u_int
mediatek_platform_uart_freq(void)
{
	return MEDIATEK_UART_FREQ;
}

static void
mediatek_platform_bootstrap(void)
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

#define	MT7623_TOPRGU_VADDR	\
	((MT7623_TOPRGU_PADDR - MEDIATEK_CORE_PBASE) + MEDIATEK_CORE_VBASE)

static void
mediatek_platform_reset(void)
{
	uintptr_t wdt_swrst = (cpu_earlydevice_va_p() ?
	    MT7623_TOPRGU_VADDR : MT7623_TOPRGU_PADDR) + WDT_SWRST;

	for (;;) {
		*(volatile uint32_t *)wdt_swrst = WDT_SWRST_UNLOCK_KEY;
	}
}

static const struct arm_platform mt7623_platform = {
	.ap_devmap = mediatek_platform_devmap,
	.ap_bootstrap = mediatek_platform_bootstrap,
	.ap_init_attach_args = mediatek_platform_init_attach_args,
	.ap_device_register = mediatek_platform_device_register,
	.ap_reset = mediatek_platform_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = mediatek_platform_uart_freq,
	.ap_mpstart = mtk_mt7623_mpstart,
};

ARM_PLATFORM(mt7623, "mediatek,mt7623", &mt7623_platform);
