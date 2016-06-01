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

#include <sys/bus.h>

#include <arm/mainbus/mainbus.h>
#include <arm/broadcom/bcm2835var.h>

#include <dev/fdt/fdtvar.h>
#include <dev/ofw/openfirm.h>

static int	bcmfdt_match(device_t, cfdata_t, void *);
static void	bcmfdt_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bcmfdt, 0,
    bcmfdt_match, bcmfdt_attach, NULL, NULL);

static bool bcmfdt_found = false;

int
bcmfdt_match(device_t parent, cfdata_t cf, void *aux)
{
	if (bcmfdt_found)
		return 0;
	return 1;
}

void
bcmfdt_attach(device_t parent, device_t self, void *aux)
{
	const char *bcmfdt_init[] = {
		"interrupt-controller",
		"timer",
		"gpio",
		"clocks",
		"mailbox",
		"firmware",
		"power",
		"dma"
	};

	bcmfdt_found = true;

	aprint_naive("\n");
	aprint_normal("\n");

	struct fdt_attach_args faa = {
		.faa_name = "",
		.faa_bst = &bcm2835_bs_tag,
		.faa_a4x_bst = &bcm2835_a4x_bs_tag,
		.faa_dmat = &bcm2835_bus_dma_tag,
		.faa_phandle = OF_peer(0),
		.faa_init = bcmfdt_init,
		.faa_ninit = __arraycount(bcmfdt_init)
	};
	config_found(self, &faa, NULL);
}
