/* $NetBSD$ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#define	__INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>

#include <machine/locore.h>	/* mips3_cp0_*() */
#include <machine/intr.h>

#include <playstation2/playstation2/interrupt.h>

#include <playstation2/ee/eevar.h>
#include <playstation2/ee/intcvar.h>
#include <playstation2/ee/intcreg.h>
#include <playstation2/ee/dmacreg.h>
#include <playstation2/ee/dmacvar.h>
#include <playstation2/ee/timervar.h>

/*
 * SPL support
 */

#define ee_splhigh_noprof	ee_splhigh
#define	ee_splx_noprof		ee_splx

extern volatile uint32_t md_imask;
extern uint32_t __icu_mask[_IPL_N];

static uint32_t ee_softint_ipending = 0;

static int ee_intr_status = -1;

static int
ee_md_splraise(int npl)
{
	struct cpu_info *ci = curcpu();
	int s, opl;

	opl = ci->ci_cpl;

	KASSERT(npl < _IPL_N);

	if (opl < npl) {
		s = _intr_suspend();
		md_imask = __icu_mask[npl];
		md_imask_update();
		ci->ci_cpl = npl;
		if (npl == IPL_HIGH)
			ee_intr_status = s;
		else
			_intr_resume(s);
	}

	return opl;
}

static int
ee_splhigh(void)
{
	return ee_md_splraise(IPL_HIGH);
}

static int
ee_splsched(void)
{
	return ee_md_splraise(IPL_SCHED);
}

static int
ee_splvm(void)
{
	return ee_md_splraise(IPL_VM);
}

static int
ee_splsoftserial(void)
{
	return ee_md_splraise(IPL_SOFTSERIAL);
}

static int
ee_splsoftnet(void)
{
	return ee_md_splraise(IPL_SOFTNET);
}

static int
ee_splsoftbio(void)
{
	return ee_md_splraise(IPL_SOFTBIO);
}

static int
ee_splsoftclock(void)
{
	return ee_md_splraise(IPL_SOFTCLOCK);
}

static int
ee_splraise(int npl)
{
	return ee_md_splraise(npl);
}

static void
ee_splx(int npl)
{
	struct cpu_info *ci = curcpu();
	int s, opl;

	opl = ci->ci_cpl;
	if (npl == opl)
		return;

	if (opl == IPL_HIGH)
		s = ee_intr_status;
	else
		s = _intr_suspend();
	md_imask = __icu_mask[npl];
	md_imask_update();
	ci->ci_cpl = npl;

	if (ee_softint_ipending)
		softint_process(ee_softint_ipending);

	_intr_resume(s);
}

static void
ee_spl0(void)
{
	ee_splx(IPL_NONE);
}

static void
ee__setsoftintr(uint32_t ipending)
{
	ee_softint_ipending |= ipending;
}

static void
ee__clrsoftintr(uint32_t ipending)
{
	ee_softint_ipending &= ~ipending;
}

static int
ee_splintr(uint32_t *pipending)
{
	const uint32_t cause = mips_cp0_cause_read();

	if ((cause & MIPS_HARD_INT_MASK) == 0)
		return IPL_NONE;

	*pipending = (cause & MIPS_INT_MASK);
	return IPL_HIGH;	/* XXX */
}

static void
ee_splcheck(void)
{
}

struct splsw ee_splsw = {
	.splsw_splhigh		= ee_splhigh,
	.splsw_splsched		= ee_splsched,
	.splsw_splvm		= ee_splvm,
	.splsw_splsoftserial	= ee_splsoftserial,
	.splsw_splsoftnet	= ee_splsoftnet,
	.splsw_splsoftbio	= ee_splsoftbio,
	.splsw_splsoftclock	= ee_splsoftclock,
	.splsw_splraise		= ee_splraise,
	.splsw_spl0		= ee_spl0,
	.splsw_splx		= ee_splx,
	.splsw_splhigh_noprof	= ee_splhigh_noprof,
	.splsw_splx_noprof	= ee_splx_noprof,
	.splsw__setsoftintr	= ee__setsoftintr,
	.splsw__clrsoftintr	= ee__clrsoftintr,
	.splsw_splintr		= ee_splintr,
	.splsw_splcheck		= ee_splcheck,
};
