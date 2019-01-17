/* $NetBSD$ */

/*-
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
__KERNEL_RCSID(0, "$NetBSD: meson_clk_fixed.c,v 1.2 2017/06/29 17:06:21 jmcneill Exp $");

#include <sys/param.h>

#include <arm/amlogic/meson_pinctrl.h>

/* CBUS pinmux registers */
#define	CBUS_REG(n)	((n) << 2)
#define	REG0		CBUS_REG(0)
#define	REG1		CBUS_REG(1)
#define	REG2		CBUS_REG(2)
#define	REG3		CBUS_REG(3)
#define	REG4		CBUS_REG(4)
#define	REG5		CBUS_REG(5)
#define	REG6		CBUS_REG(6)
#define	REG7		CBUS_REG(7)
#define	REG8		CBUS_REG(8)
#define	REG9		CBUS_REG(9)

/* AO pinmux registers */
#define	REG		0x00

enum {
	NO_BANK = 0,

	GPIOX_0,
	GPIOX_1,
	GPIOX_2,
	GPIOX_3,
	GPIOX_4,
	GPIOX_5,
	GPIOX_6,
	GPIOX_7,
	GPIOX_8,
	GPIOX_9,
	GPIOX_10,
	GPIOX_11,
	GPIOX_16,
	GPIOX_17,
	GPIOX_18,
	GPIOX_19,
	GPIOX_20,
	GPIOX_21,

	GPIOY_0,
	GPIOY_1,
	GPIOY_3,
	GPIOY_6,
	GPIOY_7,
	GPIOY_8,
	GPIOY_9,
	GPIOY_10,
	GPIOY_11,
	GPIOY_12,
	GPIOY_13,
	GPIOY_14,

	GPIODV_9,
	GPIODV_24,
	GPIODV_25,
	GPIODV_26,
	GPIODV_27,
	GPIODV_28,
	GPIODV_29,

	GPIOH_0,
	GPIOH_1,
	GPIOH_2,
	GPIOH_3,
	GPIOH_4,
	GPIOH_5,
	GPIOH_6,
	GPIOH_7,
	GPIOH_8,
	GPIOH_9,

	GPIOAO_0,
	GPIOAO_1,
	GPIOAO_2,
	GPIOAO_3,
	GPIOAO_4,
	GPIOAO_5,
	GPIOAO_6,
	GPIOAO_7,
	GPIOAO_8,
	GPIOAO_9,
	GPIOAO_10,
	GPIOAO_11,
	GPIOAO_12,
	GPIOAO_13,
	TEST_N,

	BOOT_0,
	BOOT_1,
	BOOT_2,
	BOOT_3,
	BOOT_4,
	BOOT_5,
	BOOT_6,
	BOOT_7,
	BOOT_8,
	BOOT_9,
	BOOT_10,
	BOOT_11,
	BOOT_12,
	BOOT_13,
	BOOT_14,
	BOOT_15,
	BOOT_18,

	CARD_0,
	CARD_1,
	CARD_2,
	CARD_3,
	CARD_4,
	CARD_5,
	CARD_6,

	DIF_0_P,
	DIF_0_N,
	DIF_1_P,
	DIF_1_N,
	DIF_2_P,
	DIF_2_N,
	DIF_3_P,
	DIF_3_N,
	DIF_4_P,
	DIF_4_N,
};

