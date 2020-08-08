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

#include "opt_soc.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/gpio.h>
#include <sys/bitops.h>
#include <sys/lwp.h>

#include <dev/fdt/fdtvar.h>

#include <arm/mediatek/mtk_eintc.h>

static inline const struct mtk_eintc_reg_group *
mtk_eintc_get_regs(const struct mtk_eintc_softc * const sc, const u_int which)
{
	KASSERT(which < MTK_EINTC_NREGS);
	return &sc->sc_eintc->reg_groups[which];
}

void
mtk_eintc_init(struct mtk_eintc_softc * const sc)
{

	KASSERT(sc->sc_dev != NULL);
	KASSERT(sc->sc_eintc != NULL);

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_VM);
	sc->sc_intrs = kmem_zalloc(sizeof(*sc->sc_intrs) * sc->sc_eintc->nintrs,
				   KM_SLEEP);

	for (u_int irq = 0; irq < sc->sc_eintc->nintrs; irq++) {
		sc->sc_intrs[irq].intr_debounce = -1;	/* not set */
	}

	/*
	 * Make sure all of the interrupts are masked and disabled before
	 * our container registers with the interrupt parent.
	 */

	const struct mtk_eintc_reg_group * const mask_regs =
	    mtk_eintc_get_regs(sc, MTK_EINTC_REGS_MASK);

	const uint32_t mask = __BITS(0, sc->sc_eintc->irqs_per_bank - 1);

	for (u_int bank = 0; bank < sc->sc_eintc->nbanks; bank++) {
		EINTC_WRITE(sc, mask_regs->set_base +
		    MTK_EINT_REG_BANK(bank), mask);
		EINTC_WRITE(sc, sc->sc_eintc->domen_base +
		    MTK_EINT_REG_BANK(bank), 0);
	}
}

static inline int
mtk_eintc_process_intrs(struct mtk_eintc_softc * const sc, const u_int bank)
{
	const struct mtk_eintc_reg_group * const sta_regs =
	    mtk_eintc_get_regs(sc, MTK_EINTC_REGS_STA_ACK);
	struct mtk_eintc_intr *intr;
	uint32_t status, pending, bit;
	uint32_t clear_level;
	int (*func)(void *);
	int rv = 0;
	const u_int irqoff = bank * sc->sc_eintc->irqs_per_bank;

	for (;;) {
		status = pending =
		    EINTC_READ(sc, sta_regs->base + MTK_EINT_REG_BANK(bank));
		if (status == 0)
			return rv;

		/*
		 * This will clear the indicator for any pending
		 * edge-triggered pins, but level-triggered pins
		 * sill still be indicated until the pin is
		 * de-asserted.  We'll have to clear level-triggered
		 * indicators below.
		 */
		EINTC_WRITE(sc, sta_regs->clr_base + MTK_EINT_REG_BANK(bank),
		    status);
		clear_level = 0;

		while ((bit = ffs32(pending)) != 0) {
			pending &= ~__BIT(bit - 1);
			intr = &sc->sc_intrs[irqoff + (bit - 1)];
			if ((func = intr->intr_func) == NULL)
				continue;
			if (intr->intr_type & (FDT_INTR_TYPE_HIGH_LEVEL |
					       FDT_INTR_TYPE_LOW_LEVEL))
				clear_level |= __BIT(bit - 1);
			const bool mpsafe =
			    (intr->intr_flags & MTK_EINTC_INTR_MPSAFE) != 0;
			if (!mpsafe)
				KERNEL_LOCK(1, curlwp);
			rv |= (*func)(intr->intr_arg);
			if (!mpsafe)
				KERNEL_UNLOCK_ONE(curlwp);
		}

		/*
		 * Now that all of the handlers have been called,
		 * we can clear the indicators for any level-triggered
		 * pins.
		 */
		if (clear_level)
			EINTC_WRITE(sc, sta_regs->clr_base +
			    MTK_EINT_REG_BANK(bank), clear_level);

	}

	return rv;
}

int
mtk_eintc_intr(void *arg)
{
	struct mtk_eintc_softc * const sc = arg;
	int rv = 0;

	mutex_enter(&sc->sc_mutex);

	for (u_int bank = 0; bank < sc->sc_eintc->nbanks; bank++) {
		rv |= mtk_eintc_process_intrs(sc, bank);
	}

	mutex_exit(&sc->sc_mutex);

	return rv;
}

