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
#include <arm/mediatek/mt2701_topckgen.h>

/* Relative to TOPCKGEN Base address */
#define	TOPCKGEN_CLK_MODE		0x000
#define	TOPCKGEN_DCM_CFG		0x004
#define	TOPCKGEN_CLK_CFG_0		0x040
#define	TOPCKGEN_CLK_CFG_0_SET		0x044
#define	TOPCKGEN_CLK_CFG_0_CLR		0x048
#define	TOPCKGEN_CLK_CFG_1		0x050
#define	TOPCKGEN_CLK_CFG_1_SET		0x054
#define	TOPCKGEN_CLK_CFG_1_CLR		0x058
#define	TOPCKGEN_CLK_CFG_2		0x060
#define	TOPCKGEN_CLK_CFG_2_SET		0x064
#define	TOPCKGEN_CLK_CFG_2_CLR		0x068
#define	TOPCKGEN_CLK_CFG_3		0x070
#define	TOPCKGEN_CLK_CFG_3_SET		0x074
#define	TOPCKGEN_CLK_CFG_3_CLR		0x078
#define	TOPCKGEN_CLK_CFG_4		0x080
#define	TOPCKGEN_CLK_CFG_4_SET		0x084
#define	TOPCKGEN_CLK_CFG_4_CLR		0x088
#define	TOPCKGEN_CLK_CFG_5		0x090
#define	TOPCKGEN_CLK_CFG_5_SET		0x094
#define	TOPCKGEN_CLK_CFG_5_CLR		0x098
#define	TOPCKGEN_CLK_CFG_6		0x0a0
#define	TOPCKGEN_CLK_CFG_6_SET		0x0a4
#define	TOPCKGEN_CLK_CFG_6_CLR		0x0a8
#define	TOPCKGEN_CLK_CFG_7		0x0b0
#define	TOPCKGEN_CLK_CFG_7_SET		0x0b4
#define	TOPCKGEN_CLK_CFG_7_CLR		0x0b8
#define	TOPCKGEN_CLK_CFG_12		0x0c0
#define	TOPCKGEN_CLK_CFG_12_SET		0x0c4
#define	TOPCKGEN_CLK_CFG_12_CLR		0x0c8
#define	TOPCKGEN_CLK_CFG_13		0x0d0
#define	TOPCKGEN_CLK_CFG_13_SET		0x0d4
#define	TOPCKGEN_CLK_CFG_13_CLR		0x0d8
#define	TOPCKGEN_CLK_CFG_14		0x0e0
#define	TOPCKGEN_CLK_CFG_14_SET		0x0e4
#define	TOPCKGEN_CLK_CFG_14_CLR		0x0e8
#define	TOPCKGEN_CLK_CFG_15		0x0f0
#define	TOPCKGEN_CLK_CFG_15_SET		0x0f4
#define	TOPCKGEN_CLK_CFG_15_CLR		0x0f8
#define	TOPCKGEN_CLK_CFG_8		0x100
#define	TOPCKGEN_CLK_CFG_9		0x104
#define	TOPCKGEN_CLK_CFG_10		0x108
#define	TOPCKGEN_CLK_CFG_11		0x10c
#define	TOPCKGEN_CLK_AUDDIV0		0x120
#define	TOPCKGEN_CLK_AUDDIV1		0x124
#define	TOPCKGEN_CLK_AUDDIV2		0x128
#define	TOPCKGEN_CLK_AUDDIV3		0x12c
#define	TOPCKGEN_CLK_8BDAC_CFG		0x150
#define	TOPCKGEN_CLK_SCP_CFG_0		0x200
#define	TOPCKGEN_CLK_SCP_CFG_1		0x204
#define	TOPCKGEN_CLK_MISC_CFG_0		0x210
#define	TOPCKGEN_CLK_MISC_CFG_1		0x214
#define	TOPCKGEN_CLK26CALI_0		0x220
#define	TOPCKGEN_CLK26CALI_1		0x224
#define	TOPCKGEN_CLK26CALI_2		0x228
#define	TOPCKGEN_CKSTA_REG		0x22c
#define	TOPCKGEN_TEST_MODE_CFG		0x230
#define	TOPCKGEN_MBIST_CFG_1		0x30c
#define	TOPCKGEN_RESET_DEGLITCH_KEY	0x310
#define	TOPCKGEN_MBIST_CFG_3		0x314
#define	TOPCKGEN_BOOT_TRAP		0x318

static int	mt2701_topckgen_match(device_t, cfdata_t, void *);
static void	mt2701_topckgen_attach(device_t, device_t, void *);

static const char * compatible[] = {
	"mediatek,mt2701-topckgen",
	NULL
};

struct mt2701_topckgen_softc {
	struct mtk_cru_softc	sc_mtk_cru;
};

CFATTACH_DECL_NEW(mt2701_topckgen, sizeof(struct mt2701_topckgen_softc),
    mt2701_topckgen_match, mt2701_topckgen_attach, NULL, NULL);

static const char *mt2701_topckgen_axi_parents[] = {
	"clk26m",
	"syspll1_d2",
	"syspll_d5",
	"syspll1_d4",
	"univpll_d5",
	"univpll2_d2",
	"mmpll_d2",
	"dmpll_d2",
};

static const char *mt2701_topckgen_mem_parents[] = {
	"clk26m",
	"dmpll_ck",
};

