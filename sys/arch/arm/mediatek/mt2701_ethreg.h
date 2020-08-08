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

#ifndef _MEDIATEK_MT2701_ETHREG_H_
#define	_MEDIATEK_MT2701_ETHREG_H_

/* RX descriptor; must be 16-byte aligned */
struct fe_rx_desc {
	uint32_t	rxd1;		/* DMA address */
	uint32_t	rxd2;
	uint32_t	rxd3;
	uint32_t	rxd4;
};

#define	RXD2_PLEN0			__BITS(16,29)
#define	RXD2_DDONE			__BIT(31)

#define	RXD3_VID			__BITS(0,11)

#define	RXD4_FPORT			__BITS(19,21)
#define	RXD4_L4_VLD			__BIT(24)

/* TX descriptor; must be 16-byte aligned */
struct fe_tx_desc {
	uint32_t	txd1;		/* DMA address */
	uint32_t	txd2;
	uint32_t	txd3;
	uint32_t	txd4;
};

#define	TXD3_SWC			__BIT(14)
#define	TXD3_PLEN0			__BITS(16,29)
#define	TXD3_LS0			__BIT(30)
#define	TXD3_OWNER_CPU			__BIT(31)

#define	TXD4_INS_VLAN			__BIT(16)
#define	TXD4_FPORT			__BITS(25,27)
#define	TXD4_TSO			__BIT(28)
#define	TXD4_TCP_EN			__BIT(29)
#define	TXD4_UDP_EN			__BIT(30)
#define	TXD4_IP_EN			__BIT(31)

/* Relative to Frame Engine Base address */
#define	FE_FE_FLO_CFG			0x0000
#define	 FE_FLO_CFG_INF_SPACE		__BITS(0,3)
#define	 FE_FLO_CFG_L2_SPACE		__BITS(4,7)
#define	 FE_FLO_CFG_LINK_DWN		__BITS(8,15)
#define	 FE_FLO_CFG_EXT_TPID		__BITS(16,31)

#define	FE_FE_RST_GLO			0x0004
#define	 FE_RST_GLO_PSE_RESET		__BIT(0)
#define	 FE_RST_GLO_PSE_MEM_EN		__BIT(1)
#define	 FE_RST_GLO_PSE_RAM		__BIT(2)

#define	FE_FE_INT_STATUS		0x0008
#define	FE_FE_INT_ENABLE		0x000c
#define	 FE_INT_PSE_FC_ON		__BITS(0,7)
#define	 FE_INT_FQ_EMPTY		__BIT(8)
#define	 FE_INT_PSE_DROP		__BIT(9)
#define	 FE_INT_CDM_TSO_FAIL		__BIT(12)
#define	 FE_INT_CDM_TSO_ILLEGAL		__BIT(13)
#define	 FE_INT_CDM_TSO_ALIGN		__BIT(14)
#define	 FE_INT_AFIFO_GET_ERR		__BIT(16)
#define	 FE_INT_INFIFO_GET_ERR		__BIT(17)
#define	 FE_INT_RFIFO_OV		__BIT(18)
#define	 FE_INT_RFIFO_UF		__BIT(19)
#define	 FE_INT_GDM1_CRC		__BIT(20)
#define	 FE_INT_GDM1_ERR		__BIT(21)
#define	 FE_INT_GDM2_CRC		__BIT(22)
#define	 FE_INT_GDM2_ERR		__BIT(23)
#define	 FE_INT_MAC1_LINK		__BIT(24)
#define	 FE_INT_MAC2_LINK		__BIT(25)
#define	 FE_INT_GDM1_AF			__BIT(28)
#define	 FE_INT_GDM2_AF			__BIT(29)
#define	 FE_INT_PPE_AF			__BIT(31)

#define	FE_FE_FOE_TS_T			0x0010
#define	 FE_FOE_TS_T			__BITS(0,15)

