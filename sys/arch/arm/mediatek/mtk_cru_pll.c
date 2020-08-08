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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/bus.h>

#include <arm/mediatek/mtk_cru.h>

static u_int
mtk_cru_clk_pll_get_parent_rate(struct clk *clkp)
{
	struct clk *clkp_parent;

	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	return clk_get_rate(clkp_parent);
}

#define	PCW_INTEGER_BITS	7

static u_int
mtk_cru_clk_pll_compute_rate(struct mtk_cru_clk_pll *pll, u_int parent_rate,
    uint32_t pcw, u_int postdiv)
{
	uint64_t val;
	u_int frac_bits;
	bool carry;

	/* Calculate the number of fractional bits. */
	if (pll->pcw_nbits > PCW_INTEGER_BITS)
		frac_bits = pll->pcw_nbits - PCW_INTEGER_BITS;
	else
		frac_bits = 0;

	val = (uint64_t)parent_rate * pcw;

	if (frac_bits && (val & __BITS(0, frac_bits - 1)))
		carry = true;
	else
		carry = false;

	val >>= frac_bits;
	if (carry)
		val++;

	return (u_int)((val + postdiv - 1) / postdiv);
}

static void
mtk_cru_clk_pll_compute_values(const struct mtk_cru_clk_pll *pll,
    u_int target_rate, u_int parent_rate,
    uint32_t *pcw_out, u_int *postdiv_out)
{
	const u_int min_freq = 1 * 1000 * 1000 * 1000;	/* 1GHz */
	uint64_t pcw;
	u_int postdiv;

	if (target_rate > pll->max_freq)
		target_rate = pll->max_freq;
	
	u_int pd_shift;
	for (pd_shift = 0; pd_shift < 5; pd_shift++) {
		postdiv = 1U << pd_shift;
		if ((uint64_t)target_rate * postdiv >= min_freq)
			break;
	}

	pcw = (((uint64_t)target_rate << pd_shift) <<
	    (pll->pcw_nbits - PCW_INTEGER_BITS)) / parent_rate;
	
	*pcw_out = (uint32_t)pcw;
	*postdiv_out = postdiv;
}

int
mtk_cru_clk_pll_enable(struct mtk_cru_softc *sc, struct mtk_cru_clk *clk,
		       int enable)
{
	struct mtk_cru_clk_pll *pll = &clk->u.pll;

	KASSERT(clk->type == MTK_CLK_PLL);

	mutex_enter(&sc->sc_mutex);

	uint32_t val;

	if (enable) {
		val = CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_PWR]);
		val |= pll->pwr_en;
		CRU_WRITE(sc, pll->regs[MTK_CLK_PLL_REG_PWR], val);
		delay(1);

		val = CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_PWR]);
		val &= ~pll->iso_en;
		CRU_WRITE(sc, pll->regs[MTK_CLK_PLL_REG_PWR], val);
		delay(1);

		val = CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_CON]);
		val |= pll->pll_en | pll->pll_en_aux;
		CRU_WRITE(sc, pll->regs[MTK_CLK_PLL_REG_CON], val);

		/* XXX Tuner PLLs */

		delay(20);

		if (pll->rst_bar_mask) {
			val = CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_CON]);
			val |= pll->rst_bar_mask;
			CRU_WRITE(sc, pll->regs[MTK_CLK_PLL_REG_CON], val);
		}
	} else {
		if (pll->rst_bar_mask) {
			val = CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_CON]);
			val &= ~pll->rst_bar_mask;
			CRU_WRITE(sc, pll->regs[MTK_CLK_PLL_REG_CON], val);
		}

		/* XXX Tuner PLLs */

		val = CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_CON]);
		val &= ~pll->pll_en;
		CRU_WRITE(sc, pll->regs[MTK_CLK_PLL_REG_CON], val);

		val = CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_PWR]);
		val |= pll->iso_en;
		CRU_WRITE(sc, pll->regs[MTK_CLK_PLL_REG_PWR], val);

		val = CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_PWR]);
		val &= ~pll->pwr_en;
		CRU_WRITE(sc, pll->regs[MTK_CLK_PLL_REG_PWR], val);
	}

	mutex_exit(&sc->sc_mutex);

	return 0;
}