static const char *mt2701_topckgen_ddrphycfg_parents[] = {
	"clk26m",
	"syspll1_d8",
};

static const char *mt2701_topckgen_mm_parents[] = {
	"clk26m",
	"vencpll_ck",
	"syspll1_d2",
	"syspll1_d4",
	"univpll_d5",
	"univpll1_d2",
	"univpll2_d2",
	"dmpll_ck",
};

static const char *mt2701_topckgen_pwm_parents[] = {
	"clk26m",
	"univpll2_d4",
	"univpll3_d2",
	"univpll1_d4",
};

static const char *mt2701_topckgen_vdec_parents[] = {
	"clk26m",
	"vdecpll_ck",
	"syspll_d5",
	"syspll1_d4",
	"univpll_d5",
	"univpll2_d2",
	"vencpll_ck",
	"msdcpll_d2",
	"mmpll_d2",
};

static const char *mt2701_topckgen_mfg_parents[] = {
	"clk26m",
	"mmpll_ck",
	"dmpll_x2_ck",
	"msdcpll_ck",
	"clk26m",
	"syspll_d3",
	"univpll_d3",
	"univpll1_d2",
};

static const char *mt2701_topckgen_camtg_parents[] = {
	"clk26m",
	"univpll_d26",
	"univpll2_d2",
	"syspll3_d2",
	"syspll3_d4",
	"msdcpll_d2",
	"mmpll_d2",
};

static const char *mt2701_topckgen_uart_parents[] = {
	"clk26m",
	"univpll2_d8",
};

static const char *mt2701_topckgen_spi_parents[] = {
	"clk26m",
	"syspll3_d2",
	"syspll4_d2",
	"univpll2_d4",
	"univpll1_d8",
};

static const char *mt2701_topckgen_usb20_parents[] = {
	"clk26m",
	"univpll1_d8",
	"univpll3_d4",
};

static const char *mt2701_topckgen_msdc30_parents[] = {
	"clk26m",
	"msdcpll_d2",
	"syspll2_d2",
	"syspll1_d4",
	"univpll1_d4",
	"univpll2_d4",
};

static const char *mt2701_topckgen_audio_parents[] = {
	"clk26m",
	"syspll1_d16",
};

static const char *mt2701_topckgen_aud_intbus_parents[] = {
	"clk26m",
	"syspll1_d4",
	"syspll3_d2",
	"syspll4_d2",
	"univpll3_d2",
	"univpll2_d4",
};

static const char *mt2701_topckgen_pmicspi_parents[] = {
	"clk26m",
	"syspll1_d8",
	"syspll2_d4",
	"syspll4_d2",
	"syspll3_d4",
	"syspll2_d8",
	"syspll1_d16",
	"univpll3_d4",
	"univpll_d26",
	"dmpll_d2",
	"dmpll_d4",
};

static const char *mt2701_topckgen_scp_parents[] = {
	"clk26m",
	"syspll1_d8",
	"dmpll_d2",
	"dmpll_d4",
};

static const char *mt2701_topckgen_dpi0_parents[] = {
	"clk26m",
	"mipipll",
	"mipipll_d2",
	"mipipll_d4",
	"clk26m",
	"tvdpll_ck",
	"tvdpll_d2",
	"tvdpll_d4",
};

static const char *mt2701_topckgen_dpi1_parents[] = {
	"clk26m",
	"tvdpll_ck",
	"tvdpll_d2",
	"tvdpll_d4",
};

static const char *mt2701_topckgen_tve_parents[] = {
	"clk26m",
	"mipipll",
	"mipipll_d2",
	"mipipll_d4",
	"clk26m",
	"tvdpll_ck",
	"tvdpll_d2",
	"tvdpll_d4",
};

static const char *mt2701_topckgen_hdmi_parents[] = {
	"clk26m",
	"hdmipll_ck",
	"hdmipll_d2",
	"hdmipll_d3",
};

static const char *mt2701_topckgen_apll_parents[] = {
	"clk26m",
	"audpll",
	"audpll_d4",
	"audpll_d8",
	"audpll_d16",
	"audpll_d24",
	"clk26m",
	"clk26m",
};

static const char *mt2701_topckgen_rtc_parents[] = {
	"32k_internal",
	"32k_external",
	"clk26m",
	"univpll3_d8",
};

static const char *mt2701_topckgen_nfi2x_parents[] = {
	"clk26m",
	"syspll2_d2",
	"syspll_d7",
	"univpll3_d2",
	"syspll2_d4",
	"univpll3_d4",
	"syspll4_d4",
	"clk26m",
};

static const char *mt2701_topckgen_emmc_hclk_parents[] = {
	"clk26m",
	"syspll1_d2",
	"syspll1_d4",
	"syspll2_d2",
};

static const char *mt2701_topckgen_flash_parents[] = {
	"clk26m_d8",
	"clk26m",
	"syspll2_d8",
	"syspll3_d4",
	"univpll3_d4",
	"syspll4_d2",
	"syspll2_d4",
	"univpll2_d4",
};

static const char *mt2701_topckgen_di_parents[] = {
	"clk26m",
	"tvd2pll_ck",
	"tvd2pll_d2",
	"clk26m",
};

static const char *mt2701_topckgen_nr_osd_parents[] = {
	"clk26m",
	"vencpll_ck",
	"syspll1_d2",
	"syspll1_d4",
	"univpll_d5",
	"univpll1_d2",
	"univpll2_d2",
	"dmpll_ck",
};