#define	FE_FE_IPV6_EXT			0x0014
#define	 FE_IPV6_EXT_IP6_EXT0		__BITS(0,7)
#define	 FE_IPV6_EXT_IP6_EXT1		__BITS(8,15)
#define	 FE_IPV6_EXT_IP6_EXT2		__BITS(16,23)
#define	 FE_IPV6_EXT_IP6_EXT3		__BITS(24,31)

#define	FE_FE_RATE_COMP			0x0018
#define	 FE_RATE_COMP_PSE_RATE_BYTE	__BITS(0,6)
#define	 FE_RATE_COMP_PSE_RATE_MINUS	__BIT(7)
#define	 FE_RATE_COMP_DMA_RATE_BYTE	__BITS(8,14)
#define	 FE_RATE_COMP_DMA_RATE_MINUS	__BIT(15)

#define	FE_FE_INT_GRP			0x0020
#define	 FE_INT_GRP_FE_MISC_INT_ASG	__BITS(0,3)
#define	 FE_INT_GRP_PDMA_INT_G0_ASG	__BITS(8,11)
#define	 FE_INT_GRP_PDMA_INT_G1_ASG	__BITS(12,15)
#define	 FE_INT_GRP_PDMA_INT_G2_ASG	__BITS(16,19)
#define	 FE_INT_GRP_QDMA_INT_G0_ASG	__BITS(20,23)
#define	 FE_INT_GRP_QDMA_INT_G1_ASG	__BITS(24,27)
#define	 FE_INT_GRP_QDMA_INT_G2_ASG	__BITS(28,31)

#define	FE_PSE_FQFC_CFG			0x0100
#define	 PSE_FQFC_CFG_PPE_LTH		__BITS(0,7)
#define	 PSE_FQFC_CFG_PPQ_VIQ		__BITS(8,15)
#define	 PSE_FQFC_CFG_FQ_MAX_PCNT	__BITS(16,23)
#define	 PSE_FQFC_CFG_FQ_PCNT		__BITS(24,31)

#define	FE_PSE_IQ_REV1			0x0108
#define	 PSE_IQ_REV1_P0_IQ_RES		__BITS(0,7)
#define	 PSE_IQ_REV1_P1_IQ_RES		__BITS(8,15)
#define	 PSE_IQ_REV1_P2_IQ_RES		__BITS(16,23)

#define	FE_PSE_IQ_REV2			0x010c
#define	 PSE_IQ_REV2_PPTE_HTH		__BITS(0,7)
#define	 PSE_IQ_REV2_P5_IQ_RES		__BITS(8,15)
#define	 PSE_IQ_REV2_P6_IQ_RES		__BITS(16,23)
#define	 PSE_IQ_REV2_FREE_DROP		__BITS(24,31)

#define	FE_PSE_IQ_STA1			0x0110
#define	 PSE_IQ_STA1_P0_IQ_PCNT		__BITS(0,7)
#define	 PSE_IQ_STA1_P1_IQ_PCNT		__BITS(8,15)
#define	 PSE_IQ_STA1_P2_IQ_PCNT		__BITS(16,23)

#define	FE_PSE_IQ_STA2			0x0114
#define	 PSE_IQ_STA2_P5_IQ_PCNT		__BITS(8,15)
#define	 PSE_IQ_STA2_P6_IQ_PCNT		__BITS(16,23)

#define	FE_PSE_OQ_STA1			0x0118
#define	 PSE_OQ_STA1_P0_OQ_PCNT		__BITS(0,7)
#define	 PSE_OQ_STA1_P1_OQ_PCNT		__BITS(8,15)
#define	 PSE_OQ_STA1_P2_OQ_PCNT		__BITS(16,23)

#define	FE_PSE_OQ_STA2			0x011c
#define	 PSE_OQ_STA2_PPE_OQ_PCNT	__BITS(0,7)
#define	 PSE_OQ_STA2_P5_IQ_PCNT		__BITS(8,15)

