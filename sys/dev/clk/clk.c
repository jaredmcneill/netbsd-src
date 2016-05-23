/* $NetBSD: clk.c,v 1.1 2015/12/05 13:31:07 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: clk.c,v 1.1 2015/12/05 13:31:07 jmcneill Exp $");

#include <sys/param.h>
#include <sys/kmem.h>

#include <dev/clk/clk.h>
#include <dev/clk/clk_backend.h>

struct clk_backend {
	device_t dev;
	const struct clk_funcs *funcs;
	void *priv;
	struct clk_backend *next;
};

static struct clk_backend *clk_backends = NULL;

int
clk_backend_register(device_t dev, const struct clk_funcs *funcs, void *priv)
{
	struct clk_backend *cb;

	cb = kmem_zalloc(sizeof(*cb), KM_SLEEP);
	cb->dev = dev;
	cb->funcs = funcs;
	cb->priv = priv;
	cb->next = clk_backends;
	clk_backends = cb;

	return 0;
}

struct clk *
clk_get(device_t dev, const char *name)
{
	struct clk_backend *cb;
	struct clk *clk;

	for (cb = clk_backends; cb; cb = cb->next) {
		if (dev == NULL || dev == cb->dev) {
			clk = cb->funcs->get(cb->priv, name);
			if (clk != NULL) {
				KASSERT(clk->cb == NULL || clk->cb == cb);
				clk->cb = cb;
				return clk;
			}
		}
	}

	return NULL;
}

void
clk_put(struct clk *clk)
{
	return clk->cb->funcs->put(clk->cb->priv, clk);
}

u_int
clk_get_rate(struct clk *clk)
{
	return clk->cb->funcs->get_rate(clk->cb->priv, clk);
}

int
clk_set_rate(struct clk *clk, u_int rate)
{
	if (clk->flags & CLK_SET_RATE_PARENT) {
		return clk_set_rate(clk_get_parent(clk), rate);
	} else {
		return clk->cb->funcs->set_rate(clk->cb->priv, clk, rate);
	}
}

int
clk_enable(struct clk *clk)
{
	return clk->cb->funcs->enable(clk->cb->priv, clk);
}

int
clk_disable(struct clk *clk)
{
	return clk->cb->funcs->disable(clk->cb->priv, clk);
}

int
clk_set_parent(struct clk *clk, struct clk *parent_clk)
{
	return clk->cb->funcs->set_parent(clk->cb->priv, clk, parent_clk);
}

struct clk *
clk_get_parent(struct clk *clk)
{
	return clk->cb->funcs->get_parent(clk->cb->priv, clk);
}
