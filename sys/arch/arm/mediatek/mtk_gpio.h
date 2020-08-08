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

#ifndef _ARM_MEDIATEK_MTK_GPIO_H_
#define _ARM_MEDIATEK_MTK_GPIO_H_

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <arch/arm/mediatek/mtk_eintc.h>

struct mtk_gpio_softc;

struct mtk_gpio_drive {
	const uint8_t *sel_to_mA;
	size_t nsel;
};

#define	MTK_IES_SMT_BIT_IES	0
#define	MTK_IES_SMT_BIT_SMT	1

struct mtk_ies_smt_group {
	bus_size_t	regs[2];
	uint16_t	first_pin;
	uint16_t	last_pin;
	uint16_t	bits[2];
};

#define	_IES_SMT(_fp, _lp, _ireg, _sreg, _ival, _sval)			\
	{								\
		.first_pin = (_fp),					\
		.last_pin = (_lp),					\
		.regs[MTK_IES_SMT_BIT_IES] = (_ireg),			\
		.regs[MTK_IES_SMT_BIT_SMT] = (_sreg),			\
		.bits[MTK_IES_SMT_BIT_IES] = (_ival),			\
		.bits[MTK_IES_SMT_BIT_SMT] = (_sval),			\
	}

#define	IES_SMT(_fp, _lp, _ireg, _sreg, _bit)				\
	_IES_SMT(_fp, _lp, _ireg, _sreg, __BIT(_bit), __BIT(_bit))

#define	IES(_fp, _lp, _ireg, _bit)					\
	_IES_SMT(_fp, _lp, _ireg, 0, __BIT(_bit), 0)

#define	SMT(_fp, _lp, _sreg, _bit)					\
	_IES_SMT(_fp, _lp, 0, _sreg, 0, __BIT(_bit))

#define	MTK_GPIO_MAXFUNC	8

struct mtk_gpio_pinconf {
	const char *name;
	const char *functions[MTK_GPIO_MAXFUNC];
	struct {
		const struct mtk_gpio_drive *params;
		bus_size_t reg;
		uint16_t sel;
		uint16_t sr;		/* slew rate control bit */
	} drive;
	struct {
		bus_size_t reg;
		uint16_t pupd;		/* 0=pull-up, 1=pull-down bit */
		uint16_t r1;		/* 50K resistor control bit */
		uint16_t r0;		/* 10K resistor control bit */
	} pupdr1r0;
	struct {
		uint8_t eint_flags;
		uint8_t pin_func;	/* this function selects for intrs */
		uint16_t eint_num;	/* EINT interrupt number */
	} eint;
};

#define	_DRIVE(_params, _reg, _sel, _srval)				\
	.drive = {							\
		.params = &mt2701_gpio_drive_ ## _params,		\
		.reg = (_reg),						\
		.sel = (_sel),						\
		.sr = (_srval),						\
	}

#define	DRIVE(_params, _reg, _sel)					\
	_DRIVE(_params, _reg, _sel, 0)

#define	DRIVE_SLEW(_params, _reg, _sel, _sr)				\
	_DRIVE(_params, _reg, _sel, __BIT(_sr))

#define	PUPDR1R0(_reg, _pupd, _r1, _r0)					\
	.pupdr1r0 = {							\
		.reg = (_reg),						\
		.pupd = __BIT(_pupd),					\
		.r1 = __BIT(_r1),					\
		.r0 = __BIT(_r0),					\
	}

#define	MTK_EINT_SOURCE		0x01
#define	MTK_EINT_DEBOUNCE_REQD	0x02

#define	EINT_FLAGS(_func, _num, _flags)					\
	.eint = {							\
		.eint_flags = MTK_EINT_SOURCE | (_flags),		\
		.pin_func = (_func),					\
		.eint_num = (_num),					\
	}

#define	EINT(_func, _num)						\
	EINT_FLAGS(_func, _num, 0)

struct mtk_gpio_reg_group {
	bus_size_t	base;
	void		(*addr_fixup)(bus_size_t *, u_int);
	uint8_t		pins_per_reg;
	uint8_t		bits_per_pin;	/* 1 if not otherwise set */
};

#define	MTK_GPIO_REGS_DIR	0
#define	MTK_GPIO_REGS_PULLEN	1
#define	MTK_GPIO_REGS_PULLSEL	2
#define	MTK_GPIO_REGS_DOUT	3
#define	MTK_GPIO_REGS_DIN	4
#define	MTK_GPIO_REGS_MODE	5
#define	MTK_GPIO_NREGS		6

struct mtk_gpio_padconf {
	const struct mtk_gpio_pinconf * const pins;
	size_t npins;
	const struct mtk_ies_smt_group * const ies_smt_groups;
	size_t nies_smt_groups;
	uint16_t reg_index_shift;
	struct mtk_gpio_reg_group reg_groups[MTK_GPIO_NREGS];
	int (*setfunc_hook)(struct mtk_gpio_softc * const, const u_int,
			    int * const);
	const struct mtk_eintc_config *eintc;
};

struct mtk_gpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	const struct mtk_gpio_padconf *sc_padconf;
	kmutex_t		sc_lock;

	bus_space_handle_t	sc_bsh;

	struct mtk_eintc_softc	sc_eintc;

	struct gpio_chipset_tag	sc_gp;
	gpio_pin_t		*sc_pins;
	device_t		sc_gpiodev;
};

#define	GPIO_READ(sc, reg)		\
	bus_space_read_2((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	GPIO_WRITE(sc, reg, val)	\
	bus_space_write_2((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

/*
 * Device tree bindings:
 *
 * MediaTek device trees use the pinctrl "pinmux" property in their
 * pin configurations to describe pin configurations in a compact
 * way.
 *
 * Furthermore, the bias-pull-up and bias-pull-down properies will
 * specify a resistor configuration for the pins that have those
 * configuration options.
 */
#define	MTK_PINMUX_PIN(x)	((x) >> 8)
#define	MTK_PINMUX_FUNC(x)	((x) & 0xf)

#define	MTK_BIAS_R1R0_00	100
#define	MTK_BIAS_R1R0_01	101
#define	MTK_BIAS_R1R0_10	102
#define	MTK_BIAS_R1R0_11	103

#endif /* _ARM_MEDIATEK_MTK_GPIO_H_ */
