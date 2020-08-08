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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <arm/mediatek/mtk_cru.h>

static void *
mtk_cru_reset_acquire(device_t dev, const void *data, size_t len)
{
	struct mtk_cru_softc * const sc = device_private(dev);
	struct mtk_cru_reset *reset;

	if (len != 4)
		return NULL;

	const u_int reset_id = be32dec(data);

	if (reset_id >= sc->sc_nresets)
		return NULL;

	reset = &sc->sc_resets[reset_id];
	if (reset->mask == 0)
		return NULL;

	return reset;
}

static void
mtk_cru_reset_release(device_t dev, void *priv)
{
}

static int
mtk_cru_reset_assert(device_t dev, void *priv)
{
	struct mtk_cru_softc * const sc = device_private(dev);
	struct mtk_cru_reset * const reset = priv;

	mutex_enter(&sc->sc_mutex);
	const uint32_t val = CRU_READ(sc, reset->reg);
	CRU_WRITE(sc, reset->reg, val | reset->mask);
	mutex_exit(&sc->sc_mutex);

	return 0;
}

static int
mtk_cru_reset_deassert(device_t dev, void *priv)
{
	struct mtk_cru_softc * const sc = device_private(dev);
	struct mtk_cru_reset * const reset = priv;

	mutex_enter(&sc->sc_mutex);
	const uint32_t val = CRU_READ(sc, reset->reg);
	CRU_WRITE(sc, reset->reg, val & ~reset->mask);
	mutex_exit(&sc->sc_mutex);

	return 0;
}

static const struct fdtbus_reset_controller_func mtk_cru_fdtreset_funcs = {
	.acquire = mtk_cru_reset_acquire,
	.release = mtk_cru_reset_release,
	.reset_assert = mtk_cru_reset_assert,
	.reset_deassert = mtk_cru_reset_deassert,
};

static LIST_HEAD(, mtk_cru_softc) mtk_cru_list =
    LIST_HEAD_INITIALIZER(&mtk_cru_list);

static struct mtk_cru_clk *
mtk_cru_clock_lookup(const char *name)
{
	struct mtk_cru_softc *sc;
	struct mtk_cru_clk *clk;

	LIST_FOREACH(sc, &mtk_cru_list, sc_cru_list) {
		if ((clk = mtk_cru_clock_find(sc, name)) != NULL)
			return clk;
	}

	return NULL;
}

static struct clk *
mtk_cru_clock_decode(device_t dev, int cc_phandle, const void *data,
		       size_t len)
{
	struct mtk_cru_softc * const sc = device_private(dev);
	struct mtk_cru_clk *clk;

	if (len != 4)
		return NULL;

	const u_int clock_id = be32dec(data);
	if (clock_id >= sc->sc_nclks)
		return NULL;

	clk = &sc->sc_clks[clock_id];
	if (clk->type == MTK_CLK_UNKNOWN)
		return NULL;

	return &clk->base;
}

static const struct fdtbus_clock_controller_func mtk_cru_fdtclock_funcs = {
	.decode = mtk_cru_clock_decode,
};

static struct clk *
mtk_cru_clock_get(void *priv, const char *name)
{
	struct mtk_cru_softc * const sc = priv;
	struct mtk_cru_clk *clk;

	clk = mtk_cru_clock_find(sc, name);
	if (clk == NULL)
		return NULL;

	return &clk->base;
}

static void
mtk_cru_clock_put(void *priv, struct clk *clk)
{
}

static u_int
mtk_cru_clock_get_rate(void *priv, struct clk *clkp)
{
	struct mtk_cru_softc * const sc = priv;
	struct mtk_cru_clk *clk = (struct mtk_cru_clk *)clkp;
	struct clk *clkp_parent;

	if (clk->get_rate)
		return clk->get_rate(sc, clk);

	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL) {
		aprint_error("%s: no parent for %s\n", __func__,
		    clk->base.name);
		return 0;
	}

	return clk_get_rate(clkp_parent);
}