static const char *mt2701_topckgen_hdmirx_bist_parents[] = {
	"clk26m",
	"syspll_d3",
	"clk26m",
	"syspll1_d16",
	"syspll4_d2",
	"syspll1_d4",
	"vencpll_ck",
	"clk26m",
};

static const char *mt2701_topckgen_intdir_parents[] = {
	"clk26m",
	"mmpll_ck",
	"syspll_d2",
	"univpll_d2",
};

static const char *mt2701_topckgen_asm_parents[] = {
	"clk26m",
	"univpll2_d4",
	"univpll2_d2",
	"syspll_d5",
};

static const char *mt2701_topckgen_ms_card_parents[] = {
	"clk26m",
	"univpll3_d8",
	"syspll4_d4",
};

static const char *mt2701_topckgen_ethif_parents[] = {
	"clk26m",
	"syspll1_d2",
	"syspll_d5",
	"syspll1_d4",
	"univpll_d5",
	"univpll1_d2",
	"dmpll_ck",
	"dmpll_d2",
};

static const char *mt2701_topckgen_hdmirx_parents[] = {
	"clk26m",
	"univpll_d52",
};

static const char *mt2701_topckgen_cmsys_parents[] = {
	"clk26m",
	"syspll1_d2",
	"univpll1_d2",
	"univpll_d5",
	"syspll_d5",
	"syspll2_d2",
	"syspll1_d4",
	"syspll3_d2",
	"syspll2_d4",
	"syspll1_d8",
	"clk26m",
	"clk26m",
	"clk26m",
	"clk26m",
	"clk26m",
};

static const char *mt2701_topckgen_8bdac_parents[] = {
	"32k_internal",
	"8bdac_ck",
	"clk26m",
	"clk26m",
};

static const char *mt2701_topckgen_aud2dvd_parents[] = {
	"a1sys_hp_ck",
	"a2sys_hp_ck",
};

static const char *mt2701_topckgen_padmclk_parents[] = {
	"clk26m",
	"univpll_d26",
	"univpll_d52",
	"univpll_d108",
	"univpll2_d8",
	"univpll2_d16",
	"univpll2_d32",
};

static const char *mt2701_topckgen_aud_mux_parents[] = {
	"clk26m",
	"aud1pll_98m_ck",
	"aud2pll_90m_ck",
	"hadds2pll_98m",
	"audio_ext1_ck",
	"audio_ext2_ck",
};

static const char *mt2701_topckgen_aud_src_parents[] = {
	"aud_mux1_sel",
	"aud_mux2_sel",
};

static const bus_size_t	mt2701_clk_cfg0_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_CFG_0_SET,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_CFG_0_CLR,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_CFG_0,
};

static const bus_size_t	mt2701_clk_cfg1_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_CFG_1_SET,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_CFG_1_CLR,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_CFG_1,
};

static const bus_size_t	mt2701_clk_cfg2_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_CFG_2_SET,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_CFG_2_CLR,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_CFG_2,
};

static const bus_size_t	mt2701_clk_cfg3_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_CFG_3_SET,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_CFG_3_CLR,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_CFG_3,
};

static const bus_size_t	mt2701_clk_cfg4_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_CFG_4_SET,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_CFG_4_CLR,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_CFG_4,
};

static const bus_size_t	mt2701_clk_cfg5_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_CFG_5_SET,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_CFG_5_CLR,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_CFG_5,
};

static const bus_size_t	mt2701_clk_cfg6_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_CFG_6_SET,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_CFG_6_CLR,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_CFG_6,
};

static const bus_size_t	mt2701_clk_cfg7_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_CFG_7_SET,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_CFG_7_CLR,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_CFG_7,
};

static const bus_size_t	mt2701_clk_cfg12_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_CFG_12_SET,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_CFG_12_CLR,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_CFG_12,
};

static const bus_size_t	mt2701_clk_cfg13_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_CFG_13_SET,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_CFG_13_CLR,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_CFG_13,
};

static const bus_size_t	mt2701_clk_cfg14_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_CFG_14_SET,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_CFG_14_CLR,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_CFG_14,
};

static const bus_size_t	mt2701_clk_cfg15_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_CFG_15_SET,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_CFG_15_CLR,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_CFG_15,
};

static const bus_size_t	mt2701_clk_auddiv3_regs[] = {
	[MTK_CLK_GATE_REG_SET] = TOPCKGEN_CLK_AUDDIV3,
	[MTK_CLK_GATE_REG_CLR] = TOPCKGEN_CLK_AUDDIV3,
	[MTK_CLK_GATE_REG_STA] = TOPCKGEN_CLK_AUDDIV3,
};

static struct mtk_cru_clk mt2701_topckgen_clks[] = {
	/*
	 * FIXED CLOCKS
	 */

	MTK_CLK_FIXED(MT2701_CLK_TOP_DPI, "dpi_ck",
	    "clk26m",
	    108000000),				/* rate */
	
	MTK_CLK_FIXED(MT2701_CLK_TOP_DMPLL, "dmpll_ck",
	    "clk26m",
	    400000000),				/* rate */
	
	MTK_CLK_FIXED(MT2701_CLK_TOP_VENCPLL, "vencpll_ck",
	    "clk26m",
	    295750000),				/* rate */
	
	MTK_CLK_FIXED(MT2701_CLK_TOP_HDMI_0_PIX340M, "hdmi_0_pix340m",
	    "clk26m",
	    340000000),				/* rate */
	
