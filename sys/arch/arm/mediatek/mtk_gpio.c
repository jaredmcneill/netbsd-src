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

#include "opt_soc.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/gpio.h>
#include <sys/bitops.h>
#include <sys/lwp.h>

#include <dev/fdt/fdtvar.h>
#include <dev/gpio/gpiovar.h>

#include <arm/mediatek/mtk_gpio.h>

#ifdef SOC_MT7623
/*
 * The MT7623 GPIO block is compatible with the MT2701 GPIO block, with
 * just tweaks to the banking configuration of a few pins that we are
 * largely insulated from because of how the "pinmux" bindings for these
 * SoCs work.  (We would have to care about these differences if we were
 * selecting pin functions by name.)
 */
extern struct mtk_gpio_padconf mt2701_gpio_padconf;
#endif

static const struct of_compat_data compat_data[] = {
#ifdef SOC_MT7623
	/*
	 * Device trees, however, don't list both mediatek,mt2701-pinctrl
	 * and mediatek,mt7623-pinctrl as "compatible" properties, so
	 * we need to match the MT7623 explcitly.
	 */
	{ "mediatek,mt7623-pinctrl",	(uintptr_t)&mt2701_gpio_padconf },
#endif
	{ NULL }
};

struct mtk_gpio_pin {
	struct mtk_gpio_softc	*pin_sc;
	const struct mtk_gpio_pinconf *pin_def;
	int			pin_flags;
	bool			pin_actlo;
};

static int	mtk_gpio_match(device_t, cfdata_t, void *);
static void	mtk_gpio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mtk_gpio, sizeof(struct mtk_gpio_softc),
	mtk_gpio_match, mtk_gpio_attach, NULL, NULL);

static const struct mtk_gpio_pinconf *
mtk_gpio_lookup(const struct mtk_gpio_softc *sc, const u_int pin)
{

	if (pin >= sc->sc_padconf->npins)
		return NULL;

	const struct mtk_gpio_pinconf * const pin_def =
	    &sc->sc_padconf->pins[pin];
	if (pin_def->name == NULL || pin_def->functions[0] == NULL)
		return NULL;

	return (pin_def);
}

#if 0
static const struct mtk_gpio_pinconf *
mtk_gpio_lookup_byname(struct mtk_gpio_softc * const sc,
		       const char * const name)
{
	const struct mtk_gpio_pinconf *pin_def;
	u_int n;

	for (n = 0; n < sc->sc_padconf->npins; n++) {
		pin_def = &sc->sc_padconf->pins[n];
		/* XXXJRT case-insensitive comparison? */
		if (strcmp(pin_def->name, name) == 0)
			return pin_def;
	}

	return NULL;
}
#endif

#if 0
static int
mtk_gpio_sel_to_mA(const struct mtk_gpio_drive * const drive, u_int sel,
		   uint8_t * const mAp)
{

	if (sel > drive->nsel)
		return EINVAL;
	if (drive->sel_to_mA[sel] == 0)
		return EINVAL;
	if (mAp)
		*mAp = drive->sel_to_mA[sel];
	return 0;
}
#endif

static int
mtk_gpio_mA_to_sel(const struct mtk_gpio_drive * const drive, uint8_t mA,
		   u_int * const selp)
{
	u_int sel;

	if (mA == 0)
		return EINVAL;

	for (sel = 0; sel < drive->nsel; sel++) {
		if (drive->sel_to_mA[sel] == mA) {
			if (selp)
				*selp = sel;
			return 0;
		}
	}
	return EINVAL;
}

static inline u_int
mtk_gpio_pinconf_to_pin(const struct mtk_gpio_softc * const sc,
			const struct mtk_gpio_pinconf * const pin_def)
{
	return (u_int)((uintptr_t)(pin_def - sc->sc_padconf->pins));
}

static const struct mtk_ies_smt_group *
mtk_gpio_ies_smt_for_pin(struct mtk_gpio_softc * const sc,
			 const struct mtk_gpio_pinconf * const pin_def,
			 const u_int which)
{
	const struct mtk_ies_smt_group *group;
	const u_int pin = mtk_gpio_pinconf_to_pin(sc, pin_def);
	u_int i;

	KASSERT(which == MTK_IES_SMT_BIT_IES || which == MTK_IES_SMT_BIT_SMT);

	for (i = 0; i < sc->sc_padconf->nies_smt_groups; i++) {
		group = &sc->sc_padconf->ies_smt_groups[i];

		if (pin >= group->first_pin && pin <= group->last_pin &&
		    group->bits[which] != 0) {
			return group;
		}
	}
	return NULL;
}

