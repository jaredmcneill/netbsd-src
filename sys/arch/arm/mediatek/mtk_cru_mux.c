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
mtk_cru_clk_mux_set_parent(struct mtk_cru_softc *sc,
    struct mtk_cru_clk *clk, const char *name)
{
	const char **parents;
	u_int nparents;
	uint32_t sel;
	uint32_t reg;
	uint32_t val;
	u_int index;

	KASSERT(clk->type == MTK_CLK_MUX ||
		clk->type == MTK_CLK_MUXGATE);

	if (clk->type == MTK_CLK_MUX) {
		parents = clk->u.mux.parents;
		nparents = clk->u.mux.nparents;
		sel = clk->u.mux.sel;
		reg = clk->u.mux.reg;
	} else {
		parents = clk->u.muxgate.parents;
		nparents = clk->u.muxgate.nparents;
		sel = clk->u.muxgate.sel;
		reg = clk->u.muxgate.regs[MTK_CLK_GATE_REG_STA];
	}

	if (sel == 0)
		return ENODEV;
	
	for (index = 0; index < nparents; index++) {
		if (parents[index] != NULL &&
		    strcmp(parents[index], name) == 0)
		    	break;
	}
	if (index == nparents)
		return EINVAL;

	mutex_enter(&sc->sc_mutex);
	val = CRU_READ(sc, reg);
	val &= ~sel;
	val |= __SHIFTIN(index, sel);
	CRU_WRITE(sc, reg, val);
	mutex_exit(&sc->sc_mutex);

	return 0;
}

const char *
mtk_cru_clk_mux_get_parent(struct mtk_cru_softc *sc,
    struct mtk_cru_clk *clk)
{
	const char **parents;
	u_int nparents;
	uint32_t sel;
	uint32_t reg;
	uint32_t val;
	u_int index;

	KASSERT(clk->type == MTK_CLK_MUX ||
		clk->type == MTK_CLK_MUXGATE);

	if (clk->type == MTK_CLK_MUX) {
		parents = clk->u.mux.parents;
		nparents = clk->u.mux.nparents;
		sel = clk->u.mux.sel;
		reg = clk->u.mux.reg;
	} else {
		parents = clk->u.muxgate.parents;
		nparents = clk->u.muxgate.nparents;
		sel = clk->u.muxgate.sel;
		reg = clk->u.muxgate.regs[MTK_CLK_GATE_REG_STA];
	}


	val = CRU_READ(sc, reg);
	index = __SHIFTOUT(val, sel);

	if (index >= nparents)
		return NULL;

	return parents[index];
}
