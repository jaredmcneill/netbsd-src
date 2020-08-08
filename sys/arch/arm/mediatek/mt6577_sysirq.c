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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

struct mt6577_sysirq_intrpol_block {
	bus_space_handle_t	b_bsh;
	u_int			b_first_irq;
	u_int			b_last_irq;
};

struct mt6577_sysirq_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;

	int			sc_intr_parent;

	struct mt6577_sysirq_intrpol_block *sc_blocks;
	u_int			sc_nblocks;

	kmutex_t		sc_mutex;
};

#define	NCELLS	3U	/* expected "#interrupt-cells" */

static int	mt6577_sysirq_match(device_t, cfdata_t, void *);
static void	mt6577_sysirq_attach(device_t, device_t, void *);

static void *	mt6577_sysirq_fdt_intr_establish(device_t, u_int *, int, int,
		    int (*func)(void *), void *);
static void	mt6577_sysirq_fdt_intr_disestablish(device_t, void *);
static bool	mt6577_sysirq_fdt_intr_intrstr(device_t, u_int *, char *,
		    size_t);

static struct fdtbus_interrupt_controller_func mt6577_sysirq_fdt_intrfuncs = {
	.establish = mt6577_sysirq_fdt_intr_establish,
	.disestablish = mt6577_sysirq_fdt_intr_disestablish,
	.intrstr = mt6577_sysirq_fdt_intr_intrstr,
};

CFATTACH_DECL_NEW(mt6577_sysirq, sizeof(struct mt6577_sysirq_softc),
    mt6577_sysirq_match, mt6577_sysirq_attach, NULL, NULL);

static const char * const compatible[] = {
	"mediatek,mt6577-sysirq",
	NULL,
};

static int
mt6577_sysirq_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mt6577_sysirq_attach(device_t parent, device_t self, void *aux)
{
	struct mt6577_sysirq_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;
	u_int i, next_irq;
	uint32_t ncells;

	const int phandle = faa->faa_phandle;

	/* First, count the number of blocks we have to manage. */
	for (sc->sc_nblocks = 0;
	     fdtbus_get_reg(phandle, sc->sc_nblocks, NULL, NULL) == 0;
	     sc->sc_nblocks++) {
		/* doop de doo. */;
	}
	if (sc->sc_nblocks == 0) {
		aprint_error(": unable to find any INTRPOL register blocks\n");
		return;
	}

	/* Fetch our interrupt parent. */
	sc->sc_intr_parent = fdtbus_get_phandle(phandle, "interrupt-parent");
	if (sc->sc_intr_parent <= 0) {
		aprint_error(": unable to find interrupt-parent\n");
		return;
	}

	/*
	 * DT bindings say that we use the same interrupt specifier
	 * format as "arm,gic", and the DT bindings specify 3 cells
	 * for that.  We make assumptions about that format, so assert
	 * one of our assumptions here.
	 */
	if (of_getprop_uint32(phandle, "#interrupt-cells", &ncells) != 0) {
		aprint_error(": unable to find #interrupt-cells\n");
		return;
	}
	if (ncells != NCELLS) {
		aprint_error(": incorrect #interrupt-cells (%u != %u)\n",
		    ncells, NCELLS);
		return;
	}
	if (of_getprop_uint32(sc->sc_intr_parent, "#interrupt-cells",
			      &ncells) != 0) {
		aprint_error(": unable to find parent #interrupt-cells\n");
		return;
	}
	if (ncells != NCELLS) {
		aprint_error(": incorrect parent #interrupt-cells (%u != %u)\n",
		    ncells, NCELLS);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Interrupt polarity controller\n");

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_VM);

	sc->sc_blocks = kmem_zalloc(sc->sc_nblocks * sizeof(*sc->sc_blocks),
				    KM_SLEEP);

	/* Now map each individual block. */
	for (next_irq = 0, i = 0; i < sc->sc_nblocks; i++) {
		error = fdtbus_get_reg(phandle, i, &addr, &size);
		if (error) {
			aprint_error_dev(self,
			    "unable to get registers for cell #%u (error=%d)\n",
			    i, error);
			return;
		}
		error = bus_space_map(sc->sc_bst, addr, size, 0,
				      &sc->sc_blocks[i].b_bsh);
		if (error) {
			aprint_error_dev(self,
			    "unable to map regsiters for cell #%u (error=%d)\n",
			    i, error);
			return;
		}

		sc->sc_blocks[i].b_first_irq = next_irq;
		next_irq += size * NBBY;
		sc->sc_blocks[i].b_last_irq = next_irq - 1;
	}

	fdtbus_register_interrupt_controller(self, phandle,
	    &mt6577_sysirq_fdt_intrfuncs);
}

