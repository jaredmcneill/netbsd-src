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
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/mediatek/mtk_cru.h>
#include <arm/mediatek/mt2701_apmsys.h>

#define	AP_PLL_CON0		0x000
#define	AP_PLL_CON1		0x004
#define	AP_PLL_CON2		0x008
#define	PLL_HP_CON0		0x014
#define	PLL_TEST_CON0		0x038
#define	HDMI_CON0		0x100
#define	HDMI_CON1		0x104
#define	HDMI_CON2		0x108
#define	HDMI_CON3		0x10c
#define	HDMI_CON4		0x110
#define	HDMI_CON5		0x114
#define	HDMI_CON6		0x118
#define	HDMI_CON7		0x11c
#define	HDMI_CON8		0x120
#define	ARMPLL_CON0		0x200
#define	ARMPLL_CON1		0x204
#define	ARMPLL_PWR_CON0		0x20c
#define	MAINPLL_CON0		0x210
#define	MAINPLL_CON1		0x214
#define	MAINPLL_PWR_CON0	0x21c
#define	UNIVPLL_CON0		0x220
#define	UNIVPLL_CON1		0x224
#define	UNIVPLL_PWR_CON0	0x22c
#define	MMPLL_CON0		0x230
#define	MMPLL_CON1		0x234
#define	MMPLL_PWR_CON0		0x23c
#define	MSDCPLL_CON0		0x240
#define	MSDCPLL_CON1		0x244
#define	MSDCPLL_PWR_CON0	0x24c
#define	TVDPLL_CON0		0x250
#define	TVDPLL_CON1		0x254
#define	TVDPLL_PWR_CON0		0x25c
#define	TVDPLL_SSC_CON0		0x260
#define	TVDPLL_SSC_CON1		0x264
#define	AUD1PLL_CON0		0x270
#define	AUD1PLL_CON1		0x274
#define	AUD1PLL_PWR_CON0	0x27c
#define	TRGPLL_CON0		0x280
#define	TRGPLL_CON1		0x284
#define	TRGPLL_PWR_CON0		0x28c
#define	ETHPLL_CON0		0x290
#define	ETHPLL_CON1		0x294
#define	ETHPLL_PWR_CON0		0x29c
#define	VDECPLL_CON0		0x2a0
#define	VDECPLL_CON1		0x2a4
#define	VDECPLL_PWR_CON0	0x2ac
#define	HADDS2PLL_CON0		0x2b0
#define	HADDS2PLL_CON1		0x2b4
#define	HADDS2PLL_PWR_CON0	0x2bc
#define	AUD2PLL_CON0		0x2c0
#define	AUD2PLL_CON1		0x2c4
#define	AUD2PLL_PWR_CON0	0x2cc
#define	TVD2PLL_CON0		0x2d0
#define	TVD2PLL_CON1		0x2d4
#define	TVD2PLL_PWR_CON0	0x2dc
#define	TVD2PLL_SSC_CON0	0x2f0
#define	TVD2PLL_SSC_CON1	0x2f4
#define	AP_AUXADC_CON0		0x400
#define	AP_AUXADC_CON1		0x404
#define	AP_AUXADC_CON2		0x40c
#define	TS_CON0			0x600
#define	TS_CON1			0x604
#define	VENCPLL_CON0		0x800
#define	VENCPLL_CON1		0x804
#define	VENCPLL_PWR_CON0	0x80c

static int	mt2701_apmsys_match(device_t, cfdata_t, void *);
static void	mt2701_apmsys_attach(device_t, device_t, void *);

static const char * compatible[] = {
	"mediatek,mt2701-apmixedsys",
	NULL
};

struct mt2701_apmsys_softc {
	struct mtk_cru_softc	sc_mtk_cru;
};

CFATTACH_DECL_NEW(mt2701_apmsys, sizeof(struct mt2701_apmsys_softc),
    mt2701_apmsys_match, mt2701_apmsys_attach, NULL, NULL);