#define	FE_PSE_MIR_PORT			0x0120
#define	 PSE_MIR_PORT_P0_MIR_PX		__BITS(0,3)
#define	 PSE_MIR_PORT_P1_MIR_PX		__BITS(4,7)
#define	 PSE_MIR_PORT_P2_MIR_PX		__BITS(8,11)
#define	 PSE_MIR_PORT_P5_MIR_PX		__BITS(20,23)
#define	  PX_PDMA			0	/* PX_* used in		*/
#define	  PX_GDM1			1	/* other registers	*/
#define	  PX_GDM2			2	/* below		*/
#define	  PX_PPE			4
#define	  PX_QDMA			5
#define	  PX_DISCARD			7

#define	FE_FE_GDM_RXID1			0x0130
#define	 FE_GDM_RXID1_GDM_RXID_PRI_SEL	__BITS(0,2)
#define	  RXID_PRI_PID_STAG		0
#define	  RXID_PRI_VLAN_PRI_PID_STAG	1
#define	  RXID_PRI_TCP_ACK_PID_STAG	2
#define	  RXID_PRI_VLAN_PRI_TCP_ACK_PID_STAG 3
#define	  RXID_PRI_TCP_ACK_VLAN_PRI_PID_STAG 4
#define	 FE_GDM_RXID1_GDM_TCP_ACK_WZPC	__BIT(3)
#define	 FE_GDM_RXID1_GDM_TCP_ACK_RXID_SEL __BITS(4,5)
#define	 FE_GDM_RXID1_GDM_VLAN_PRI0_RXID_SEL __BITS(16,17)
#define	 FE_GDM_RXID1_GDM_VLAN_PRI1_PXID_SEL __BITS(18.19)
#define	 FE_GDM_RXID1_GDM_VLAN_PRI2_RXID_SEL __BITS(20,21)
#define	 FE_GDM_RXID1_GDM_VLAN_PRI3_RXID_SEL __BITS(22,23)
#define	 FE_GDM_RXID1_GDM_VLAN_PRI4_RXID_SEL __BITS(24,25)
#define	 FE_GDM_RXID1_GDM_VLAN_PRI5_RXID_SEL __BITS(26,27)
#define	 FE_GDM_RXID1_GDM_VLAN_PRI6_RXID_SEL __BITS(28,29)
#define	 FE_GDM_RXID1_GDM_VLAN_PRI7_RXID_SEL __BITS(30,31)

#define	FE_FE_GDM_RXID2			0x0134
#define	 FE_GDM_RXID2_GDM_PID1_RXID_SEL	__BITS(0,1)
#define	 FE_GDM_RXID2_GDM_PID2_RXID_SEL	__BITS(2,3)
#define	 FE_GDM_RXID2_GDM_STAG0_RXID_SEL __BITS(16,17)
#define	 FE_GDM_RXID2_GDM_STAG1_RXID_SEL __BITS(18,19)
#define	 FE_GDM_RXID2_GDM_STAG2_RXID_SEL __BITS(20,21)
#define	 FE_GDM_RXID2_GDM_STAG3_RXID_SEL __BITS(22,23)
#define	 FE_GDM_RXID2_GDM_STAG4_RXID_SEL __BITS(24,25)
#define	 FE_GDM_RXID2_GDM_STAG5_RXID_SEL __BITS(26,27)
#define	 FE_GDM_RXID2_GDM_STAG6_RXID_SEL __BITS(28,29)
#define	 FE_GDM_RXID2_GDM_STAG7_RXID_SEL __BITS(30,31)

#define	FE_CDMP_IG_CTRL			0x0400
#define	 CDMP_IG_CTRL_STAG_EN		__BIT(0)
#define	 CDMP_IG_CTRL_CDM_TPID		__BITS(16,31)

#define	FE_CDMP_EG_CTRL			0x0404
#define	 CDMP_EG_CTRL_UNTAG_EN

