/* $NetBSD$ */

/*-
 * Copyright (c) 2019 Jason R. Thorpe
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

#include <arm/mediatek/mtk_cru.h>

int
mtk_cru_clk_gate_enable(struct mtk_cru_softc *sc, struct mtk_cru_clk *clk,
			int enable)
{
	const bus_size_t *regs;
	uint32_t mask;
	u_int which;
	u_int flags;

	KASSERT(clk->type == MTK_CLK_GATE ||
		clk->type == MTK_CLK_MUXGATE);

	if (clk->type == MTK_CLK_GATE) {
		regs = clk->u.gate.regs;
		mask = clk->u.gate.mask;
		flags = clk->u.gate.flags;
	} else {
		regs = clk->u.muxgate.regs;
		mask = clk->u.muxgate.mask;
		flags = clk->u.muxgate.flags;
	}

	if (flags & MTK_CLK_GATE_ACT_LOW)
		which = enable ? MTK_CLK_GATE_REG_CLR : MTK_CLK_GATE_REG_SET;
	else
		which = enable ? MTK_CLK_GATE_REG_SET : MTK_CLK_GATE_REG_CLR;

	/*
	 * If we have separate SET and CLR registers, use them.
	 * We don't have to take the mutex in this case.
	 */
	if (regs[MTK_CLK_GATE_REG_SET] != regs[MTK_CLK_GATE_REG_CLR]) {
		CRU_WRITE(sc, regs[which], mask);
	} else {
		mutex_enter(&sc->sc_mutex);
		uint32_t val = CRU_READ(sc, regs[which]);
		if (which == MTK_CLK_GATE_REG_CLR)
			val &= ~mask;
		else
			val |= mask;
		CRU_WRITE(sc, regs[which], val);
		mutex_exit(&sc->sc_mutex);
	}

	return 0;
}

const char *
mtk_cru_clk_gate_get_parent(struct mtk_cru_softc *sc,
    struct mtk_cru_clk *clk)
{

	KASSERT(clk->type == MTK_CLK_GATE);

	return clk->u.gate.parent;
}