	MTK_CLK_FIXED(MT2701_CLK_TOP_HDMI_0_DEEP340M, "hdmi_0_deep340m",
	    "clk26m",
	    340000000),				/* rate */
	
	MTK_CLK_FIXED(MT2701_CLK_TOP_HDMI_0_PLL340M, "hdmi_0_pll340m",
	    "clk26m",
	    340000000),				/* rate */
	
	MTK_CLK_FIXED(MT2701_CLK_TOP_HADDS2_FB, "hadds2_fbclk",
	    "clk26m",
	    27000000),				/* rate */
	
	MTK_CLK_FIXED(MT2701_CLK_TOP_WBG_DIG_416M, "wbg_dig_ck_416m",
	    "clk26m",
	    416000000),				/* rate */
	
	MTK_CLK_FIXED(MT2701_CLK_TOP_DSI0_LNTC_DSI, "dsi0_lntc_dsi",
	    "clk26m",
	    143000000),				/* rate */
	
	MTK_CLK_FIXED(MT2701_CLK_TOP_HDMI_SCL_RX, "hdmi_scl_rx",
	    "clk26m",
	    27000000),				/* rate */
	
	MTK_CLK_FIXED(MT2701_CLK_TOP_AUD_EXT1, "aud_ext1",
	    "clk26m",
	    0),					/* rate */
	
	MTK_CLK_FIXED(MT2701_CLK_TOP_AUD_EXT2, "aud_ext2",
	    "clk26m",
	    0),					/* rate */
	
	MTK_CLK_FIXED(MT2701_CLK_TOP_NFI1X_PAD, "nfi1x_pad",
	    "clk26m",
	    0),					/* rate */