static const bus_size_t mt2701_apmsys_armpll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = ARMPLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = ARMPLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = ARMPLL_CON1,
	[MTK_CLK_PLL_REG_PWR] = ARMPLL_PWR_CON0,
};

static const bus_size_t mt2701_apmsys_mainpll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = MAINPLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = MAINPLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = MAINPLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = MAINPLL_PWR_CON0,
};

static const bus_size_t mt2701_apmsys_univpll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = UNIVPLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = UNIVPLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = UNIVPLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = UNIVPLL_PWR_CON0,
};

static const bus_size_t mt2701_apmsys_mmpll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = MMPLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = MMPLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = MMPLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = MMPLL_PWR_CON0,
};

static const bus_size_t mt2701_apmsys_msdcpll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = MSDCPLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = MSDCPLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = MSDCPLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = MSDCPLL_PWR_CON0,
};

static const bus_size_t mt2701_apmsys_tvdpll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = TVDPLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = TVDPLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = TVDPLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = TVDPLL_PWR_CON0,
};

static const bus_size_t mt2701_apmsys_aud1pll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = AUD1PLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = AUD1PLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = AUD1PLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = AUD1PLL_PWR_CON0,
};

static const bus_size_t mt2701_apmsys_trgpll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = TRGPLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = TRGPLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = TRGPLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = TRGPLL_PWR_CON0,
};

static const bus_size_t mt2701_apmsys_ethpll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = ETHPLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = ETHPLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = ETHPLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = ETHPLL_PWR_CON0,
};

static const bus_size_t mt2701_apmsys_vdecpll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = VDECPLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = VDECPLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = VDECPLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = VDECPLL_PWR_CON0,
};

static const bus_size_t mt2701_apmsys_hadds2pll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = HADDS2PLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = HADDS2PLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = HADDS2PLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = HADDS2PLL_PWR_CON0,
};

static const bus_size_t mt2701_apmsys_aud2pll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = AUD2PLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = AUD2PLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = AUD2PLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = AUD2PLL_PWR_CON0,
};

static const bus_size_t mt2701_apmsys_tvd2pll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = TVD2PLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = TVD2PLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = TVD2PLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = TVD2PLL_PWR_CON0,
};

#if 0 /* Not in the DT bindings. */
static const bus_size_t mt2701_apmsys_vencpll_regs[] = {
	[MTK_CLK_PLL_REG_CON] = VENCPLL_CON0,
	[MTK_CLK_PLL_REG_PCW] = VENCPLL_CON1,
	[MTK_CLK_PLL_REG_PD]  = VENCPLL_CON0,
	[MTK_CLK_PLL_REG_PWR] = VENCPLL_PWR_CON0,
#endif

#define	APMIXEDSYS_PLL_MAX_FREQ		(2000U * 1000 * 1000)

#define	APMIXEDSYS_PLL(_id, _name, _regs, _pll_en_aux_mask,	\
		       _pcw_nbits, _pcw_shift,			\
		       _pd_mask, _rst_bar_mask, _flags)		\
	MTK_CLK_PLL((_id), (_name), "clk26m", (_regs), 		\
		    APMIXEDSYS_PLL_MAX_FREQ,			\
		    0,			/* pll_en bit */	\
		    (_pll_en_aux_mask),				\
		    0,			/* pwr_en bit */	\
		    1,			/* iso_en bit */	\
		    31,			/* pcw_chg bit */	\
		    (_pcw_nbits), (_pcw_shift),			\
		    (_pd_mask), (_rst_bar_mask), (_flags))


