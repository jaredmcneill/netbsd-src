/* $NetBSD: gtmr_fdt.c,v 1.7 2017/11/30 14:51:01 skrll Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gtmr_fdt.c,v 1.7 2017/11/30 14:51:01 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <arm/cortex/gtmr_intr.h>
#include <arm/cortex/mpcore_var.h>
#include <arm/cortex/gtmr_var.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

static int	gtmr_fdt_match(device_t, cfdata_t, void *);
static void	gtmr_fdt_attach(device_t, device_t, void *);

static int	gtmr_fdt_select_timer(const int, char *, size_t);

static void	gtmr_fdt_cpu_hatch(void *, struct cpu_info *);

CFATTACH_DECL_NEW(gtmr_fdt, 0, gtmr_fdt_match, gtmr_fdt_attach, NULL, NULL);

/* Interrupt resources */
#define	GTMR_NONE		-1	/* Not selected */
#define	GTMR_PHYS		0	/* Physical, secure */
#define	GTMR_PHYS_NS		1	/* Physical, non-secure */
#define GTMR_VIRT		2	/* Virtual */
#define	GTMR_HYP		3	/* Hypervisor */

static int
gtmr_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"arm,armv7-timer",
		"arm,armv8-timer",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_compatible(faa->faa_phandle, compatible) >= 0;
}

static void
gtmr_fdt_attach(device_t parent, device_t self, void *aux)
{
	prop_dictionary_t dict = device_private(self);

	aprint_naive("\n");
	aprint_normal(": Generic Timer\n");

	struct mpcore_attach_args mpcaa = {
		.mpcaa_name = "armgtmr",
		.mpcaa_irq = -1		/* setup handler locally */
	};
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	char intrstr[128];

	const int index = gtmr_fdt_select_timer(phandle, intrstr, sizeof(intrstr));
	if (index == GTMR_NONE) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	if (index == GTMR_PHYS || index == GTMR_PHYS_NS)
		prop_dictionary_set_bool(dict, "physical", true);

	u_int freq;
	if (of_getprop_uint32(phandle, "clock-frequency", &freq) == 0)
		prop_dictionary_set_uint32(dict, "frequency", freq);

	void *ih = fdtbus_intr_establish(phandle, index, IPL_CLOCK,
	    FDT_INTR_MPSAFE, gtmr_intr, NULL);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't install interrupt handler\n");
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	config_found(self, &mpcaa, NULL);

	arm_fdt_cpu_hatch_register(self, gtmr_fdt_cpu_hatch);
	arm_fdt_timer_register(gtmr_cpu_initclocks);
}

static int
gtmr_fdt_select_timer(const int phandle, char *intrstr, size_t len)
{
	int index = 0;

#ifdef __arm__
	const int timer_sel[] = { GTMR_VIRT, GTMR_PHYS, GTMR_NONE };

	/*
	 * For 32-bit Arm, if we can not guarantee that firmware has
	 * configured the timer just use the physical secure timer.
	 */
	if (of_hasprop(phandle, "arm,cpu-registers-not-fw-configured"))
		index++;
#else
	const int timer_sel[] = { GTMR_VIRT, GTMR_PHYS_NS, GTMR_NONE };
#endif

	for (; timer_sel[index] != GTMR_NONE; index++) {
		/* If we can decode this interrupt, we are done. */
		if (fdtbus_intr_str(phandle, index, intrstr, len))
			break;
	}

	return timer_sel[index];
}

static void
gtmr_fdt_cpu_hatch(void *priv, struct cpu_info *ci)
{
	gtmr_init_cpu_clock(ci);
}