#define	FE_CDMP_PPE_GEN			0x0408
#define	 CDMP_PPE_GEN_SESS_ID		__BITS(0,15)
#define	 CDMP_PPE_GEN_PPP_INS		__BIT(16)

#define	FE_GDM1_IG_CTRL			0x0500
#define	 GDM_IG_CTRL_UN_DP		__BITS(0,3)	/* PX_* */
#define	 GDM_IG_CTRL_MC_DP		__BITS(4,7)	/* PX_* */
#define	 GDM_IG_CTRL_BC_DP		__BITS(8,11)	/* PX_* */
#define	 GDM_IG_CTRL_MY_MAC		__BITS(12,15)	/* PX_* */
#define	 GDM_IG_CTRL_STRP_CRC		__BIT(16)
#define	 GDM_IG_CTRL_DROP_256B		__BIT(19)
#define	 GDM_IG_CTRL_GDM_UCS_EN		__BIT(20)
#define	 GDM_IG_CTRL_GDM_TCS_EN		__BIT(21)
#define	 GDM_IG_CTRL_GDM_ICS_EN		__BIT(22)
#define	 GDM_IG_CTRL_STAG_EN		__BIT(24)
#define	 GDM_IG_CTRL_INSV_EN		__BIT(25)

#define	FE_GDM1_EG_CTRL			0x0504
#define	 GDM_EG_CTRL_TK_RATE		__BITS(0,13)
#define	 GDM_EG_CTRL_TK_TICK		__BIT(15)
#define	 GDM_EG_CTRL_BK_SIZE		__BITS(16,23)
#define	 GDM_EG_CTRL_SHPR_EN		__BIT(24)
#define	 GDM_EG_CTRL_DIS_CRC		__BIT(28)
#define	 GDM_EG_CTRL_DIS_PAD		__BIT(29)
#define	 GDM_EG_CTRL_UNTAG_EN		__BIT(30)

#define	FE_GDM1_MAC_LSB			0x0508

#define	FE_GDM1_MAC_MSB			0x050c

#define	FE_GDM1_VLAN_GEN		0x0510
#define	 GDM_VLAN_GEN_GDM_VID		__BITS(0,11)
#define	 GDM_VLAN_GEN_GDM_CFI		__BIT(12)
#define	 GDM_VLAN_GEN_GDM_PRI		__BITS(13,15)
#define	 GDM_VLAN_GEN_GDM_TPID		__BITS(16,31)

#define	FE_TX_BASE_PTR_0		0x0800

#define	FE_TX_MAX_CNT_0			0x0804
#define	 TX_MAX_CNT			__BITS(0,11)

#define	FE_TX_CTX_IDX_0			0x0808
#define	 TX_CTX_IDX			__BITS(0,11)

#define	FE_TX_DTX_IDX_0			0x080c
#define	 TX_DTX_IDX			__BITS(0,11)

#define	FE_TX_BASE_PTR_1		0x0810

#define	FE_TX_MAX_CNT_1			0x0814

#define	FE_TX_CTX_IDX_1			0x0818

#define	FE_TX_DTX_IDX_1			0x081c

#define	FE_TX_BASE_PTR_2		0x0820

#define	FE_TX_MAX_CNT_2			0x0824

#define	FE_TX_CTX_IDX_2			0x0828

#define	FE_TX_DTX_IDX_2			0x082c

#define	FE_TX_BASE_PTR_3		0x0830

#define	FE_TX_MAX_CNT_3			0x0834

#define	FE_TX_CTX_IDX_3			0x0838

#define	FE_TX_DTX_IDX_3			0x083c

#define	FE_RX_BASE_PTR_0		0x0900

#define	FE_RX_MAX_CNT_0			0x0904
#define	 RX_MAX_CNT			__BITS(0,11)

#define	FE_RX_CRX_IDX_0			0x0908
#define	 RX_CRX_IDX			__BITS(0,11)