static int
mtk_cru_clock_set_rate(void *priv, struct clk *clkp, u_int rate)
{
	struct mtk_cru_softc * const sc = priv;
	struct mtk_cru_clk *clk = (struct mtk_cru_clk *)clkp;
	struct clk *clkp_parent;

	if (clkp->flags & CLK_SET_RATE_PARENT) {
		clkp_parent = clk_get_parent(clkp);
		if (clkp_parent == NULL) {
			aprint_error("%s: no parent for %s\n", __func__,
			    clk->base.name);
			return ENXIO;
		}
		return clk_set_rate(clkp_parent, rate);
	}

	if (clk->set_rate)
		return clk->set_rate(sc, clk, rate);

	return ENXIO;
}

static u_int
mtk_cru_clock_round_rate(void *priv, struct clk *clkp, u_int rate)
{
	struct mtk_cru_softc * const sc = priv;
	struct mtk_cru_clk *clk = (struct mtk_cru_clk *)clkp;
	struct clk *clkp_parent;

	if (clkp->flags & CLK_SET_RATE_PARENT) {
		clkp_parent = clk_get_parent(clkp);
		if (clkp_parent == NULL) {
			aprint_error("%s: no parent for %s\n", __func__,
			    clk->base.name);
			return 0;
		}
		return clk_round_rate(clkp_parent, rate);
	}

	if (clk->round_rate)
		return clk->round_rate(sc, clk, rate);

	return 0;
}

static int
mtk_cru_clock_enable(void *priv, struct clk *clkp)
{
	struct mtk_cru_softc * const sc = priv;
	struct mtk_cru_clk *clk = (struct mtk_cru_clk *)clkp;
	struct clk *clkp_parent;
	int error = 0;

	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent != NULL) {
		error = clk_enable(clkp_parent);
		if (error != 0)
			return error;
	}

	if (clk->enable)
		error = clk->enable(sc, clk, 1);

	return error;
}

static int
mtk_cru_clock_disable(void *priv, struct clk *clkp)
{
	struct mtk_cru_softc * const sc = priv;
	struct mtk_cru_clk *clk = (struct mtk_cru_clk *)clkp;
	int error = EINVAL;

	if (clk->enable)
		error = clk->enable(sc, clk, 0);

	return error;
}

static int
mtk_cru_clock_set_parent(void *priv, struct clk *clkp,
    struct clk *clkp_parent)
{
	struct mtk_cru_softc * const sc = priv;
	struct mtk_cru_clk *clk = (struct mtk_cru_clk *)clkp;

	if (clk->set_parent == NULL)
		return EINVAL;

	return clk->set_parent(sc, clk, clkp_parent->name);
}

static struct clk *
mtk_cru_clock_get_parent(void *priv, struct clk *clkp)
{
	struct mtk_cru_softc * const sc = priv;
	struct mtk_cru_clk *clk = (struct mtk_cru_clk *)clkp;
	struct mtk_cru_clk *clk_parent;
	const char *parent;

	if (clk->get_parent == NULL)
		return NULL;

	parent = clk->get_parent(sc, clk);
	if (parent == NULL)
		return NULL;

	clk_parent = mtk_cru_clock_find(sc, parent);
	if (clk_parent != NULL)
		return &clk_parent->base;

	/* No parent in this domain, try FDT */
	clkp = fdtbus_clock_byname(parent);
	if (clkp != NULL)
		return clkp;

	/*
	 * Finally, try looking in all of our own clock domains.
	 *
	 * XXX In an ideal world, the DT would have clock-output-names
	 * on all of our clock controllers, but we don't live in an
	 * ideal world.
	 */
	clk_parent = mtk_cru_clock_lookup(parent);
	if (clk_parent != NULL)
		return &clk_parent->base;

	return NULL;
}

static const struct clk_funcs mtk_cru_clock_funcs = {
	.get = mtk_cru_clock_get,
	.put = mtk_cru_clock_put,
	.get_rate = mtk_cru_clock_get_rate,
	.set_rate = mtk_cru_clock_set_rate,
	.round_rate = mtk_cru_clock_round_rate,
	.enable = mtk_cru_clock_enable,
	.disable = mtk_cru_clock_disable,
	.set_parent = mtk_cru_clock_set_parent,
	.get_parent = mtk_cru_clock_get_parent,
};