static int
mtk_gpio_reg_for_pin(struct mtk_gpio_softc * const sc,
		     const struct mtk_gpio_pinconf * const pin_def,
		     const u_int which, bus_size_t * const regp,
		     u_int * const shiftp)
{
	const struct mtk_gpio_reg_group *group;
	u_int pin, idx, off;

	KASSERT(which < MTK_GPIO_NREGS);
	group = &sc->sc_padconf->reg_groups[which];
	if (group->pins_per_reg == 0)
		return ENOTSUP;

	pin = mtk_gpio_pinconf_to_pin(sc, pin_def);
	idx = pin / group->pins_per_reg;
	off = pin % group->pins_per_reg;

	if (group->bits_per_pin)
		off *= group->bits_per_pin;

	if (regp) {
		*regp = group->base + (idx << sc->sc_padconf->reg_index_shift);
		if (group->addr_fixup)
			(*group->addr_fixup)(regp, pin);
	}
	if (shiftp)
		*shiftp = off;

	return 0;
}

static int
mtk_gpio_setfunc(struct mtk_gpio_softc * const sc,
		 const struct mtk_gpio_pinconf * const pin_def,
		 const char *func, int func_num)
{
	bus_size_t mode_reg;
	u_int mode_shift;
	int error;
	uint16_t reg;

	KASSERT(mutex_owned(&sc->sc_lock));

	if ((error = mtk_gpio_reg_for_pin(sc, pin_def, MTK_GPIO_REGS_MODE,
					  &mode_reg, &mode_shift)) != 0) {
		return error;
	}

	const uint16_t mode_mask = (7U << mode_shift);

	/*
	 * If the function is specified by name, look it up.
	 * If the name isn't specified, then we use the provided
	 * function number instead.
	 */
	if (func) {
		func_num = -1;
		for (int idx = 0; idx < MTK_GPIO_MAXFUNC; idx++) {
			if (pin_def->functions[idx] == NULL)
				continue;
			if (strcmp(pin_def->functions[idx], func) == 0) {
				func_num = idx;
				break;
			}
		}
	}

	/*
	 * Now that we have the function selector, perform any special
	 * handling this pin may require.  NOTE: This may change the
	 * function selector!
	 */
	if (sc->sc_padconf->setfunc_hook != NULL &&
	    (error = (*sc->sc_padconf->setfunc_hook)(sc,
	    		mtk_gpio_pinconf_to_pin(sc, pin_def), &func_num))) {
		return error;
	}
	if (func_num >= 0 && func_num < MTK_GPIO_MAXFUNC) {
		func = pin_def->functions[func_num];
	}
	if (func == NULL || func_num < 0 || func_num >= MTK_GPIO_MAXFUNC) {
		/* Function not found. */
		if (func) {
			device_printf(sc->sc_dev,
			    "function '%s' not supported on P%03u\n",
			    func, mtk_gpio_pinconf_to_pin(sc, pin_def));
		} else {
			device_printf(sc->sc_dev,
			    "function %u not supported on P%03u\n",
			    func_num, mtk_gpio_pinconf_to_pin(sc, pin_def));
		}
		return ENXIO;
	}

	reg = GPIO_READ(sc, mode_reg);
	reg &= ~mode_mask;
	reg |= __SHIFTIN(func_num, mode_mask);
#ifdef MTK_GPIO_DEBUG
	device_printf(sc->sc_dev, "P%03u mode %04x -> %04x\n",
	    mtk_gpio_pinconf_to_pin(sc, pin_def), GPIO_READ(sc, mode_reg), reg);
#endif
	GPIO_WRITE(sc, mode_reg, reg);

	return 0;
}

