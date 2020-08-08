/*	$NetBSD$	*/

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

#include "opt_soc.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

#include <uvm/uvm_extern.h>

#include <arm/mediatek/mediatek_platform.h>
#include <arm/mediatek/mtk_mc_smp.h>

extern struct bus_space arm_generic_bs_tag;

#define	MTK_MAX_CORES	4

struct mtk_mpstart_data {
	bus_size_t	mpstart_regs_offset;	/* offset from SoC VBASE */
	bus_size_t	mpstart_pc_offset;
	struct {
		bus_size_t	core_offset;
		uint32_t	core_key;
	} mpstart_core_info[MTK_MAX_CORES];
};

static const struct mtk_mpstart_data *mpstart_data;

static uint32_t
mtk_mc_smp_pa(void)
{
	bool ok __diagused;
	paddr_t pa;

	ok = pmap_extract(pmap_kernel(), (vaddr_t)cpu_mpstart, &pa);
	KASSERT(ok);

	return pa;
}

static int
mtk_cpu_enable(int phandle)
{
	bus_space_tag_t bst = &arm_generic_bs_tag;
	uint64_t mpidr;

	fdtbus_get_reg64(phandle, 0, &mpidr, NULL);

	const u_int cpuno = __SHIFTOUT(mpidr, MPIDR_AFF0);

	if (cpuno >= MTK_MAX_CORES ||
	    mpstart_data == NULL ||
	    mpstart_data->mpstart_core_info[cpuno].core_key == 0)
	    	return EINVAL;

	const bus_space_handle_t bsh = 
	    MEDIATEK_CORE_VBASE + mpstart_data->mpstart_regs_offset;

	bus_space_write_4(bst, bsh,
	    mpstart_data->mpstart_pc_offset, mtk_mc_smp_pa());
	bus_space_write_4(bst, bsh,
	    mpstart_data->mpstart_core_info[cpuno].core_offset,
	    mpstart_data->mpstart_core_info[cpuno].core_key);

	return 0;
}

ARM_CPU_METHOD(mt6589, "mediatek,mt6589-smp", mtk_cpu_enable);

#if defined(SOC_MT7623)
static const struct mtk_mpstart_data mt7623_mpstart_data = {
	.mpstart_regs_offset		= 0x00202000,
	.mpstart_pc_offset		= 0x34,
	.mpstart_core_info = {
		[1]	=	{
			.core_offset	= 0x38,
			.core_key	= 0x534c4131,
		},
		[2]	=	{
			.core_offset	= 0x3c,
			.core_key	= 0x4c415332,
		},
		[3]	=	{
			.core_offset	= 0x40,
			.core_key	= 0x41534c33,
		},
	},
};
int
mtk_mt7623_mpstart(void)
{

	mpstart_data = &mt7623_mpstart_data;
	return 0; // arm_fdt_cpu_mpstart();
}
#endif /* SOC_MT7623 */
