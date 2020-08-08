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

#ifndef _ARM_MEDIATEK_MTK_EINTC_H_
#define _ARM_MEDIATEK_MTK_EINTC_H_

#define	MTK_EINT_REG_BANK(x)		((x) * 4)
#define	MTK_EINT_REG_OFF(b, x)		((b) + MTK_EINT_REG_BANK(x))

struct mtk_eintc_reg_group {
	bus_size_t	base;		/* base registers */
	bus_size_t	set_base;	/* base for setting bits */
	bus_size_t	clr_base;	/* base for clearing bits */
};

#define	MTK_EINTC_REGS_STA_ACK		0	/* status / ack */
#define	MTK_EINTC_REGS_MASK		1	/* mask */
#define	MTK_EINTC_REGS_SENS		2	/* sensitivity */
#define	MTK_EINTC_REGS_SOFT		3	/* software interrupts */
#define	MTK_EINTC_REGS_POL		4	/* interrupt polarity */
#define	MTK_EINTC_NREGS			5

struct mtk_eintc_config {
	u_int nintrs;
	struct mtk_eintc_reg_group reg_groups[MTK_EINTC_NREGS];
	uint16_t nbanks;
	uint16_t irqs_per_bank;
	bus_size_t domen_base;
};

struct mtk_eintc_intr {
	int		(*intr_func)(void *);
	void		*intr_arg;
	uint16_t	intr_type;	/* FDT_INTR_TYPE_* */
	uint16_t	intr_flags;
	int		intr_debounce;
};

#define	MTK_EINTC_INTR_MPSAFE		__BIT(0)

struct mtk_eintc_softc {
	/* Container device initializes these... */
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	const struct mtk_eintc_config *sc_eintc;

	/* mtk_eintc_init() initializes these... */
	kmutex_t		sc_mutex;
	struct mtk_eintc_intr	*sc_intrs;
};

#define	EINTC_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	EINTC_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

void	mtk_eintc_init(struct mtk_eintc_softc * const);
int	mtk_eintc_intr(void *);
void *	mtk_eintc_intr_enable(struct mtk_eintc_softc * const,
			      int (*)(void *), void *, const int,
			      const u_int, const u_int, const u_int);
void	mtk_eintc_intr_disable(struct mtk_eintc_softc * const,
			       struct mtk_eintc_intr * const);

#endif /* _ARM_MEDIATEK_MTK_EINTC_H_ */