#define	FE_RX_DRX_IDX_0			0x090c
#define	 RX_DRX_IDX			__BITS(0,11)

#define	FE_RX_BASE_PTR_1		0x0910

#define	FE_RX_MAX_CNT_1			0x0914

#define	FE_RX_CRX_IDX_1			0x0918

#define	FE_RX_DRX_IDX_1			0x091c

#define	FE_RX_BASE_PTR_2		0x0920

#define	FE_RX_MAX_CNT_2			0x0924

#define	FE_RX_CRX_IDX_2			0x0928

#define	FE_RX_DRX_IDX_2			0x092c

#define	FE_RX_BASE_PTR_3		0x0930

#define	FE_RX_MAX_CNT_3			0x0934

#define	FE_RX_CRX_IDX_3			0x0938

#define	FE_RX_DRX_IDX_3			0x093c

#define	FE_PDMA_INFO			0x0a00
#define	 PDMA_INFO_TX_RING_NUM		__BITS(0,7)
#define	 PDMA_INFO_TX_RING_NUM		__BITS(8,15)
#define	 PDMA_INFO_BASE_PTR_WIDTH	__BITS(16,23)
#define	 PDMA_INFO_INDEX_WIDTH		__BITS(24,27)

#define	FE_PDMA_GLO_CFG			0x0a04
#define	 PDMA_GLO_CFG_TX_DMA_EN		__BIT(0)
#define	 PDMA_GLO_CFG_TC_DMA_BUSY	__BIT(1)
#define	 PDMA_GLO_CFG_RX_DMA_EN		__BIT(2)
#define	 PDMA_GLO_CFG_RX_DMA_BUSY	__BIT(3)
#define	 PDMA_GLO_CFG_PDMA_BT_SIZE	__BITS(4,5)
#define	 PDMA_BT_SIZE_16B		0
#define	 PDMA_BT_SIZE_32B		1
#define	 PDMA_BT_SIZE_64B		2
#define	 PDMA_BT_SIZE_128B		3
#define	 PDMA_GLO_CFG_TX_WB_DDONE	__BIT(6)
#define	 PDMA_GLO_CFG_BIG_ENDIAN	__BIT(7)
#define	 PDMA_GLO_CFG_DESC_32B_EN	__BIT(8)
#define	 PDMA_GLO_CFG_EXT_FIFO_EN	__BIT(9)
#define	 PDMA_GLO_CFG_MULTI_EN		__BIT(10)
#define	 PDMA_GLO_CFG_ADMA_RX_BT_SIZE	__BITS(11,12)
#define	 PDMA_GLO_CFG_BYTE_SWAP		__BIT(29)
#define	 PDMA_GLO_CFG_CSR_CLKGATE_BYP	__BIT(30)
#define	 PDMA_GLO_CFG_RTX_2B_OFFSET	__BIT(31)

#define	FE_PDMA_RST_IDX			0x0a08
#define	 PDMA_RST_DTX_IDX0		__BIT(0)
#define	 PDMA_RST_DTX_IDX1		__BIT(1)
#define	 PDMA_RST_DTX_IDX2		__BIT(2)
#define	 PDMA_RST_DTX_IDX3		__BIT(3)
#define	 PDMA_RST_DRX_IDX0		__BIT(16)
#define	 PDMA_RST_DRX_IDX1		__BIT(17)

#define	FE_DELAY_INT_CFG		0x0a0c
#define	 DELAY_INT_CFG_RXMAX_PTIME	__BITS(0,7)
#define	 DELAY_INT_CFG_RXMAX_PINT	__BITS(8,14)
#define	 DELAY_INT_CFG_RXDLY_INT_EN	__BIT(15)
#define	 DELAY_INT_CFG_TXMAX_PTIME	__BITS(16,23)
#define	 DELAY_INT_CFG_TXMAX_PINT	__BITS(24,30)
#define	 DELAY_INT_CFG_TXDLY_INT_EN	__BIT(31)

