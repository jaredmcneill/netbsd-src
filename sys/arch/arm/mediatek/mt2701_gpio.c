/* $NetBSD$ */

/*-
 * Copyright (c) 2018, 2019 Jason R. Thorpe
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

#include <sys/types.h>
#include <sys/bus.h>

#include <arm/mediatek/mtk_gpio.h>

/* Relative to GPIO Base address */
#define	GPIO_GPIO_DIR1			0x000
#define	GPIO_GPIO_DIR2			0x010
#define	GPIO_GPIO_DIR3			0x020
#define	GPIO_GPIO_DIR4			0x030
#define	GPIO_GPIO_DIR5			0x040
#define	GPIO_GPIO_DIR6			0x050
#define	GPIO_GPIO_DIR7			0x060
#define	GPIO_GPIO_DIR8			0x070
#define	GPIO_GPIO_DIR9			0x080
#define	GPIO_GPIO_DIR10			0x090
#define	GPIO_GPIO_DIR11			0x0a0
#define	GPIO_MSDC1_CTRL6		0x0b0
#define	GPIO_GPIO_DIR12			0x0c0
#define	GPIO_GPIO_DIR13			0x0d0
#define	GPIO_GPIO_DIR14			0x0e0
#define	GPIO_GPIO_DIR15			0x0f0
#define	GPIO_GPIO_DIR16			0x100
#define	GPIO_GPIO_DIR17			0x110
#define	GPIO_GPIO_DIR18			0x120
#define	GPIO_SDIO_CTRL4			0x130	/* a.k.a. MSDC3_CTRL4 */
#define	GPIO_SDIO_CTRL5			0x140	/* a.k.a. MSDC3_CTRL5 */
#define	GPIO_GPIO_PULLEN1		0x150
#define	GPIO_GPIO_PULLEN2		0x160
#define	GPIO_GPIO_PULLEN3		0x170
#define	GPIO_GPIO_PULLEN4		0x180
#define	GPIO_GPIO_PULLEN5		0x190
#define	GPIO_GPIO_PULLEN6		0x1a0
#define	GPIO_GPIO_PULLEN7		0x1b0
#define	GPIO_GPIO_PULLEN8		0x1c0
#define	GPIO_GPIO_PULLEN9		0x1d0
#define	GPIO_GPIO_PULLEN10		0x1e0
#define	GPIO_GPIO_PULLEN11		0x1f0
#define	GPIO_GPIO_PULLEN12		0x200
#define	GPIO_GPIO_PULLEN13		0x210
#define	GPIO_GPIO_PULLEN14		0x220
#define	GPIO_GPIO_PULLEN15		0x230
#define	GPIO_GPIO_PULLEN16		0x240
#define	GPIO_GPIO_PULLEN17		0x250
#define	GPIO_GPIO_PULLEN18		0x260
#define	GPIO_GPIO_PULLSEL1		0x280
#define	GPIO_GPIO_PULLSEL2		0x290
#define	GPIO_GPIO_PULLSEL3		0x2a0
#define	GPIO_GPIO_PULLSEL4		0x2b0
#define	GPIO_GPIO_PULLSEL5		0x2c0
#define	GPIO_GPIO_PULLSEL6		0x2d0
#define	GPIO_GPIO_PULLSEL7		0x2e0
#define	GPIO_GPIO_PULLSEL8		0x2f0
#define	GPIO_GPIO_PULLSEL9		0x300
#define	GPIO_GPIO_PULLSEL10		0x310
#define	GPIO_GPIO_PULLSEL11		0x320
#define	GPIO_GPIO_PULLSEL12		0x330
#define	GPIO_GPIO_PULLSEL13		0x340
#define	GPIO_GPIO_PULLSEL14		0x350
#define	GPIO_GPIO_PULLSEL15		0x360
#define	GPIO_GPIO_PULLSEL16		0x370
#define	GPIO_GPIO_PULLSEL17		0x380
#define	GPIO_GPIO_PULLSEL18		0x390
#define	GPIO_SDIO_CTRL7			0x3a0	/* a.k.a. MSDC3_CTRL7 */
#define	GPIO_BIAS_CTRL3			0x410
#define	GPIO_BIAS_CTRL4			0x420
#define	GPIO_SDIO_CTRL8			0x430
#define	GPIO_OD33_CTRL11		0x440
#define	GPIO_OD33_CTRL12		0x450
#define	GPIO_OD33_CTRL13		0x460
#define	GPIO_OD33_CTRL14		0x470
#define	GPIO_OD33_CTRL8			0x4c0
#define	GPIO_OD33_CTRL9			0x4d0
#define	GPIO_OD33_CTRL10		0x4e0
#define	GPIO_GPIO_DOUT1			0x500
#define	GPIO_GPIO_DOUT2			0x510
#define	GPIO_GPIO_DOUT3			0x520
#define	GPIO_GPIO_DOUT4			0x530
#define	GPIO_GPIO_DOUT5			0x540
#define	GPIO_GPIO_DOUT6			0x550
#define	GPIO_GPIO_DOUT7			0x560
#define	GPIO_GPIO_DOUT8			0x570
#define	GPIO_GPIO_DOUT9			0x580
#define	GPIO_GPIO_DOUT10		0x590
#define	GPIO_GPIO_DOUT11		0x5a0
#define	GPIO_GPIO_DOUT12		0x5b0
#define	GPIO_GPIO_DOUT13		0x5c0
#define	GPIO_GPIO_DOUT14		0x5d0
#define	GPIO_GPIO_DOUT15		0x5e0
#define	GPIO_GPIO_DOUT16		0x5f0
#define	GPIO_GPIO_DOUT17		0x600
#define	GPIO_GPIO_DOUT18		0x610
#define	GPIO_SDIO_CTRL6			0x620
#define	GPIO_GPIO_DIN1			0x630
#define	GPIO_GPIO_DIN2			0x640
#define	GPIO_GPIO_DIN3			0x650
#define	GPIO_GPIO_DIN4			0x660
#define	GPIO_GPIO_DIN5			0x670
#define	GPIO_GPIO_DIN6			0x680
#define	GPIO_GPIO_DIN7			0x690
#define	GPIO_GPIO_DIN8			0x6a0
#define	GPIO_GPIO_DIN9			0x6b0
#define	GPIO_GPIO_DIN10			0x6c0
#define	GPIO_GPIO_DIN11			0x6d0
#define	GPIO_GPIO_DIN12			0x6e0
#define	GPIO_GPIO_DIN13			0x6f0
#define	GPIO_GPIO_DIN14			0x700
#define	GPIO_GPIO_DIN15			0x710
#define	GPIO_GPIO_DIN16			0x720
#define	GPIO_GPIO_DIN17			0x730
#define	GPIO_GPIO_DIN18			0x740
#define	GPIO_GPIO_MODE1			0x760
#define	GPIO_GPIO_MODE2			0x770
#define	GPIO_GPIO_MODE3			0x780
#define	GPIO_GPIO_MODE4			0x790
#define	GPIO_GPIO_MODE5			0x7a0
#define	GPIO_GPIO_MODE6			0x7b0
#define	GPIO_GPIO_MODE7			0x7c0
#define	GPIO_GPIO_MODE8			0x7d0
#define	GPIO_GPIO_MODE9			0x7e0
#define	GPIO_GPIO_MODE10		0x7f0
#define	GPIO_GPIO_MODE11		0x800
#define	GPIO_GPIO_MODE12		0x810
#define	GPIO_GPIO_MODE13		0x820
#define	GPIO_GPIO_MODE14		0x830
#define	GPIO_GPIO_MODE15		0x840
#define	GPIO_GPIO_MODE16		0x850
#define	GPIO_GPIO_MODE17		0x860
#define	GPIO_GPIO_MODE18		0x870
#define	GPIO_GPIO_MODE19		0x880
#define	GPIO_GPIO_MODE20		0x890
#define	GPIO_GPIO_MODE21		0x8a0
#define	GPIO_GPIO_MODE22		0x8b0
#define	GPIO_GPIO_MODE23		0x8c0
#define	GPIO_GPIO_MODE24		0x8d0
#define	GPIO_GPIO_MODE25		0x8e0
#define	GPIO_GPIO_MODE26		0x8f0
#define	GPIO_GPIO_MODE27		0x900
#define	GPIO_GPIO_MODE28		0x910
#define	GPIO_GPIO_MODE29		0x920
#define	GPIO_GPIO_MODE30		0x930
#define	GPIO_GPIO_MODE31		0x940
#define	GPIO_GPIO_MODE32		0x950
#define	GPIO_GPIO_MODE33		0x960
#define	GPIO_GPIO_MODE34		0x970
#define	GPIO_GPIO_MODE35		0x980
#define	GPIO_GPIO_MODE36		0x990
#define	GPIO_GPIO_MODE37		0x9a0
#define	GPIO_GPIO_MODE38		0x9b0
#define	GPIO_GPIO_MODE39		0x9c0
#define	GPIO_GPIO_MODE40		0x9d0
#define	GPIO_GPIO_MODE41		0x9e0
#define	GPIO_GPIO_MODE42		0x9f0
#define	GPIO_GPIO_MODE43		0xa00
#define	GPIO_GPIO_MODE44		0xa10
#define	GPIO_GPIO_MODE45		0xa20
#define	GPIO_GPIO_MODE46		0xa30
#define	GPIO_GPIO_MODE47		0xa40
#define	GPIO_GPIO_MODE48		0xa50
#define	GPIO_GPIO_MODE49		0xa60
#define	GPIO_GPIO_MODE50		0xa70
#define	GPIO_GPIO_MODE51		0xa80
#define	GPIO_GPIO_MODE52		0xa90
#define	GPIO_GPIO_MODE53		0xaa0
#define	GPIO_GPIO_MODE54		0xab0
#define	GPIO_GPIO_MODE55		0xac0
#define	GPIO_GPIO_MODE56		0xad0
#define	GPIO_BANK_CTRL0			0xb10
#define	GPIO_IES_EN0			0xb20
#define	GPIO_IES_EN1			0xb30
#define	GPIO_IES_EN2			0xb40
#define	GPIO_SMT_EN0			0xb50
#define	GPIO_SMT_EN1			0xb60
#define	GPIO_SMT_EN2			0xb70
#define	GPIO_TDSEL0			0xb80
#define	GPIO_TDSEL1			0xb90
#define	GPIO_TDSEL2			0xba0
#define	GPIO_TDSEL3			0xbb0
#define	GPIO_TDSEL4			0xbc0
#define	GPIO_TDSEL5			0xbd0
#define	GPIO_OD33_CTRL4			0xbe0
#define	GPIO_OD33_CTRL5			0xbf0
#define	GPIO_OD33_CTRL6			0xc00
#define	GPIO_OD33_CTRL7			0xc10
#define	GPIO_RDSEL0			0xc20
#define	GPIO_RDSEL1			0xc30
#define	GPIO_RDSEL2			0xc40
#define	GPIO_RDSEL3			0xc50
#define	GPIO_RDSEL4			0xc60
#define	GPIO_RDSEL5			0xc70
#define	GPIO_DRVN0_EN			0xc80
#define	GPIO_MSDC3_CTRL0		0xc90
#define	GPIO_DRVP0_EN			0xca0
#define	GPIO_MSDC3_CTRL1		0xcb0
#define	GPIO_MSDC0_CTRL0		0xcc0
#define	GPIO_MSDC0_CTRL1		0xcd0
#define	GPIO_MSDC0_CTRL2		0xce0
#define	GPIO_MSDC0_CTRL3		0xcf0
#define	GPIO_MSDC0_CTRL4		0xd00
#define	GPIO_MSDC0_CTRL5		0xd10
#define	GPIO_MSDC0_CTRL6		0xd20
#define	GPIO_MSDC1_CTRL0		0xd30
#define	GPIO_MSDC1_CTRL1		0xd40
#define	GPIO_MSDC1_CTRL2		0xd50
#define	GPIO_MSDC1_CTRL3		0xd60
#define	GPIO_MSDC1_CTRL4		0xd70
#define	GPIO_MSDC1_CTRL5		0xd80
#define	GPIO_MSDC2_CTRL0		0xd90
#define	GPIO_MSDC2_CTRL1		0xda0
#define	GPIO_MSDC2_CTRL2		0xdb0
#define	GPIO_MSDC2_CTRL3		0xdc0
#define	GPIO_MSDC2_CTRL4		0xdd0
#define	GPIO_MSDC2_CTRL5		0xde0
#define	GPIO_GPIO_TM			0xdf0
#define	GPIO_GPIO_USB			0xe00
#define	GPIO_OD33_CTRL0			0xe10
#define	GPIO_OD33_CTRL1			0xe20
#define	GPIO_OD33_CTRL2			0xe30
#define	GPIO_OD33_CTRL3			0xe40
#define	GPIO_KPAD_CTRL0			0xe50
#define	GPIO_KPAD_CTRL1			0xe60
#define	GPIO_EINT_CTRL0			0xe70
#define	GPIO_EINT_CTRL1			0xe80
#define	GPIO_BIAS_CTRL0			0xeb0
#define	GPIO_BIAS_CTRL1			0xec0
#define	GPIO_BIAS_CTRL2			0xed0
#define	GPIO_BANK_CTRL2			0xef0
#define	GPIO_DRV_SEL10			0xf00
#define	GPIO_DRV_SEL11			0xf10
#define	GPIO_BANK_CTRL1			0xf20
#define	GPIO_DRV_SEL12			0xf30
#define	GPIO_SDIO_CTRL3			0xf40	/* a.k.a. MSDC3_CTRL3 */
#define	GPIO_DRV_SEL0			0xf50
#define	GPIO_DRV_SEL1			0xf60
#define	GPIO_DRV_SEL2			0xf70
#define	GPIO_DRV_SEL3			0xf80
#define	GPIO_DRV_SEL4			0xf90
#define	GPIO_DRV_SEL5			0xfa0
#define	GPIO_DRV_SEL6			0xfb0
#define	GPIO_SDIO_CTRL2			0xfc0	/* a.k.a. MSDC3_CTRL2 */
#define	GPIO_DRV_SEL8			0xfd0
#define	GPIO_DRV_SEL7			0xfe0
#define	GPIO_DRV_SEL9			0xff0