static int
mtk_gpio_setpull(struct mtk_gpio_softc * const sc,
		 const struct mtk_gpio_pinconf * const pin_def,
		 const int flags, int pull_strength)
{

	KASSERT(mutex_owned(&sc->sc_lock));

	/*
	 * Some pins have a special combined configuration register
	 * for bias.
	 */
	if (pin_def->pupdr1r0.pupd) {
		uint16_t pupdr1r0;

		pupdr1r0 = GPIO_READ(sc, pin_def->pupdr1r0.reg);
		pupdr1r0 &= ~(pin_def->pupdr1r0.pupd |
			      pin_def->pupdr1r0.r1   |
			      pin_def->pupdr1r0.r0);
		
		/* Only pull-up and pull-down supported. */
		if (flags & GPIO_PIN_PULLDOWN)
			pupdr1r0 |= pin_def->pupdr1r0.pupd;
		else if ((flags & GPIO_PIN_PULLUP) == 0) {
			/*
			 * This is how we disable bias with
			 * these pins.
			 */
			pull_strength = MTK_BIAS_R1R0_00;
		}

		/*
		 * In this case, pull_strength is actually a constant
		 * representing the resistor configuration.
		 */
		switch (pull_strength) {
		case MTK_BIAS_R1R0_00:
			/* bits cleared above. */
			break;
		case MTK_BIAS_R1R0_01:
			pupdr1r0 |= pin_def->pupdr1r0.r0;
			break;
		case MTK_BIAS_R1R0_11:
			pupdr1r0 |= pin_def->pupdr1r0.r0;
			/* FALLTHROUGH */
		case -1:/* unspecified == 50K */	/* XXXJRT */
		case MTK_BIAS_R1R0_10:
			pupdr1r0 |= pin_def->pupdr1r0.r1;
			break;
		default:
			/* invalid bias configuration */
			return ENXIO;
		}
#ifdef MTK_GPIO_DEBUG
		device_printf(sc->sc_dev, "P%03u pupdr1r0 %04x -> %04x\n",
		    mtk_gpio_pinconf_to_pin(sc, pin_def),
		    GPIO_READ(sc, pin_def->pupdr1r0.reg), pupdr1r0);
#endif
		GPIO_WRITE(sc, pin_def->pupdr1r0.reg, pupdr1r0);

		return 0;
	}

	bus_size_t pullen_reg, pullsel_reg;
	u_int pullen_shift, pullsel_shift;
	uint16_t pullen, pullsel;
	int error;

	if ((error = mtk_gpio_reg_for_pin(sc, pin_def, MTK_GPIO_REGS_PULLEN,
					  &pullen_reg, &pullen_shift)) != 0) {
		return error;
	}

	if ((error = mtk_gpio_reg_for_pin(sc, pin_def, MTK_GPIO_REGS_PULLSEL,
					  &pullsel_reg, &pullsel_shift)) != 0) {
		return error;
	}

	const uint16_t pullen_mask = (1U << pullen_shift);
	const uint16_t pullsel_mask = (1U << pullsel_shift);

	pullen = GPIO_READ(sc, pullen_reg);
	pullsel = GPIO_READ(sc, pullsel_reg);

	pullen &= ~pullen_mask;
	pullsel &= ~pullsel_mask;

	if (flags & (GPIO_PIN_PULLUP|GPIO_PIN_PULLDOWN)) {
		if (flags & GPIO_PIN_PULLUP)
			pullsel |= pullsel_mask;
		pullen |= pullen_mask;
	}
#ifdef MTK_GPIO_DEBUG
	device_printf(sc->sc_dev,
	    "P%03u pullen/pullsel %04x/%04x -> %04x/%04x\n",
	    mtk_gpio_pinconf_to_pin(sc, pin_def),
	    GPIO_READ(sc, pullen_reg), GPIO_READ(sc, pullsel_reg),
	    pullen, pullsel);
#endif
	GPIO_WRITE(sc, pullen_reg, pullen);
	GPIO_WRITE(sc, pullsel_reg, pullsel);

	return 0;
}

static int
mtk_gpio_setdrv(struct mtk_gpio_softc * const sc,
		const struct mtk_gpio_pinconf * const pin_def,
		const u_int drive /*mA*/)
{
	u_int sel;
	int error;
	uint16_t drv;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (pin_def->drive.params == NULL) {
		device_printf(sc->sc_dev,
		    "drive strength not supported on P%03u\n",
		    mtk_gpio_pinconf_to_pin(sc, pin_def));
		return ENXIO;
	}

	if ((error = mtk_gpio_mA_to_sel(pin_def->drive.params, drive,
					&sel)) != 0) {
		device_printf(sc->sc_dev,
		    "drive strength '%u mA' not supported on P%03u\n",
		    drive, mtk_gpio_pinconf_to_pin(sc, pin_def));
		return error;
	}

	drv = GPIO_READ(sc, pin_def->drive.reg);
	drv &= ~pin_def->drive.sel;
	drv |= __SHIFTIN(sel, pin_def->drive.sel);
#ifdef MTK_GPIO_DEBUG
	device_printf(sc->sc_dev, "P%03u drv %04x -> %04x\n",
	    mtk_gpio_pinconf_to_pin(sc, pin_def),
	    GPIO_READ(sc, pin_dev->drive.reg), drv);
#endif
	GPIO_WRITE(sc, pin_def->drive.reg, drv);

	return 0;
}