	/*
	 * FIXED FACTOR CLOCKS
	 */

	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL, "syspll_ck",
	    "mainpll",
	    1),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL_D2, "syspll_d2",
	    "mainpll",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL_D3, "syspll_d3",
	    "mainpll",
	    3),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL_D5, "syspll_d5",
	    "mainpll",
	    5),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL_D7, "syspll_d7",
	    "mainpll",
	    7),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL1_D2, "syspll1_d2",
	    "syspll_d2",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL1_D4, "syspll1_d4",
	    "syspll_d2",
	    4),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL1_D8, "syspll1_d8",
	    "syspll_d2",
	    8),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL1_D16, "syspll1_d16",
	    "syspll_d2",
	    16),				/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL2_D2, "syspll2_d2",
	    "syspll_d3",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL2_D4, "syspll2_d4",
	    "syspll_d3",
	    4),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL2_D8, "syspll2_d8",
	    "syspll_d3",
	    8),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL3_D2, "syspll3_d2",
	    "syspll_d5",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL3_D4, "syspll3_d4",
	    "syspll_d5",
	    4),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL4_D2, "syspll4_d2",
	    "syspll_d7",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_SYSPLL4_D4, "syspll4_d4",
	    "syspll_d7",
	    4),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL, "univpll_ck",
	    "univpll",
	    1),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL_D2, "univpll_d2",
	    "univpll",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL_D3, "univpll_d3",
	    "univpll",
	    3),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL_D5, "univpll_d5",
	    "univpll",
	    5),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL_D7, "univpll_d7",
	    "univpll",
	    7),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL_D26, "univpll_d26",
	    "univpll",
	    26),				/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL_D52, "univpll_d52",
	    "univpll",
	    52),				/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL_D108, "univpll_d108",
	    "univpll",
	    108),				/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_USB_PHY48M, "usb_phy48m_ck",
	    "univpll",
	    26),				/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL1_D2, "univpll1_d2",
	    "univpll_d2",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL1_D4, "univpll1_d4",
	    "univpll_d2",
	    4),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL1_D8, "univpll1_d8",
	    "univpll_d2",
	    8),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_8BDAC, "8bdac_ck",
	    "univpll_d2",
	    1),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL2_D2, "univpll2_d2",
	    "univpll_d3",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL2_D4, "univpll2_d4",
	    "univpll_d3",
	    4),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL2_D8, "univpll2_d8",
	    "univpll_d3",
	    8),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL2_D16, "univpll2_d16",
	    "univpll_d3",
	    16),				/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL2_D32, "univpll2_d32",
	    "univpll_d3",
	    32),				/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL3_D2, "univpll3_d2",
	    "univpll_d5",
	    2),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL3_D4, "univpll3_d4",
	    "univpll_d5",
	    4),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_UNIVPLL3_D8, "univpll3_d8",
	    "univpll_d5",
	    8),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_MSDCPLL, "msdcpll_ck",
	    "msdcpll",
	    1),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_MSDCPLL_D2, "msdcpll_d2",
	    "msdcpll",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_MSDCPLL_D4, "msdcpll_d4",
	    "msdcpll",
	    4),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_MSDCPLL_D8, "msdcpll_d8",
	    "msdcpll",
	    8),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_MMPLL, "mmpll_ck",
	    "mmpll",
	    1),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_MMPLL_D2, "mmpll_d2",
	    "mmpll",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_DMPLL_D2, "dmpll_d2",
	    "dmpll_ck",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_DMPLL_D4, "dmpll_d4",
	    "dmpll_ck",
	    4),					/* div */
	
	MTK_CLK_FMUL(MT2701_CLK_TOP_DMPLL_X2, "dmpll_x2",
	    "dmpll_ck",
	    2),					/* mul */

	MTK_CLK_FDIV(MT2701_CLK_TOP_TVDPLL, "tvdpll_ck",
	    "tvdpll",
	    1),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_TVDPLL_D2, "tvdpll_d2",
	    "tvdpll",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_TVDPLL_D4, "tvdpll_d4",
	    "tvdpll",
	    4),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_TVD2PLL, "tvd2pll_ck",
	    "tvd2pll",
	    1),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_TVD2PLL_D2, "tvd2pll_d2",
	    "tvd2pll",
	    2),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_HADDS2PLL_98M, "hadds2pll_98m",
	    "hadds2pll",
	    3),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_HADDS2PLL_294M, "hadds2pll_294m",
	    "hadds2pll",
	    1),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_MIPIPLL, "mipipll",
	    "dpi_ck",
	    1),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_MIPIPLL_D2, "mipipll_d2",
	    "dpi_ck",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_MIPIPLL_D4, "mipipll_d4",
	    "dpi_ck",
	    4),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_HDMIPLL, "hdmipll_ck",
	    "hdmitx_dig_cts",
	    1),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_HDMIPLL_D2, "hdmipll_d2",
	    "hdmitx_dig_cts",
	    2),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_HDMIPLL_D3, "hdmipll_d3",
	    "hdmitx_dig_cts",
	    3),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_AUD1PLL_98M, "aud1pll_98m_ck",
	    "aud1pll",
	    3),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_AUD2PLL_90M, "aud2pll_90m_ck",
	    "aud2pll",
	    3),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_AUDPLL, "audpll",
	    "audpll_sel",
	    1),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_AUDPLL_D4, "audpll_d4",
	    "audpll_sel",
	    4),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_AUDPLL_D8, "audpll_d8",
	    "audpll_sel",
	    8),					/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_AUDPLL_D16, "audpll_d16",
	    "audpll_sel",
	    16),				/* div */
	
	MTK_CLK_FDIV(MT2701_CLK_TOP_AUDPLL_D24, "audpll_d24",
	    "audpll_sel",
	    24),				/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_ETHPLL_500M, "ethpll_500m_ck",
	    "ethpll",
	    1),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_VDECPLL, "vdecpll_ck",
	    "vdecpll",
	    1),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_ARMPLL_1P3G, "armpll_1p3g_ck",
	    "armpll",
	    1),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_32K_INTERNAL, "32k_internal",
	    "clk26m",
	    793),				/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_32K_EXTERNAL, "32k_external",
	    "rtc32k",
	    1),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_CLK26M_D8, "clk26m_d8",
	    "clk26m",
	    8),					/* div */

	MTK_CLK_FDIV(MT2701_CLK_TOP_AXISEL_D4, "axisel_d4",
	    "axi_sel",
	    4),					/* div */

	/*
	 * GATED / MUXED CLOCKS
	 */

	MTK_CLK_MUXGATE_CLKF(MT2701_CLK_TOP_AXI_SEL, "axi_sel",
	    mt2701_topckgen_axi_parents,
	    mt2701_clk_cfg0_regs,
	    __BITS(0,2),			/* sel */
	    __BIT(7),				/* enable */
	    MTK_CLK_GATE_ACT_LOW,
	    0),					/* clk flags */

	MTK_CLK_MUXGATE_CLKF(MT2701_CLK_TOP_MEM_SEL, "mem_sel",
	    mt2701_topckgen_mem_parents,
	    mt2701_clk_cfg0_regs,
	    __BIT(8),				/* sel */
	    __BIT(15),				/* enable */
	    MTK_CLK_GATE_ACT_LOW,
	    0),					/* clk flags */
	
	MTK_CLK_MUXGATE_CLKF(MT2701_CLK_TOP_DDRPHYCFG_SEL, "ddrphycfg_sel",
	    mt2701_topckgen_ddrphycfg_parents,
	    mt2701_clk_cfg0_regs,
	    __BIT(16),				/* sel */
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW,
	    0),					/* clk flags */
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_MM_SEL, "mm_sel",
	    mt2701_topckgen_mm_parents,
	    mt2701_clk_cfg0_regs,
	    __BITS(24,26),			/* sel */
	    __BIT(31),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_PWM_SEL, "pwm_sel",
	    mt2701_topckgen_pwm_parents,
	    mt2701_clk_cfg1_regs,
	    __BITS(0,1),			/* sel */
	    __BIT(7),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_VDEC_SEL, "vdec_sel",
	    mt2701_topckgen_vdec_parents,
	    mt2701_clk_cfg1_regs,
	    __BITS(8,11),			/* sel */
	    __BIT(15),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_MFG_SEL, "mfg_sel",
	    mt2701_topckgen_mfg_parents,
	    mt2701_clk_cfg1_regs,
	    __BITS(16,18),			/* sel */
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_CAMTG_SEL, "camtg_sel",
	    mt2701_topckgen_camtg_parents,
	    mt2701_clk_cfg1_regs,
	    __BITS(24,26),			/* sel */
	    __BIT(31),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_MUXGATE(MT2701_CLK_TOP_UART_SEL, "uart_sel",
	    mt2701_topckgen_uart_parents,
	    mt2701_clk_cfg2_regs,
	    __BIT(0),				/* sel */
	    __BIT(7),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_SPI0_SEL, "spi0_sel",
	    mt2701_topckgen_spi_parents,
	    mt2701_clk_cfg2_regs,
	    __BITS(8,10),			/* sel */
	    __BIT(15),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_USB20_SEL, "usb20_sel",
	    mt2701_topckgen_usb20_parents,
	    mt2701_clk_cfg2_regs,
	    __BITS(16,17),			/* sel */
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_MSDC30_0_SEL, "msdc30_0_sel",
	    mt2701_topckgen_msdc30_parents,
	    mt2701_clk_cfg2_regs,
	    __BITS(24,26),			/* sel */
	    __BIT(31),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_MUXGATE(MT2701_CLK_TOP_MSDC30_1_SEL, "msdc30_1_sel",
	    mt2701_topckgen_msdc30_parents,
	    mt2701_clk_cfg3_regs,
	    __BITS(0,2),			/* sel */
	    __BIT(7),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_MSDC30_2_SEL, "msdc30_2_sel",
	    mt2701_topckgen_msdc30_parents,
	    mt2701_clk_cfg3_regs,
	    __BITS(8,10),			/* sel */
	    __BIT(15),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_MUXGATE(MT2701_CLK_TOP_AUDIO_SEL, "audio_sel",
	    mt2701_topckgen_audio_parents,
	    mt2701_clk_cfg3_regs,
	    __BIT(16),				/* sel */
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_AUDINTBUS_SEL, "aud_intbus_sel",
	    mt2701_topckgen_aud_intbus_parents,
	    mt2701_clk_cfg3_regs,
	    __BITS(24,26),			/* sel */
	    __BIT(31),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_PMICSPI_SEL, "pmicspi_sel",
	    mt2701_topckgen_pmicspi_parents,
	    mt2701_clk_cfg4_regs,
	    __BITS(0,3),			/* sel */
	    __BIT(7),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_SCP_SEL, "scp_sel",
	    mt2701_topckgen_scp_parents,
	    mt2701_clk_cfg4_regs,
	    __BITS(8,9),			/* sel */
	    __BIT(15),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_DPI0_SEL, "dpi0_sel",
	    mt2701_topckgen_dpi0_parents,
	    mt2701_clk_cfg4_regs,
	    __BITS(16,18),			/* sel */
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_DPI1_SEL, "dpi1_sel",
	    mt2701_topckgen_dpi1_parents,
	    mt2701_clk_cfg4_regs,
	    __BITS(24,25),			/* sel */
	    __BIT(31),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_TVE_SEL, "tve_sel",
	    mt2701_topckgen_tve_parents,
	    mt2701_clk_cfg5_regs,
	    __BITS(0,2),			/* sel */
	    __BIT(7),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_HDMI_SEL, "hdmi_sel",
	    mt2701_topckgen_hdmi_parents,
	    mt2701_clk_cfg5_regs,
	    __BITS(8,9),			/* sel */
	    __BIT(15),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_APLL_SEL, "apll_sel",
	    mt2701_topckgen_apll_parents,
	    mt2701_clk_cfg5_regs,
	    __BITS(16,18),			/* sel */
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE_CLKF(MT2701_CLK_TOP_RTC_SEL, "rtc_sel",
	    mt2701_topckgen_rtc_parents,
	    mt2701_clk_cfg6_regs,
	    __BITS(0,1),			/* sel */
	    __BIT(7),				/* enable */
	    MTK_CLK_GATE_ACT_LOW,
	    0),					/* clk flags */

	MTK_CLK_MUXGATE(MT2701_CLK_TOP_NFI2X_SEL, "nfi2x_sel",
	    mt2701_topckgen_nfi2x_parents,
	    mt2701_clk_cfg6_regs,
	    __BITS(8,10),			/* sel */
	    __BIT(15),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_EMMC_HCLK_SEL, "emmc_hclk_sel",
	    mt2701_topckgen_emmc_hclk_parents,
	    mt2701_clk_cfg6_regs,
	    __BITS(24,25),			/* sel */
	    __BIT(31),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_MUXGATE(MT2701_CLK_TOP_FLASH_SEL, "flash_sel",
	    mt2701_topckgen_flash_parents,
	    mt2701_clk_cfg7_regs,
	    __BITS(0,2),			/* sel */
	    __BIT(7),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_DI_SEL, "di_sel",
	    mt2701_topckgen_di_parents,
	    mt2701_clk_cfg7_regs,
	    __BITS(8,9),			/* sel */
	    __BIT(15),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_NR_SEL, "nr_sel",
	    mt2701_topckgen_nr_osd_parents,
	    mt2701_clk_cfg7_regs,
	    __BITS(16,18),			/* sel */
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_OSD_SEL, "osd_sel",
	    mt2701_topckgen_nr_osd_parents,
	    mt2701_clk_cfg7_regs,
	    __BITS(24,26),			/* sel */
	    __BIT(31),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_HDMIRX_BIST_SEL, "hdmirx_bist_sel",
	    mt2701_topckgen_hdmirx_bist_parents,
	    mt2701_clk_cfg12_regs,
	    __BITS(0,2),			/* sel */
	    __BIT(7),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_INTDIR_SEL, "intdir_sel",
	    mt2701_topckgen_intdir_parents,
	    mt2701_clk_cfg12_regs,
	    __BITS(8,9),			/* sel */
	    __BIT(15),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_ASM_I_SEL, "asm_i_sel",
	    mt2701_topckgen_asm_parents,
	    mt2701_clk_cfg12_regs,
	    __BITS(16,17),			/* sel */
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_ASM_M_SEL, "asm_m_sel",
	    mt2701_topckgen_asm_parents,
	    mt2701_clk_cfg12_regs,
	    __BITS(24,25),			/* sel */
	    __BIT(31),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_ASM_H_SEL, "asm_h_sel",
	    mt2701_topckgen_asm_parents,
	    mt2701_clk_cfg13_regs,
	    __BITS(0,1),			/* sel */
	    __BIT(7),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_MS_CARD_SEL, "ms_card_sel",
	    mt2701_topckgen_ms_card_parents,
	    mt2701_clk_cfg13_regs,
	    __BITS(16,17),			/* sel */
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_ETHIF_SEL, "ethif_sel",
	    mt2701_topckgen_ethif_parents,
	    mt2701_clk_cfg13_regs,
	    __BITS(24,26),			/* sel */
	    __BIT(31),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_HDMIRX26_24_SEL, "hdmirx26_24_sel",
	    mt2701_topckgen_hdmirx_parents,
	    mt2701_clk_cfg14_regs,
	    __BIT(0),				/* sel */
	    __BIT(7),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_MSDC30_3_SEL, "msdc30_3_sel",
	    mt2701_topckgen_msdc30_parents,
	    mt2701_clk_cfg14_regs,
	    __BITS(8,10),			/* sel */
	    __BIT(15),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_MUXGATE(MT2701_CLK_TOP_CMSYS_SEL, "cmsys_sel",
	    mt2701_topckgen_cmsys_parents,
	    mt2701_clk_cfg14_regs,
	    __BITS(16,19),			/* sel */
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	/*
	 * XXXJRT Data sheet says this is spi2_sel, and Linux identifies
	 * this as spi2_sel, but uses CLK_TOP_SPI1_SEL clock ID??
	 */
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_SPI1_SEL, "spi2_sel",
	    mt2701_topckgen_spi_parents,
	    mt2701_clk_cfg14_regs,
	    __BITS(24,26),			/* sel */
	    __BIT(31),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	/*
	 * XXXJRT Data sheet says this is spi1_sel, and Linux identifies
	 * this as spi1_sel, but uses CLK_TOP_SPI2_SEL clock ID??
	 */
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_SPI2_SEL, "spi1_sel",
	    mt2701_topckgen_spi_parents,
	    mt2701_clk_cfg15_regs,
	    __BITS(0,2),			/* sel */
	    __BIT(7),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_MUXGATE(MT2701_CLK_TOP_8BDAC_SEL, "8bdac_sel",
	    mt2701_topckgen_8bdac_parents,
	    mt2701_clk_cfg15_regs,
	    __BITS(8,9),			/* sel */
	    __BIT(15),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_AUD2DVD_SEL, "aud2dvd_sel",
	    mt2701_topckgen_aud2dvd_parents,
	    mt2701_clk_cfg15_regs,
	    __BIT(16),				/* sel */
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUX(MT2701_CLK_TOP_PADMCLK_SEL, "padmclk_sel",
	    mt2701_topckgen_padmclk_parents,
	    TOPCKGEN_CLK_CFG_8,
	    __BITS(0,2)),			/* sel */

	MTK_CLK_MUX(MT2701_CLK_TOP_AUD_MUX1_SEL, "aud_mux1_sel",
	    mt2701_topckgen_aud_mux_parents,
	    TOPCKGEN_CLK_AUDDIV3,
	    __BITS(0,2)),			/* sel */
	
	MTK_CLK_MUX(MT2701_CLK_TOP_AUD_MUX2_SEL, "aud_mux2_sel",
	    mt2701_topckgen_aud_mux_parents,
	    TOPCKGEN_CLK_AUDDIV3,
	    __BITS(3,5)),			/* sel */
	
	MTK_CLK_MUX(MT2701_CLK_TOP_AUDPLL_MUX_SEL, "audpll_sel",
	    mt2701_topckgen_aud_mux_parents,
	    TOPCKGEN_CLK_AUDDIV3,
	    __BITS(6,8)),			/* sel */
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_AUD_K1_SRC_SEL, "aud_k1_src_sel",
	    mt2701_topckgen_aud_src_parents,
	    mt2701_clk_auddiv3_regs,
	    __BIT(15),				/* sel */
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_AUD_K2_SRC_SEL, "aud_k2_src_sel",
	    mt2701_topckgen_aud_src_parents,
	    mt2701_clk_auddiv3_regs,
	    __BIT(16),				/* sel */
	    __BIT(24),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_AUD_K3_SRC_SEL, "aud_k3_src_sel",
	    mt2701_topckgen_aud_src_parents,
	    mt2701_clk_auddiv3_regs,
	    __BIT(17),				/* sel */
	    __BIT(25),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_AUD_K4_SRC_SEL, "aud_k4_src_sel",
	    mt2701_topckgen_aud_src_parents,
	    mt2701_clk_auddiv3_regs,
	    __BIT(18),				/* sel */
	    __BIT(26),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_MUXGATE(MT2701_CLK_TOP_AUD_K5_SRC_SEL, "aud_k5_src_sel",
	    mt2701_topckgen_aud_src_parents,
	    mt2701_clk_auddiv3_regs,
	    __BIT(19),				/* sel */
	    __BIT(27),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_MUXGATE(MT2701_CLK_TOP_AUD_K6_SRC_SEL, "aud_k6_src_sel",
	    mt2701_topckgen_aud_src_parents,
	    mt2701_clk_auddiv3_regs,
	    __BIT(20),				/* sel */
	    __BIT(28),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_GATE(MT2701_CLK_TOP_AUD_48K_TIMING, "a1sys_hp_ck",
	    "aud_mux1_div",
	    mt2701_clk_auddiv3_regs,
	    __BIT(21),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_GATE(MT2701_CLK_TOP_AUD_44K_TIMING, "a2sys_hp_ck",
	    "aud_mux2_div",
	    mt2701_clk_auddiv3_regs,
	    __BIT(22),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_GATE(MT2701_CLK_TOP_AUD_I2S1_MCLK, "aud_i2s1_mclk",
	    "aud_k1_src_div",
	    mt2701_clk_auddiv3_regs,
	    __BIT(23),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_GATE(MT2701_CLK_TOP_AUD_I2S2_MCLK, "aud_i2s2_mclk",
	    "aud_k2_src_div",
	    mt2701_clk_auddiv3_regs,
	    __BIT(24),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_GATE(MT2701_CLK_TOP_AUD_I2S3_MCLK, "aud_i2s3_mclk",
	    "aud_k3_src_div",
	    mt2701_clk_auddiv3_regs,
	    __BIT(25),
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_GATE(MT2701_CLK_TOP_AUD_I2S4_MCLK, "aud_i2s4_mclk",
	    "aud_k4_src_div",
	    mt2701_clk_auddiv3_regs,
	    __BIT(26),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_GATE(MT2701_CLK_TOP_AUD_I2S5_MCLK, "aud_i2s5_mclk",
	    "aud_k5_src_div",
	    mt2701_clk_auddiv3_regs,
	    __BIT(27),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),
	
	MTK_CLK_GATE(MT2701_CLK_TOP_AUD_I2S6_MCLK, "aud_i2s6_mclk",
	    "aud_k6_src_div",
	    mt2701_clk_auddiv3_regs,
	    __BIT(28),				/* enable */
	    MTK_CLK_GATE_ACT_LOW),

	MTK_CLK_DIV(MT2701_CLK_TOP_AUD_EXTCK1_DIV, "audio_ext1_ck",
	    "aud_ext1",
	    TOPCKGEN_CLK_AUDDIV0,
	    __BITS(0,7),			/* div */
	    0),
	
	MTK_CLK_DIV(MT2701_CLK_TOP_AUD_EXTCK2_DIV, "audio_ext2_ck",
	    "aud_ext2",
	    TOPCKGEN_CLK_AUDDIV0,
	    __BITS(8,15),			/* div */
	    0),
	
	MTK_CLK_DIV(MT2701_CLK_TOP_AUD_MUX1_DIV, "aud_mux1_div",
	    "aud_mux1_sel",
	    TOPCKGEN_CLK_AUDDIV0,
	    __BITS(16,23),			/* div */
	    0),
	
	MTK_CLK_DIV(MT2701_CLK_TOP_AUD_MUX2_DIV, "aud_mux2_div",
	    "aud_mux2_sel",
	    TOPCKGEN_CLK_AUDDIV0,
	    __BITS(24,31),			/* div */
	    0),
	
	MTK_CLK_DIV(MT2701_CLK_TOP_AUD_K1_SRC_DIV, "aud_k1_src_div",
	    "aud_k1_src_sel",
	    TOPCKGEN_CLK_AUDDIV1,
	    __BITS(0,7),			/* div */
	    0),

	MTK_CLK_DIV(MT2701_CLK_TOP_AUD_K2_SRC_DIV, "aud_k2_src_div",
	    "aud_k2_src_sel",
	    TOPCKGEN_CLK_AUDDIV1,
	    __BITS(8,15),			/* div */
	    0),
	
	MTK_CLK_DIV(MT2701_CLK_TOP_AUD_K3_SRC_DIV, "aud_k3_src_div",
	    "aud_k3_src_sel",
	    TOPCKGEN_CLK_AUDDIV1,
	    __BITS(16,23),			/* div */
	    0),
	
	MTK_CLK_DIV(MT2701_CLK_TOP_AUD_K4_SRC_DIV, "aud_k4_src_div",
	    "aud_k4_src_sel",
	    TOPCKGEN_CLK_AUDDIV1,
	    __BITS(24,31),			/* div */
	    0),
	
	MTK_CLK_DIV(MT2701_CLK_TOP_AUD_K5_SRC_DIV, "aud_k5_src_div",
	    "aud_k5_src_sel",
	    TOPCKGEN_CLK_AUDDIV2,
	    __BITS(0,7),			/* div */
	    0),
	
	MTK_CLK_DIV(MT2701_CLK_TOP_AUD_K6_SRC_DIV, "aud_k6_src_div",
	    "aud_k6_src_sel",
	    TOPCKGEN_CLK_AUDDIV2,
	    __BITS(8,15),			/* div */
	    0),
};

static int
mt2701_topckgen_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mt2701_topckgen_attach(device_t parent, device_t self, void *aux)
{
	struct mt2701_topckgen_softc * const topckgen_sc = device_private(self);
	struct mtk_cru_softc * const sc = &topckgen_sc->sc_mtk_cru;
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_clks = mt2701_topckgen_clks;
	sc->sc_nclks = __arraycount(mt2701_topckgen_clks);

	if (mtk_cru_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": Top Clock Generator\n");

	mtk_cru_print(sc);
}
