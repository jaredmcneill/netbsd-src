/* $NetBSD$ */

/*-
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

#ifndef _DEV_I2C_ALC56XX_H
#define _DEV_I2C_ALC56XX_H

#include <sys/audioio.h>

#define	MX_DEVICE_ID_REG		0x00
#define	 MX_DEVICE_ID			__BITS(2,1)
#define	  MX_DEVICE_ID_ALC5639		0x1
#define	  MX_DEVICE_ID_ALC5640		0x2

#define	MX_HPOUT_REG			0x02
#define	 MX_HPOUT_HPOL_MUTE		__BIT(15)
#define	 MX_HPOUT_HPOVOLL_MUTE		__BIT(14)
#define	 MX_HPOUT_HPOVOLL		__BITS(13,8)
#define	 MX_HPOUT_HPOR_MUTE		__BIT(7)
#define	 MX_HPOUT_HPOVOLR_MUTE		__BIT(6)
#define	 MX_HPOUT_HPOVOLR		__BITS(5,0)
#define	 MX_HPOUT_MUTE					\
	  (MX_HPOUT_HPOL_MUTE|MX_HPOUT_HPOVOLL_MUTE|	\
	   MX_HPOUT_HPOR_MUTE|MX_HPOUT_HPOVOLR_MUTE)

#define	MX_DAC1_DVOL_REG		0x19
#define	 MX_DAC1_LEFT			__BITS(15,8)
#define	 MX_DAC1_RIGHT			__BITS(7,0)
#define	 MX_DAC1_MAX			0xaf

#define	MX_POWER_CTRL1_REG		0x61
#define	 MX_POWER_CTRL1_I2S1		__BIT(15)
#define	 MX_POWER_CTRL1_I2S2		__BIT(14)
#define	 MX_POWER_CTRL1_DACL1		__BIT(12)
#define	 MX_POWER_CTRL1_DACR1		__BIT(11)
#define	 MX_POWER_CTRL1_DACL2		__BIT(7)
#define	 MX_POWER_CTRL1_DACR2		__BIT(6)
#define	 MX_POWER_CTRL1_ADCL		__BIT(2)
#define	 MX_POWER_CTRL1_ADCR		__BIT(1)
#define	 MX_POWER_CTRL1_CLASSD		__BIT(0)

#define	MX_POWER_CTRL3_REG		0x63
#define	 MX_POWER_CTRL3_VREF1		__BIT(15)
#define	 MX_POWER_CTRL3_VREF1_FASTMODE	__BIT(14)
#define	 MX_POWER_CTRL3_MAIN_BIAS	__BIT(13)
#define	 MX_POWER_CTRL3_LOUTMIX		__BIT(12)
#define	 MX_POWER_CTRL3_MBIAS_BG	__BIT(11)
#define	 MX_POWER_CTRL3_MONOMIX		__BIT(10)
#define	 MX_POWER_CTRL3_MONOAMP		__BIT(8)
#define	 MX_POWER_CTRL3_HP_LEFT		__BIT(7)
#define	 MX_POWER_CTRL3_HP_RIGHT	__BIT(6)
#define	 MX_POWER_CTRL3_HP_AMP		__BIT(5)
#define	 MX_POWER_CTRL3_VREF2		__BIT(4)
#define	 MX_POWER_CTRL3_VREF2_FASTMODE	__BIT(3)
#define	 MX_POWER_CTRL3_LDO2		__BIT(2)

#define	MX_PR_INDEX			0x6a
#define	MX_PR_DATA			0x6c

#define	MX_GLOBAL_CLOCK_CTRL_REG	0x80
#define	 MX_GLOBAL_CLOCK_CTRL_MUX	__BITS(15,14)
#define	  MX_GLOBAL_CLOCK_CTRL_MUX_MCLK	0
#define	  MX_GLOBAL_CLOCK_CTRL_MUX_PLL	1
#define	 MX_GLOBAL_CLOCK_CTRL_PLL_SRC	__BITS(13,12)
#define	  MX_GLOBAL_CLOCK_CTRL_PLL_SRC_MCLK	0
#define	  MX_GLOBAL_CLOCK_CTRL_PLL_SRC_BCLK1	1
#define	  MX_GLOBAL_CLOCK_CTRK_PLL_SRC_BCLK2	2
#define	 MX_GLOBAL_CLOCK_CTRL_PLL_PREDIV __BIT(3)

#define	MX_GEN_CTRL1_REG		0xfa
#define	 MX_GEN_CTRL1_MCLK_DET		__BIT(11)
#define	 MX_GEN_CTRL1_EN_IN1_SE		__BIT(9)
#define	 MX_GEN_CTRL1_EN_IN2_SE		__BIT(8)
#define	 MX_GEN_CTRL1_CLK_GATE		__BIT(0)

#define	MX_VENDOR_ID_REG		0xfe
#define	 MX_VENDOR_ID_REALTEK		0x10ec

device_t	alc56xx_lookup(int);
int		alc56xx_start(device_t);
int		alc56xx_query_devinfo(device_t, mixer_devinfo_t *);
int		alc56xx_set_port(device_t, mixer_ctrl_t *);
int		alc56xx_get_port(device_t, mixer_ctrl_t *);

#endif /* !_DEV_I2C_ALC56XX_H */