#define	FE_FREEQ_THRES			0x0a10
#define	 FREEQ_THRES			__BITS(0,3)

#define	FE_INT_STATUS			0x0a20
#define	 INT_TX_DONE_INT0		__BIT(0)
#define	 INT_TX_DOME_INT1		__BIT(1)
#define	 INT_TX_DONE_INT2		__BIT(2)
#define	 INT_TX_DONE_INT3		__BIT(3)
#define	 INT_RX_DONE_INT0		__BIT(16)
#define	 INT_RX_DONE_INT1		__BIT(17)
#define	 INT_RX_DONE_INT2		__BIT(18)
#define	 INT_RX_DONE_INT3		__BIT(19)
#define	 INT_ALT_RPLC_INT1		__BIT(21)
#define	 INT_ALT_RPLC_INT2		__BIT(22)
#define	 INT_ALT_RPLC_INT3		__BIT(23)
#define	 INT_RDX_ERROR			__BIT(24)
#define	 INT_RING1_RX_DLY_INT		__BIT(25)
#define	 INT_RING2_RX_DLY_INT		__BIT(26)
#define	 INT_RING2_RX_DLY_INT		__BIT(27)
#define	 INT_TX_DLY_INT			__BIT(28)
#define	 INT_TX_COHERENT		__BIT(29)
#define	 INT_RX_DLY_INT			__BIT(30)
#define	 INT_RX_COHERENT		__BIT(31)

#define	FE_INT_MASK			0x0a28
#define	 INT_MASK_VALID			(INT_TX_DONE_INT0 | \
					 INT_TX_DOME_INT1 | \
					 INT_TX_DONE_INT2 | \
					 INT_TX_DONE_INT3 | \
					 INT_RX_DONE_INT0 | \
					 INT_RX_DONE_INT1 | \
					 INT_TX_DLY_INT   | \
					 INT_TX_COHERENT  | \
					 INT_RX_DLY_INT   | \
					 INT_RX_COHERENT)

#define	FE_PDMA_INT1_VEC_GRP0		0x0a40

#define	FE_PDMA_INT1_VEC_GRP1		0x0a44

#define	FE_PDMA_INT1_VEC_GRP2		0x0a48

#define	FE_PDMA_INT_GRP1		0x0a50

#define	FE_PDMA_INT_GRP2		0x0a54

#define	FE_CDMQ_IG_CTRL			0x1400
#define	 CDMQ_IG_CTRL_STAG_EN		__BIT(0)
#define	 CDMQ_IG_CTRL_UNTAG_EN		__BIT(1)
#define	 CDMQ_IG_CTRL_CDM_TPID		__BITS(16,31)

#define	FE_CDMQ_EG_CTRL			0x1404
#define	 CDMQ_EG_CTRL_UNTAG_EN		__BIT(0)

#define	FE_CDMQ_PPP_GEN			0x1408
#define	 CDMQ_PPP_GEN_SESS_ID		__BITS(0,15)
#define	 CDMQ_PPP_GEN_PPP_INS		__BIT(16)

#define	FE_GDM2_IG_CTRL			0x1500
	/* See FE_GDM1_IG_CTRL */

#define	FE_GDM2_EG_CTRL			0x1504
	/* See FE_GDM1_EG_CTRL */

#define	FE_GDM2_MAC_LSB			0x1508

#define	FE_GDM2_MAC_MSB			0x150c

#define	FE_GDM2_VLAN_GEN		0x1510
	/* See FE_GDM1_VLAN_GEN */

#define	FE_GDM2_FILTER_CRTL		0x1514
#define	 GDM2_FILTER_CRTL_GDM_DAF_MODE	__BITS(0,1)
#define	 DAF_MODE_PROMISC		0
#define	 DAF_MODE_MAC_BROAD_HASH	1
#define	 DAF_MODE_MAC_BROAD		2
	/*
	 * 0 = direct-map: DA40 and DA7-0 -> hash key
	 * 1 = CRC32 of DA8-0 -> hash key
	 */