static int
mtk_gpio_setdir(struct mtk_gpio_softc * const sc,
		const struct mtk_gpio_pinconf * const pin_def,
		const int flags)
{
	bus_size_t dir_reg;
	u_int dir_shift;
	uint16_t dir;
	int error;

	KASSERT(mutex_owned(&sc->sc_lock));

	if ((error = mtk_gpio_reg_for_pin(sc, pin_def, MTK_GPIO_REGS_DIR,
					  &dir_reg, &dir_shift)) != 0) {
		return error;
	}

	const uint16_t dir_mask = (1U << dir_shift);

	dir = GPIO_READ(sc, dir_reg);

	if (flags & GPIO_PIN_INPUT)
		dir &= ~dir_mask;
	else if (flags & GPIO_PIN_OUTPUT)
		dir |= dir_mask;
	else
		return EINVAL;

#ifdef MTK_GPIO_DEBUG
	device_printf(sc->sc_dev, "P%03u dir %04x -> %04x\n",
	    mtk_gpio_pinconf_to_pin(sc, pin_def),
	    GPIO_READ(sc, dir_reg), dir);
#endif
	GPIO_WRITE(sc, dir_reg, dir);

	return 0;
}

static int
mtk_gpio_setinput(struct mtk_gpio_softc * const sc,
		  const struct mtk_gpio_pinconf * const pin_def,
		  const bool enable, const bool schmitt)
{
	const struct mtk_ies_smt_group *group;
	const u_int which = schmitt ? MTK_IES_SMT_BIT_SMT : MTK_IES_SMT_BIT_IES;
	uint16_t val;

	KASSERT(mutex_owned(&sc->sc_lock));

	group = mtk_gpio_ies_smt_for_pin(sc, pin_def,
	    schmitt ? MTK_IES_SMT_BIT_SMT : MTK_IES_SMT_BIT_IES);

	if (group == NULL) {
		device_printf(sc->sc_dev,
		    "input%s-%sable not supported on P%03u\n",
		    schmitt ? "-schmitt" : "", enable ? "en" : "dis",
		    mtk_gpio_pinconf_to_pin(sc, pin_def));
		return ENXIO;
	}

	val = GPIO_READ(sc, group->regs[which]);
	if (enable)
		val |= group->bits[which];
	else
		val &= ~group->bits[which];
#ifdef MTK_GPIO_DEBUG
	device_printf(sc->sc_dev, "P%03u %s %04x -> %04x\n",
	    mtk_gpio_pinconf_to_pin(sc, pin_def),
	    GPIO_READ(sc, group->regs[which]), val);
#endif
	GPIO_WRITE(sc, group->regs[which], val);

	return 0;
}

static int
mtk_gpio_ctl(struct mtk_gpio_softc * const sc,
	     const struct mtk_gpio_pinconf * const pin_def,
	     const int flags)
{
	int error;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (flags & GPIO_PIN_INPUT) {
		/*
		 * Set the direction first, then enable the input
		 * buffer.
		 */
		error = mtk_gpio_setdir(sc, pin_def, flags);
		if (error == 0) {
			error = mtk_gpio_setinput(sc, pin_def,
						  true /*enable*/,
						  false /*schmitt*/);
		}
		return error;
	}
	if (flags & GPIO_PIN_OUTPUT) {
		/*
		 * We don't ever disable input buffers here, because
		 * a single enable bit may be shared by multiple pins.
		 */
		return mtk_gpio_setdir(sc, pin_def, flags);
	}

	return EINVAL;
}

static int
mtk_gpio_getval(struct mtk_gpio_softc * const sc,
		const struct mtk_gpio_pinconf * const pin_def, int * const valp)
{
	bus_size_t din_reg;
	u_int din_shift;
	uint16_t din;
	int error;

	/* No lock needed for read. */

	if ((error = mtk_gpio_reg_for_pin(sc, pin_def, MTK_GPIO_REGS_DIN,
					  &din_reg, &din_shift)) != 0) {
		return error;
	}

	const uint16_t din_mask = (1U << din_shift);

	din = GPIO_READ(sc, din_reg);
	*valp = (din & din_mask) ? true : false;

	return 0;
}