static struct mtk_cru_clk mt2701_apmsys_clks[] = {
	/*
	 * PLL CLOCKS
	 */
	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_ARMPLL, "armpll",
	    mt2701_apmsys_armpll_regs,
	    0,				/* pll_en_aux mask */
	    21,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(24,26),		/* pd_mask */
	    0,				/* rst_bar_mask */
	    MTK_CLK_PLL_ALWAYS_ON),

	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_MAINPLL, "mainpll",
	    mt2701_apmsys_mainpll_regs,
	    __BITS(27,30),		/* pll_en_aux mask */
	    21,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    __BIT(24),			/* rst_bar_mask */
	    0),				/* flags */

	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_UNIVPLL, "univpll",
	    mt2701_apmsys_univpll_regs,
	    __BITS(26,31),		/* pll_en_aux mask */
	    7,				/* pcw_nbits */
	    14,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    __BIT(24),			/* rst_bar_mask */
	    0),				/* flags */

	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_MMPLL, "mmpll",
	    mt2701_apmsys_mmpll_regs,
	    0,				/* pll_en_aux mask */
	    21,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    0,				/* rst_bar_mask */
	    0),				/* flags */

	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_MSDCPLL, "msdcpll",
	    mt2701_apmsys_msdcpll_regs,
	    0,				/* pll_en_aux mask */
	    21,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    0,				/* rst_bar_mask */
	    0),				/* flags */

	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_TVDPLL, "tvdpll",
	    mt2701_apmsys_tvdpll_regs,
	    0,				/* pll_en_aux mask */
	    21,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    0,				/* rst_bar_mask */
	    0),				/* flags */

	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_AUD1PLL, "aud1pll",
	    mt2701_apmsys_aud1pll_regs,
	    0,				/* pll_en_aux mask */
	    31,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    0,				/* rst_bar_mask */
	    0),				/* flags */

	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_TRGPLL, "trgpll",
	    mt2701_apmsys_trgpll_regs,
	    0,				/* pll_en_aux mask */
	    31,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    0,				/* rst_bar_mask */
	    0),				/* flags */

	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_ETHPLL, "ethpll",
	    mt2701_apmsys_ethpll_regs,
	    0,				/* pll_en_aux mask */
	    31,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    0,				/* rst_bar_mask */
	    0),				/* flags */

	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_VDECPLL, "vdecpll",
	    mt2701_apmsys_vdecpll_regs,
	    0,				/* pll_en_aux mask */
	    31,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    0,				/* rst_bar_mask */
	    0),				/* flags */

	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_HADDS2PLL, "hadds2pll",
	    mt2701_apmsys_hadds2pll_regs,
	    0,				/* pll_en_aux mask */
	    31,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    0,				/* rst_bar_mask */
	    0),				/* flags */

	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_AUD2PLL, "aud2pll",
	    mt2701_apmsys_aud2pll_regs,
	    0,				/* pll_en_aux mask */
	    31,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    0,				/* rst_bar_mask */
	    0),				/* flags */

	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_TVD2PLL, "tvd2pll",
	    mt2701_apmsys_tvd2pll_regs,
	    0,				/* pll_en_aux mask */
	    31,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    0,				/* rst_bar_mask */
	    0),				/* flags */

#if 0 /* Not in the DT bindings. */
	APMIXEDSYS_PLL(MT2701_CLK_APMIXED_VENCPLL, "vencpll",
	    mt2701_apmsys_vencpll_regs,
	    __BITS(8,11),		/* pll_en_aux mask *//* XXX ?? */
	    21,				/* pcw_nbits */
	    0,				/* pcw_shift */
	    __BITS(4,6),		/* pd_mask */
	    0,				/* rst_bar_mask */
	    0),				/* flags */
#endif

	/*
	 * FIXED FACTOR CLOCKS
	 */

	MTK_CLK_FDIV(MT2701_CLK_APMIXED_HDMI_REF, "hdmi_ref",
	    "tvdpll",
	    1),			/* div */
};

static int
mt2701_apmsys_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mt2701_apmsys_attach(device_t parent, device_t self, void *aux)
{
	struct mt2701_apmsys_softc * const infracfg_sc = device_private(self);
	struct mtk_cru_softc * const sc = &infracfg_sc->sc_mtk_cru;
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_clks = mt2701_apmsys_clks;
	sc->sc_nclks = __arraycount(mt2701_apmsys_clks);

	if (mtk_cru_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": APMIXEDSYS PLL\n");

	mtk_cru_print(sc);
}