#define	 GDM2_FILTER_CRTL_GDM_HASH_ALG	__BIT(2)
#define	 GDM2_FILTER_CRTL_GDM_VDIF_EN	__BIT(3)

#define	FE_GDM2_VIDF01			0x1518
#define	 GDM2_VIDF01_VID0		__BITS(0,11)
#define	 GDM2_VIDF01_VID0_VLD		__BIT(15)
#define	 GDM2_VIDF01_VID1		__BITS(16,27)
#define	 GDM2_VIDF01_VID1_VLD		__BIT(31)

#define	FE_GDM2_VIDF23			0x151c
#define	 GDM2_VIDF23_VID3		__BITS(0,11)
#define	 GDM2_VIDF23_VID3_VLD		__BIT(15)
#define	 GDM2_VIDF23_VID2		__BITS(16,27)
#define	 GDM2_VIDF23_VID2_VLD		__BIT(31)

/* MIB counters */
	/* 0..63 */
#define	_MIBx_REG(b, x)			((b) + ((x) * 0x10))
#define	FE_PPE_AC_BCNT(x)		_MIBx_REG(0x2000, (x))	/* 64-bit */
#define	FE_PPE_AC_PCNT(x)		_MIBx_REG(0x2008, (x))
#define	FE_PPE_MTR_CNT(x)		_MIBx_REG(0x200c, (x))
#undef _MIBx_REG

	/* 1..2 */
#define	_MIBx_REG(b, x)			((b) + (((x) - 1) * 0x40))
#define	FE_GDM_RX_GBCNT(x)		_MIBx_REG(0x2400, (x))	/* 64-bit */
#define	FE_GDM_RX_GPCNT(x)		_MIBx_REG(0x2408, (x))
#define	FE_GDM_RX_OERCNT(x)		_MIBx_REG(0x2410, (x))
#define	FE_GDM_RX_FERCNT(x)		_MIBx_REG(0x2414, (x))
#define	FE_GDM_RX_SERCNT(x)		_MIBx_REG(0x2418, (x))
#define	FE_GDM_RX_LERCNT(x)		_MIBx_REG(0x241c, (x))
#define	FE_GDM_RX_CERCNT(x)		_MIBx_REG(0x2420, (x))
#define	FE_GDM_RX_FCCNT(x)		_MIBx_REG(0x2424, (x))
#define	FE_GDM_TX_SKIPCNT(x)		_MIBx_REG(0x2428, (x))
#define	FE_GDM_TX_COLCNT(x)		_MIBx_REG(0x242c, (x))
#define	FE_GDM_TX_GBCNT(x)		_MIBx_REG(0x2430, (x))	/* 64-bit */
#define	FE_GDM_TX_GPCNT(x)		_MIBx_REG(0x2438, (x))
#undef _MIBx_REG

/* GMAC registers */
#define	GMAC_PIAC			0x10004
#define	 PIAC_MDIO_RW_DATA		__BITS(0,15)
#define	 PIAC_NMDIO_ST			__BITS(16,17)
#define	  NMDIO_ST_CLAUSE_45		0
#define	  NMDIO_ST_CLAUSE_22		1
#define	 PIAC_MDIO_CMD			__BITS(18,19)
#define	  MDIO_CMD_ADDR_CLAUSE_45	0
#define	  MDIO_CMD_WRITE		1
#define	  MDIO_CMD_READ_CLAUSE_22	2
#define	  MDIO_CMD_READ_INC_CLAUSE_45	2
#define	  MDIO_CMD_READ_CLAUSE_45	3
#define	 PIAC_MDIO_PHY_ADDR		__BITS(20,24)
#define	 PIAC_MDIO_REG_ADDR		__BITS(25,29)
#define	 PIAC_PHY_ACS_ST		__BIT(31)

#endif /* _MEDIATEK_MT2701_ETHREG_H_ */