/* Relative to EINTC base address */
#define	GPIO_EINT_STA(x)		MTK_EINT_REG_OFF(0x000, x)
#define	GPIO_EINT_ACK(x)		MTK_EINT_REG_OFF(0x040, x)
#define	GPIO_EINT_MASK(x)		MTK_EINT_REG_OFF(0x080, x)
#define	GPIO_EINT_MASK_SET(x)		MTK_EINT_REG_OFF(0x0c0, x)
#define	GPIO_EINT_MASK_CLR(x)		MTK_EINT_REG_OFF(0x100, x)
#define	GPIO_EINT_SENS(x)		MTK_EINT_REG_OFF(0x140, x)
#define	GPIO_EINT_SENS_SET(x)		MTK_EINT_REG_OFF(0x180, x)
#define	GPIO_EINT_SENS_CLR(x)		MTK_EINT_REG_OFF(0x1c0, x)
#define	GPIO_EINT_SOFT(x)		MTK_EINT_REG_OFF(0x200, x)
#define	GPIO_EINT_SOFT_SET(x)		MTK_EINT_REG_OFF(0x240, x)
#define	GPIO_EINT_SOFT_CLR(x)		MTK_EINT_REG_OFF(0x280, x)
#define	GPIO_EINT_POL(x)		MTK_EINT_REG_OFF(0x300, x)
#define	GPIO_EINT_POL_SET(x)		MTK_EINT_REG_OFF(0x340, x)
#define	GPIO_EINT_POL_CLR(x)		MTK_EINT_REG_OFF(0x380, x)
#define	GPIO_EINT_D0EN(x)		MTK_EINT_REG_OFF(0x400, x)
#define	GPIO_EINT_D1EN(x)		MTK_EINT_REG_OFF(0x420, x)
#define	GPIO_EINT_D2EN(x)		MTK_EINT_REG_OFF(0x440, x)
#define	GPIO_EINT_DBNC_3_0(x)		MTK_EINT_REG_OFF(0x500, x)
#define	GPIO_EINT_DBNC_SET_3_0(x)	MTK_EINT_REG_OFF(0x600, x)
#define	GPIO_EINT_DBNC_CLR_3_0(x)	MTK_EINT_REG_OFF(0x700, x)

static const uint8_t sel_to_mA_4_8_12_16[] = {
	[0] = 4,
	[2] = 8,
	[4] = 12,
	[6] = 16,
};

static const struct mtk_gpio_drive mt2701_gpio_drive_4_8_12_16 = {
	.sel_to_mA = sel_to_mA_4_8_12_16,
	.nsel = __arraycount(sel_to_mA_4_8_12_16),
};

static const uint8_t sel_to_mA_2_4_6_8[] = {
	[0] = 2,
	[2] = 4,
	[4] = 6,
	[6] = 8,
};

static const struct mtk_gpio_drive mt2701_gpio_drive_2_4_6_8 = {
	.sel_to_mA = sel_to_mA_2_4_6_8,
	.nsel = __arraycount(sel_to_mA_2_4_6_8),
};

static const uint8_t sel_to_mA_2_4_6_8_10_12_14_16[] = {
	[0] = 2,
	[1] = 4,
	[2] = 6,
	[3] = 8,
	[4] = 10,
	[5] = 12,
	[6] = 14,
	[7] = 16,
};

static const struct mtk_gpio_drive mt2701_gpio_drive_2_4_6_8_10_12_14_16 = {
	.sel_to_mA = sel_to_mA_2_4_6_8_10_12_14_16,
	.nsel = __arraycount(sel_to_mA_2_4_6_8_10_12_14_16),
};

static const struct mtk_ies_smt_group mt2701_ies_smt_groups[] = {
	IES_SMT(  0,   6, GPIO_IES_EN0, GPIO_SMT_EN0,  0),
	IES_SMT(  7,   9, GPIO_IES_EN0, GPIO_SMT_EN0,  1),
	IES_SMT( 10,  13, GPIO_IES_EN1, GPIO_SMT_EN1,  3),
	IES_SMT( 14,  15, GPIO_IES_EN1, GPIO_SMT_EN1, 13),
	IES_SMT( 16,  17, GPIO_IES_EN2, GPIO_SMT_EN2,  7),
	IES_SMT( 18,  21, GPIO_IES_EN2, GPIO_SMT_EN2, 13),
	IES_SMT( 22,  26, GPIO_IES_EN2, GPIO_SMT_EN2, 13),
	IES_SMT( 27,  29, GPIO_IES_EN2, GPIO_SMT_EN2, 13),
	IES_SMT( 30,  32, GPIO_IES_EN2, GPIO_SMT_EN2,  7),
	IES_SMT( 33,  37, GPIO_IES_EN2, GPIO_SMT_EN2, 13),
	IES_SMT( 38,  38, GPIO_IES_EN0, GPIO_SMT_EN0, 13),
	IES_SMT( 39,  42, GPIO_IES_EN2, GPIO_SMT_EN2, 13),
	IES_SMT( 43,  45, GPIO_IES_EN0, GPIO_SMT_EN0, 10),
	IES_SMT( 47,  48, GPIO_IES_EN0, GPIO_SMT_EN0, 11),
	IES_SMT( 49,  49, GPIO_IES_EN0, GPIO_SMT_EN0, 12),
	IES_SMT( 50,  52, GPIO_IES_EN0, GPIO_SMT_EN0, 13),
	IES_SMT( 53,  56, GPIO_IES_EN0, GPIO_SMT_EN0, 14),
	IES_SMT( 57,  58, GPIO_IES_EN0, GPIO_SMT_EN0, 15),
	IES_SMT( 59,  59, GPIO_IES_EN1, GPIO_SMT_EN1, 10),
	IES_SMT( 60,  62, GPIO_IES_EN1, GPIO_SMT_EN1,  0),
	IES_SMT( 63,  65, GPIO_IES_EN1, GPIO_SMT_EN1,  1),
	IES_SMT( 66,  71, GPIO_IES_EN1, GPIO_SMT_EN1,  2),
	IES_SMT( 72,  74, GPIO_IES_EN0, GPIO_SMT_EN0, 12),
	IES_SMT( 75,  76, GPIO_IES_EN1, GPIO_SMT_EN1,  3),
	IES_SMT( 77,  78, GPIO_IES_EN1, GPIO_SMT_EN1,  4),
	IES_SMT( 79,  82, GPIO_IES_EN1, GPIO_SMT_EN1,  5),
	IES_SMT( 83,  84, GPIO_IES_EN1, GPIO_SMT_EN1,  2),

	IES(     85,  85, GPIO_MSDC2_CTRL1,            4),
	SMT(     85,  85, GPIO_MSDC2_CTRL1,           11),

	IES(     86,  86, GPIO_MSDC2_CTRL0,            4),
	SMT(     86,  86, GPIO_MSDC2_CTRL0,           11),

	IES(     87,  90, GPIO_MSDC2_CTRL2,            4),
	SMT(     87,  87, GPIO_MSDC2_CTRL3,            3),
	SMT(     88,  88, GPIO_MSDC2_CTRL3,            7),
	SMT(     89,  89, GPIO_MSDC2_CTRL3,           11),
	SMT(     90,  90, GPIO_MSDC2_CTRL3,           15),

	IES_SMT(101, 104, GPIO_IES_EN1, GPIO_SMT_EN1,  6),

	IES(    105, 105, GPIO_MSDC1_CTRL1,            4),
	SMT(    105, 105, GPIO_MSDC1_CTRL1,           11),

	IES(    106, 106, GPIO_MSDC1_CTRL0,            4),
	SMT(    106, 106, GPIO_MSDC1_CTRL0,           11),

	IES(    107, 110, GPIO_MSDC1_CTRL2,            4),
	SMT(    107, 107, GPIO_MSDC1_CTRL3,            3),
	SMT(    108, 108, GPIO_MSDC1_CTRL3,            7),
	SMT(    109, 109, GPIO_MSDC1_CTRL3,           11),
	SMT(    110, 110, GPIO_MSDC1_CTRL3,           15),

	IES(    111, 115, GPIO_MSDC0_CTRL2,            4),
	SMT(    111, 111, GPIO_MSDC0_CTRL4,           15),
	SMT(    112, 112, GPIO_MSDC0_CTRL4,           11),
	SMT(    113, 113, GPIO_MSDC0_CTRL4,            7),
	SMT(    114, 114, GPIO_MSDC0_CTRL4,            3),
	SMT(    115, 115, GPIO_MSDC0_CTRL5,            3),

	IES(    116, 116, GPIO_MSDC0_CTRL1,            4),
	SMT(    116, 116, GPIO_MSDC0_CTRL1,           11),

	IES(    117, 117, GPIO_MSDC0_CTRL0,            4),
	SMT(    117, 117, GPIO_MSDC0_CTRL0,           11),

	IES(    118, 121, GPIO_MSDC0_CTRL2,            4),
	SMT(    118, 118, GPIO_MSDC0_CTRL3,           15),
	SMT(    119, 119, GPIO_MSDC0_CTRL3,           11),
	SMT(    120, 120, GPIO_MSDC0_CTRL3,            7),
	SMT(    121, 121, GPIO_MSDC0_CTRL3,            3),

	IES_SMT(122, 125, GPIO_IES_EN1, GPIO_SMT_EN1,  7),
	IES_SMT(126, 126, GPIO_IES_EN0, GPIO_SMT_EN0, 12),
	IES_SMT(127, 142, GPIO_IES_EN1, GPIO_SMT_EN1,  9),
	IES_SMT(143, 160, GPIO_IES_EN1, GPIO_SMT_EN1, 10),
	IES_SMT(161, 168, GPIO_IES_EN1, GPIO_SMT_EN1, 12),
	IES_SMT(169, 183, GPIO_IES_EN1, GPIO_SMT_EN1, 10),
	IES_SMT(184, 186, GPIO_IES_EN1, GPIO_SMT_EN1,  9),
	IES_SMT(187, 187, GPIO_IES_EN1, GPIO_SMT_EN1, 14),
	IES_SMT(188, 188, GPIO_IES_EN0, GPIO_SMT_EN0, 13),
	IES_SMT(189, 193, GPIO_IES_EN1, GPIO_SMT_EN1, 15),
	IES_SMT(194, 198, GPIO_IES_EN2, GPIO_SMT_EN2,  0),
	IES_SMT(199, 199, GPIO_IES_EN0, GPIO_SMT_EN0,  1),
	IES_SMT(200, 202, GPIO_IES_EN2, GPIO_SMT_EN2,  1),
	IES_SMT(203, 207, GPIO_IES_EN2, GPIO_SMT_EN2,  2),
	IES_SMT(208, 209, GPIO_IES_EN2, GPIO_SMT_EN2,  3),
	IES_SMT(210, 210, GPIO_IES_EN2, GPIO_SMT_EN2,  4),
	IES_SMT(211, 235, GPIO_IES_EN2, GPIO_SMT_EN2,  5),
	IES_SMT(236, 241, GPIO_IES_EN2, GPIO_SMT_EN2,  6),
	IES_SMT(242, 243, GPIO_IES_EN2, GPIO_SMT_EN2,  7),
	IES_SMT(244, 247, GPIO_IES_EN2, GPIO_SMT_EN2,  8),
	IES_SMT(248, 248, GPIO_IES_EN2, GPIO_SMT_EN2,  9),

	IES(    249, 257, GPIO_SDIO_CTRL2,             4),
	SMT(    249, 249, GPIO_SDIO_CTRL5,             3),
	SMT(    250, 250, GPIO_SDIO_CTRL4,            15),
	SMT(    251, 251, GPIO_SDIO_CTRL4,            11),
	SMT(    252, 252, GPIO_SDIO_CTRL4,             7),
	SMT(    253, 253, GPIO_SDIO_CTRL4,             3),
	SMT(    254, 254, GPIO_SDIO_CTRL3,            15),
	SMT(    255, 255, GPIO_SDIO_CTRL3,            11),
	SMT(    256, 256, GPIO_SDIO_CTRL3,             7),
	SMT(    257, 257, GPIO_SDIO_CTRL3,             3),

	IES(    258, 258, GPIO_MSDC3_CTRL1,            4),
	SMT(    258, 258, GPIO_MSDC3_CTRL1,           11),

	IES(    259, 259, GPIO_MSDC3_CTRL0,            4),
	SMT(    259, 259, GPIO_MSDC3_CTRL0,           11),

	IES(    260, 260, GPIO_SDIO_CTRL7,             4),
	SMT(    260, 260, GPIO_SDIO_CTRL7,            11),

	IES(    261, 261, GPIO_MSDC1_CTRL2,            4),
	SMT(    261, 261, GPIO_MSDC1_CTRL6,            3),

	IES_SMT(262, 277, GPIO_IES_EN2, GPIO_SMT_EN2, 12),
	IES_SMT(278, 278, GPIO_IES_EN2, GPIO_SMT_EN2, 13),
};

