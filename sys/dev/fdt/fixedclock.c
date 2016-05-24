/* $NetBSD$ */

/*-
 * Copyright (c) 2016 Jared McNeill <jmcneill@invisible.ca>
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
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

static int	fixedclk_match(device_t, cfdata_t, void *);
static void	fixedclk_attach(device_t, device_t, void *);

static struct clk *fixedclk_decode(device_t, const void *, size_t);

static const struct fdtbus_clock_controller_func fixedclk_fdt_funcs = {
	.decode = fixedclk_decode
};

static struct clk *fixedclk_get(void *, const char *);
static void	fixedclk_put(void *, struct clk *);
static u_int	fixedclk_get_rate(void *, struct clk *);
static int	fixedclk_set_rate(void *, struct clk *, u_int);
static int	fixedclk_enable(void *, struct clk *);
static int	fixedclk_disable(void *, struct clk *);
static int	fixedclk_set_parent(void *, struct clk *, struct clk *);
static struct clk *fixedclk_get_parent(void *, struct clk *);

static struct clk_funcs fixedclk_funcs = {
	.get = fixedclk_get,
	.put = fixedclk_put,
	.get_rate = fixedclk_get_rate,
	.set_rate = fixedclk_set_rate,
	.enable = fixedclk_enable,
	.disable = fixedclk_disable,
	.set_parent = fixedclk_set_parent,
	.get_parent = fixedclk_get_parent,
};

struct fixedclk_softc {
	device_t	sc_dev;
	int		sc_phandle;
	struct clk	sc_clk;

	u_int		sc_type;
#define	FIXEDCLK_FIXED		0
#define	FIXEDCLK_FIXEDFACTOR	1

	/* Fixed clock */
	u_int		sc_clkfreq;

	/* Fixed factor clock */
	u_int		sc_clkdiv;
	u_int		sc_clkmult;
	struct clk	*sc_clkparent;
};

CFATTACH_DECL_NEW(fixedclk, sizeof(struct fixedclk_softc),
    fixedclk_match, fixedclk_attach, NULL, NULL);

static const char * const fixed_clock_compat[] = {
    "fixed-clock", NULL
};
static const char * const fixed_factor_clock_compat[] = {
    "fixed-factor-clock", NULL
};

static int
fixedclk_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;

	return of_match_compatible(phandle, fixed_clock_compat) ||
	       of_match_compatible(phandle, fixed_factor_clock_compat);
}

static void
fixedclk_attach(device_t parent, device_t self, void *aux)
{
	struct fixedclk_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk = &sc->sc_clk;
	char *name = NULL;
	int len;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	aprint_naive("\n");

	len = OF_getproplen(phandle, "clock-output-names");
	if (len > 0) {
		name = kmem_zalloc(len, KM_SLEEP);
		if (OF_getprop(phandle, "clock-output-names", name, len) <= 0) {
			kmem_free(name, len);
			name = NULL;
		}
	}
	if (name == NULL) {
		len = OF_getproplen(phandle, "name");
		name = kmem_zalloc(len, KM_SLEEP);
		if (OF_getprop(phandle, "name", name, len) <= 0) {
			kmem_free(name, len);
			name = NULL;
		}
	}
	if (name == NULL) {
		aprint_error(": couldn't determine clock output name\n");
		return;
	}

	clk->name = name;
	clk->flags = 0;

	if (of_match_compatible(phandle, fixed_factor_clock_compat)) {
		/* Fixed factor clock */
		sc->sc_type = FIXEDCLK_FIXEDFACTOR;
		sc->sc_clkparent = fdtbus_clock_get_index(phandle, 0);
		if (sc->sc_clkparent == NULL) {
			aprint_error(": couldn't get clock parent\n");
			return;
		}
		if (of_getprop_uint32(phandle, "clock-div", &sc->sc_clkdiv)) {
			aprint_error(": couldn't get clock div\n");
			return;
		}
		if (of_getprop_uint32(phandle, "clock-mult", &sc->sc_clkmult)) {
			aprint_error(": couldn't get clock mult\n");
			return;
		}
		aprint_normal(" <%s>: /%u x%u\n",
		    name, sc->sc_clkdiv, sc->sc_clkmult);
	} else {
		/* Fixed clock */
		sc->sc_type = FIXEDCLK_FIXED;
		if (of_getprop_uint32(phandle, "clock-frequency",
		    &sc->sc_clkfreq)) {
			aprint_error(": couldn't get clock frequency\n");
			return;
		}
		aprint_normal(" <%s>: %u Hz\n", name, sc->sc_clkfreq);
	}

	clk->cb = clk_backend_register(self, &fixedclk_funcs, sc);
	fdtbus_register_clock_controller(self, phandle, &fixedclk_fdt_funcs);
}

static struct clk *
fixedclk_get(void *priv, const char *name)
{
	struct fixedclk_softc *sc = priv;

	if (strcmp(name, sc->sc_clk.name) != 0)
		return NULL;

	return &sc->sc_clk;
}

static void
fixedclk_put(void *priv, struct clk *clk)
{
}

static u_int
fixedclk_get_rate(void *priv, struct clk *clk)
{
	struct fixedclk_softc *sc = priv;

	if (sc->sc_type == FIXEDCLK_FIXED)
		return sc->sc_clkfreq;
	else
		return (clk_get_rate(sc->sc_clkparent) * sc->sc_clkmult)
		    / sc->sc_clkdiv;
}

static int
fixedclk_set_rate(void *priv, struct clk *clk, u_int rate)
{
	return EINVAL;
}

static int
fixedclk_enable(void *priv, struct clk *clk)
{
	return 0;
}

static int
fixedclk_disable(void *priv, struct clk *clk)
{
	return 0;
}

static int
fixedclk_set_parent(void *priv, struct clk *clk, struct clk *clk_parent)
{
	return EINVAL;
}

static struct clk *
fixedclk_get_parent(void *priv, struct clk *clk)
{
	struct fixedclk_softc *sc = priv;

	return sc->sc_clkparent;
}

static struct clk *
fixedclk_decode(device_t dev, const void *data, size_t len)
{
	struct fixedclk_softc *sc = device_private(dev);

	return &sc->sc_clk;
}
