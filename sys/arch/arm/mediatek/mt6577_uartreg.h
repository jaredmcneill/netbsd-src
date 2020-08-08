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

#ifndef _MEDIATEK_MT6577_UARTREG_H_
#define	_MEDIATEK_MT6577_UARTREG_H_

#define	MT7623_UART0_PADDR	0x11002000
#define	MT7623_UART1_PADDR	0x11003000
#define	MT7623_UART2_PADDR	0x11004000
#define	MT7623_UART3_PADDR	0x11005000

#define	MTK_UART_RBR		0x00
#define	MTK_UART_THR		0x00
#define	MTK_UART_DLL		0x00
#define	MTK_UART_IER		0x04
#define	MTK_UART_DLM		0x04
#define	MTK_UART_IIR		0x08
#define	MTK_UART_FCR		0x08
#define	MTK_UART_ECR		0x08
#define	MTK_UART_LCR		0x0c
#define	MTK_UART_MCR		0x10
#define	MTK_UART_XON1		0x10
#define	MTK_UART_LSR		0x14
#define	MTK_UART_XON2		0x14
#define	MTK_UART_MSR		0x18
#define	MTK_UART_XOFF1		0x18
#define	MTK_UART_SCR		0x1c
#define	MTK_UART_XOFF2		0x1c
#define	MTK_UART_AUTOBAUD_EN	0x20
#define	MTK_UART_HIGHSPEED	0x24
#define	MTK_UART_SAMPLE_COUNT	0x28
#define	MTK_UART_SAMPLE_POINT	0x2c
#define	MTK_UART_AUTOBAUD_REG	0x30
#define	MTK_UART_RATE_FIX	0x34
#define	MTK_UART_GUARD		0x3c
#define	MTK_UART_ESCAPE_DAT	0x40
#define	MTK_UART_ESCAPE_EN	0x44
#define	MTK_UART_SLEEP_EN	0x48
#define	MTK_UART_DMA_EN		0x4c
#define	MTK_UART_RXTRI_AD	0x50
#define	MTK_UART_FRACDIV_L	0x54
#define	MTK_UART_FRACDIV_M	0x58
#define	MTK_UART_FCR_RD		0x5c
#define	MTK_UART_DEBUG0		0x60
#define	MTK_UART_DEBUG1		0x64
#define	MTK_UART_RX_SEL		0x90

/* Additional IER bits not present on ns16550 */
#define	IER_RX_ABOVE_TRG	__BIT(4)
#define	IER_XOFFI		__BIT(5)

/* Additional FCR bits not present on ns16550 */
#define	FIFO_TXTRIGGER_1	0x00
#define	FIFO_TXTRIGGER_4	0x10
#define	FIFO_TXTRIGGER_8	0x20
#define	FIFI_TXTRIGGER_14	0x30

/* Additional MCR bits not present in ns16550 */
#define	MCR_XOFF_STATUS		__BIT(7)

/* Autobaud Enable Register */
#define	AUTOBAUD_EN		__BIT(0)
#define	AUTOBAUD_SEL		__BIT(1)

/* Highspeed Register */
#define	HSU_DIV16		0x00	/* normal operation */
#define	HSU_DIV8		0x01
#define	HSU_DIV4		0x02
#define	HSU_DIV1		0x03

/* Autobaud status */
#define	BAUD_STAT_RATE_MASK	0x0f
#define	BAUD_STAT_RATE_115200	0
#define	BAUD_STAT_RATE_57600	1
#define	BAUD_STAT_RATE_38400	2
#define	BAUD_STAT_RATE_19200	3
#define	BAUD_STAT_RATE_9600	4
#define	BAUD_STAT_RATE_4800	5
#define	BAUD_STAT_RATE_2400	6
#define	BAUD_STAT_RATE_1200	7
#define	BAUD_STAT_RATE_300	8
#define	BAUD_STAT_RATE_110	9
#define	BUAD_STAT_STAT_MASK	0xf0
#define	BAUD_STAT_DETECTING	(0U << 4)
#define	BAUD_STAT_AT_7N1	(1U << 4)
#define	BAUD_STAT_AT_7O1	(2U << 4)
#define	BAUD_STAT_AT_7E1	(3U << 4)
#define	BAUD_STAT_AT_8N1	(4U << 4)
#define	BAUD_STAT_AT_8O1	(5U << 4)
#define	BAUD_STAT_AT_8E1	(6U << 4)
#define	BAUD_STAT_at_7N1	(7U << 4)
#define	BAUD_STAT_at_7E1	(8U << 4)
#define	BAUD_STAT_at_7O1	(9U << 4)
#define	BAUD_STAT_at_8N1	(10U << 4)
#define	BAUD_STAT_at_8E1	(11U << 4)
#define	BAUD_STAT_at_8O1	(12U << 4)
#define	BAUD_STAT_FAILED	(13U << 4)

/* Guard Register */
#define	GUARD_CNT_MASK		0x0f
#define	GUARD_CNT(x)		((x) & GUARD_CNT_MASK)
#define	GUARD_EN		__BIT(4)

/* Escape Data */
#define	ESCAPE_DAT_MASK		0x7f
#define	ESCAPE_DAT(x)		((x) & ESCAPE_DAT_MASK)

/* Escsape Enable */
#define	ESCAPE_EN		__BIT(0)

/* Sleep Enable */
#define	SLEEP_EN		__BIT(0)

/* DMA Enable Register */
#define	DMA_EN_RX_DMA_EN	__BIT(0)
#define	DMA_EN_TX_DMA_EN	__BIT(1)
#define	DMA_EN_TO_CNT_AUTO_RST	__BIT(2)

/* RX Trigger Address */
#define	RXTRI_AD_RXTRIG_MASK	0x0f
#define	RXTRI_AD_RXTRIG(x)	((x) & RXTRI_AD_RXTRIG_MASK)

/* UART Rx Pin Sel */
#define	RX_SEL_USB		__BIT(0)	/* UART0 only */

#endif /* _MEDIATEK_MT6577_UARTREG_H_ */
