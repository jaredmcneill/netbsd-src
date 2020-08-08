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
#include <arm/mediatek/mt2701_infracfg.h>

static int	mt2701_infracfg_match(device_t, cfdata_t, void *);
static void	mt2701_infracfg_attach(device_t, device_t, void *);

static const char * compatible[] = {
	"mediatek,mt2701-infracfg",
	NULL
};

struct mt2701_infracfg_softc {
	struct mtk_cru_softc	sc_mtk_cru;
};

CFATTACH_DECL_NEW(mt2701_infracfg, sizeof(struct mt2701_infracfg_softc),
    mt2701_infracfg_match, mt2701_infracfg_attach, NULL, NULL);

static struct mtk_cru_reset mt2701_infracfg_resets[] = {
	MTK_CRU_RESET(MT2701_RST_INFRA_EMI_REG_RST, 0x30, 0),
	MTK_CRU_RESET(MT2701_RST_INFRA_DRAMC0_A0_RST, 0x30, 1),
	MTK_CRU_RESET(MT2701_RST_INFRA_FHCTL_RST, 0x30, 2),
	MTK_CRU_RESET(MT2701_RST_INFRA_APCIRQ_EINT_RST, 0x30, 3),
	MTK_CRU_RESET(MT2701_RST_INFRA_APXGPT_RST, 0x30, 4),
	MTK_CRU_RESET(MT2701_RST_INFRA_SCPSYS_RST, 0x30, 5),
	MTK_CRU_RESET(MT2701_RST_INFRA_KP_RST, 0x30, 6),
	MTK_CRU_RESET(MT2701_RST_INFRA_PMIC_WRAP_RST, 0x30, 7),
	MTK_CRU_RESET(MT2701_RST_INFRA_MIPI_RST, 0x30, 8),
	MTK_CRU_RESET(MT2701_RST_INFRA_IRRX_RST, 0x30, 9),
	MTK_CRU_RESET(MT2701_RST_INFRA_CEC_RST, 0x30, 10),

	MTK_CRU_RESET(MT2701_RST_INFRA_EMI_RST, 0x34, 0),
	MTK_CRU_RESET(MT2701_RST_INFRA_DRAMC0_RST, 0x34, 2),
	MTK_CRU_RESET(MT2701_RST_INFRA_TRNG_RST, 0x34, 5),
	MTK_CRU_RESET(MT2701_RST_INFRA_SYSIRQ_RST, 0x34, 6),
};

static const char *cpu_parents[] = {
	"clk26m",
	"armpll",
	"mainpll",
	"mmpll",
};

static const bus_size_t mt2701_infra_cg_regs[] = {
	[MTK_CLK_GATE_REG_SET] = 0x0040,
	[MTK_CLK_GATE_REG_CLR] = 0x0044,
	[MTK_CLK_GATE_REG_STA] = 0x0048,
};

static struct mtk_cru_clk mt2701_infracfg_clks[] = {
	/*
	 * FIXED FACTOR CLOCKS
	 */

	MTK_CLK_FDIV(MT2701_CLK_INFRA_CLK_13M, "clk13m",
	    "clk26m",
	    2),					/* div */

	/*
	 * MUX CLOCKS
	 */
	
	MTK_CLK_MUX(MT2701_CLK_INFRA_CPUSEL, "infra_cpu_sel",
	    cpu_parents,
	    0x0000,
	    __BITS(2,3)),			/* sel */

	/*
	 * GATED CLOCKS
	 */

	MTK_CLK_GATE(MT2701_CLK_INFRA_DBG, "dbgclk",
	    "axi_sel",
	    mt2701_infra_cg_regs,
	    __BIT(0),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_SMI, "smi_ck",
	    "mm_sel",
	    mt2701_infra_cg_regs,
	    __BIT(1),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_QAXI_CM4, "cm4_ck",
	    "axi_sel",
	    mt2701_infra_cg_regs,
	    __BIT(2),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_AUD_SPLIN_B, "audio_splin_bck",
	    "hadds2pll_294m",
	    mt2701_infra_cg_regs,
	    __BIT(4),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_AUDIO, "audio_ck",
	    "clk26m",
	    mt2701_infra_cg_regs,
	    __BIT(5),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_EFUSE, "efuse_ck",
	    "clk26m",
	    mt2701_infra_cg_regs,
	    __BIT(6),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_L2C_SRAM, "l2c_sram_ck",
	    "mm_sel",
	    mt2701_infra_cg_regs,
	    __BIT(7),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_M4U, "m4u_ck",
	    "mem_sel",
	    mt2701_infra_cg_regs,
	    __BIT(8),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_CONNMCU, "connsys_bus",
	    "wbg_dig_ck_416m",
	    mt2701_infra_cg_regs,
	    __BIT(12),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_TRNG, "trng_ck",
	    "axi_sel",
	    mt2701_infra_cg_regs,
	    __BIT(13),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_RAMBUFIF, "rambufif_ck",
	    "mem_sel",
	    mt2701_infra_cg_regs,
	    __BIT(14),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_CPUM, "cpum_ck",
	    "mem_sel",
	    mt2701_infra_cg_regs,
	    __BIT(15),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_KP, "kp_ck",
	    "axi_sel",
	    mt2701_infra_cg_regs,
	    __BIT(16),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_CEC, "cec_ck",
	    "rtc_sel",
	    mt2701_infra_cg_regs,
	    __BIT(18),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_IRRX, "irrx_ck",
	    "axi_sel",
	    mt2701_infra_cg_regs,
	    __BIT(19),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_PMICSPI, "pmicspi_ck",
	    "pmicspi_sel",
	    mt2701_infra_cg_regs,
	    __BIT(22),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_PMICWRAP, "pmicwrap_ck",
	    "axi_sel",
	    mt2701_infra_cg_regs,
	    __BIT(23),				/* enable */
	    0),					/* flags */

	MTK_CLK_GATE(MT2701_CLK_INFRA_DDCCI, "ddcci_ck",
	    "axi_sel",
	    mt2701_infra_cg_regs,
	    __BIT(24),				/* enable */
	    0),					/* flags */
};

static int
mt2701_infracfg_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mt2701_infracfg_attach(device_t parent, device_t self, void *aux)
{
	struct mt2701_infracfg_softc * const infracfg_sc = device_private(self);
	struct mtk_cru_softc * const sc = &infracfg_sc->sc_mtk_cru;
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = mt2701_infracfg_resets;
	sc->sc_nresets = __arraycount(mt2701_infracfg_resets);

	sc->sc_clks = mt2701_infracfg_clks;
	sc->sc_nclks = __arraycount(mt2701_infracfg_clks);

	if (mtk_cru_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": Infrastructure Configuration\n");

	mtk_cru_print(sc);
}