struct mtk_cru_clk *
mtk_cru_clock_find(struct mtk_cru_softc *sc, const char *name)
{
	for (int i = 0; i < sc->sc_nclks; i++) {
		if (sc->sc_clks[i].base.name == NULL)
			continue;
		if (strcmp(sc->sc_clks[i].base.name, name) == 0)
			return &sc->sc_clks[i];
	}

	return NULL;
}

static void
mtk_cru_syscon_lock(void *v)
{
	struct mtk_cru_softc * const sc = v;

	mutex_enter(&sc->sc_mutex);
}

static void
mtk_cru_syscon_unlock(void *v)
{
	struct mtk_cru_softc * const sc = v;

	mutex_exit(&sc->sc_mutex);
}

static uint32_t
mtk_cru_syscon_read_4(void *v, bus_size_t reg)
{
	struct mtk_cru_softc * const sc = v;

	KASSERT(mutex_owned(&sc->sc_mutex));

	return CRU_READ(sc, reg);
}

static void
mtk_cru_syscon_write_4(void *v, bus_size_t reg, uint32_t val)
{
	struct mtk_cru_softc * const sc = v;

	KASSERT(mutex_owned(&sc->sc_mutex));

	CRU_WRITE(sc, reg, val);
}

int
mtk_cru_attach(struct mtk_cru_softc *sc)
{
	bus_addr_t addr;
	bus_size_t size;
	int i;

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_VM);

	if (fdtbus_get_reg(sc->sc_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return ENXIO;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return ENXIO;
	}

	sc->sc_clkdom.name = device_xname(sc->sc_dev);
	sc->sc_clkdom.funcs = &mtk_cru_clock_funcs;
	sc->sc_clkdom.priv = sc;
	for (i = 0; i < sc->sc_nclks; i++) {
		sc->sc_clks[i].base.domain = &sc->sc_clkdom;
		clk_attach(&sc->sc_clks[i].base);
	}

	fdtbus_register_clock_controller(sc->sc_dev, sc->sc_phandle,
	    &mtk_cru_fdtclock_funcs);

	fdtbus_register_reset_controller(sc->sc_dev, sc->sc_phandle,
	    &mtk_cru_fdtreset_funcs);

	sc->sc_syscon.priv = sc;
	sc->sc_syscon.lock = mtk_cru_syscon_lock;
	sc->sc_syscon.unlock = mtk_cru_syscon_unlock;
	sc->sc_syscon.read_4 = mtk_cru_syscon_read_4;
	sc->sc_syscon.write_4 = mtk_cru_syscon_write_4;
	fdtbus_register_syscon(sc->sc_dev, sc->sc_phandle, &sc->sc_syscon);

	LIST_INSERT_HEAD(&mtk_cru_list, sc, sc_cru_list);

	return 0;
}

void
mtk_cru_print(struct mtk_cru_softc *sc)
{
	struct mtk_cru_clk *clk;
	struct clk *clkp_parent;
	const char *type;
	int i;

	for (i = 0; i < sc->sc_nclks; i++) {
		clk = &sc->sc_clks[i];
		if (clk->type == MTK_CLK_UNKNOWN)
			continue;

		clkp_parent = clk_get_parent(&clk->base);

		switch (clk->type) {
		case MTK_CLK_FIXED:		type = "fixed"; break;
		case MTK_CLK_FACTOR:		type = "factor"; break;
		case MTK_CLK_GATE:		type = "gate"; break;
		case MTK_CLK_MUX:		type = "mux"; break;
		case MTK_CLK_MUXGATE:		type = "muxgate"; break;
		default:			type = "???"; break;
		}

        	aprint_debug_dev(sc->sc_dev,
		    "%3d %-12s %2s %-12s %-7s ",
		    i,
        	    clk->base.name,
        	    clkp_parent ? "<-" : "",
        	    clkp_parent ? clkp_parent->name : "",
        	    type);
		aprint_debug("%10u Hz\n", clk_get_rate(&clk->base));
	}
}