static bool
invert_specifier(u_int *orig_specifier, u_int *new_specifier, bool *setbitp)
{

	memcpy(new_specifier, orig_specifier, sizeof(*new_specifier) * NCELLS);

	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

	if (orig_specifier == NULL)
		return false;

	u_int flags = be32toh(orig_specifier[2]);
	u_int new_trigger;
	bool setbit = false;

	switch (flags & 0xfU) {
	case 1:				/* low-to-high edge triggered */
		new_trigger = 2;	/* -> high-to-low */
		break;

	case 2:				/* high-to-low edge triggered */
		new_trigger = 1;	/* -> low-to-high */
		setbit = true;
		break;

	case 4:				/* active-high level sensitive */
		new_trigger = 8;	/* -> active-low */
		break;

	case 8:				/* active-low level sensitive */
		new_trigger = 4;	/* -> active-high */
		setbit = true;
		break;

	default:
		/* Invalid trigger encoding */
		return false;
	}

	flags = (flags & ~0xfU) | new_trigger;
	new_specifier[2] = htobe32(flags);
	if (setbitp != NULL)
		*setbitp = setbit;

	return true;
}

static const struct mt6577_sysirq_intrpol_block *
lookup_register_block(struct mt6577_sysirq_softc * const sc, u_int irq)
{
	u_int i;

	for (i = 0; i < sc->sc_nblocks; i++) {
		if (irq >= sc->sc_blocks[i].b_first_irq &&
		    irq <= sc->sc_blocks[i].b_last_irq) {
		    	return &sc->sc_blocks[i];
		}
	}
	return NULL;
}

static void *
mt6577_sysirq_fdt_intr_establish(device_t self, u_int *specifier,
				 int ipl, int flags,
				 int (*func)(void *), void *arg)
{
	struct mt6577_sysirq_softc * const sc = device_private(self);
	u_int inverted_specifier[NCELLS];
	bool setbit;

	if (! invert_specifier(specifier, inverted_specifier, &setbit))
		return NULL;

	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

	const u_int irq = be32toh(inverted_specifier[1]);
	const struct mt6577_sysirq_intrpol_block * const block =
	    lookup_register_block(sc, irq);
	if (block == NULL) {
		/* IRQ out of range. */
		return NULL;
	}

	const u_int irq_off = irq - block->b_first_irq;
	const bus_size_t reg = (irq_off / 32) * sizeof(uint32_t);
	const bus_size_t bit = irq_off % 32;

	mutex_enter(&sc->sc_mutex);
	uint32_t regval = bus_space_read_4(sc->sc_bst, block->b_bsh, reg);
	if (setbit)
		regval |= __BIT(bit);
	else
		regval &= ~__BIT(bit);
	bus_space_write_4(sc->sc_bst, block->b_bsh, reg, regval);
	mutex_exit(&sc->sc_mutex);

	return fdtbus_intr_establish_raw(sc->sc_intr_parent,
					 inverted_specifier, ipl, flags,
					 func, arg);
}

static void
mt6577_sysirq_fdt_intr_disestablish(device_t self __unused, void *arg __unused)
{
	/*
	 * We should never be called, because we explicitly provide our
	 * interrupt-parent to fdtbus_intr_establish_raw(), and thus
	 * the interrupt cookie registered references our parent's
	 * interrupt controller funcs.  Thus our parent's disestablish
	 * function will be called directly, without us having to pass
	 * it through.
	 */
	panic("mt6577_sysirq_fdt_intr_disestablish: why were we called?");
}

static bool
mt6577_sysirq_fdt_intr_intrstr(device_t self, u_int *specifier,
			       char *buf, size_t bufsize)
{
	struct mt6577_sysirq_softc * const sc = device_private(self);
	u_int inverted_specifier[NCELLS];
	char parent_str[64];

	if (! invert_specifier(specifier, inverted_specifier, NULL))
		return false;

	if (! fdtbus_intr_str_raw(sc->sc_intr_parent, inverted_specifier,
				  parent_str, sizeof(parent_str)))
		return false;

	snprintf(buf, bufsize, "%s (via %s)", parent_str, device_xname(self));

	return true;
}
