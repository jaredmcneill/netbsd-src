/* $NetBSD$ */

/*-
 * Copyright (c) 2019 Jared D. McNeill <jmcneill@invisible.ca>
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
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>

#include <dev/fdt/fdtvar.h>

#include <arm/amlogic/meson_pinctrl.h>

struct meson_pinctrl_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh_mux;
	bus_space_handle_t	sc_bsh_pull;
	bus_space_handle_t	sc_bsh_gpio;
	int			sc_phandle;

	const struct meson_pinctrl_config *sc_conf;
};

static const struct of_compat_data compat_data[] = {
#ifdef SOC_MESON8B
	{ "amlogic,meson8b-aobus-pinctrl",	(uintptr_t)&meson8b_aobus_pinctrl_config },
	{ "amlogic,meson8b-cbus-pinctrl",	(uintptr_t)&meson8b_cbus_pinctrl_config },
#endif
	{ NULL, 0 }
};

#define	MUX_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh_mux, (reg))
#define	MUX_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh_mux, (reg), (val))

static const struct meson_pinctrl_group *
meson_pinctrl_find_group(struct meson_pinctrl_softc *sc,
    const char *name)
{
	const struct meson_pinctrl_group *group;
	u_int n;

	for (n = 0; n < sc->sc_conf->ngroups; n++) {
		group = &sc->sc_conf->groups[n];
		if (strcmp(group->name, name) == 0)
			return group;
	}

	return NULL;
}

static bool
meson_pinctrl_group_in_bank(struct meson_pinctrl_softc *sc,
    const struct meson_pinctrl_group *group, u_int bankno)
{
	u_int n;

	for (n = 0; n < group->nbank; n++) {
		if (group->bank[n] == bankno)
			return true;
	}

	return false;
}

static void
meson_pinctrl_set_group(struct meson_pinctrl_softc *sc,
    const struct meson_pinctrl_group *group, bool enable)
{
	uint32_t val;

	val = MUX_READ(sc, group->reg);
	if (enable)
		val |= __BIT(group->bit);
	else
		val &= ~__BIT(group->bit);
	MUX_WRITE(sc, group->reg, val);
}

static void
meson_pinctrl_setfunc(struct meson_pinctrl_softc *sc, const char *name)
{
	const struct meson_pinctrl_group *group, *target_group;
	u_int n, bank;

	target_group = meson_pinctrl_find_group(sc, name);
	if (target_group == NULL) {
		aprint_error_dev(sc->sc_dev, "function '%s' not supported\n", name);
		return;
	}

	/* Disable conflicting groups */
	for (n = 0; n < sc->sc_conf->ngroups; n++) {
		group = &sc->sc_conf->groups[n];
		if (target_group == group)
			continue;
		for (bank = 0; bank < target_group->nbank; bank++) {
			if (meson_pinctrl_group_in_bank(sc, group, target_group->bank[bank]))
				meson_pinctrl_set_group(sc, group, false);
		}
	}

	/* Enable target group */
	meson_pinctrl_set_group(sc, target_group, true);
}

static int
meson_pinctrl_set_config(device_t dev, const void *data, size_t len)
{
	struct meson_pinctrl_softc * const sc = device_private(dev);
	const char *groups;
	int groups_len;

	if (len != 4)
		return -1;

	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));
	const int mux = of_find_firstchild_byname(phandle, "mux");
	if (mux == -1)
		return -1;

	groups_len = OF_getproplen(mux, "groups");
	if (groups_len <= 0)
		return -1;
	groups = fdtbus_get_string(mux, "groups");

	for (; groups_len > 0;
	    groups_len -= strlen(groups) + 1, groups += strlen(groups) + 1) {
		meson_pinctrl_setfunc(sc, groups);
	}

	return 0;
}

static struct fdtbus_pinctrl_controller_func meson_pinctrl_funcs = {
	.set_config = meson_pinctrl_set_config,
};

static int
meson_pinctrl_initres(struct meson_pinctrl_softc *sc)
{
	bool gpio_found = false;
	bus_addr_t addr;
	bus_size_t size;
	int child;

	for (child = OF_child(sc->sc_phandle); child; child = OF_peer(child)) {
		if (of_hasprop(child, "gpio-controller")) {
			if (gpio_found)
				continue;
			gpio_found = true;

			if (fdtbus_get_reg_byname(child, "mux", &addr, &size) != 0 ||
			    bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh_mux) != 0) {
				aprint_error(": couldn't map mux registers\n");
				return ENXIO;
			}
			if (fdtbus_get_reg_byname(child, "pull", &addr, &size) != 0 ||
			    bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh_pull) != 0) {
				aprint_error(": couldn't map pull registers\n");
				return ENXIO;
			}
			if (fdtbus_get_reg_byname(child, "gpio", &addr, &size) != 0 ||
			    bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh_gpio) != 0) {
				aprint_error(": couldn't map gpio registers\n");
				return ENXIO;
			}
		} else if (of_find_firstchild_byname(child, "mux") != -1) {
			fdtbus_register_pinctrl_config(sc->sc_dev, child, &meson_pinctrl_funcs);
		}
	}

	if (!gpio_found) {
		aprint_error(": couldn't find gpio controller\n");
		return ENOENT;
	}

	return 0;
}

static int
meson_pinctrl_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
meson_pinctrl_attach(device_t parent, device_t self, void *aux)
{
	struct meson_pinctrl_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;
	sc->sc_conf = (void *)of_search_compatible(sc->sc_phandle, compat_data)->data;

	if (meson_pinctrl_initres(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": %s\n", sc->sc_conf->name);

	fdtbus_pinctrl_configure();
}

CFATTACH_DECL_NEW(meson_pinctrl, sizeof(struct meson_pinctrl_softc),
	meson_pinctrl_match, meson_pinctrl_attach, NULL, NULL);
