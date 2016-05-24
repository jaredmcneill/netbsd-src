/*	$NetBSD: gtmr.c,v 1.16 2015/04/20 20:19:52 matt Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gtmr.c,v 1.16 2015/04/20 20:19:52 matt Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/percpu.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <prop/proplib.h>

#include <arm/locore.h>

#include <arm/cortex/gtmr_var.h>

#include <arm/cortex/mpcore_var.h>

static int gtmr_match(device_t, cfdata_t, void *);
static void gtmr_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(armgtmr_armperiph, 0, gtmr_match, gtmr_attach, NULL, NULL);

extern struct gtmr_softc gtmr_sc;

/* ARGSUSED */
static int
gtmr_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mpcore_attach_args * const mpcaa = aux;

	if (gtmr_sc.sc_dev != NULL)
		return 0;

	if ((armreg_pfr1_read() & ARM_PFR1_GTIMER_MASK) == 0)
		return 0;

	if (strcmp(mpcaa->mpcaa_name, cf->cf_name) != 0)
		return 0;

	return 1;
}

static void
gtmr_attach(device_t parent, device_t self, void *aux)
{
	struct mpcore_attach_args * const mpcaa = aux;
	struct gtmr_softc *sc = &gtmr_sc;
	prop_dictionary_t dict = device_properties(self);

	/*
	 * This runs at a fixed frequency of 1 to 50MHz.
	 */
	prop_dictionary_get_uint32(dict, "frequency", &sc->sc_freq);
	KASSERT(sc->sc_freq != 0);

	sc->sc_global_ih = intr_establish(mpcaa->mpcaa_irq, IPL_CLOCK,
	    IST_LEVEL | IST_MPSAFE, gtmr_intr, NULL);
	if (sc->sc_global_ih == NULL)
		panic("%s: unable to register timer interrupt", __func__);

	gtmr_init(self);

	aprint_normal_dev(self, "interrupting on irq %d\n",
	    mpcaa->mpcaa_irq);
}
