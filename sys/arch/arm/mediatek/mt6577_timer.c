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
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>

#define	GPT_IRQEN		0x0000	/* IRQ enable */
#define	GPT_IRQSTA		0x0004	/* IRQ status */
#define	GPT_IRQACK		0x0008	/* IRQ acknowledge */
#define	GPT1_CON		0x0010	/* GPT 1 control */
#define	GPT1_CLK		0x0014	/* GPT 1 clock setting */
#define	GPT1_COUNT		0x0018	/* GPT 1 counter */
#define	GPT1_COMPARE		0x001c	/* GPT 1 compare */
#define	GPT2_CON		0x0020	/* GPT 2 control */
#define	GPT2_CLK		0x0024	/* GPT 2 clock setting */
#define	GPT2_COUNT		0x0028	/* GPT 2 counter */
#define	GPT2_COMPARE		0x002c	/* GPT 2 compare */
#define	GPT3_CON		0x0030	/* GPT 3 control */
#define	GPT3_CLK		0x0034	/* GPT 3 clock setting */
#define	GPT3_COUNT		0x0038	/* GPT 3 counter */
#define	GPT3_COMPARE		0x003c	/* GPT 3 compare */
#define	GPT4_CON		0x0040	/* GPT 4 control */
#define	GPT4_CLK		0x0044	/* GPT 4 clock setting */
#define	GPT4_COUNT		0x0048	/* GPT 4 counter */
#define	GPT4_COMPARE		0x004c	/* GPT 4 compare */
#define	GPT5_CON		0x0050	/* GPT 5 control */
#define	GPT5_CLK		0x0054	/* GPT 5 clock setting */
#define	GPT5_COUNT		0x0058	/* GPT 5 counter */
#define	GPT5_COMPARE		0x005c	/* GPT 5 compare */
#define	GPT6_CON		0x0060	/* GPT 6 control */
#define	GPT6_CLK		0x0064	/* GPT 6 clock setting */
#define	GPT6_COUNTL		0x0068	/* GPT 6 counter (low) */
#define	GPT6_COMPAREL		0x006c	/* GPT 6 compare (low) */
#define	GPT6_COUNTH		0x0078	/* GPT 6 counter (high) */
#define	GPT6_COMPAREH		0x007c	/* GPT 6 compare (high) */
#define	GPT7_CON		0x008c	/* GPT 7 (secure) control */
#define	GPT7_CLK		0x0090	/* GPT 7 (secure) clock setting */
#define	GPT7_COUNT		0x0094	/* GPT 7 (secure) counter */
#define	GPT7_COMPARE		0x0098	/* GPT 7 (secure) compare */
#define	GPT7_SECURE		0x009c	/* GPT 7 (secore) secure control */
#define	  GPT_SECURE_ON		__BIT(0)
#define	GPT7_IRQEN_SECURE	0x00a0	/* GPT 7 (secure) IRQ enable */
#define	  GPT_IRQEN_SECURE	__BIT(0)
#define	GPT7_IRQACK_SECURE	0x00a0	/* GPT 7 (secure) IRQ acknowledge */
#define	  GPT_IRQACK_SECURE	__BIT(0)
#define	GPT8_CON		0x00a8	/* GPT 8 control */
#define	GPT8_CLK		0x00ac	/* GPT 8 clock setting */
#define	GPT8_COUNT		0x00b0	/* GPT 8 counter */
#define	GPT8_COMPARE		0x00b4	/* GPT 8 compare */
#define	GPT9_CON		0x00b8	/* GPT 9 control */
#define	GPT9_CLK		0x00bc	/* GPT 9 clock setting */
#define	GPT9_COUNT		0x00c0	/* GPT 9 counter */
#define	GPT9_COMPARE		0x00c4	/* GPT 9 compare */
#define	GPT10_CON		0x00c8	/* GPT 10 control */
#define	GPT10_CLK		0x00cc	/* GPT 10 clock setting */
#define	GPT10_COUNT		0x00d0	/* GPT 10 counter */
#define	GPT10_COMPARE		0x00d4	/* GPT 10 compare */

/* GPT control bits */
#define	GPT_CON_EN		__BIT(0)
#define	GPT_CON_CLR		__BIT(1)
#define	GPT_CON_MODE_ONE_SHOT	(0U << 4)
#define	GPT_CON_MODE_REPEAT	(1U << 4)
#define	GPT_CON_MODE_KEEP_GO	(2U << 4)
#define	GPT_CON_MODE_FREERUN	(3U << 4)

/* CLK control bits */
#define	CLK_DIV_1		0
#define	CLK_DIV_2		1
#define	CLK_DIV_3		2
#define	CLK_DIV_4		3
#define	CLK_DIV_5		4
#define	CLK_DIV_6		5
#define	CLK_DIV_7		6
#define	CLK_DIV_8		7
#define	CLK_DIV_9		8
#define	CLK_DIV_10		9
#define	CLK_DIV_11		10
#define	CLK_DIV_12		11
#define	CLK_DIV_13		12
#define	CLK_DIV_16		13
#define	CLK_DIV_32		14
#define	CLK_DIV_64		15
#define	CLK_SOURCE		__BIT(4)	/* 0=system clk, 1=rtc clk */

struct mt6577_timer_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void			*sc_ih;
	uintptr_t		sc_flags;
};

static int	mt6577_timer_match(device_t, cfdata_t, void *);
static void	mt6577_timer_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mt6577_timer, sizeof(struct mt6577_timer_softc),
    mt6577_timer_match, mt6577_timer_attach, NULL, NULL);

#define	MTK_TMR_F_GPT6_GTMR		__BIT(0)

static const struct of_compat_data compat_data[] = {
	{ "mediatek,mt7623-timer",	MTK_TMR_F_GPT6_GTMR },
	{ "mediatek,mt6577-timer",	0 },
	{ NULL }
};

static int	mt6577_timer_intr(void *);

static int
mt6577_timer_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
mt6577_timer_attach(device_t parent, device_t self, void *aux)
{
	struct mt6577_timer_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	bus_addr_t addr;
	bus_size_t size;

	int error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": unable to get device address\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": unable to map device\n");
		return;
	}

	/* Enable clocks. */
	struct clk *clk;
	for (int i = 0; (clk = fdtbus_clock_get_index(phandle, i)); i++) {
		if (clk_enable(clk) != 0) {
			aprint_error(": failed to enable clock #%d\n", i);
			return;
		}
	}

	aprint_naive("\n");
	aprint_normal(": General Purpose Timers\n");

	sc->sc_flags = of_search_compatible(phandle, compat_data)->data;

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_SERIAL,
	    FDT_INTR_MPSAFE, mt6577_timer_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	if (ISSET(sc->sc_flags, MTK_TMR_F_GPT6_GTMR)) {
		/*
		 * GPT6 is the counter source for the ARMv7 architected timer.
		 * Enable it in the appropriate mode for the "gtmr" driver.
		 */
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, GPT6_CON,
		    GPT_CON_MODE_FREERUN | GPT_CON_EN);
	}
}

static int
mt6577_timer_intr(void *v)
{

	/* This driver doesn't do anything yet. */
	panic("mt6577_timer_intr");
}