static const struct meson_pinctrl_group meson8b_cbus_groups[] = {
	/* GPIOX */
	{ "sd_d0_a",		REG8,	5,	{ GPIOX_0 } },
	{ "sd_d1_a",		REG8,	4,	{ GPIOX_1 } },
	{ "sd_d2_a",		REG8,	3,	{ GPIOX_2 } },
	{ "sd_d3_a",		REG8,	2,	{ GPIOX_3 } },
	{ "sdxc_d0_0_a",	REG5,	29,	{ GPIOX_4 } },
	{ "sdxc_d47_a",		REG5,	12,	{ GPIOX_4, GPIOX_5, GPIOX_6, GPIOX_7 } },
	{ "sdxc_d13_0_a",	REG5,	28,	{ GPIOX_5, GPIOX_6, GPIOX_7 } },
	{ "sd_clk_a",		REG8,	1,	{ GPIOX_8 } },
	{ "sd_cmd_a",		REG8,	0,	{ GPIOX_9 } },
	{ "xtal_32k_out",	REG3,	22,	{ GPIOX_10 } },
	{ "xtal_24m_out",	REG3,	20,	{ GPIOX_11 } },
	{ "uart_tx_b0",		REG4,	9,	{ GPIOX_16 } },
	{ "uart_rx_b0",		REG4,	8,	{ GPIOX_17 } },
	{ "uart_cts_b0",	REG4,	7,	{ GPIOX_18 } },
	{ "uart_rts_b0",	REG4,	6,	{ GPIOX_19 } },
	{ "sdxc_d0_1_a",	REG5,	14,	{ GPIOX_0 } },
	{ "sdxc_d13_1_a",	REG5,	13,	{ GPIOX_1, GPIOX_2, GPIOX_3 } },
	{ "pcm_out_a",		REG3,	30,	{ GPIOX_4 } },
	{ "pcm_in_a",		REG3,	29,	{ GPIOX_5 } },
	{ "pcm_fs_a",		REG3,	28,	{ GPIOX_6 } },
	{ "pcm_clk_a",		REG3,	27,	{ GPIOX_7 } },
	{ "sdxc_clk_a",		REG5,	11,	{ GPIOX_8 } },
	{ "sdxc_cmd_a",		REG5,	10,	{ GPIOX_9 } },
	{ "pwm_vs_0",		REG7,	31,	{ GPIOX_10 } },
	{ "pwm_e",		REG9,	19,	{ GPIOX_10 } },
	{ "pwm_vs_1",		REG7,	30,	{ GPIOX_11 } },
	{ "uart_tx_a",		REG4,	17,	{ GPIOX_4 } },
	{ "uart_rx_a",		REG4,	16,	{ GPIOX_5 } },
	{ "uart_cts_a",		REG4,	15,	{ GPIOX_6 } },
	{ "uart_rts_a",		REG4,	14,	{ GPIOX_7 } },
	{ "uart_tx_b1",		REG6,	19,	{ GPIOX_8 } },
	{ "uart_rx_b1",		REG6,	18,	{ GPIOX_9 } },
	{ "uart_cts_b1",	REG6,	17,	{ GPIOX_10 } },
	{ "uart_rts_b1",	REG6,	16,	{ GPIOX_20 } },
	{ "iso7816_0_clk",	REG5,	9,	{ GPIOX_6 } },
	{ "iso7816_0_data",	REG5,	8,	{ GPIOX_7 } },
	{ "spi_sclk_0",		REG4,	22,	{ GPIOX_8 } },
	{ "spi_miso_0",		REG4,	24,	{ GPIOX_9 } },
	{ "spi_mosi_0",		REG4,	23,	{ GPIOX_10 } },
	{ "iso7816_det",	REG4,	21,	{ GPIOX_16 } },
	{ "iso7816_reset",	REG4,	20,	{ GPIOX_17 } },
	{ "iso7816_1_clk",	REG4,	19,	{ GPIOX_18 } },
	{ "iso7816_1_data",	REG4,	18,	{ GPIOX_19 } },
	{ "spi_ss0_0",		REG4,	25,	{ GPIOX_20 } },
	{ "tsin_clk_b",		REG3,	6,	{ GPIOX_8 } },
	{ "tsin_sop_b",		REG3,	7,	{ GPIOX_9 } },
	{ "tsin_d0_b",		REG3,	8,	{ GPIOX_10 } },
	{ "pwm_b",		REG2,	3,	{ GPIOX_11 } },
	{ "i2c_sda_d0",		REG4,	5,	{ GPIOX_16 } },
	{ "i2c_sck_d0",		REG4,	4,	{ GPIOX_17 } },
	{ "tsin_d_valid_b",	REG3,	9,	{ GPIOX_20 } },

	/* GPIOY */
	{ "tsin_d_valid_a",	REG3,	2,	{ GPIOY_0 } },
	{ "tsin_sop_a",		REG3,	1,	{ GPIOY_1 } },
	{ "tsin_d17_a",		REG3,	5,	{ GPIOY_6, GPIOY_7, GPIOY_10, GPIOY_11, GPIOY_12, GPIOY_13, GPIOY_14 } },
	{ "tsin_clk_a",		REG3,	0,	{ GPIOY_8 } },
	{ "tsin_d0_a",		REG3,	4,	{ GPIOY_9 } },
	{ "spdif_out_0",	REG1,	7,	{ GPIOY_3 } },
	{ "xtal_24m",		REG3,	18,	{ GPIOY_3 } },
	{ "iso7816_2_clk",	REG5,	7,	{ GPIOY_13 } },
	{ "iso7816_2_data",	REG5,	6,	{ GPIOY_14 } },

	/* GPIODV */
	{ "pwm_d",		REG3,	26,	{ GPIODV_28 } },
	{ "pwm_c0",		REG3,	25,	{ GPIODV_29 } },
	{ "pwm_vs_2",		REG7,	28,	{ GPIODV_9 } },
	{ "pwm_vs_3",		REG7,	27,	{ GPIODV_28 } },
	{ "pwm_vs_4",		REG7,	26,	{ GPIODV_29 } },
	{ "xtal24_out",		REG7,	25,	{ GPIODV_29 } },
	{ "uart_tx_c",		REG6,	23,	{ GPIODV_24 } },
	{ "uart_rx_c",		REG6,	22,	{ GPIODV_25 } },
	{ "uart_cts_c",		REG6,	21,	{ GPIODV_26 } },
	{ "uart_rts_c",		REG6,	20,	{ GPIODV_27 } },
	{ "pwm_c1",		REG3,	24,	{ GPIODV_9 } },
	{ "i2c_sda_a",		REG9,	31,	{ GPIODV_24 } },
	{ "i2c_sck_a",		REG9,	30,	{ GPIODV_25 } },
	{ "i2c_sda_b0",		REG9,	29,	{ GPIODV_26 } },
	{ "i2c_sck_b0",		REG9,	28,	{ GPIODV_27 } },
	{ "i2c_sda_c0",		REG9,	27,	{ GPIODV_28 } },
	{ "i2c_sck_c0",		REG9,	26,	{ GPIODV_29 } },

	/* GPIOH */
	{ "hdmi_hpd",		REG1,	26,	{ GPIOH_0 } },
	{ "hdmi_sda",		REG1,	25,	{ GPIOH_1 } },
	{ "hdmi_scl",		REG1,	24,	{ GPIOH_2 } },
	{ "hdmi_cec_0",		REG1,	23,	{ GPIOH_3 } },
	{ "eth_txd1_0",		REG7,	21,	{ GPIOH_5 } },
	{ "eth_txd0_0",		REG7,	20,	{ GPIOH_6 } },
	{ "clk_24m_out",	REG4,	1,	{ GPIOH_9 } },
	{ "spi_ss1",		REG8,	11,	{ GPIOH_0 } },
	{ "spi_ss2",		REG8,	12,	{ GPIOH_1 } },
	{ "spi_ss0_1",		REG9,	13,	{ GPIOH_3 } },
	{ "spi_miso_1",		REG9,	12,	{ GPIOH_4 } },
	{ "spi_mosi_1",		REG9,	11,	{ GPIOH_5 } },
	{ "spi_sclk_1",		REG9,	10,	{ GPIOH_6 } },
	{ "eth_txd3",		REG6,	13,	{ GPIOH_7 } },
	{ "eth_txd2",		REG6,	12,	{ GPIOH_8 } },
	{ "eth_tx_clk",		REG6,	11,	{ GPIOH_9 } },
	{ "i2c_sda_b1",		REG5,	27,	{ GPIOH_3 } },
	{ "i2c_sck_b1",		REG5,	26,	{ GPIOH_4 } },
	{ "i2c_sda_c1",		REG5,	25,	{ GPIOH_5 } },
	{ "i2c_sck_c1",		REG5,	24,	{ GPIOH_6 } },
	{ "i2c_sda_d1",		REG4,	3,	{ GPIOH_7 } },
	{ "i2c_sck_d1",		REG4,	2,	{ GPIOH_8 } },

	/* BOOT */
	{ "nand_io",		REG2,	26,	{ BOOT_0, BOOT_1, BOOT_2, BOOT_3, BOOT_4, BOOT_5, BOOT_6, BOOT_7 } },
	{ "nand_io_ce0",	REG2,	25,	{ BOOT_8 } },
	{ "nand_io_ce1",	REG2,	24,	{ BOOT_9 } },
	{ "nand_io_rb0",	REG2,	17,	{ BOOT_10 } },
	{ "nand_ale",		REG2,	21,	{ BOOT_11 } },
	{ "nand_cle",		REG2,	20,	{ BOOT_12 } },
	{ "nand_wen_clk",	REG2,	19,	{ BOOT_13 } },
	{ "nand_ren_clk",	REG2,	18,	{ BOOT_14 } },
	{ "nand_dqs_15",	REG2,	27,	{ BOOT_15 } },
	{ "nand_dqs_18",	REG2,	28,	{ BOOT_18 } },
	{ "sdxc_d0_c",		REG4,	30,	{ BOOT_0 } },
	{ "sdxc_d13_c",		REG4,	29,	{ BOOT_1, BOOT_2, BOOT_3 } },
	{ "sdxc_d47_c",		REG4,	28,	{ BOOT_4, BOOT_5, BOOT_6, BOOT_7 } },
	{ "sdxc_clk_c",		REG7,	19,	{ BOOT_8 } },
	{ "sdxc_cmd_c",		REG7,	18,	{ BOOT_10 } },
	{ "nor_d",		REG5,	1,	{ BOOT_11 } },
	{ "nor_q",		REG5,	3,	{ BOOT_12 } },
	{ "nor_c",		REG5,	2,	{ BOOT_13 } },
	{ "nor_cs",		REG5,	0,	{ BOOT_18 } },
	{ "sd_d0_c",		REG6,	29,	{ BOOT_0 } },
	{ "sd_d1_c",		REG6,	28,	{ BOOT_1 } },
	{ "sd_d2_c",		REG6,	27,	{ BOOT_2 } },
	{ "sd_d3_c",		REG6,	26,	{ BOOT_3 } },
	{ "sd_cmd_c",		REG6,	30,	{ BOOT_8 } },
	{ "sd_clk_c",		REG6,	31,	{ BOOT_10 } },

	/* CARD */
	{ "sd_d1_b",		REG2,	14,	{ CARD_0 } },
	{ "sd_d0_b",		REG2,	15,	{ CARD_1 } },
	{ "sd_clk_b",		REG2,	11,	{ CARD_2 } },
	{ "sd_cmd_b",		REG2,	10,	{ CARD_3 } },
	{ "sd_d3_b",		REG2,	12,	{ CARD_4 } },
	{ "sd_d2_b",		REG2,	13,	{ CARD_5 } },
	{ "sdxc_d13_b",		REG2,	6,	{ CARD_0, CARD_4, CARD_5 } },
	{ "sdxc_d0_b",		REG2,	7,	{ CARD_1 } },
	{ "sdxc_clk_b",		REG2,	5,	{ CARD_2 } },
	{ "sdxc_cmd_b",		REG2,	4,	{ CARD_3 } },

	/* DIF */
	{ "eth_rxd1",		REG6,	0,	{ DIF_0_P } },
	{ "eth_rxd0",		REG6,	1,	{ DIF_0_N } },
	{ "eth_rx_dv",		REG6,	2,	{ DIF_1_P } },
	{ "eth_rx_clk",		REG6,	3,	{ DIF_1_N } },
	{ "eth_txd0_1",		REG6,	4,	{ DIF_2_P } },
	{ "eth_txd1_1",		REG6,	5,	{ DIF_2_N } },
	{ "eth_tx_en",		REG6,	6,	{ DIF_3_P } },
	{ "eth_ref_clk",	REG6,	8,	{ DIF_3_N } },
	{ "eth_mdc",		REG6,	9,	{ DIF_4_P } },
	{ "eth_mdio_en",	REG6,	10,	{ DIF_4_N } },
};