static int
mtk_gpio_setval(struct mtk_gpio_softc * const sc,
		const struct mtk_gpio_pinconf * const pin_def, const int val)
{
	bus_size_t dout_reg;
	u_int dout_shift;
	uint16_t dout;
	int error;

	KASSERT(mutex_owned(&sc->sc_lock));

	if ((error = mtk_gpio_reg_for_pin(sc, pin_def, MTK_GPIO_REGS_DOUT,
					  &dout_reg, &dout_shift)) != 0) {
		return error;
	}

	const uint16_t dout_mask = (1U << dout_shift);

	dout = GPIO_READ(sc, dout_reg);
	if (val)
		dout |= dout_mask;
	else
		dout &= ~dout_mask;
	GPIO_WRITE(sc, dout_reg, dout);

	return 0;
}

static void *
mtk_gpio_fdt_acquire(device_t dev, const void *data, const size_t len,
		     const int flags)
{
	struct mtk_gpio_softc * const sc = device_private(dev);
	const struct mtk_gpio_pinconf *pin_def;
	struct mtk_gpio_pin *gpin;
	const u_int *gpio = data;
	int error;

	if (len != 12)
		return NULL;

	const u_int pin = be32toh(gpio[1]);
	const bool actlo = be32toh(gpio[2]) & 1;

	pin_def = mtk_gpio_lookup(sc, pin);
	if (pin_def == NULL)
		return NULL;
	
	mutex_enter(&sc->sc_lock);
	error = mtk_gpio_ctl(sc, pin_def, flags);
	mutex_exit(&sc->sc_lock);

	if (error != 0)
		return NULL;
	
	gpin = kmem_zalloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_sc = sc;
	gpin->pin_def = pin_def;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	return gpin;
}

static void
mtk_gpio_fdt_release(device_t dev, void *priv)
{
	struct mtk_gpio_softc * const sc = device_private(dev);
	struct mtk_gpio_pin *pin = priv;

	mutex_enter(&sc->sc_lock);
	/* XXXJRT maybe not -- this (maybe) enables the input buffer! */
	mtk_gpio_ctl(sc, pin->pin_def, GPIO_PIN_INPUT);
	mutex_exit(&sc->sc_lock);

	kmem_free(pin, sizeof(*pin));
}

static int
mtk_gpio_fdt_read(device_t dev, void *priv, bool raw)
{
	struct mtk_gpio_softc * const sc = device_private(dev);
	struct mtk_gpio_pin * const pin = priv;
	const struct mtk_gpio_pinconf * const pin_def = pin->pin_def;
	int error, val;

	KASSERT(sc == pin->pin_sc);

	/* No lock required for reads. */
	error = mtk_gpio_getval(sc, pin_def, &val);
	KASSERT(error == 0);

	if (!raw && pin->pin_actlo)
		val = !val;

	return val;
}

static void
mtk_gpio_fdt_write(device_t dev, void *priv, int val, bool raw)
{
	struct mtk_gpio_softc * const sc = device_private(dev);
	struct mtk_gpio_pin * const pin = priv;
	const struct mtk_gpio_pinconf * const pin_def = pin->pin_def;
	int error;

	KASSERT(sc == pin->pin_sc);

	if (!raw && pin->pin_actlo)
		val = !val;
	
	mutex_enter(&sc->sc_lock);
	error = mtk_gpio_setval(sc, pin_def, val);
	mutex_exit(&sc->sc_lock);
	KASSERT(error == 0);
}

static struct fdtbus_gpio_controller_func mtk_gpio_fdt_funcs = {
	.acquire = mtk_gpio_fdt_acquire,
	.release = mtk_gpio_fdt_release,
	.read = mtk_gpio_fdt_read,
	.write = mtk_gpio_fdt_write,
};

static const struct mtk_gpio_pinconf *
mtk_gpio_lookup_eint_pin(const struct mtk_gpio_softc * const sc, u_int pin)
{
	const struct mtk_gpio_pinconf * const pin_def =
	    mtk_gpio_lookup(sc, pin);

	if (pin_def == NULL)
		return NULL;

	if ((pin_def->eint.eint_flags & MTK_EINT_SOURCE) == 0) {
		/* This pin is not a valid interrupt source. */
		return NULL;
	}

	return pin_def;
}

static bool
mtk_gpio_intrstr(const struct mtk_gpio_softc * const sc,
		  const struct mtk_gpio_pinconf * const pin_def,
		  char *buf, const size_t buflen)
{
	snprintf(buf, buflen, "GPIO %u (EINT %" PRIu16 ")",
		 mtk_gpio_pinconf_to_pin(sc, pin_def), pin_def->eint.eint_num);

	return true;
}

