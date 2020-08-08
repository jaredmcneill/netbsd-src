/* $NetBSD$ */

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
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/bus.h>

#include <arm/mediatek/mtk_cru.h>

u_int
mtk_cru_clk_div_get_rate(struct mtk_cru_softc *sc,
    struct mtk_cru_clk *clk)
{
	struct mtk_cru_clk_div *div = &clk->u.div;
	struct clk *clkp, *clkp_parent;
	u_int rate, ratio;
	uint32_t val;

	KASSERT(clk->type == MTK_CLK_DIV);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	rate = clk_get_rate(clkp_parent);
	if (rate == 0)
		return 0;

	val = CRU_READ(sc, div->reg);
	if (div->div)
		ratio = __SHIFTOUT(val, div->div);
	else
		ratio = 0;

	if ((div->flags & MTK_CLK_DIV_ZERO_IS_ONE) != 0 && ratio == 0)
		ratio = 1;
	if (div->flags & MTK_CLK_DIV_POWER_OF_TWO)
		ratio = 1 << ratio;
	else if (div->flags & MTK_CLK_DIV_TIMES_TWO) {
		ratio = ratio << 1;
		if (ratio == 0)
			ratio = 1;
	} else
		ratio++;

	return rate / ratio;
}

int
mtk_cru_clk_div_set_rate(struct mtk_cru_softc *sc,
    struct mtk_cru_clk *clk, u_int new_rate)
{
	struct mtk_cru_clk_div *div = &clk->u.div;
	struct clk *clkp, *clkp_parent;
	int parent_rate;
	uint32_t val, raw_div;
	int ratio;
	int error = 0;

	KASSERT(clk->type == MTK_CLK_DIV);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return ENXIO;

	if (div->div == 0)
		return ENXIO;

	mutex_enter(&sc->sc_mutex);
	val = CRU_READ(sc, div->reg);

	parent_rate = clk_get_rate(clkp_parent);
	if (parent_rate == 0) {
		error = (new_rate == 0) ? 0 : ERANGE;
		goto out;
	}

	ratio = howmany(parent_rate, new_rate);
	if ((div->flags & MTK_CLK_DIV_TIMES_TWO) != 0) {
		if (ratio > 1 && (ratio & 1) != 0)
			ratio++;
		raw_div = ratio >> 1;
	} else if ((div->flags & MTK_CLK_DIV_POWER_OF_TWO) != 0) {
		error = EINVAL;
		goto out;
	} else {
		raw_div = (ratio > 0 ) ? ratio - 1 : 0;
	}
	if (raw_div > __SHIFTOUT_MASK(div->div)) {
		error = ERANGE;
		goto out;
	}

	val &= ~div->div;
	val |= __SHIFTIN(raw_div, div->div);
	CRU_WRITE(sc, div->reg, val);

 out:
	mutex_exit(&sc->sc_mutex);

	return error;
}

const char *
mtk_cru_clk_div_get_parent(struct mtk_cru_softc *sc,
    struct mtk_cru_clk *clk)
{
	struct mtk_cru_clk_div *div = &clk->u.div;

	KASSERT(clk->type == MTK_CLK_DIV);

	return div->parent;
}