void *
mtk_eintc_intr_enable(struct mtk_eintc_softc * const sc,
		      int (*func)(void *), void *arg, const int ipl,
		      const u_int irq, const u_int type, const u_int flags)
{
	const u_int bank = irq / sc->sc_eintc->irqs_per_bank;
	const u_int bit = irq & sc->sc_eintc->irqs_per_bank;

	if (irq > sc->sc_eintc->nintrs)
		return NULL;
	KASSERT(bank < sc->sc_eintc->nbanks);

	if (ipl != IPL_VM) {
		aprint_error_dev(sc->sc_dev, "%s: wrong IPL %d (expected %d)\n",
				 __func__, ipl, IPL_VM);
		return NULL;
	}

	const struct mtk_eintc_reg_group * const mask_regs =
	    mtk_eintc_get_regs(sc, MTK_EINTC_REGS_MASK);
	const struct mtk_eintc_reg_group * const sens_regs =
	    mtk_eintc_get_regs(sc, MTK_EINTC_REGS_SENS);
	const struct mtk_eintc_reg_group * const pol_regs =
	    mtk_eintc_get_regs(sc, MTK_EINTC_REGS_POL);

	mutex_enter(&sc->sc_mutex);

	struct mtk_eintc_intr * const intr = &sc->sc_intrs[irq];

	if (intr->intr_func != NULL) {
		mutex_exit(&sc->sc_mutex);
		return NULL;	/* in use */
	}

	const uint32_t mask = __BIT(bit);

	switch (type) {
	case FDT_INTR_TYPE_POS_EDGE:	/* sens=0, pol=0 */
		EINTC_WRITE(sc, sens_regs->clr_base +
		    MTK_EINT_REG_BANK(bank), mask);
		EINTC_WRITE(sc, pol_regs->clr_base +
		    MTK_EINT_REG_BANK(bank), mask);
		break;

	case FDT_INTR_TYPE_NEG_EDGE:	/* sens=0, pol=1 */
		EINTC_WRITE(sc, sens_regs->clr_base +
		    MTK_EINT_REG_BANK(bank), mask);
		EINTC_WRITE(sc, pol_regs->set_base +
		    MTK_EINT_REG_BANK(bank), mask);
		break;

#if 0 /* catch this in the "default" case below. */
	case FDT_INTR_TYPE_DOUBLE_EDGE:
		/*
		 * Hardware does not support this natively, but it can
		 * be emulated by flipping the polarity of the irq on
		 * each interrupt.
		 * XXX Not now.
		 */
		mutex_exit(&sc->sc_mutex);
		return NULL;
#endif

	case FDT_INTR_TYPE_HIGH_LEVEL:	/* sens=1, pol=0 */
		EINTC_WRITE(sc, sens_regs->set_base +
		    MTK_EINT_REG_BANK(bank), mask);
		EINTC_WRITE(sc, pol_regs->clr_base +
		    MTK_EINT_REG_BANK(bank), mask);
		break;

	case FDT_INTR_TYPE_LOW_LEVEL:	/* sens=1, pol=1 */
		EINTC_WRITE(sc, sens_regs->set_base +
		    MTK_EINT_REG_BANK(bank), mask);
		EINTC_WRITE(sc, pol_regs->set_base +
		    MTK_EINT_REG_BANK(bank), mask);
		break;

	default:
		aprint_error_dev(sc->sc_dev,
				 "%s: unsupported intr type 0x%x\n",
				 __func__, type);
		mutex_exit(&sc->sc_mutex);
		return NULL;
	}

	intr->intr_func = func;
	intr->intr_arg = arg;
	intr->intr_type = type;
	intr->intr_flags = flags;

	if (intr->intr_debounce != -1) {
		/* XXXJRT program debounce register */
	}

	uint32_t dom_en = EINTC_READ(sc, sc->sc_eintc->domen_base +
	    MTK_EINT_REG_BANK(bank));
	dom_en |= mask;
	EINTC_WRITE(sc, sc->sc_eintc->domen_base +
	    MTK_EINT_REG_BANK(bank), dom_en);

	EINTC_WRITE(sc, mask_regs->clr_base + MTK_EINT_REG_BANK(bank), mask);

	mutex_exit(&sc->sc_mutex);
	return (intr);
}

static u_int
mtk_eintc_intr_to_irq(struct mtk_eintc_softc * const sc,
		      struct mtk_eintc_intr * const intr)
{
	KASSERT((uintptr_t)intr >= (uintptr_t)&sc->sc_intrs[0]);
	KASSERT((uintptr_t)intr < (uintptr_t)&sc->sc_intrs[sc->sc_eintc->nintrs]);
	return (u_int)((uintptr_t)(intr - sc->sc_intrs));
}

void
mtk_eintc_intr_disable(struct mtk_eintc_softc * const sc,
		       struct mtk_eintc_intr * const intr)
{
	const u_int irq = mtk_eintc_intr_to_irq(sc, intr);
	const u_int bank = irq / sc->sc_eintc->irqs_per_bank;
	const u_int bit = irq & sc->sc_eintc->irqs_per_bank;
	const uint32_t mask = __BIT(bit);

	const struct mtk_eintc_reg_group * const mask_regs =
	    mtk_eintc_get_regs(sc, MTK_EINTC_REGS_MASK);

	KASSERT(intr->intr_func != NULL);

	mutex_enter(&sc->sc_mutex);

	EINTC_WRITE(sc, mask_regs->set_base + MTK_EINT_REG_BANK(bank), mask);

	uint32_t dom_en = EINTC_READ(sc, sc->sc_eintc->domen_base +
	    MTK_EINT_REG_BANK(bank));
	dom_en &= ~mask;
	EINTC_WRITE(sc, sc->sc_eintc->domen_base +
	    MTK_EINT_REG_BANK(bank), dom_en);
	
	intr->intr_func = NULL;
	intr->intr_arg = NULL;
	intr->intr_type = 0;
	intr->intr_flags = 0;

	mutex_exit(&sc->sc_mutex);
}