static void *
mtk_gpio_fdt_intr_establish(device_t dev, u_int *specifier, int ipl,
			    int flags, int (*func)(void *), void *arg)
{
	struct mtk_gpio_softc * const sc = device_private(dev);
	u_int eint_flags = (flags & FDT_INTR_MPSAFE)
	    ? MTK_EINTC_INTR_MPSAFE : 0;

	/* 1st cell is the GPIO pin number. */
	/* 2nd cell is the interrupt type. */
	const u_int pin = be32toh(specifier[0]);
	const u_int type = be32toh(specifier[1]);

	const struct mtk_gpio_pinconf * const pin_def =
	    mtk_gpio_lookup_eint_pin(sc, pin);

	if (pin_def == NULL)
		return NULL;

	return mtk_eintc_intr_enable(&sc->sc_eintc, func, arg, ipl,
				     pin_def->eint.eint_num, type, eint_flags);
}

static void
mtk_gpio_fdt_intr_disestablish(device_t dev, void *ih)
{
	struct mtk_gpio_softc * const sc = device_private(dev);

	mtk_eintc_intr_disable(&sc->sc_eintc, ih);
}

static bool
mtk_gpio_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{

	if (!specifier)
		return false;
	
	/* 1st cell is the GPIO pin number. */
	const u_int pin = be32toh(specifier[0]);

	const struct mtk_gpio_softc * const sc = device_private(dev);
	const struct mtk_gpio_pinconf * const pin_def =
	    mtk_gpio_lookup_eint_pin(sc, pin);

	if (pin_def == NULL)
		return false;

	return mtk_gpio_intrstr(sc, pin_def, buf, buflen);
}

static struct fdtbus_interrupt_controller_func mtk_gpio_fdt_intrfuncs = {
	.establish = mtk_gpio_fdt_intr_establish,
	.disestablish = mtk_gpio_fdt_intr_disestablish,
	.intrstr = mtk_gpio_fdt_intrstr,
};

static void *
mtk_gpio_gpio_intr_establish(void *vsc, int pin, int ipl, int irqmode,
			     int (*func)(void *), void *arg)
{
	struct mtk_gpio_softc * const sc = vsc;
	u_int eint_flags = (irqmode & GPIO_INTR_MPSAFE)
	    ? MTK_EINTC_INTR_MPSAFE : 0;
	u_int type;

	switch (irqmode & GPIO_INTR_MODE_MASK) {
	case GPIO_INTR_POS_EDGE:
		type = FDT_INTR_TYPE_POS_EDGE;
		break;
	case GPIO_INTR_NEG_EDGE:
		type = FDT_INTR_TYPE_NEG_EDGE;
		break;
	case GPIO_INTR_DOUBLE_EDGE:
		type = FDT_INTR_TYPE_DOUBLE_EDGE;
		break;
	case GPIO_INTR_HIGH_LEVEL:
		type = FDT_INTR_TYPE_HIGH_LEVEL;
		break;
	case GPIO_INTR_LOW_LEVEL:
		type = FDT_INTR_TYPE_LOW_LEVEL;
		break;
	default:
		aprint_error_dev(sc->sc_dev, "%s: unsupported irq type 0x%x\n",
				 __func__, irqmode & GPIO_INTR_MODE_MASK);
		return NULL;
	}

	const struct mtk_gpio_pinconf * const pin_def =
	    mtk_gpio_lookup_eint_pin(sc, pin);

	if (pin_def == NULL)
		return NULL;

	return mtk_eintc_intr_enable(&sc->sc_eintc, func, arg, ipl,
				     pin_def->eint.eint_num, type, eint_flags);
}

static void
mtk_gpio_gpio_intr_disestablish(void *vsc, void *ih)
{
	struct mtk_gpio_softc * const sc = vsc;

	mtk_eintc_intr_disable(&sc->sc_eintc, ih);
}

static bool
mtk_gpio_gpio_intrstr(void *vsc, int pin, int irqmode, char *buf, size_t buflen)
{

	const struct mtk_gpio_softc * const sc = vsc;
	const struct mtk_gpio_pinconf * const pin_def =
	    mtk_gpio_lookup(sc, (u_int)pin);

	if (pin_def == NULL)
		return false;

	return mtk_gpio_intrstr(sc, pin_def, buf, buflen);
}

