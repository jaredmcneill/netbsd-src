/* $NetBSD$ */

/*-
 * Copyright (c) 2018 Jason R. Thorpe
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MEDIATEK_MT6589_TOPRGUREG_H_
#define	_MEDIATEK_MT6589_TOPRGUREG_H_

#define	MT7623_TOPRGU_PADDR		0x10007000

/* Relative to TOPRGU Base address */
#define	WDT_MODE			0x00
#define	WDT_LENGTH			0x04
#define	WDT_RESTART			0x08
#define	WDT_STA				0x0c
#define	WDT_INTERVAL			0x10
#define	WDT_SWRST			0x14
#define	WDT_SWSYSRST			0x18
#define	WDT_REQ_MODE			0x30
#define	WDT_REQ_IRQ_EN			0x34
#define	WDT_DEBUG_CTL			0x40
#define	WDT_INTERCORE_SYNC		0x50
#define	WDT_INTERCORE_SYNC_SET		0x54
#define	WDT_INTERCORE_SYNC_CLR		0x58

/* Relative to TOPRGU_2ND Base address */
#define	RESET_DEGLITCH_KEY		0x310

/* Watchdog Mode Register */
#define	WDT_MODE_WDT_EN			__BIT(0)
#define	WDT_MODE_EXTPOL			__BIT(1)
#define	WDT_MODE_EXTEN			__BIT(2)
#define	WDT_MODE_WDT_IRQ		__BIT(3)
#define	WDT_MODE_IRQ_LVL_EN		__BIT(5)
#define WDT_MODE_DUAL_MODE		__BIT(6)
#define	WDT_MODE_DDR_RESERVE_MODE	__BIT(7)
#define	WDT_MODE_UNLOCK_KEY		(0x22U << 24)

/* Watchdog Counter Setting Register */
#define	WDT_LENGTH_UNLOCK_KEY		0x08
#define	WDT_LENGTH_WDT_LENGTH		__BITS(5,15)

/* Watchdog Counter Restart Register */
#define	WDT_RESTART_WDT_RESTART		0x1971

/* Watchdog Status Register */
#define	WDT_STA_THERMAL_RST		__BIT(0)
#define	WDT_STA_SPM_WDT_RST		__BIT(1)
#define	WDT_STA_DEBUG_RST		__BIT(19)
#define	WDT_STA_IRQ_ASSERT		__BIT(29)
#define	WDT_STA_SW_WDT_RST		__BIT(30)
#define	WDT_STA_HW_WDT_RST		__BIT(31)

/* Watchdog Reset Pulse Width Register */
#define	WDT_INTERVAL_WDT_RESET_INTERVAL_MASK 0x7ff

/* Software Watchdog Reset Register */
#define	WDT_SWRST_UNLOCK_KEY		0x1209

/* System Software Reset Register */
#define	WDT_SWSYSR_INFRA_RST		__BIT(0)
#define	WDT_SWSYSR_MM_RST		__BIT(1)
#define	WDT_SWSYSR_MFG_RST		__BIT(2)
#define	WDT_SWSYSR_ETHDMA_RST		__BIT(3)
#define	WDT_SWSYSR_VDEC_RST		__BIT(4)
#define	WDT_SWSYSR_VENC_IMG_RST		__BIT(5)
#define	WDT_SWSYSR_DDRPHY_RST		__BIT(6)
#define	WDT_SWSYSR_MD_RST		__BIT(7)
#define	WDT_SWSYSR_INFRA_AO_RST		__BIT(8)
#define	WDT_SWSYSR_CONN_RST		__BIT(9)
#define	WDT_SWSYSR_APMIXED_RST		__BIT(10)
#define	WDT_SWSYSR_HIFSYS_RST		__BIT(11)
#define	WDT_SWSYSR_CONN_MCU_RST		__BIT(12)
#define	WDT_SWSYSR_BDP_DISP_RST		__BIT(13)
#define	WDT_SWSYSR_UNLOCK_KEY		(0x88U << 24)

/* Reset Request Mode Register */
#define	WDT_REQ_MODE_THERMAL_EN		__BIT(0)
#define	WDT_REQ_MODE_SCPSYS_EN		__BIT(1)
#define	WDT_REQ_MODE_DEBUG_EN		__BIT(19)
#define	WDT_REQ_MODE_UNLOCK_KEY		(0x33U << 24)

/* Reset Request IRQ Enable Register */
#define	WDT_REQ_IRQ_EN_THERMAL_IRQ	__BIT(0)
#define	WDT_REQ_IRQ_EN_SCPSYS_IRQ	__BIT(1)
#define	WDT_REQ_IRQ_EN_DEBUG_IRQ	__BIT(19)
#define	WDT_REQ_IRQ_EN_UNLOCK_KEY	(0x44U << 24)

/* Debug Control Register */
#define	WDT_DEBUG_CTL_RG_DDR_PROTECT_EN	__BIT(0)
#define	WDT_DEBUG_CTL_RG_MCU_LATH_EN	__BIT(1)
#define	WDT_DEBUG_CTL_DRAMC_TIMEOUT_MASK 0xf
#define	WDT_DEBUG_CTL_DRAMC_TIMEOUT(x)	(((x) & WDT_DEBUG_CTL_DRAMC_TIMEOUT_MASK) << 4)
#define	WDT_DEBUG_CTL_DRAMC_SREF	__BIT(8)
#define	WDT_DEBUG_CTL_DRAMC_ISO		__BIT(9)
#define	WDT_DEBUG_CTL_DRAMC_CONF_ISO	__BIT(10)
#define	WDT_DEBUG_CTL_DDR_RESERVE_STA	__BIT(16)
#define	WDT_DEBUG_CTL_DDR_SREF_STA	__BIT(17)
#define	WDT_DEBUG_CTL_UNLOCK_KEY	(0x59U << 24)

/* Reset Deglitch Enable Key Register */
#define	RESET_DEGLITCH_KEY_DGRST_EN_KEY	0x67d2a357U

#endif /* _MEDIATEK_MT6589_TOPRGUREG_H_ */