u_int
mtk_cru_clk_pll_get_rate(struct mtk_cru_softc *sc, struct mtk_cru_clk *clk)
{
	struct mtk_cru_clk_pll *pll = &clk->u.pll;
	struct clk *clkp = &clk->base;

	KASSERT(clk->type == MTK_CLK_PLL);

	const u_int p_rate = mtk_cru_clk_pll_get_parent_rate(clkp);
	if (p_rate == 0)
		return 0;

	mutex_enter(&sc->sc_mutex);

	const uint32_t pcw_mask = __BITS(0, pll->pcw_nbits - 1);
	const uint32_t pcw = (CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_PCW])
	    >> pll->pcw_shift) & pcw_mask;
	const u_int postdiv = 1U <<
	    __SHIFTOUT(CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_PD]),
		       pll->pd_mask);

	mutex_exit(&sc->sc_mutex);

	return mtk_cru_clk_pll_compute_rate(pll, p_rate, pcw, postdiv);
}

int
mtk_cru_clk_pll_set_rate(struct mtk_cru_softc *sc, struct mtk_cru_clk *clk,
			 u_int rate)
{
	struct mtk_cru_clk_pll *pll = &clk->u.pll;
	struct clk *clkp = &clk->base;

	KASSERT(clk->type == MTK_CLK_PLL);

	const u_int p_rate = mtk_cru_clk_pll_get_parent_rate(clkp);
	if (p_rate == 0)
		return ENXIO;

	uint32_t pcw;
	u_int postdiv;

	mtk_cru_clk_pll_compute_values(pll, rate, p_rate, &pcw, &postdiv);

	mutex_enter(&sc->sc_mutex);

	const uint32_t pll_enabled =
	    CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_CON]) & pll->pll_en;
	
	uint32_t val;

	/* Set postdiv. */
	val = CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_PD]);
	val &= ~pll->pd_mask;
	val |= __SHIFTIN((ffs(postdiv) - 1), pll->pd_mask);

	/* Write it out if PCW and postdiv are in different registers. */
	if (pll->regs[MTK_CLK_PLL_REG_PD] != pll->regs[MTK_CLK_PLL_REG_PCW]) {
		CRU_WRITE(sc, pll->regs[MTK_CLK_PLL_REG_PD], val);
		val = CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_PCW]);
	}

	/* Set PCW. */
	const uint32_t pcw_mask = __BITS(0, pll->pcw_nbits - 1);
	val &= ~(pcw_mask << pll->pcw_shift);
	val |= pcw << pll->pcw_shift;
	CRU_WRITE(sc, pll->regs[MTK_CLK_PLL_REG_PCW], val);

	val = CRU_READ(sc, pll->regs[MTK_CLK_PLL_REG_PCW]);
	if (pll_enabled)
		val |= pll->pcw_chg;
	CRU_WRITE(sc, pll->regs[MTK_CLK_PLL_REG_PCW], val);

	/* XXX Tuner PLLs */

	if (pll_enabled)
		delay(20);

	mutex_exit(&sc->sc_mutex);

	return 0;
}

u_int
mtk_cru_clk_pll_round_rate(struct mtk_cru_softc *sc, struct mtk_cru_clk *clk,
			   u_int try_rate)
{
	struct mtk_cru_clk_pll *pll = &clk->u.pll;
	struct clk *clkp = &clk->base;

	KASSERT(clk->type == MTK_CLK_PLL);

	const u_int p_rate = mtk_cru_clk_pll_get_parent_rate(clkp);
	if (p_rate == 0)
		return 0;

	uint32_t pcw;
	u_int postdiv;

	mtk_cru_clk_pll_compute_values(pll, try_rate, p_rate, &pcw, &postdiv);
	return mtk_cru_clk_pll_compute_rate(pll, p_rate, pcw, postdiv);
}

const char *
mtk_cru_clk_pll_get_parent(struct mtk_cru_softc *sc, struct mtk_cru_clk *clk)
{

	KASSERT(clk->type == MTK_CLK_PLL);

	return clk->u.pll.parent;
}