static int
mtk_pinctrl_set_config_for_group(struct mtk_gpio_softc * const sc,
				 const int phandle)
{
	const struct mtk_gpio_pinconf *pin_def;
	int pinmux_len;

	/*
	 * Required: pinmux
	 * Optional: bias, drive-strength, input-enable
	 */

	const u_int *pinmux = fdtbus_pinctrl_parse_pinmux(phandle, &pinmux_len);
	if (pinmux == NULL)
		return -1;

	int pull_strength;
	const int bias = fdtbus_pinctrl_parse_bias(phandle, &pull_strength);
	const int drive_strength = fdtbus_pinctrl_parse_drive_strength(phandle);
	int output_value;
	const int direction =
	    fdtbus_pinctrl_parse_input_output(phandle, &output_value);

	mutex_enter(&sc->sc_lock);

	for (; pinmux_len >= 4; pinmux_len -= 4, pinmux++) {
		uint32_t pinmux_native = be32dec(pinmux);
		u_int pin = MTK_PINMUX_PIN(pinmux_native);
		int func_num = MTK_PINMUX_FUNC(pinmux_native);

		pin_def = mtk_gpio_lookup(sc, pin);
		if (pin_def == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "unknown pin number %u\n", pin);
			continue;
		}

		if (mtk_gpio_setfunc(sc, pin_def, NULL, func_num) != 0)
			continue;

		if (bias != -1)
			mtk_gpio_setpull(sc, pin_def, bias, pull_strength);

		if (drive_strength != -1)
			mtk_gpio_setdrv(sc, pin_def, drive_strength);

		if (direction != -1)
			mtk_gpio_ctl(sc, pin_def, direction);

		/*
		 * XXXJRT discard output_value; we don't need it right
		 * now anyway.
		 */
	}

	mutex_exit(&sc->sc_lock);

	return 0;
}

static int
mtk_pinctrl_set_config(device_t dev, const void *data, size_t len)
{
	struct mtk_gpio_softc * const sc = device_private(dev);

	if (len != 4)
		return -1;

	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));

	int group;
	for (group = OF_child(phandle); group; group = OF_peer(group)) {
		int rv = mtk_pinctrl_set_config_for_group(sc, group);
		if (rv) {
			device_printf(sc->sc_dev,
			    "failed to set config for pin group \"%s\"\n",
			    fdtbus_get_string(group, "name"));
			return rv;
		}
	}

	return 0;
}

static struct fdtbus_pinctrl_controller_func mtk_pinctrl_funcs = {
	.set_config = mtk_pinctrl_set_config,
};

static int
mtk_gpio_pin_read(void *priv, int pin)
{
	struct mtk_gpio_softc * const sc = priv;
	const struct mtk_gpio_pinconf * const pin_def =
	    &sc->sc_padconf->pins[pin];
	int error, val;

	KASSERT((u_int)pin < sc->sc_padconf->npins);

	/* No lock required for reads. */
	error = mtk_gpio_getval(sc, pin_def, &val);
	KASSERT(error == 0);

	return val;
}

static void
mtk_gpio_pin_write(void *priv, int pin, int val)
{
	struct mtk_gpio_softc * const sc = priv;
	const struct mtk_gpio_pinconf * const pin_def =
	    &sc->sc_padconf->pins[pin];
	int error;

	KASSERT((u_int)pin < sc->sc_padconf->npins);

	mutex_enter(&sc->sc_lock);
	error = mtk_gpio_setval(sc, pin_def, val);
	mutex_exit(&sc->sc_lock);
	KASSERT(error == 0);
}

static void
mtk_gpio_pin_ctl(void *priv, int pin, int flags)
{
	struct mtk_gpio_softc * const sc = priv;
	const struct mtk_gpio_pinconf * const pin_def =
	    &sc->sc_padconf->pins[pin];
	int error;

	KASSERT((u_int)pin < sc->sc_padconf->npins);

	mutex_enter(&sc->sc_lock);
	error = mtk_gpio_ctl(sc, pin_def, flags);
	if (error == 0)
		error = mtk_gpio_setpull(sc, pin_def, flags, -1);
	mutex_exit(&sc->sc_lock);
	KASSERT(error == 0);
}

