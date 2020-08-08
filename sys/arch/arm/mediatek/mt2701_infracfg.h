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

#ifndef _MEDIATEK_MT2701_INFRACFG_H_
#define	_MEDIATEK_MT2701_INFRACFG_H_

/*
 * Reset IDs - must match DT bindings.
 *
 * These also happen to map to the INFRACFG reset register bits.
 */
#define	MT2701_RST_INFRA_EMI_REG_RST		0
#define	MT2701_RST_INFRA_DRAMC0_A0_RST		1
#define	MT2701_RST_INFRA_FHCTL_RST		2
#define	MT2701_RST_INFRA_APCIRQ_EINT_RST	3
#define	MT2701_RST_INFRA_APXGPT_RST		4
#define	MT2701_RST_INFRA_SCPSYS_RST		5
#define	MT2701_RST_INFRA_KP_RST			6
#define	MT2701_RST_INFRA_PMIC_WRAP_RST		7
#define	MT2701_RST_INFRA_MIPI_RST		8
#define	MT2701_RST_INFRA_IRRX_RST		9
#define	MT2701_RST_INFRA_CEC_RST		10
#define	MT2701_RST_INFRA_EMI_RST		32
#define	MT2701_RST_INFRA_DRAMC0_RST		34
#define	MT2701_RST_INFRA_TRNG_RST		37
#define	MT2701_RST_INFRA_SYSIRQ_RST		38

/*
 * Clock IDs - must match DT bindings.
 */
#define	MT2701_CLK_INFRA_DBG			1
#define	MT2701_CLK_INFRA_SMI			2
#define	MT2701_CLK_INFRA_QAXI_CM4		3
#define	MT2701_CLK_INFRA_AUD_SPLIN_B		4
#define	MT2701_CLK_INFRA_AUDIO			5
#define	MT2701_CLK_INFRA_EFUSE			6
#define	MT2701_CLK_INFRA_L2C_SRAM		7
#define	MT2701_CLK_INFRA_M4U			8
#define	MT2701_CLK_INFRA_CONNMCU		9
#define	MT2701_CLK_INFRA_TRNG			10
#define	MT2701_CLK_INFRA_RAMBUFIF		11
#define	MT2701_CLK_INFRA_CPUM			12
#define	MT2701_CLK_INFRA_KP			13
#define	MT2701_CLK_INFRA_CEC			14
#define	MT2701_CLK_INFRA_IRRX			15
#define	MT2701_CLK_INFRA_PMICSPI		16
#define	MT2701_CLK_INFRA_PMICWRAP		17
#define	MT2701_CLK_INFRA_DDCCI			18
#define	MT2701_CLK_INFRA_CLK_13M		19
#define	MT2701_CLK_INFRA_CPUSEL			20
#define	MT2701_CLK_INFRA_NR			21

#endif /* _MEDIATEK_MT2701_INFRACFG_H_ */
