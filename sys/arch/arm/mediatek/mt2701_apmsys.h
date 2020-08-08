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

#ifndef _MEDIATEK_MT2701_APMSYS_H_
#define	_MEDIATEK_MT2701_APMSYS_H_

/*
 * Clock IDs - must match DT bindings.
 */
#define	MT2701_CLK_APMIXED_ARMPLL		1
#define	MT2701_CLK_APMIXED_MAINPLL		2
#define	MT2701_CLK_APMIXED_UNIVPLL		3
#define	MT2701_CLK_APMIXED_MMPLL		4
#define	MT2701_CLK_APMIXED_MSDCPLL		5
#define	MT2701_CLK_APMIXED_TVDPLL		6
#define	MT2701_CLK_APMIXED_AUD1PLL		7
#define	MT2701_CLK_APMIXED_TRGPLL		8
#define	MT2701_CLK_APMIXED_ETHPLL		9
#define	MT2701_CLK_APMIXED_VDECPLL		10
#define	MT2701_CLK_APMIXED_HADDS2PLL		11
#define	MT2701_CLK_APMIXED_AUD2PLL		12
#define	MT2701_CLK_APMIXED_TVD2PLL		13
#define	MT2701_CLK_APMIXED_HDMI_REF		14

#endif /* _MEDIATEK_MT2701_APMSYS_H_ */