static void
mtk_gpio_attach_pins(struct mtk_gpio_softc * const sc)
{
	const struct mtk_gpio_pinconf *pin_def;
	struct gpio_chipset_tag * const gp = &sc->sc_gp;
	struct gpiobus_attach_args gba;
	u_int pin;

	gp->gp_cookie = sc;
	gp->gp_pin_read = mtk_gpio_pin_read;
	gp->gp_pin_write = mtk_gpio_pin_write;
	gp->gp_pin_ctl = mtk_gpio_pin_ctl;
	gp->gp_intr_establish = mtk_gpio_gpio_intr_establish;
	gp->gp_intr_disestablish = mtk_gpio_gpio_intr_disestablish;
	gp->gp_intr_str = mtk_gpio_gpio_intrstr;

	const u_int npins = sc->sc_padconf->npins;
	sc->sc_pins = kmem_zalloc(sizeof(*sc->sc_pins) * npins, KM_SLEEP);

	for (pin = 0; pin < npins; pin++) {
		pin_def = &sc->sc_padconf->pins[pin];
		sc->sc_pins[pin].pin_num = pin;
		sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		    GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;
		sc->sc_pins[pin].pin_intrcaps =
		    GPIO_INTR_POS_EDGE |
		    GPIO_INTR_NEG_EDGE |
#if 0
		    /*
		     * Double-edge is not supported natively by the
		     * hardware.  We could emulate it, but we don't
		     * right now.
		     */
		    GPIO_INTR_DOUBLE_EDGE |
#endif
		    GPIO_INTR_HIGH_LEVEL |
		    GPIO_INTR_LOW_LEVEL |
		    GPIO_INTR_MPSAFE;
		sc->sc_pins[pin].pin_state = mtk_gpio_pin_read(sc, pin);
		strlcpy(sc->sc_pins[pin].pin_defname, pin_def->name,
		    sizeof(sc->sc_pins[pin].pin_defname));
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = gp;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = npins;
	sc->sc_gpiodev = config_found_ia(sc->sc_dev, "gpiobus", &gba, NULL);
}

static int
mtk_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
mtk_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct mtk_gpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t eint_addr, gpio_addr;
	bus_size_t eint_size, gpio_size;
	uint32_t val;

	/*
	 * We currently only support "pins-are-numbered" pinctrl
	 * bindings.
	 */
	if (!of_hasprop(phandle, "pins-are-numbered")) {
		aprint_error(
		    ": non-\"pins-are-numbered\" bindings not supported\n");
		return;
	}

	/*
	 * The device tree for MediaTek pin controllers is a little goofy.
	 * The "reg" property references the EINT register block, and
	 * there is a separate "mediatek,pctl-regmap" that points to
	 * another device tree node that contains the register block
	 * for the pin controller itself.  I don't know why they did
	 * it this way (when they could have just used an array along
	 * with the "regnames" property), but there you have it.
	 */

	if (of_getprop_uint32(phandle, "mediatek,pctl-regmap", &val) != 0) {
		aprint_error(": couldn't get regmap property\n");
		return;
	}
	const int regmap_phandle = fdtbus_get_phandle_from_native(val);

	if (fdtbus_get_reg(phandle, 0, &eint_addr, &eint_size) != 0) {
		aprint_error(": couldn't get eint registers\n");
		return;
	}

	if (fdtbus_get_reg(regmap_phandle, 0, &gpio_addr, &gpio_size) != 0) {
		aprint_error(": couldn't get pinctrl registers\n");
		return;
	}

	sc->sc_dev = sc->sc_eintc.sc_dev = self;
	sc->sc_bst = sc->sc_eintc.sc_bst = faa->faa_bst;

	if (bus_space_map(sc->sc_bst, eint_addr, eint_size, 0,
			  &sc->sc_eintc.sc_bsh) != 0) {
		aprint_error(": couldn't map eint registers\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, gpio_addr, gpio_size, 0,
			  &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map gpio registers\n");
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	sc->sc_padconf =
	    (void *)of_search_compatible(phandle, compat_data)->data;
	sc->sc_eintc.sc_eintc = sc->sc_padconf->eintc;

	aprint_naive("\n");
	aprint_normal(": Pin MUX controller\n");

	mtk_eintc_init(&sc->sc_eintc);

	fdtbus_register_gpio_controller(self, phandle, &mtk_gpio_fdt_funcs);

	/*
	 * MediaTek "pins-are-numbered" pinctrl configurations are nodes
	 * that themselves contain sub-nodes that group the pins of similar
	 * settings together.  Each sub-node group is configured when that
	 * pinctrl configuration is selected.
	 */
	int child;
	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		fdtbus_register_pinctrl_config(self, child, &mtk_pinctrl_funcs);
	}

	fdtbus_pinctrl_configure();

	fdtbus_register_interrupt_controller(self, phandle,
					     &mtk_gpio_fdt_intrfuncs);

	mtk_gpio_attach_pins(sc);
}