static const struct mtk_gpio_pinconf mt2701_gpio_pins[] = {
	/* 0 */
[0] =	{	.name = "PWRAP_SPI0_MI",
		.functions = {
	  		[0] = "GPIO0",
			[1] = "PWRAP_SPIDO",
			[2] = "PWRAP_SPIDI",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(0,3)),
		EINT(0, 148),
	},
[1] =	{	.name = "PWRAP_SPI0_MO",
		.functions = {
			[0] = "GPIO1",
			[1] = "PWRAP_SPIDI",
			[2] = "PWRAP_SPIDO",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(0,3)),
		EINT(0, 149),
	},
[2] =	{	.name = "PWRAP_INT",
		.functions = {
			[0] = "GPIO2",
			[1] = "PWRAP_INT"
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(0,3)),
		EINT(0, 150),
	},
[3] =	{	.name = "PWRAP_SPI0_CK",
		.functions = {
			[0] = "GPIO3",
			[1] = "PWRAP_SPICK_I",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(0,3)),
		EINT(0, 151),
	},
[4] =	{	.name = "PWRAP_SPI0_CSN",
		.functions = {
			[0] = "GPIO4",
			[1] = "PWRAP_SPICS_B_I",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(0,3)),
		EINT(0, 152),
	},
[5] =	{	.name = "PWRAP_SPI0_CK2",
		.functions = {
			[0] = "GPIO5",
			[1] = "PWRAP_SPICK2_I",
			[5] = "ANT_SEL1",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(0,3)),
		EINT(0, 153),
	},
[6] =	{	.name = "PWRAP_SPI0_CSN2",
		.functions = {
			[0] = "GPIO6",
			[1] = "PWRAP_SPICS2_B_I",
			[5] = "ANT_SEL0",
			[7] = "DBG_MON_A[0]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(0,3)),
		EINT(0, 154),
	},
[7] =	{	.name = "SPI1_CSN",
		.functions = {
			[0] = "GPIO7",
			[1] = "SPI1_CS",
			[4] = "KCOL0",
			[7] = "DBG_MON_B[12]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(4,7)),
		EINT(0, 155),
	},
[8] =	{	.name = "SPI1_MI",
		.functions = {
			[0] = "GPIO8",
			[1] = "SPI1_MI",
			[2] = "SPI1_MO",
			[4] = "KCOL1",
			[7] = "DBG_MON_B[13]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(4,7)),
		EINT(0, 156),
	},
[9] =	{	.name = "SPI1_MO",
		.functions = {
			[0] = "GPIO9",
			[1] = "SPI1_MO",
			[2] = "SPI1_MI",
			[3] = "EXT_FRAME_SYNC",
			[4] = "KCOL2",
			[7] = "DBG_MON_B[14]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(4,7)),
		EINT(0, 157),
	},
[10] =	{	.name = "RTC32K_CK",
		.functions = {
			[0] = "GPIO10",
			[1] = "RTC32K_CK",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(8,11)),
		EINT(0, 158),
	},
[11] =	{	.name = "WATCHDOG",
		.functions = {
			[0] = "GPIO11",
			[1] = "WATCHDOG",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(8,11)),
		EINT(0, 159),
	},
[12] =	{	.name = "SRCLKENA",
		.functions = {
			[0] = "GPIO12",
			[1] = "SRCLKENA",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(8,11)),
		EINT(0, 160),
	},
[13] =	{	.name = "SRCLKENAI",
		.functions = {
			[0] = "GPIO13",
			[1] = "SRCLKENAI",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(8,11)),
		EINT(0, 161),
	},
[14] =	{	.name = "URXD2",
		.functions = {
			[0] = "GPIO14",
			[1] = "URXD2",
			[2] = "UTXD2",
			[5] = "SRCCLKENAI2",
			[7] = "DBG_MON_B[30]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL0, __BITS(12,15)),
		EINT(0, 162),
	},
[15] =	{	.name = "UTXD2",
		.functions = {
			[0] = "GPIO15",
			[1] = "UTXD2",
			[2] = "URXD2",
			[7] = "DBG_MON_B[31]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL0, __BITS(12,15)),
		EINT(0, 163),
	},
	/* 16 */
[16] =	{	.name = "I2S5_DATA_IN",
		.functions = {
			[0] = "GPIO16",
			[1] = "I2S5_DATA_IN",
			[3] = "PCM_RX",
			[4] = "ANT_SEL4",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(0,3)),
	},
[17] =	{	.name = "I2S5_BCK",
		.functions = {
			[0] = "GPIO17",
			[1] = "I2S5_BCK",
			[3] = "PCM_CLK0",
			[4] = "ANT_SEL2",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(0,3)),
	},
[18] =	{	.name = "PCM_CLK",
		.functions = {
			[0] = "GPIO18",
			[1] = "PCM_CLK0",
			[2] = "MRG_CLK",
			[4] = "MM_TEST_CK",
			[5] = "CONN_DSP_JCK",
			[6] = "WCN_PCM_CLKO",
			[7] = "DBG_MON_A[3]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(4,7)),
		EINT(0, 166),
	},
[19] =	{	.name = "PCM_SYNC",
		.functions = {
			[0] = "GPIO19",
			[1] = "PCM_SYNC",
			[2] = "MRG_SYNC",
			[5] = "CONN_DSP_JINTP",
			[6] = "WCN_PCM_SYNC",
			[7] = "DBG_MON_A[5]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(4,7)),
		EINT(0, 167),
	},
[20] =	{	.name = "PCM_RX",
		.functions = {
			[0] = "GPIO20",
			[1] = "PCM_RX",
			[2] = "MRG_RX",
			[3] = "MRG_TX",
			[4] = "PCM_TX",
			[5] = "CONN_DSP_JDI",
			[6] = "WCN_PCM_RX",
			[7] = "DBG_MON_A[4]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(4,7)),
	},
[21] =	{	.name = "PCM_TX",
		.functions = {
			[0] = "GPIO21",
			[1] = "PCM_TX",
			[2] = "MRG_TX",
			[3] = "MRG_RX",
			[4] = "PCM_RX",
			[5] = "CONN_DSP_JMS",
			[6] = "WCN_PCM_TX",
			[7] = "DBG_MON_A[2]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(4,7)),
	},
[22] =	{	.name = "EINT0",
		.functions = {
			[0] = "GPIO22",
			[1] = "UCTS0",
			[2] = "PCIE0_PERST_N",	/* MT7623 */
			[3] = "KCOL3",
			[4] = "CONN_DSP_JDO",
			[5] = "EXT_FRAME_SYNC",
			[7] = "DBG_MON_A[30]",
		/*	10 == "PCIE0_PERST_N"	   MT2701 */
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(8,11)),
		EINT_FLAGS(0, 0, MTK_EINT_DEBOUNCE_REQD),
	},
[23] =	{	.name = "EINT1",
		.functions = {
			[0] = "GPIO23",
			[1] = "URTS0",
			[2] = "PCIE1_PERST_N",	/* MT7623 */
			[3] = "KCOL2",
			[4] = "CONN_MCU_TDO",
			[5] = "EXT_FRAME_SYNC",
			[7] = "DBG_MON_A[29]",
		/*	10 == "PCIE1_PERST_N"	  MT2701 */
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(8,11)),
		EINT_FLAGS(0, 1, MTK_EINT_DEBOUNCE_REQD),
	},
[24] =	{	.name = "EINT2",
		.functions = {
			[0] = "GPIO24",
			[1] = "UCTS1",
			[2] = "PCIE2_PERST_N",	/* MT7623 */
			[3] = "KCOL1",
			[4] = "CONN_MCU_DBGACK_N",
			[7] = "DBG_MON_A[28]",
		/*	10 == "PCIE2_PERST_N"	   MT2701 */
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(8,11)),
		EINT_FLAGS(0, 2, MTK_EINT_DEBOUNCE_REQD),
	},
[25] =	{	.name = "EINT3",
		.functions = {
			[0] = "GPIO25",
			[1] = "URTS1",
			[3] = "KCOL0",
			[4] = "CONN_MCU_DBGI_N",
			[7] = "DBG_MON_A[27]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(8,11)),
		EINT_FLAGS(0, 3, MTK_EINT_DEBOUNCE_REQD),
	},
[26] =	{	.name = "EINT4",
		.functions = {
			[0] = "GPIO26",
			[1] = "UCTS3",
			[2] = "DRV_VBUS_P1",
			[3] = "KROW3",
			[4] = "CONN_MCU_TCK0",
			[5] = "CONN_MCU_AICE_JCKC",
			[6] = "PCIE2_WAKE_N",
			[7] = "DBG_MON_A[26]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(8,11)),
		EINT_FLAGS(0, 4, MTK_EINT_DEBOUNCE_REQD),
	},
[27] =	{	.name = "EINT5",
		.functions = {
			[0] = "GPIO27",
			[1] = "URTS3",
			[2] = "IDDIG_P1",
			[3] = "KROW2",
			[4] = "CONN_MCU_TDI",
			[6] = "PCIE1_WAKE_N",
			[7] = "DBG_MON_A[25]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(12,15)),
		EINT_FLAGS(0, 5, MTK_EINT_DEBOUNCE_REQD),
	},
[28] =	{	.name = "EINT6",
		.functions = {
			[0] = "GPIO28",
			[1] = "DRV_VBUS",
			[3] = "KROW1",
			[4] = "CONN_MCU_TRST_B",
			[6] = "PCIE0_WAKE_N",
			[7] = "DBG_MON_A[24]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(12,15)),
		EINT_FLAGS(0, 6, MTK_EINT_DEBOUNCE_REQD),
	},
[29] =	{	.name = "EINT7",
		.functions = {
			[0] = "GPIO29",
			[1] = "IDDIG",
			[2] = "MSDC1_WP",
			[3] = "KROW0",
			[4] = "CONN_MCU_TMS",
			[5] = "CONN_MCU_AICE_JMSC",
			[6] = "PCIE2_PERST_N",	/* MT7623 */
			[7] = "DBG_MON_A[23]",
		/*	14 == "PCIE2_PERST_N"	   MT2701 */
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(12,15)),
		EINT_FLAGS(0, 7, MTK_EINT_DEBOUNCE_REQD),
	},
[30] =	{	.name = "I2S5_LRCK",
		.functions = {
			[0] = "GPIO30",
			[1] = "I2S5_LRCK",
			[3] = "PCM_SYNC",
			[4] = "ANT_SEL1",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(0,3)),
	},
[31] =	{	.name = "I2S5_MCLK",
		.functions = {
			[0] = "GPIO31",
			[1] = "I2S5_MCLK",
			[4] = "ANT_SEL0",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(0,3)),
	},
	/* 32 */
[32] =	{	.name = "I2S5_DATA",
		.functions = {
			[0] = "GPIO32",
			[1] = "I2S5_DATA",
			[2] = "I2S5_DATA_BYPS",
			[3] = "PCM_TX",
			[4] = "ANT_SEL3",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL1, __BITS(0,3)),
	},
[33] =	{	.name = "I2S1_DATA",
		.functions = {
			[0] = "GPIO33",
			[1] = "I2S1_DATA",
			[2] = "I2S1_DATA_BYPS",
			[3] = "PCM_TX",
			[4] = "IMG_TEST_CK",
			[5] = "G1_RXD0",
			[6] = "WCN_PCM_TX",
			[7] = "DBG_MON_B[8]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(0,3)),
		EINT(0, 15),
	},
[34] =	{	.name = "I2S1_DATA_IN",
		.functions = {
			[0] = "GPIO34",
			[1] = "I2S1_DATA_IN",
			[3] = "PCM_RX",
			[4] = "VDEC_TEST_CK",
			[5] = "G1_RXD1",
			[6] = "WCN_PCM_RX",
			[7] = "DBG_MON_B[7]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(0,3)),
		EINT(0, 16),
	},
[35] =	{	.name = "I2S1_BCK",
		.functions = {
			[0] = "GPIO35",
			[1] = "I2S1_BCK",
			[3] = "PCM_CLK0",
			[5] = "G1_RXD2",
			[6] = "WCN_PCM_CLKO",
			[7] = "DBG_MON_B[9]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(0,3)),
		EINT(0, 17),
	},
[36] =	{	.name = "I2S1_LRCK",
		.functions = {
			[0] = "GPIO36",
			[1] = "I2S1_LRCK",
			[3] = "PCM_SYNC",
			[5] = "G1_RXD3",
			[6] = "WCN_PCM_SYNC",
			[7] = "DBG_MON_B[10]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(0,3)),
		EINT(0, 18),
	},
[37] =	{	.name = "I2S1_MCLK",
		.functions = {
			[0] = "GPIO37",
			[1] = "I2S1_MCLK",
			[5] = "G1_RXDV",
			[7] = "DBG_MON_B[11]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(0,3)),
		EINT(0, 19),
	},
[38] =	{	.name = "I2S2_DATA",
		.functions = {
			[0] = "GPIO38",
			[1] = "I2S2_DATA",
			[2] = "I2S2_DATA_BYPS",
			[3] = "PCM_TX",
			[4] = "DMIC_DAT0",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(4,7)),
	},
[39] =	{	.name = "JTMS",
		.functions = {
			[0] = "GPIO39",
			[1] = "JTMS",
			[2] = "CONN_MCU_TMS",
			[3] = "CONN_MCU_AICE_JMSC",
			[4] = "DFD_TMS_XI",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL2, __BITS(8,11)),
		EINT(0, 21),
	},
[40] =	{	.name = "JTCK",
		.functions = {
			[0] = "GPIO40",
			[1] = "JTCK",
			[2] = "CONN_MCU_TCK1",
			[3] = "CONN_MCU_AICE_JCKC",
			[4] = "DFD_TCK_XI",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL2, __BITS(8,11)),
		EINT(0, 22),
	},
[41] =	{	.name = "JTDI",
		.functions = {
			[0] = "GPIO41",
			[1] = "JTDI",
			[2] = "CONN_MCU_TDI",
			[4] = "DFD_TDI_XI",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL2, __BITS(8,11)),
		EINT(0, 23),
	},
[42] =	{	.name = "JTDO",
		.functions = {
			[0] = "GPIO42",
			[1] = "JTDO",
			[2] = "CONN_MCU_TDO",
			[4] = "DFD_TDO",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL2, __BITS(8,11)),
		EINT(0, 24),
	},
[43] =	{	.name = "NCLE",
		.functions = {
			[0] = "GPIO43",
			[1] = "NCLE",
			[2] = "EXT_XCS2",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(12,15)),
		EINT(0, 25),
	},
[44] =	{	.name = "NCEB1",
		.functions = {
			[0] = "GPIO44",
			[1] = "NCEB1",
			[2] = "IDDIG",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(12,15)),
		EINT(0, 26),
	},
[45] =	{	.name = "NCEB0",
		.functions = {
			[0] = "GPIO45",
			[1] = "NCEB0",
			[2] = "DRV_VBUS",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(12,15)),
		EINT(0, 27),
	},
[46] =	{	.name = "IR",
		.functions = {
			[0] = "GPIO46",
			[1] = "IR",
		},
		EINT(0, 28),
	},
[47] =	{	.name = "NREB",
		.functions = {
			[0] = "GPIO47",
			[1] = "NREB",
			[2] = "IDDIG_P1",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL3, __BITS(0,3)),
		EINT(0, 29),
	},
	/* 48 */
[48] =	{	.name = "NRNB",
		.functions = {
			[0] = "GPIO48",
			[1] = "NRNB",
			[2] = "DRV_VBUS_P1",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL3, __BITS(0,3)),
		EINT(0, 30),
	},
[49] =	{	.name = "I2S0_DATA",
		.functions = {
			[0] = "GPIO49",
			[1] = "I2S0_DATA",
			[2] = "I2S0_DATA_BYPS",
			[3] = "PCM_TX",
			[6] = "WCN_I2S_DO",
			[7] = "DBG_MON_B[3]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL3, __BITS(4,7)),
		EINT(0, 31),
	},
[50] =	{	.name = "I2S2_BCK",
		.functions = {
			[0] = "GPIO50",
			[1] = "I2S2_BCK",
			[3] = "PCM_CLK0",
			[4] = "DMIC_SCK1",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(4,7)),
	},
[51] =	{	.name = "I2S2_DATA_IN",
		.functions = {
			[0] = "GPIO51",
			[1] = "I2S2_DATA_IN",
			[3] = "PCM_RX",
			[4] = "DMIC_SCK0",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(4,7)),
	},
[52] =	{	.name = "I2S2_LRCK",
		.functions = {
			[0] = "GPIO52",
			[1] = "I2S2_LRCK",
			[3] = "PCM_SYNC",
			[4] = "DMIC_DAT1",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(4,7)),
	},
[53] =	{	.name = "SPI0_CSN",
		.functions = {
			[0] = "GPIO53",
			[1] = "SPI0_CS",
			[3] = "SPDIF",
			[4] = "ADC_CK",
			[5] = "PWM1",
			[7] = "DBG_MON_A[7]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL3, __BITS(12,15)),
		EINT(0, 35),
	},
[54] =	{	.name = "SPI0_CK",
		.functions = {
			[0] = "GPIO54",
			[1] = "SPI0_CK",
			[3] = "SPDIF_IN1",
			[4] = "ADC_DAT_IN",
			[7] = "DBG_MON_A[10]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL3, __BITS(12,15)),
		EINT(0, 36),
	},
[55] =	{	.name = "SPI0_MI",
		.functions = {
			[0] = "GPIO55",
			[1] = "SPI0_MI",
			[2] = "SPI0_MO",
			[3] = "MSDC1_WP",
			[4] = "ADC_WS",
			[5] = "PWM2",
			[7] = "DBG_MON_A[8]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL3, __BITS(12,15)),
		EINT(0, 37),
	},
[56] =	{	.name = "SPI0_MO",
		.functions = {
			[0] = "GPIO56",
			[1] = "SPI0_MO",
			[2] = "SPI0_MI",
			[3] = "SPDIF_IN0",
			[7] = "DBG_MON_A[9]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL3, __BITS(12,15)),
		EINT(0, 38),
	},
[57] =	{	.name = "SDA1",
		.functions = {
			[0] = "GPIO57",
			[1] = "SDA1",
		},
		EINT(0, 39),
	},
[58] =	{	.name = "SCL1",
		.functions = {
			[0] = "GPIO58",
			[1] = "SCL1",
		},
		EINT(0, 40),
	},
[59] =	{	.name = "GPIO59",
		.functions = {
			[0] = "GPIO59",
			[1] = "RAMBUF_I_CLK",
		},
	},
[60] =	{	.name = "WB_RSTB",
		.functions = {
			[0] = "GPIO60",
			[1] = "WB_RSTB",
			[7] = "DBG_MON_A[11]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL4, __BITS(8,11)),
		EINT(0, 41),
	},
[61] =	{	.name = "F2W_DATA",
		.functions = {
			[0] = "GPIO61",
			[1] = "F2W_DATA",
			[7] = "DBG_MON_A[16]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL4, __BITS(8,11)),
		EINT(0, 42),
	},
[62] =	{	.name = "F2W_CK",
		.functions = {
			[0] = "GPIO62",
			[1] = "F2W_CK",
			[7] = "DBG_MON_A[15]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL4, __BITS(8,11)),
		EINT(0, 43),
	},
[63] =	{	.name = "WB_SCLK",
		.functions = {
			[0] = "GPIO63",
			[1] = "WB_SCLK",
			[7] = "DBG_MON_A[13]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL4, __BITS(12,15)),
		EINT(0, 44),
	},
	/* 64 */
[64] =	{	.name = "WB_SDATA",
		.functions = {
			[0] = "GPIO64",
			[1] = "WB_SDATA",
			[7] = "DBG_MON_A[12]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL4, __BITS(12,15)),
		EINT(0, 45),
	},
[65] =	{	.name = "WB_SEN",
		.functions = {
			[0] = "GPIO65",
			[1] = "WB_SEN",
			[7] = "DBG_MON_A[14]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL4, __BITS(12,15)),
		EINT(0, 46),
	},
[66] =	{	.name = "WB_CTRL0",
		.functions = {
			[0] = "GPIO66",
			[1] = "WB_CTRL0",
			[5] = "DFD_NTRST_XI",
			[7] = "DBG_MON_A[17]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL5, __BITS(0,3)),
		EINT(0, 47),
	},
[67] =	{	.name = "WB_CTRL1",
		.functions = {
			[0] = "GPIO67",
			[1] = "WB_CTRL1",
			[5] = "DFD_TMS_XI",
			[7] = "DBG_MON_A[18]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL5, __BITS(0,3)),
		EINT(0, 48),
	},
[68] =	{	.name = "WB_CTRL2",
		.functions = {
			[0] = "GPIO68",
			[1] = "WB_CTRL2",
			[5] = "DFD_TCK_XI",
			[7] = "DBG_MON_A[19]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL5, __BITS(0,3)),
		EINT(0, 49),
	},
[69] =	{	.name = "WB_CTRL3",
		.functions = {
			[0] = "GPIO69",
			[1] = "WB_CTRL3",
			[5] = "DFD_TDI_XI",
			[7] = "DBG_MON_A[20]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL5, __BITS(0,3)),
		EINT(0, 50),
	},
[70] =	{	.name = "WB_CTRL4",
		.functions = {
			[0] = "GPIO70",
			[1] = "WB_CTRL4",
			[5] = "DFD_TDO",
			[7] = "DBG_MON_A[21]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL5, __BITS(0,3)),
		EINT(0, 51),
	},
[71] =	{	.name = "WB_CTRL5",
		.functions = {
			[0] = "GPIO71",
			[1] = "WB_CTRL5",
			[7] = "DBG_MON_A[22]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL5, __BITS(0,3)),
		EINT(0, 52),
	},
[72] =	{	.name = "I2S0_DATA_IN",
		.functions = {
			[0] = "GPIO72",
			[1] = "I2S0_DATA_IN",
			[3] = "PCM_RX",
			[4] = "PWM0",
			[5] = "DISP_PWM",
			[6] = "WCN_I2S_DI",
			[7] = "DBG_MON_B[2]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL3, __BITS(4,7)),
		EINT(0, 53),
	},
[73] =	{	.name = "I2S0_LRCK",
		.functions = {
			[0] = "GPIO73",
			[1] = "I2S0_LRCK",
			[3] = "PCM_SYNC",
			[6] = "WCN_I2S_LRCK",
			[7] = "DBG_MON_B[5]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL3, __BITS(4,7)),
		EINT(0, 54),
	},
[74] =	{	.name = "I2S0_BCK",
		.functions = {
			[0] = "GPIO74",
			[1] = "I2S0_BCK",
			[3] = "PCM_CLK0",
			[6] = "WCN_I2S_BCK",
			[7] = "DBG_MON_B[4]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL3, __BITS(4,7)),
		EINT(0, 55),
	},
[75] =	{	.name = "SDA0",
		.functions = {
			[0] = "GPIO75",
			[1] = "SDA0",
		},
		EINT(0, 56),
	},
[76] =	{	.name = "SCL0",
		.functions = {
			[0] = "GPIO76",
			[1] = "SCL0",
		},
		EINT(0, 57),
	},
[77] =	{	.name = "SDA2",
		.functions = {
			[0] = "GPIO77",
			[1] = "SDA2",
		},
		EINT(0, 58),
	},
[78] =	{	.name = "SCL2",
		.functions = {
			[0] = "GPIO78",
			[1] = "SCL2",
		},
		EINT(0, 59),
	},
[79] =	{	.name = "URXD0",
		.functions = {
			[0] = "GPIO79",
			[1] = "URXD0",
			[2] = "UTXD0",
		},
		EINT(0, 60),
	},
	/* 80 */
[80] =	{	.name = "UTXD0",
		.functions = {
			[0] = "GPIO80",
			[1] = "UTXD0",
			[2] = "URXD0",
		},
		EINT(0, 61),
	},
[81] =	{	.name = "URXD1",
		.functions = {
			[0] = "GPIO81",
			[1] = "URXD1",
			[2] = "UTXD1",
		},
		EINT(0, 62),
	},
[82] =	{	.name = "UTXD1",
		.functions = {
			[0] = "GPIO82",
			[1] = "UTXD1",
			[2] = "URXD1",
		},
		EINT(0, 63),
	},
[83] =	{	.name = "LCM_RST",
		.functions = {
			[0] = "GPIO83",
			[1] = "LCM_RST",
			[2] = "VDAC_CK_XI",
			[7] = "DBG_MON_B[1]",
		},
		EINT(0, 64),
	},
[84] =	{	.name = "DSI_TE",
		.functions = {
			[0] = "GPIO84",
			[1] = "DSI_TE",
			[7] = "DBG_MON_B[0]",
		},
		EINT(0, 65),
	},
[85] =	{	.name = "MSDC2_CMD",
		.functions = {
			[0] = "GPIO85",
			[1] = "MSDC2_CMD",
			[2] = "ANT_SEL0",
			[3] = "SDA1",
			[6] = "I2SOUT_BCK",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC2_CTRL1, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC2_CTRL1, 8, 9, 10),
	},
[86] =	{	.name = "MSDC2_CLK",
		.functions = {
			[0] = "GPIO86",
			[1] = "MSDC2_CLK",
			[2] = "ANT_SEL1",
			[3] = "SCL1",
			[6] = "I2SOUT_LRCK",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC2_CTRL0, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC2_CTRL0, 8, 9, 10),
	},
[87] =	{	.name = "MSDC2_DAT0",
		.functions = {
			[0] = "GPIO87",
			[1] = "MSDC2_DAT0",
			[2] = "ANT_SEL2",
			[5] = "UTXD0",
			[6] = "I2SOUT_DATA_OUT",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC2_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC2_CTRL3, 0, 1, 2),
	},
[88] =	{	.name = "GPIO88",
		.functions = {
			[0] = "GPIO88",
			[1] = "MSDC2_DAT1",
			[2] = "ANT_SEL3",
			[3] = "PWM0",
			[5] = "URXD0",
			[6] = "PWM1",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC2_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC2_CTRL3, 4, 5, 6),
	},
[89] =	{	.name = "MSDC2_DAT2",
		.functions = {
			[0] = "GPIO89",
			[1] = "MSDC2_DAT2",
			[2] = "ANT_SEL4",
			[3] = "SDA2",
			[5] = "UTXD1",
			[6] = "PWM2",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC2_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC2_CTRL3, 8, 9, 10),
	},
[90] =	{	.name = "MSDC2_DAT3",
		.functions = {
			[0] = "GPIO90",
			[1] = "MSDC2_DAT3",
			[2] = "ANT_SEL5",
			[3] = "SCL2",
			[4] = "EXT_FRAME_SYNC",
			[5] = "URXD1",
			[6] = "PWM3",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC2_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC2_CTRL3, 12, 13, 14),
	},
[91] =	{	.name = "TDN3",
		.functions = {
			[0] = "GPIO91",
			[1] = "TDN3",
		},
	},
[92] =	{	.name = "TDP3",
		.functions = {
			[0] = "GPIO92",
			[1] = "TDP3",
		},
	},
[93] =	{	.name = "TDN2",
		.functions = {
			[0] = "GPIO93",
			[1] = "TDN2",
		},
	},
[94] =	{	.name = "TDP2",
		.functions = {
			[0] = "GPIO94",
			[1] = "TDP2",
		},
	},
[95] =	{	.name = "TCN",
		.functions = {
			[0] = "GPIO95",
			[1] = "TCN",
		},
	},
	/* 96 */
[96] =	{	.name = "TCP",
		.functions = {
			[0] = "GPIO96",
			[1] = "TCP",
		},
	},
[97] =	{	.name = "TDN1",
		.functions = {
			[0] = "GPIO97",
			[1] = "TDN1",
		},
	},
[98] =	{	.name = "TDP1",
		.functions = {
			[0] = "GPIO98",
			[1] = "TDP1",
		},
	},
[99] =	{	.name = "TDN0",
		.functions = {
			[0] = "GPIO99",
			[1] = "TDN0",
		},
	},
[100] =	{	.name = "TDP0",
		.functions = {
			[0] = "GPIO100",
			[1] = "TDP0",
		},
	},
[101] =	{	.name = "SPI2_CSN",
		.functions = {
			[0] = "GPIO101",
			[1] = "SPI2_CS",
			[3] = "SCL3",
			[4] = "KROW0",
		},
		EINT(0, 74),
	},
[102] =	{	.name = "SPI2_MI",
		.functions = {
			[0] = "GPIO102",
			[1] = "SPI2_MI",
			[2] = "SPI2_MO",
			[3] = "SDA3",
			[4] = "KROW1",
		},
		EINT(0, 75),
	},
[103] =	{	.name = "SPI2_MO",
		.functions = {
			[0] = "GPIO103",
			[1] = "SPI2_MO",
			[2] = "SPI2_MI",
			[3] = "SCL3",
			[4] = "KROW2",
		},
		EINT(0, 76),
	},
[104] =	{	.name = "SPI2_CK",
		.functions = {
			[0] = "GPIO104",
			[1] = "SPI2_CK",
			[3] = "SDA3",
			[4] = "KROW3",
		},
		EINT(0, 77),
	},
[105] =	{	.name = "MSDC1_CMD",
		.functions = {
			[0] = "GPIO105",
			[1] = "MSDC1_CMD",
			[2] = "ANT_SEL0",
			[3] = "SDA1",
			[6] = "I2SOUT_BCK",
			[7] = "DBG_MON_B[27]",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC1_CTRL1, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC1_CTRL1, 8, 9, 10),
		EINT(0, 78),
	},
[106] =	{	.name = "MSDC1_CLK",
		.functions = {
			[0] = "GPIO106",
			[1] = "MSDC1_CLK",
			[2] = "ANT_SEL1",
			[3] = "SCL1",
			[6] = "I2SOUT_LRCK",
			[7] = "DBG_MON_B[28]",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC1_CTRL0, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC1_CTRL0, 8, 9, 10),
		EINT(0, 79),
	},
[107] =	{	.name = "MSDC1_DAT0",
		.functions = {
			[0] = "GPIO107",
			[1] = "MSDC1_DAT0",
			[2] = "ANT_SEL2",
			[5] = "UTXD0",
			[6] = "I2SOUT_DATA_OUT",
			[7] = "DBG_MON_B[26]",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC1_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC1_CTRL3, 0, 1, 2),
		EINT(0, 80),
	},
[108] =	{	.name = "MSDC1_DAT1",
		.functions = {
			[0] = "GPIO108",
			[1] = "MSDC1_DAT1",
			[2] = "ANT_SEL3",
			[3] = "PWM0",
			[5] = "URXD0",
			[6] = "PWM1",
			[7] = "DBG_MON_B[25]",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC1_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC1_CTRL3, 4, 5, 6),
		EINT(0, 81),
	},
[109] =	{	.name = "MSDC1_DAT2",
		.functions = {
			[0] = "GPIO109",
			[1] = "MSDC1_DAT2",
			[2] = "ANT_SEL4",
			[3] = "SDA2",
			[5] = "UTXD0",
			[6] = "PWM2",
			[7] = "DBG_MON_B[24]",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC1_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC1_CTRL3, 8, 9, 10),
		EINT(0, 82),
	},
[110] =	{	.name = "MSDC1_DAT3",
		.functions = {
			[0] = "GPIO110",
			[1] = "MSDC1_DAT3",
			[2] = "ANT_SEL5",
			[3] = "SCL2",
			[4] = "EXT_FRAME_SYNC",
			[5] = "URXD1",
			[6] = "PWM3",
			[7] = "DBG_MON_B[23]",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC1_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC1_CTRL3, 12, 13, 14),
		EINT(0, 83),
	},
[111] =	{	.name = "MSDC0_DAT7",
		.functions = {
			[0] = "GPIO111",
			[1] = "MSDC1_DAT7",
			[4] = "NLD7",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC0_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC0_CTRL4, 12, 13, 14),
		EINT(0, 84),
	},
	/* 112 */
[112] =	{	.name = "MSDC0_DAT6",
		.functions = {
			[0] = "GPIO112",
			[1] = "MSDC0_DAT6",
			[4] = "NLD6",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC0_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC0_CTRL4, 8, 9, 10),
		EINT(0, 85),
	},
[113] =	{	.name = "MSDC0_DAT5",
		.functions = {
			[0] = "GPIO113",
			[1] = "MSDC0_DAT5",
			[4] = "NLD5",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC0_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC0_CTRL4, 4, 5, 6),
		EINT(0, 86),
	},
[114] =	{	.name = "MSDC0_DAT4",
		.functions = {
			[0] = "GPIO114",
			[1] = "MSDC0_DAT4",
			[4] = "NLD4",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC0_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC0_CTRL4, 0, 1, 2),
		EINT(0, 87),
	},
[115] =	{	.name = "MSDC0_RSTB",
		.functions = {
			[0] = "GPIO115",
			[1] = "MSDC0_RSTB",
			[4] = "NLD8",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC0_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC0_CTRL5, 0, 1, 2),
		EINT(0, 88),
	},
[116] =	{	.name = "MSDC0_CMD",
		.functions = {
			[0] = "GPIO116",
			[1] = "MSDC0_CMD",
			[4] = "NALE",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC0_CTRL1, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC0_CTRL1, 8, 9, 10),
		EINT(0, 89),
	},
[117] =	{	.name = "MSDC0_CLK",
		.functions = {
			[0] = "GPIO117",
			[1] = "MSDC0_CLK",
			[4] = "NWEB",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC0_CTRL0, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC0_CTRL0, 8, 9, 10),
		EINT(0, 90),
	},
[118] =	{	.name = "MSDC0_DAT3",
		.functions = {
			[0] = "GPIO118",
			[1] = "MSDC0_DAT3",
			[4] = "NLD3",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC0_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC0_CTRL3, 12, 13, 14),
		EINT(0, 91),
	},
[119] =	{	.name = "MSDC0_DAT2",
		.functions = {
			[0] = "GPIO119",
			[1] = "MSDC0_DAT2",
			[4] = "NLD2",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC0_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC0_CTRL3, 8, 9, 10),
		EINT(0, 92),
	},
[120] =	{	.name = "MSDC0_DAT1",
		.functions = {
			[0] = "GPIO120",
			[1] = "MSDC0_DAT1",
			[4] = "NLD1",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC0_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC0_CTRL3, 4, 5, 6),
		EINT(0, 93),
	},
[121] =	{	.name = "MSDC0_DAT0",
		.functions = {
			[0] = "GPIO121",
			[1] = "MSDC0_DAT0",
			[4] = "NLD0",
			[5] = "WATCHDOG",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC0_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC0_CTRL3, 0, 1, 2),
		EINT(0, 94),
	},
[122] =	{	.name = "CEC",
		.functions = {
			[0] = "GPIO122",
			[1] = "CEC",
			[4] = "SDA2",
			[5] = "URXD0",
		},
		EINT(0, 95),
	},
[123] =	{	.name = "HTPLG",
		.functions = {
			[0] = "GPIO123",
			[1] = "HTPLG",
			[4] = "SCL2",
			[5] = "UTXD0",
		},
		EINT(0, 96),
	},
[124] =	{	.name = "HDMISCK",
		.functions = {
			[0] = "GPIO124",
			[1] = "HDMISCK",
			[4] = "SDA1",
			[5] = "PWM3",
		},
		EINT(0, 97),
	},
[125] =	{	.name = "HDMISD",
		.functions = {
			[0] = "GPIO125",
			[1] = "HDMISD",
			[4] = "SCL1",
			[5] = "PWM4",
		},
		EINT(0, 98),
	},
[126] =	{	.name = "I2S0_MCLK",
		.functions = {
			[0] = "GPIO126",
			[1] = "I2S0_MCLK",
			[6] = "WCN_I2S_MCLK",
			[7] = "DBG_MON_B[6]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL3, __BITS(4,7)),
		EINT(0, 99),
	},
[127] =	{	.name = "RAMBUF_IDATA0",
		.functions = {
			[0] = "GPIO127",
			[1] = "RAMBUF_IDATA0",
		},
	},
	/* 128 */
[128] =	{	.name = "RAMBUF_IDATA1",
		.functions = {
			[0] = "GPIO128",
			[1] = "RAMBUF_IDATA1",
		},
	},
[129] =	{	.name = "RAMBUF_IDATA2",
		.functions = {
			[0] = "GPIO129",
			[1] = "RAMBUF_IDATA2",
		},
	},
[130] =	{	.name = "RAMBUF_IDATA3",
		.functions = {
			[0] = "GPIO130",
			[1] = "RAMBUF_IDATA3",
		},
	},
[131] =	{	.name = "RAMBUF_IDATA4",
		.functions = {
			[0] = "GPIO131",
			[1] = "RAMBUF_IDATA4",
		},
	},
[132] =	{	.name = "RAMBUF_IDATA5",
		.functions = {
			[0] = "GPIO132",
			[1] = "RAMBUF_IDATA5",
		},
	},
[133] =	{	.name = "RAMBUF_IDATA6",
		.functions = {
			[0] = "GPIO133",
			[1] = "RAMBUF_IDATA6",
		},
	},
[134] =	{	.name = "RAMBUF_IDATA7",
		.functions = {
			[0] = "GPIO134",
			[1] = "RAMBUF_IDATA7",
		},
	},
[135] =	{	.name = "GPIO135",
		.functions = {
			[0] = "GPIO135",
			[1] = "RAMBUF_IDATA8",
		},
	},
[136] =	{	.name = "RAMBUF_IDATA9",
		.functions = {
			[0] = "GPIO136",
			[1] = "RAMBUF_IDATA9",
		},
	},
[137] =	{	.name = "RAMBUF_IDATA10",
		.functions = {
			[0] = "GPIO137",
			[1] = "RAMBUF_IDATA10"
		},
	},
[138] =	{	.name = "RAMBUF_IDATA11",
		.functions = {
			[0] = "GPIO138",
			[1] = "RAMBUF_IDATA11",
		},
	},
[139] =	{	.name = "RAMBUF_IDATA12",
		.functions = {
			[0] = "GPIO139",
			[1] = "RAMBUF_IDATA12",
		},
	},
[140] =	{	.name = "RAMBUF_IDATA13",
		.functions = {
			[0] = "GPIO140",
			[1] = "RAMBUF_IDATA13",
		},
	},
[141] =	{	.name = "RAMBUF_IDATA14",
		.functions = {
			[0] = "GPIO141",
			[1] = "RAMBUF_IDATA14",
		},
	},
[142] =	{	.name = "RAMBUF_IDATA15",
		.functions = {
			[0] = "GPIO142",
			[1] = "RAMBUF_IDATA15",
		},
	},
[143] =	{	.name = "RAMBUF_ODATA0",
		.functions = {
			[0] = "GPIO143",
			[1] = "RAMBUF_ODATA0",
		},
	},
	/* 144 */
[144] =	{	.name = "RAMBUF_ODATA1",
		.functions = {
			[0] = "GPIO144",
			[1] = "RAMBUF_ODATA1",
		},
	},
[145] =	{	.name = "RAMBUF_ODATA2",
		.functions = {
			[0] = "GPIO145",
			[1] = "RAMBUF_ODATA2",
		},
	},
[146] =	{	.name = "RAMBUF_ODATA3",
		.functions = {
			[0] = "GPIO146",
			[1] = "RAMBUF_ODATA3",
		},
	},
[147] =	{	.name = "RAMBUF_ODATA4",
		.functions = {
			[0] = "GPIO147",
			[1] = "RAMBUF_ODATA4",
		},
	},
[148] =	{	.name = "RAMBUF_ODATA5",
		.functions = {
			[0] = "GPIO148",
			[1] = "RAMBUF_ODATA5",
		},
	},
[149] =	{	.name = "RAMBUF_ODATA6",
		.functions = {
			[0] = "GPIO149",
			[1] = "RAMBUF_ODATA6",
		},
	},
[150] =	{	.name = "RAMBUF_ODATA7",
		.functions = {
			[0] = "GPIO150",
			[1] = "RAMBUF_ODATA7",
		},
	},
[151] =	{	.name = "RAMBUF_ODATA8",
		.functions = {
			[0] = "GPIO151",
			[1] = "RAMBUF_ODATA8",
		},
	},
[152] =	{	.name = "RAMBUF_ODATA9",
		.functions = {
			[0] = "GPIO152",
			[1] = "RAMBUF_ODATA9",
		},
	},
[153] =	{	.name = "RAMBUF_ODATA10",
		.functions = {
			[0] = "GPIO153",
			[1] = "RAMBUF_ODATA10",
		},
	},
[154] =	{	.name = "RAMBUF_ODATA11",
		.functions = {
			[0] = "GPIO154",
			[1] = "RAMBUF_ODATA11",
		},
	},
[155] =	{	.name = "RAMBUF_ODATA12",
		.functions = {
			[0] = "GPIO155",
			[1] = "RAMBUF_ODATA12",
		},
	},
[156] =	{	.name = "RAMBUF_ODATA13",
		.functions = {
			[0] = "GPIO156",
			[1] = "RAMBUF_ODATA13",
		},
	},
[157] =	{	.name = "GPIO157",
		.functions = {
			[0] = "GPIO157",
			[1] = "RAMBUF_ODATA14",
		},
	},
[158] =	{	.name = "GPIO158",
		.functions = {
			[0] = "GPIO158",
			[1] = "RAMBUF_ODATA15",
		},
	},
[159] =	{	.name = "RAMBUF_BE0",
		.functions = {
			[0] = "GPIO159",
			[1] = "RAMBUF_BE0",
		},
	},
	/* 160 */
[160] =	{	.name = "RAMBUF_BE1",
		.functions = {
			[0] = "GPIO160",
			[1] = "RAMBUF_BE1",
		},
	},
[161] =	{	.name = "AP2PT_INT",
		.functions = {
			[0] = "GPIO161",
			[1] = "AP2PT_INT",
		},
	},
[162] =	{	.name = "AP2PT_INT_CLR",
		.functions = {
			[0] = "GPIO162",
			[1] = "AP2PT_INT_CLR",
		},
	},
[163] =	{	.name = "PT2AP_INT",
		.functions = {
			[0] = "GPIO163",
			[1] = "PT2AP_INT",
		},
	},
[164] =	{	.name = "PT2AP_INT_CLR",
		.functions = {
			[0] = "GPIO164",
			[1] = "PT2AP_INT_CLR",
		},
	},
[165] =	{	.name = "AP2UP_INT",
		.functions = {
			[0] = "GPIO165",
			[1] = "AP2UP_INT",
		},
	},
[166] =	{	.name = "AP2UP_INT_CLR",
		.functions = {
			[0] = "GPIO166",
			[1] = "AP2UP_INT_CLR",
		},
	},
[167] =	{	.name = "UP2AP_INT",
		.functions = {
			[0] = "GPIO167",
			[1] = "UP2AP_INT",
		},
	},
[168] =	{	.name = "UP2AP_INT_CLR",
		.functions = {
			[0] = "GPIO168",
			[1] = "UP2AP_INT_CLR",
		},
	},
[169] =	{	.name = "RAMBUF_ADDR0",
		.functions = {
			[0] = "GPIO169",
			[1] = "RAMBUF_ADDR0",
		},
	},
[170] =	{	.name = "RAMBUF_ADDR1",
		.functions = {
			[0] = "GPIO170",
			[1] = "RAMBUF_ADDR1",
		},
	},
[171] =	{	.name = "RAMBUF_ADDR2",
		.functions = {
			[0] = "GPIO171",
			[1] = "RAMBUF_ADDR2",
		},
	},
[172] =	{	.name = "RAMBUF_ADDR3",
		.functions = {
			[0] = "GPIO172",
			[1] = "RAMBUF_ADDR3",
		},
	},
[173] =	{	.name = "RAMBUF_ADDR4",
		.functions = {
			[0] = "GPIO173",
			[1] = "RAMBUF_ADDR4",
		},
	},
[174] =	{	.name = "RAMBUF_ADDR5",
		.functions = {
			[0] = "GPIO174",
			[1] = "RAMBUF_ADDR5",
		},
	},
[175] =	{	.name = "RAMBUF_ADDR6",
		.functions = {
			[0] = "GPIO175",
			[1] = "RAMBUF_ADDR6",
		},
	},
	/* 176 */
[176] =	{	.name = "RAMBUF_ADDR7",
		.functions = {
			[0] = "GPIO176",
			[1] = "RAMBUF_ADDR7",
		},
	},
[177] =	{	.name = "RAMBUF_ADDR8",
		.functions = {
			[0] = "GPIO177",
			[1] = "RAMBUF_ADDR8",
		},
	},
[178] =	{	.name = "GPIO178",
		.functions = {
			[0] = "GPIO178",
			[1] = "RAMBUF_ADDR9",
		},
	},
[179] =	{	.name = "GPIO179",
		.functions = {
			[0] = "GPIO179",
			[1] = "RAMBUF_ADDR10",
		},
	},
[180] =	{	.name = "RAMBUF_RW",
		.functions = {
			[0] = "GPIO180",
			[1] = "RAMBUF_RW",
		},
	},
[181] =	{	.name = "RAMBUF_LAST",
		.functions = {
			[0] = "GPIO181",
			[1] = "RAMBUF_LAST",
		},
	},
[182] =	{	.name = "RAMBUF_HP",
		.functions = {
			[0] = "GPIO182",
			[1] = "RAMBUF_HP",
		},
	},
[183] =	{	.name = "RAMBUF_REQ",
		.functions = {
			[0] = "GPIO183",
			[1] = "RAMBUF_REQ",
		},
	},
[184] =	{	.name = "RAMBUF_ALE",
		.functions = {
			[0] = "GPIO184",
			[1] = "RAMBUF_ALE",
		},
	},
[185] =	{	.name = "RAMBUF_DLE",
		.functions = {
			[0] = "GPIO185",
			[1] = "RAMBUF_DLE",
		},
	},
[186] =	{	.name = "GPIO186",
		.functions = {
			[0] = "GPIO186",
			[1] = "RAMBUF_WDLE",
		},
	},
[187] =	{	.name = "RAMBUF_O_CLK",
		.functions = {
			[0] = "GPIO187",
			[1] = "RAMBUF_O_CLK",
		},
	},
[188] =	{	.name = "I2S2_MCLK",
		.functions = {
			[0] = "GPIO188",
			[1] = "I2S2_MCLK",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL2, __BITS(4,7)),
	},
[189] =	{	.name = "I2S3_DATA",
		.functions = {
			[0] = "GPIO189",
			[1] = "I2S3_DATA",
			[2] = "I2S3_DATA_BYPS",
			[3] = "PCM_TX",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL7, __BITS(8,11)),
	},
[190] =	{	.name = "I2S3_DATA_IN",
		.functions = {
			[0] = "GPIO190",
			[1] = "I2S3_DATA_IN",
			[3] = "PCM_RX",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL7, __BITS(8,11)),
	},
[191] =	{	.name = "I2S3_BCK",
		.functions = {
			[0] = "GPIO191",
			[1] = "I2S3_BCK",
			[3] = "PCM_CLK0",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL7, __BITS(8,11)),
	},
	/* 192 */
[192] =	{	.name = "I2S3_LRCK",
		.functions = {
			[0] = "GPIO192",
			[1] = "I2S3_LRCK",
			[3] = "PCM_SYNC",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL7, __BITS(8,11)),
	},
[193] =	{	.name = "I2S3_MCLK",
		.functions = {
			[0] = "GPIO193",
			[1] = "I2S3_MCLK",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL7, __BITS(8,11)),
	},
[194] =	{	.name = "I2S4_DATA",
		.functions = {
			[0] = "GPIO194",
			[1] = "I2S4_DATA",
			[2] = "I2S4_DATA_BYPS",
			[3] = "PCM_TX",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL7, __BITS(12,15)),
	},
[195] =	{	.name = "I2S4_DATA_IN",
		.functions = {
			[0] = "GPIO195",
			[1] = "I2S4_DATA_IN",
			[3] = "PCM_RX",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL7, __BITS(12,15)),
	},
[196] =	{	.name = "I2S4_BCK",
		.functions = {
			[0] = "GPIO196",
			[1] = "I2S4_BCK",
			[3] = "PCM_CLK0",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL7, __BITS(12,15)),
	},
[197] =	{	.name = "I2S4_LRCK",
		.functions = {
			[0] = "GPIO197",
			[1] = "I2S4_LRCK",
			[3] = "PCM_SYNC",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL7, __BITS(12,15)),
	},
[198] =	{	.name = "I2S4_MCLK",
		.functions = {
			[0] = "GPIO198",
			[1] = "I2S4_MCLK",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL7, __BITS(12,15)),
	},
[199] =	{	.name = "SPI1_CK",
		.functions = {
			[0] = "GPIO199",
			[1] = "SPI1_CK",
			[3] = "EXT_FRAME_SYNC",
			[4] = "KCOL3",
			[7] = "DBG_MON_B[15]",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL0, __BITS(4,7)),
		EINT(0, 111),
	},
[200] =	{	.name = "SPDIF_OUT",
		.functions = {
			[0] = "GPIO200",
			[1] = "SPDIF_OUT",
			[5] = "G1_TXD3",
			[6] = "URXD2",
			[7] = "DBG_MON_B[16]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL8, __BITS(0,3)),
		EINT(0, 112),
	},
[201] =	{	.name = "SPDIF_IN0",
		.functions = {
			[0] = "GPIO201",
			[1] = "SPDIF_IN0",
			[5] = "G1_TXEN",
			[6] = "UTXD2",
			[7] = "DBG_MON_B[17]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL8, __BITS(0,3)),
		EINT(0, 113),
	},
[202] =	{	.name = "SPDIF_IN1",
		.functions = {
			[0] = "GPIO202",
			[1] = "SPDIF_IN1",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL8, __BITS(0,3)),
		EINT(0, 114),
	},
[203] =	{	.name = "PWM0",
		.functions = {
			[0] = "GPIO203",
			[1] = "PWM0",
			[2] = "DISP_PWM",
			[5] = "G1_TXD2",
			[7] = "DBG_MON_B[18]",
		/*	 9 == "I2S2_DATA"	*/
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL8, __BITS(4,7)),
		EINT(0, 115),
	},
[204] =	{	.name = "PWM1",
		.functions = {
			[0] = "GPIO204",
			[1] = "PWM1",
			[2] = "CLKM3",
			[5] = "G1_TXD1",
			[7] = "DBG_MON_B[19]",
		/*	 9 == "I2S3_DATA"	*/
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL8, __BITS(4,7)),
		EINT(0, 116),
	},
[205] =	{	.name = "PWM2",
		.functions = {
			[0] = "GPIO205",
			[1] = "PWM2",
			[2] = "CLKM2",
			[5] = "G1_TXD0",
			[7] = "DBG_MON_B[20]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL8, __BITS(4,7)),
		EINT(0, 117),
	},
[206] =	{	.name = "PWM3",
		.functions = {
			[0] = "GPIO206",
			[1] = "PWM3",
			[2] = "CLKM1",
			[3] = "EXT_FRAME_SYNC",
			[5] = "G1_TXC",
			[7] = "DBG_MON_B[21]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL8, __BITS(4,7)),
		EINT(0, 118),
	},
[207] =	{	.name = "PWM4",
		.functions = {
			[0] = "GPIO207",
			[1] = "PWM4",
			[2] = "CLKM0",
			[3] = "EXT_FRAME_SYNC",
			[5] = "G1_RXC",
			[7] = "DBG_MON_B[22]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL8, __BITS(4,7)),
		EINT(0, 119),
	},
	/* 208 */
[208] =	{	.name = "AUD_EXT_CK1",
		.functions = {
			[0] = "GPIO208",
			[1] = "AUD_EXT_CK1",
			[2] = "PWM0",
			[3] = "PCIE0_PERST_N",	/* MT7623 */
			[4] = "ANT_SEL5",
			[5] = "DISP_PWM",
			[7] = "DBG_MON_A[31]",
		/*	11 == "PCIE0_PERST_N"	   MT2701 */
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL8, __BITS(8,11)),
		EINT(0, 120),
	},
[209] =	{	.name = "AUD_EXT_CK2",
		.functions = {
			[0] = "GPIO209",
			[1] = "AUD_EXT_CK2",
			[2] = "MSDC1_WP",
			[3] = "PCIE1_PERST_N",	/* MT7623 */
			[5] = "PWM1",
			[7] = "DBG_MON_A[32]",
		/*	11 == "PCIE1_PERST_N"	   MT2701 */
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL8, __BITS(8,11)),
		EINT(0, 121),
	},
[210] =	{	.name = "AUD_CLOCK",
		.functions = {
			[0] = "GPIO210",
			[1] = "AUD_CLOCK",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL8, __BITS(12,15)),
	},
[211] =	{	.name = "GPIO211",
		.functions = {
			[0] = "GPIO211",
			[1] = "DVP_RESET",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[212] =	{	.name = "DVP_CLOCK",
		.functions = {
			[0] = "GPIO212",
			[1] = "DVP_CLOCK",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[213] =	{	.name = "DVP_CS",
		.functions = {
			[0] = "GPIO213",
			[1] = "DVP_CS",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[214] =	{	.name = "DVP_CK",
		.functions = {
			[0] = "GPIO214",
			[1] = "DVP_CK",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[215] =	{	.name = "DVP_DI",
		.functions = {
			[0] = "GPIO215",
			[1] = "DVP_DI",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[216] =	{	.name = "DVP_DO",
		.functions = {
			[0] = "GPIO216",
			[1] = "DVP_DO",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[217] =	{	.name = "GPIO217",
		.functions = {
			[0] = "GPIO217",
			[1] = "AP_CS",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[218] =	{	.name = "AP_CK",
		.functions = {
			[0] = "GPIO218",
			[1] = "AP_CK",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[219] =	{	.name = "AP_DI",
		.functions = {
			[0] = "GPIO219",
			[1] = "AP_DI",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[220] =	{	.name = "AP_DO",
		.functions = {
			[0] = "GPIO220",
			[1] = "AP_DO",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[221] =	{	.name = "DVD_BCLK",
		.functions = {
			[0] = "GPIO221",
			[1] = "DVD_BCLK",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[222] =	{	.name = "T8032_CLK",
		.functions = {
			[0] = "GPIO222",
			[1] = "T8032_CLK",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[223] =	{	.name = "AP_BCLK",
		.functions = {
			[0] = "GPIO223",
			[1] = "AP_BCLK",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
	/* 224 */
[224] =	{	.name = "HOST_CS",
		.functions = {
			[0] = "GPIO224",
			[1] = "HOST_CS",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[225] =	{	.name = "HOST_CK",
		.functions = {
			[0] = "GPIO225",
			[1] = "HOST_CK",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[226] =	{	.name = "HOST_DO0",
		.functions = {
			[0] = "GPIO226",
			[1] = "HOST_DO0",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[227] =	{	.name = "HOST_DO1",
		.functions = {
			[0] = "GPIO227",
			[1] = "HOST_DO1",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[228] =	{	.name = "SLV_CS",
		.functions = {
			[0] = "GPIO228",
			[1] = "SLV_CS",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[229] =	{	.name = "SLV_CK",
		.functions = {
			[0] = "GPIO229",
			[1] = "SLV_CK",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[230] =	{	.name = "SLV_DI0",
		.functions = {
			[0] = "GPIO230",
			[1] = "SLV_DI0",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[231] =	{	.name = "SLV_DI1",
		.functions = {
			[0] = "GPIO231",
			[1] = "SLV_DI1",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[232] =	{	.name = "AP2DSP_INT",
		.functions = {
			[0] = "GPIO232",
			[1] = "AP2DSP_INT",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[233] =	{	.name = "AP2DSP_INT_CLR",
		.functions = {
			[0] = "GPIO233",
			[1] = "AP2DSP_INT_CLR",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[234] =	{	.name = "DSP2AP_INT",
		.functions = {
			[0] = "GPIO234",
			[1] = "DSP2AP_INT",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[235] =	{	.name = "DSP2AP_INT_CLR",
		.functions = {
			[0] = "GPIO235",
			[1] = "DSP2AP_INT_CLR",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL9, __BITS(0,3)),
	},
[236] =	{	.name = "EXT_SDIO3",
		.functions = {
			[0] = "GPIO236",
			[1] = "EXT_SDIO3",
			[2] = "IDDIG",
			[7] = "DBG_MON_A[1]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL9, __BITS(4,7)),
		EINT(0, 122),
	},
[237] =	{	.name = "EXT_SDIO2",
		.functions = {
			[0] = "GPIO237",
			[1] = "EXT_SDIO2",
			[2] = "DRV_VBUS",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL9, __BITS(4,7)),
		EINT(0, 123),
	},
[238] =	{	.name = "EXT_SDIO1",
		.functions = {
			[0] = "GPIO238",
			[1] = "EXT_SDIO1",
			[2] = "IDDIG_P1",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL9, __BITS(4,7)),
		EINT(0, 124),
	},
[239] =	{	.name = "EXT_SDIO0",
		.functions = {
			[0] = "GPIO239",
			[1] = "EXT_SDIO0",
			[2] = "DRV_VBUS_P1",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL9, __BITS(4,7)),
		EINT(0, 125),
	},
	/* 240 */
[240] =	{	.name = "EXT_XCS",
		.functions = {
			[0] = "GPIO240",
			[1] = "EXT_XCS",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL9, __BITS(4,7)),
		EINT(0, 126),
	},
[241] =	{	.name = "EXT_SCK",
		.functions = {
			[0] = "GPIO241",
			[1] = "EXT_SCK",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL9, __BITS(4,7)),
		EINT(0, 127),
	},
[242] =	{	.name = "URTS2",
		.functions = {
			[0] = "GPIO242",
			[1] = "URTS2",
			[2] = "UTXD3",
			[3] = "URXD3",
			[4] = "SCL1",
			[7] = "DBG_MON_A[6]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL9, __BITS(8,11)),
		EINT(0, 128),
	},
[243] =	{	.name = "UCTS2",
		.functions = {
			[0] = "GPIO243",
			[1] = "UCTS2",
			[2] = "URXD3",
			[3] = "UTXD3",
			[4] = "SDA1",
			[7] = "DBG_MON_A[6]",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL9, __BITS(8,11)),
		EINT(0, 129),
	},
	/* XXXJRT Linux driver lacks drive for 244-247, but manual has it. */
[244] =	{	.name = "HDMI_SDA_RX",
		.functions = {
			[0] = "GPIO244",
			[1] = "HDMI_SDA_RX",
		},
		EINT(0, 130),
	},
[245] =	{	.name = "GPIO245",
		.functions = {
			[0] = "GPIO245",
			[1] = "HDMI_SCL_RX",
		},
		EINT(0, 131),
	},
[246] =	{	.name = "MHL_SENCE",
		.functions = {
			[0] = "GPIO246",
		},
		EINT(0, 132),
	},
[247] =	{	.name = "HDMI_HPD_RX",	/* HDMI_HPD_CBUS_RX ? */
		.functions = {
			[0] = "GPIO247",
			[1] = "HDMI_HPD_RX",
		},
		EINT(0, 69),		/* XXXJRT 69/70 ?? */
	},
[248] =	{	.name = "HDMI_TESTOUTP_RX",
		.functions = {
			[0] = "GPIO248",
			[1] = "HDMI_TESTOUTP_RX",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(0,3)),
		EINT(0, 133),
	},
[249] =	{	.name = "MSDC3_RSTB",
		.functions = {
			[0] = "GPIO249",
			[1] = "MSDC3_RSTB",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_SDIO_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_SDIO_CTRL5, 0, 1, 2),
	},
[250] =	{	.name = "MSDC3_DAT7",
		.functions = {
			[0] = "GPIO250",
			[1] = "MSDC3_DAT7",
			[6] = "PCIE0_CLKREQ_N",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_SDIO_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_SDIO_CTRL4, 12, 13, 14),
		EINT(0, 135),
	},
[251] =	{	.name = "MSDC3_DAT6",
		.functions = {
			[0] = "GPIO251",
			[1] = "MSDC3_DAT6",
			[6] = "PCIE0_WAKE_N",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_SDIO_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_SDIO_CTRL4, 8, 9, 10),
		EINT(0, 136),
	},
[252] =	{	.name = "MSDC3_DAT5",
		.functions = {
			[0] = "GPIO252",
			[1] = "MSDC3_DAT5",
			[6] = "PCIE1_CLKREQ_N",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_SDIO_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_SDIO_CTRL4, 4, 5, 6),
		EINT(0, 137),
	},
[253] =	{	.name = "MSDC3_DAT4",
		.functions = {
			[0] = "GPIO253",
			[1] = "MSDC3_DAT4",
			[6] = "PCIE1_WAKE_N",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_SDIO_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_SDIO_CTRL4, 0, 1, 2),
		EINT(0, 138),
	},
[254] =	{	.name = "MSDC3_DAT3",
		.functions = {
			[0] = "GPIO254",
			[1] = "MSDC3_DAT3",
			[6] = "PCIE2_CLKREQ_N",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_SDIO_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_SDIO_CTRL3, 12, 13, 14),
		EINT(0, 139),
	},
[255] =	{	.name = "MSDC3_DAT2",
		.functions = {
			[0] = "GPIO255",
			[1] = "MSDC3_DAT2",
			[6] = "PCIE2_WAKE_N",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_SDIO_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_SDIO_CTRL3, 8, 9, 10),
		EINT(0, 140),
	},
	/* 256 */
[256] =	{	.name = "MSDC3_DAT1",
		.functions = {
			[0] = "GPIO256",
			[1] = "MSDC3_DAT1",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_SDIO_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_SDIO_CTRL3, 4, 5, 6),
		EINT(0, 141),
	},
[257] =	{	.name = "MSDC3_DAT0",
		.functions = {
			[0] = "GPIO257",
			[1] = "MSDC3_DAT0",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_SDIO_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_SDIO_CTRL3, 0, 1, 2),
		EINT(0, 142),
	},
[258] =	{	.name = "MSDC3_CMD",
		.functions = {
			[0] = "GPIO258",
			[1] = "MSDC3_CMD",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC3_CTRL1, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC3_CTRL1, 8, 9, 10),
	},
[259] =	{	.name = "MSDC3_CLK",
		.functions = {
			[0] = "GPIO259",
			[1] = "MSDC3_CLK",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC3_CTRL0, __BITS(0,2), 3),
		PUPDR1R0(GPIO_MSDC3_CTRL0, 8, 9, 10),
	},
[260] =	{	.name = "MSDC3_DSL",	/* XXX */
		.functions = {
			[0] = "GPIO260",
			[1] = "MSDC3_DSL",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_SDIO_CTRL7, __BITS(0,2), 3),
		PUPDR1R0(GPIO_SDIO_CTRL7, 8, 9, 10),
	},
[261] =	{	.name = "MSDC1_INS",	/* XXX */
		.functions = {
			[0] = "GPIO261",
			[1] = "MSDC1_INS",
			[7] = "DBG_MON_B[29]",
		},
		DRIVE_SLEW(2_4_6_8_10_12_14_16,
			     GPIO_MSDC1_CTRL2, __BITS(0,2), 3),
		PUPDR1R0(GPIO_SDIO_CTRL5, 8, 9, 10),
	},
[262] =	{	.name = "G2_TXEN",
		.functions = {
			[0] = "GPIO262",
			[1] = "G2_TXEN",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[263] =	{	.name = "G2_TXD3",
		.functions = {
			[0] = "GPIO263",
			[1] = "G2_TXD3",
			[6] = "ANT_SEL5",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[264] =	{	.name = "G2_TXD2",
		.functions = {
			[0] = "GPIO264",
			[1] = "G2_TXD2",
			[6] = "ANT_SEL4",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[265] =	{	.name = "G2_TXD1",
		.functions = {
			[0] = "GPIO265",
			[1] = "G2_TXD1",
			[6] = "ANT_SEL3",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[266] =	{	.name = "G2_TXD0",
		.functions = {
			[0] = "GPIO266",
			[1] = "G2_TXD0",
			[6] = "ANT_SEL2",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[267] =	{	.name = "G2_TXC",
		.functions = {
			[0] = "GPIO267",
			[1] = "G2_TXC",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[268] =	{	.name = "G2_RXC",
		.functions = {
			[0] = "GPIO268",
			[1] = "G2_RXC",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[269] =	{	.name = "G2_RXD0",
		.functions = {
			[0] = "GPIO269",
			[1] = "G2_RXD0",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[270] =	{	.name = "G2_RXD1",
		.functions = {
			[0] = "GPIO270",
			[1] = "G2_RXD1",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[271] =	{	.name = "G2_RXD2",
		.functions = {
			[0] = "GPIO271",
			[1] = "G2_RXD2",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
	/* 272 */
[272] =	{	.name = "G2_RXD3",
		.functions = {
			[0] = "GPIO272",
			[1] = "G2_RXD3",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[273] =	{	.name = "ESW_INT",
		.functions = {
			[0] = "GPIO273",
			[1] = "ESW_INT",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[274] =	{	.name = "G2_RXDV",
		.functions = {
			[0] = "GPIO274",
			[1] = "G2_RXDV",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[275] =	{	.name = "MDC",
		.functions = {
			[0] = "GPIO275",
			[1] = "MDC",
			[6] = "ANT_SEL0",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[276] =	{	.name = "MDIO",
		.functions = {
			[0] = "GPIO276",
			[1] = "MDIO",
			[6] = "ANT_SEL1",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[277] =	{	.name = "ESW_RST",
		.functions = {
			[0] = "GPIO277",
			[1] = "ESW_RST",
		},
		DRIVE(4_8_12_16, GPIO_DRV_SEL10, __BITS(8,11)),
	},
[278] =	{	.name = "JTAG_RESET",
		.functions = {
			[0] = "GPIO278",
			[1] = "JTAG_RESET",
		},
		DRIVE(2_4_6_8, GPIO_DRV_SEL2, __BITS(8,11)),
		EINT(0, 147),
	},
[279] = {	.name = "USB3_RES_BOND",
		.functions = {
			[0] = "GPIO279",
			[1] = "USB3_RES_BOND"
		},
	},
};

#define	BANKED_PIN(_pin, _reg, _bit)					\
	{								\
		.pin = (_pin),						\
		.bank_reg = (_reg),					\
		.mask = __BIT(_bit),					\
	}

static const struct {
	u_int		pin;
	bus_size_t	bank_reg;
	uint16_t	mask;
} mt2701_banked_pins[] = {
	BANKED_PIN( 22, GPIO_BANK_CTRL0, 3),	/* f10 == PCIE0_PERST_N */
	BANKED_PIN( 23, GPIO_BANK_CTRL0, 4),	/* f10 == PCIE1_PERST_N */
	BANKED_PIN( 24, GPIO_BANK_CTRL0, 5),	/* f10 == PCIE2_PERST_N */
	BANKED_PIN( 29, GPIO_BANK_CTRL0, 9),	/* f14 == PCIE2_PERST_N */
	BANKED_PIN(208, GPIO_BANK_CTRL0, 7),	/* f11 == PCIE0_PERST_N */
	BANKED_PIN(209, GPIO_BANK_CTRL0, 8),	/* f11 == PCIE1_PERST_N */
	BANKED_PIN(203, GPIO_BANK_CTRL1, 0),	/* f9  == I2S2_DATA */
	BANKED_PIN(204, GPIO_BANK_CTRL1, 1),	/* f9  == I2S3_DATA */
	BANKED_PIN(249, GPIO_BANK_CTRL2, 0),	/* MSDC3 ... */
	BANKED_PIN(250, GPIO_BANK_CTRL2, 0),	/* . */
	BANKED_PIN(251, GPIO_BANK_CTRL2, 0),	/* . */
	BANKED_PIN(252, GPIO_BANK_CTRL2, 0),	/* . */
	BANKED_PIN(253, GPIO_BANK_CTRL2, 0),	/* . */
	BANKED_PIN(254, GPIO_BANK_CTRL2, 0),	/* . */
	BANKED_PIN(255, GPIO_BANK_CTRL2, 0),	/* . */
	BANKED_PIN(256, GPIO_BANK_CTRL2, 0),	/* . */
	BANKED_PIN(257, GPIO_BANK_CTRL2, 0),	/* . */
	BANKED_PIN(258, GPIO_BANK_CTRL2, 0),	/* . */
	BANKED_PIN(259, GPIO_BANK_CTRL2, 0),	/* . */
	BANKED_PIN(260, GPIO_BANK_CTRL2, 0),	/* . */
};

static int
mt2701_setfunc_hook(struct mtk_gpio_softc * const sc, const u_int pin,
		    int * const func_num)
{
	if (func_num < 0) {
		/*
		 * The function wasn't found; no bank switching
		 * is necessary.
		 */
		return 0;
	}

	/*
	 * Some pins require some special handling when setting the
	 * pin function.
	 *
	 * WARNING: There is some dark magick here that's not fully
	 * documented, but that is used in device trees that exist
	 * in the wild.
	 */
	u_int idx;
	for (idx = 0; idx < __arraycount(mt2701_banked_pins); idx++) {
		if (pin == mt2701_banked_pins[idx].pin)
			break;
	}
	if (idx == __arraycount(mt2701_banked_pins))
		return 0;

	const uint16_t bank_mask = mt2701_banked_pins[idx].mask;
	uint16_t val = GPIO_READ(sc, mt2701_banked_pins[idx].bank_reg);
	if (*func_num > 7) {
		val &= ~bank_mask;
		*func_num -= 8;
	} else {
		val |= bank_mask;
	}
	GPIO_WRITE(sc, mt2701_banked_pins[idx].bank_reg, val);

	return 0;
}

static void
mt2701_dir_addr_fixup(bus_size_t *addrp, u_int pin __unused)
{
	/* There's a register between DIR11 and DIR12; hop over it. */
	if (*addrp > GPIO_GPIO_DIR11)
		*addrp += 0x10;
}

static const struct mtk_eintc_config mt2701_eintc = {
	.nintrs = 197,
	.reg_groups = {
		[MTK_EINTC_REGS_STA_ACK] = {
			.base = GPIO_EINT_STA(0),
			.clr_base = GPIO_EINT_ACK(0),
		},
		[MTK_EINTC_REGS_MASK] = {
			.base = GPIO_EINT_MASK(0),
			.set_base = GPIO_EINT_MASK_SET(0),
			.clr_base = GPIO_EINT_MASK_CLR(0),
		},
		[MTK_EINTC_REGS_SENS] = {
			.base = GPIO_EINT_SENS(0),
			.set_base = GPIO_EINT_SENS_SET(0),
			.clr_base = GPIO_EINT_SENS_CLR(0),
		},
		[MTK_EINTC_REGS_SOFT] = {
			.base = GPIO_EINT_SOFT(0),
			.set_base = GPIO_EINT_SOFT_SET(0),
			.clr_base = GPIO_EINT_SOFT_CLR(0),
		},
		[MTK_EINTC_REGS_POL] = {
			.base = GPIO_EINT_POL(0),
			.set_base = GPIO_EINT_POL_SET(0),
			.clr_base = GPIO_EINT_POL_CLR(0),
		},
	},
	.nbanks = 6,
	.irqs_per_bank = 32,
	.domen_base = GPIO_EINT_D0EN(0),
};

const struct mtk_gpio_padconf mt2701_gpio_padconf = {
	.pins = mt2701_gpio_pins,
	.npins = __arraycount(mt2701_gpio_pins),
	.ies_smt_groups = mt2701_ies_smt_groups,
	.nies_smt_groups = __arraycount(mt2701_ies_smt_groups),
	.reg_index_shift = 4,
	.reg_groups = {
		[MTK_GPIO_REGS_DIR] = {
			.base = GPIO_GPIO_DIR1,
			.addr_fixup = mt2701_dir_addr_fixup,
			.pins_per_reg = 16,
		},
		[MTK_GPIO_REGS_PULLEN] = {
			.base = GPIO_GPIO_PULLEN1,
			.pins_per_reg = 16,
		},
		[MTK_GPIO_REGS_PULLSEL] = {
			.base = GPIO_GPIO_PULLSEL1,
			.pins_per_reg = 16,
		},
		[MTK_GPIO_REGS_DOUT] = {
			.base = GPIO_GPIO_DOUT1,
			.pins_per_reg = 16,
		},
		[MTK_GPIO_REGS_DIN] = {
			.base = GPIO_GPIO_DIN1,
			.pins_per_reg = 16,
		},
		[MTK_GPIO_REGS_MODE] = {
			.base = GPIO_GPIO_MODE1,
			.pins_per_reg = 5,
			.bits_per_pin = 3,
		},
	},
	.setfunc_hook = mt2701_setfunc_hook,
	.eintc = &mt2701_eintc,
};
