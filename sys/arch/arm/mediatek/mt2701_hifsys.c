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
#include <arm/mediatek/mt2701_hifsys.h>

/* Relative to HIFSYS Base address */
#define	HIFSYS_ID0_3			0x000
#define	HIFSYS_ID4_7			0x004
#define	HIFSYS_SYSCFG1			0x014
#define	HIFSYS_CLKCFG0			0x02c
#define	HIFSYS_CLKCFG1			0x030
#define	HIFSYS_RSTCTL			0x034
#define	HIFSYS_RSTSTAT			0x038

static int	mt2701_hifsys_match(device_t, cfdata_t, void *);
static void	mt2701_hifsys_attach(device_t, device_t, void *);

static const char * compatible[] = {
	"mediatek,mt2701-hifsys",
	NULL
};

struct mt2701_hifsys_softc {
	struct mtk_cru_softc	sc_mtk_cru;
};

CFATTACH_DECL_NEW(mt2701_hifsys, sizeof(struct mt2701_hifsys_softc),
    mt2701_hifsys_match, mt2701_hifsys_attach, NULL, NULL);

static struct mtk_cru_reset mt2701_hifsys_resets[] = {
	MTK_CRU_RESET(MT2701_RST_HIFSYS_UHOST0_RST, HIFSYS_RSTCTL, 3),
	MTK_CRU_RESET(MT2701_RST_HIFSYS_UHOST1_RST, HIFSYS_RSTCTL, 4),
	MTK_CRU_RESET(MT2701_RST_HIFSYS_UPHY0_RST, HIFSYS_RSTCTL, 21),
	MTK_CRU_RESET(MT2701_RST_HIFSYS_UPHY1_RST, HIFSYS_RSTCTL, 22),
	MTK_CRU_RESET(MT2701_RST_HIFSYS_PCIE0_RST, HIFSYS_RSTCTL, 24),
	MTK_CRU_RESET(MT2701_RST_HIFSYS_PCIE1_RST, HIFSYS_RSTCTL, 25),
	MTK_CRU_RESET(MT2701_RST_HIFSYS_PCIE2_RST, HIFSYS_RSTCTL, 26),
};

static const bus_size_t mt2701_hifsys_clkcfg1_regs[] = {
	[MTK_CLK_GATE_REG_SET] = HIFSYS_CLKCFG1,
	[MTK_CLK_GATE_REG_CLR] = HIFSYS_CLKCFG1,
	[MTK_CLK_GATE_REG_STA] = HIFSYS_CLKCFG1,
};

static struct mtk_cru_clk mt2701_hifsys_clks[] = {
	MTK_CLK_GATE(MT2701_CLK_HIFSYS_USB0PHY, "usb0_phy_clk",
	    "ethpll_500m_ck",
	    mt2701_hifsys_clkcfg1_regs,
	    __BIT(21),				/* enable */
	    0),

	MTK_CLK_GATE(MT2701_CLK_HIFSYS_USB1PHY, "usb1_phy_clk",
	    "ethpll_500m_ck",
	    mt2701_hifsys_clkcfg1_regs,
	    __BIT(22),				/* enable */
	    0),

	MTK_CLK_GATE(MT2701_CLK_HIFSYS_PCIE0, "pcie0_clk",
	    "ethpll_500m_ck",
	    mt2701_hifsys_clkcfg1_regs,
	    __BIT(24),				/* enable */
	    0),

	MTK_CLK_GATE(MT2701_CLK_HIFSYS_PCIE1, "pcie1_clk",
	    "ethpll_500m_ck",
	    mt2701_hifsys_clkcfg1_regs,
	    __BIT(25),				/* enable */
	    0),

	MTK_CLK_GATE(MT2701_CLK_HIFSYS_PCIE2, "pcie2_clk",
	    "ethpll_500m_ck",
	    mt2701_hifsys_clkcfg1_regs,
	    __BIT(26),				/* enable */
	    0),
};

static int
mt2701_hifsys_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mt2701_hifsys_attach(device_t parent, device_t self, void *aux)
{
	struct mt2701_hifsys_softc * const hifsys_sc = device_private(self);
	struct mtk_cru_softc * const sc = &hifsys_sc->sc_mtk_cru;
	struct fdt_attach_args * const faa = aux;
	uint32_t val;
	char id[9];

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = mt2701_hifsys_resets;
	sc->sc_nresets = __arraycount(mt2701_hifsys_resets);

	sc->sc_clks = mt2701_hifsys_clks;
	sc->sc_nclks = __arraycount(mt2701_hifsys_clks);

	if (mtk_cru_attach(sc) != 0)
		return;

	val = CRU_READ(sc, HIFSYS_ID0_3);
	id[0] =  val        & 0xff;
	id[1] = (val >> 8)  & 0xff;
	id[2] = (val >> 16) & 0xff;
	id[3] = (val >> 24) & 0xff;

	val = CRU_READ(sc, HIFSYS_ID4_7);
	id[4] =  val        & 0xff;
	id[5] = (val >> 8)  & 0xff;
	id[6] = (val >> 16) & 0xff;
	id[7] = (val >> 24) & 0xff;

	id[8] = '\0';

	aprint_naive("\n");
	aprint_normal(": HIFSYS, ID='%s'\n", id);

	mtk_cru_print(sc);
}