static const struct meson_pinctrl_group meson8b_aobus_groups[] = {
	/* GPIOAO */
	{ "uart_tx_ao_a",	REG,	12,	{ GPIOAO_0 } },
	{ "uart_rx_ao_a",	REG,	11,	{ GPIOAO_1 } },
	{ "uart_cts_ao_a",	REG,	10,	{ GPIOAO_2 } },
	{ "uart_rts_ao_a",	REG,	9,	{ GPIOAO_3 } },
	{ "i2c_mst_sck_ao",	REG,	6,	{ GPIOAO_4 } },
	{ "i2c_mst_sda_ao",	REG,	5,	{ GPIOAO_5 } },
	{ "clk_32k_in_out",	REG,	18,	{ GPIOAO_6 } },
	{ "remote_input",	REG,	0,	{ GPIOAO_7 } },
	{ "hdmi_cec_1",		REG,	17,	{ GPIOAO_12 } },
	{ "ir_blaster",		REG,	31,	{ GPIOAO_13 } },
	{ "pwm_c2",		REG,	22,	{ GPIOAO_3 } },
	{ "i2c_sck_ao",		REG,	2,	{ GPIOAO_4 } },
	{ "i2c_sda_ao",		REG,	1,	{ GPIOAO_5 } },
	{ "ir_remote_out",	REG,	21,	{ GPIOAO_7 } },
	{ "i2s_am_clk_out",	REG,	30,	{ GPIOAO_8 } },
	{ "i2s_ao_clk_out",	REG,	29,	{ GPIOAO_9 } },
	{ "i2s_lr_clk_out",	REG,	28,	{ GPIOAO_10 } },
	{ "i2s_out_01",		REG,	27,	{ GPIOAO_11 } },
	{ "uart_tx_ao_b0",	REG,	26,	{ GPIOAO_0 } },
	{ "uart_rx_ao_b0",	REG,	25,	{ GPIOAO_1 } },
	{ "uart_cts_ao_b",	REG,	8,	{ GPIOAO_2 } },
	{ "uart_rts_ao_b",	REG,	7,	{ GPIOAO_3 } },
	{ "uart_tx_ao_b1",	REG,	24,	{ GPIOAO_4 } },
	{ "uart_rx_ao_b1",	REG,	23,	{ GPIOAO_5 } },
	{ "spdif_out_1",	REG,	16,	{ GPIOAO_6 } },
	{ "i2s_in_ch01",	REG,	13,	{ GPIOAO_6 } },
	{ "i2s_ao_clk_in",	REG,	15,	{ GPIOAO_9 } },
	{ "i2s_lr_clk_in",	REG,	14,	{ GPIOAO_10 } },
};

const struct meson_pinctrl_config meson8b_cbus_pinctrl_config = {
	.name = "Meson8b CBUS GPIO",
	.groups = meson8b_cbus_groups,
	.ngroups = __arraycount(meson8b_cbus_groups),
};

const struct meson_pinctrl_config meson8b_aobus_pinctrl_config = {
	.name = "Meson8b AO GPIO",
	.groups = meson8b_aobus_groups,
	.ngroups = __arraycount(meson8b_aobus_groups),
};
