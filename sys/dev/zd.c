/* $NetBSD$ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/vmem.h>
#include <sys/lwp.h>
#include <sys/cpu.h>
#include <sys/evcnt.h>
#include <sys/disk.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/disklabel.h>
#include <sys/module.h>

#include <dev/dkvar.h>

#include <net/zlib.h>

#include "ioconf.h"

/* Maximum region size to pre-allocate */
#define	ZD_REGION_MAXSIZE	(1 * 1024 * 1024)

/* Size of temporary per-cpu compresison buffer */
#define	ZD_BUFFER_SIZE		(6 + DEV_BSIZE + 5)

/* Compression level */
#define	ZD_COMPRESSION_LEVEL	Z_DEFAULT_COMPRESSION

struct zd_region {
	void			*zr_data;
	size_t			zr_datalen;
	LIST_ENTRY(zd_region)	zr_entries;
};

struct zd_percpu {
	z_stream		zp_deflate;
	z_stream		zp_inflate;
	uint8_t			zp_buffer[ZD_BUFFER_SIZE];
};

struct zd_block {
	void			*zb_data;
	uint16_t		zb_datalen;
};

struct zd_softc {
	struct dk_softc		sc_dk;
	kmutex_t		sc_lock;

	vmem_t			*sc_vmem;
	LIST_HEAD(, zd_region)	sc_regions;
	struct zd_block		*sc_blkmap;
	size_t			sc_blkmapsz;

	struct zd_percpu	*sc_percpu;

	struct evcnt		sc_ev_blkalloc;
	struct evcnt		sc_ev_blkfree;
	struct evcnt		sc_ev_blkreuse;
	struct evcnt		sc_ev_blkzero;
	struct evcnt		sc_ev_blkerr;
	struct evcnt		sc_ev_zused;
};

static dev_type_open(zdopen);
static dev_type_close(zdclose);
static dev_type_read(zdread);
static dev_type_write(zdwrite);
static dev_type_ioctl(zdioctl);
static dev_type_strategy(zdstrategy);
static dev_type_size(zdsize);

const struct bdevsw zd_bdevsw = {
	.d_open = zdopen,
	.d_close = zdclose,
	.d_strategy = zdstrategy,
	.d_ioctl = zdioctl,
	.d_dump = nodump,
	.d_psize = zdsize,
	.d_discard = nodiscard,
	.d_flag = D_DISK,
};

const struct cdevsw zd_cdevsw = {
	.d_open = zdopen,
	.d_close = zdclose,
	.d_read = zdread,
	.d_write = zdwrite,
	.d_ioctl = zdioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_DISK,
};

static int	zddiskstart(device_t, struct buf *);
static void	zdlabel(device_t, struct disklabel *);

static const struct dkdriver zd_dkdriver = {
	.d_strategy = zdstrategy,
	.d_minphys = minphys,
	.d_open = zdopen,
	.d_close = zdclose,
	.d_diskstart = zddiskstart,
	.d_iosize = NULL,
	.d_dumpblocks = NULL,
	.d_lastclose = NULL,
	.d_discard = NULL,
	.d_firstopen = NULL,
	.d_label = zdlabel,
};

static int	zd_match(device_t, cfdata_t, void *);
static void	zd_attach(device_t, device_t, void *);
static int	zd_detach(device_t, int);

static int	zd_init_zlib(struct zd_softc *);
static void	zd_fini_zlib(struct zd_softc *);
static void *	zd_alloc_zlib(void *, u_int, u_int);
static void	zd_free_zlib(void *, void *);
static int	zd_alloc_regions(struct zd_softc *, struct disk_geom *);
static void	zd_free_regions(struct zd_softc *);
static void	zd_get_geom(struct zd_softc *, struct disk_geom *);

static int	zd_compress(struct zd_softc *, daddr_t, void *, size_t);
static int	zd_decompress(struct zd_softc *, daddr_t, void *, size_t);

CFATTACH_DECL_NEW(zd, sizeof(struct zd_softc),
    zd_match, zd_attach, zd_detach, NULL);

static device_t zd_instance = NULL;

void
zdattach(int num)
{
	cfdata_t cf;

#ifndef _MODULE
	int error;

	error = config_cfattach_attach(zd_cd.cd_name, &zd_ca);
	if (error != 0) {
		aprint_error("%s: failed to add cfattach: %d\n",
		    zd_cd.cd_name, error);
	}
#endif

	cf = kmem_zalloc(sizeof(*cf), KM_SLEEP);
	cf->cf_name = zd_cd.cd_name;
	cf->cf_atname = zd_cd.cd_name;
	cf->cf_unit = 0;
	cf->cf_fstate = FSTATE_STAR;

	KASSERT(zd_instance == NULL);
	zd_instance = config_attach_pseudo(cf);
}

static int
zd_match(device_t parent, cfdata_t cfdata, void *aux)
{
	return 1;
}

static void
zd_attach(device_t parent, device_t self, void *aux)
{
	struct zd_softc * const sc = device_private(self);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_BIO);
	LIST_INIT(&sc->sc_regions);

	if (zd_init_zlib(sc) != 0) {
		aprint_error(": zlib init failed\n");
		return;
	}

	evcnt_attach_dynamic(&sc->sc_ev_blkalloc, EVCNT_TYPE_MISC, NULL,
	    device_xname(self), "allocated blocks");
	evcnt_attach_dynamic(&sc->sc_ev_blkfree, EVCNT_TYPE_MISC, NULL,
	    device_xname(self), "freed blocks");
	evcnt_attach_dynamic(&sc->sc_ev_blkreuse, EVCNT_TYPE_MISC, NULL,
	    device_xname(self), "reused blocks");
	evcnt_attach_dynamic(&sc->sc_ev_blkzero, EVCNT_TYPE_MISC, NULL,
	    device_xname(self), "unallocated block reads");
	evcnt_attach_dynamic(&sc->sc_ev_blkerr, EVCNT_TYPE_MISC, NULL,
	    device_xname(self), "block allocation errors");
	evcnt_attach_dynamic(&sc->sc_ev_zused, EVCNT_TYPE_MISC, NULL,
	    device_xname(self), "compressed data size");

	dk_init(&sc->sc_dk, self, DKTYPE_ZD);
	disk_init(&sc->sc_dk.sc_dkdev, sc->sc_dk.sc_xname, &zd_dkdriver);
	bufq_alloc(&sc->sc_dk.sc_bufq, "fcfs", 0);

	zd_get_geom(sc, &sc->sc_dk.sc_dkdev.dk_geom);

	if (zd_alloc_regions(sc, &sc->sc_dk.sc_dkdev.dk_geom) != 0) {
		aprint_error_dev(sc->sc_dk.sc_dev, "not enough memory\n");
		return;
	}

	dk_attach(&sc->sc_dk);
	disk_attach(&sc->sc_dk.sc_dkdev);
	disk_set_info(self, &sc->sc_dk.sc_dkdev, "zd");

	pmf_device_register(self, NULL, NULL);

	aprint_normal_dev(self, "%" PRId64 " bytes\n",
	    sc->sc_dk.sc_dkdev.dk_geom.dg_secperunit * DEV_BSIZE);
}

static int
zd_detach(device_t self, int flags)
{
	struct zd_softc * const sc = device_private(self);
	int error;

	error = disk_begindetach(&sc->sc_dk.sc_dkdev, NULL, self, flags);
	if (error != 0)
		return error;

	dk_drain(&sc->sc_dk);
	bufq_free(sc->sc_dk.sc_bufq);

	disk_detach(&sc->sc_dk.sc_dkdev);
	disk_destroy(&sc->sc_dk.sc_dkdev);
	dk_detach(&sc->sc_dk);

	pmf_device_deregister(self);

	zd_free_regions(sc);
	zd_fini_zlib(sc);

	evcnt_detach(&sc->sc_ev_blkalloc);
	evcnt_detach(&sc->sc_ev_blkfree);
	evcnt_detach(&sc->sc_ev_blkreuse);
	evcnt_detach(&sc->sc_ev_blkzero);
	evcnt_detach(&sc->sc_ev_blkerr);
	evcnt_detach(&sc->sc_ev_zused);

	if (!ISSET(flags, DETACH_QUIET))
		aprint_normal_dev(self, "detached");

	return 0;
}

static int
zd_init_zlib(struct zd_softc *sc)
{
	struct zd_percpu *zp;
	int error;
	u_int n;

	sc->sc_percpu = kmem_zalloc(sizeof(*sc->sc_percpu) * ncpu, KM_SLEEP);
	for (n = 0; n < ncpu; n++) {
		zp = &sc->sc_percpu[n];
		zp->zp_deflate.zalloc = zd_alloc_zlib;
		zp->zp_deflate.zfree = zd_free_zlib;
		error = deflateInit2(&zp->zp_deflate, ZD_COMPRESSION_LEVEL,
		    8, MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
		if (error != Z_OK)
			return ENXIO;
		zp->zp_inflate.zalloc = zd_alloc_zlib;
		zp->zp_inflate.zfree = zd_free_zlib;
		error = inflateInit2(&zp->zp_inflate, MAX_WBITS);
		if (error != Z_OK)
			return ENXIO;
	}

	return 0;
}

static void
zd_fini_zlib(struct zd_softc *sc)
{
	struct zd_percpu *zp;
	u_int n;

	if (sc->sc_percpu == NULL)
		return;

	for (n = 0; n < ncpu; n++) {
		zp = &sc->sc_percpu[n];
		deflateEnd(&zp->zp_deflate);
		inflateEnd(&zp->zp_inflate);
	}

	kmem_free(sc->sc_percpu, sizeof(*sc->sc_percpu) * ncpu);
	sc->sc_percpu = NULL;
}

static void *
zd_alloc_zlib(void *priv, u_int items, u_int size)
{
	return malloc(items * size, M_DEVBUF, M_NOWAIT);
}

static void
zd_free_zlib(void *priv, void *ptr)
{
	free(ptr, M_DEVBUF);
}

static int
zd_alloc_regions(struct zd_softc *sc, struct disk_geom *dg)
{
	size_t resid = dg->dg_secperunit * DEV_BSIZE;
	size_t allocsz = ZD_REGION_MAXSIZE;
	struct zd_region *zr;
	size_t datalen;
	void *data;
	int error;

	KASSERT(LIST_EMPTY(&sc->sc_regions));
	KASSERT(sc->sc_vmem == NULL);

	sc->sc_vmem = vmem_create(sc->sc_dk.sc_xname, 0, 0, 1,
	    NULL, NULL, NULL, 0, VM_SLEEP, IPL_BIO);
	if (sc->sc_vmem == NULL)
		return ENOMEM;

	while (resid >= DEV_BSIZE) {
		datalen = ulmin(allocsz, resid);
		data = kmem_alloc(datalen, KM_NOSLEEP);
		if (data == NULL) {
			/* Shrink region size and try again */
			allocsz >>= 1;
			continue;
		}

		zr = kmem_alloc(sizeof(*zr), KM_SLEEP);
		zr->zr_data = data;
		zr->zr_datalen = datalen;
		LIST_INSERT_HEAD(&sc->sc_regions, zr, zr_entries);

		error = vmem_add(sc->sc_vmem, (vmem_addr_t)data,
		    (vmem_size_t)datalen, VM_SLEEP);
		if (error != 0)
			return error;

		resid -= datalen;
	}

	if (resid > 0)
		return ENOMEM;

	sc->sc_blkmapsz = sizeof(struct zd_block) * dg->dg_secperunit;
	sc->sc_blkmap = kmem_zalloc(sc->sc_blkmapsz, KM_SLEEP);

	return 0;
}

static void
zd_free_regions(struct zd_softc *sc)
{
	struct zd_region *zr;

	while ((zr = LIST_FIRST(&sc->sc_regions)) != NULL) {
		LIST_REMOVE(zr, zr_entries);
		kmem_free(zr->zr_data, zr->zr_datalen);
		kmem_free(zr, sizeof(*zr));
	}

	if (sc->sc_vmem != NULL) {
		vmem_destroy(sc->sc_vmem);
		sc->sc_vmem = NULL;
	}

	if (sc->sc_blkmap != NULL) {
		kmem_free(sc->sc_blkmap, sc->sc_blkmapsz);
		sc->sc_blkmap = NULL;
		sc->sc_blkmapsz = 0;
	}
}

static void
zd_get_geom(struct zd_softc *sc, struct disk_geom *dg)
{
	/* XXX */
#define	ZD_DISK_SIZE	roundup(50 * 1000 * 1000, PAGE_SIZE)
	memset(dg, 0, sizeof(*dg));
	dg->dg_secperunit = ZD_DISK_SIZE / DEV_BSIZE;
	dg->dg_secsize = DEV_BSIZE << sc->sc_dk.sc_dkdev.dk_blkshift;
	dg->dg_ntracks = 1;
	dg->dg_nsectors = 1024 * 1024 / dg->dg_secsize;
	dg->dg_ncylinders = dg->dg_secperunit / dg->dg_nsectors;
}

static int
zd_compress(struct zd_softc *sc, daddr_t blkno, void *data, size_t datalen)
{
	struct zd_percpu *zp;
	struct zd_block *zb = &sc->sc_blkmap[blkno];
	int bound, error;
	vmem_addr_t va;
	vmem_size_t len;

	KASSERT(datalen == DEV_BSIZE);

	/* Bind LWP while using per-CPU zlib state */
	bound = curlwp_bind();

	zp = &sc->sc_percpu[cpu_index(curcpu())];
	zp->zp_deflate.next_in = data;
	zp->zp_deflate.avail_in = datalen;
	zp->zp_deflate.total_in = 0;
	zp->zp_deflate.next_out = zp->zp_buffer;
	zp->zp_deflate.avail_out = sizeof(zp->zp_buffer);
	zp->zp_deflate.total_out = 0;
	deflateReset(&zp->zp_deflate);
	error = deflate(&zp->zp_deflate, Z_FINISH);
	KASSERT(error == Z_STREAM_END);

	len = zp->zp_deflate.total_out;

	if (zb->zb_data != NULL && zb->zb_datalen != len) {
		/* Free the block */
		vmem_free(sc->sc_vmem, (vmem_addr_t)zb->zb_data,
		    zb->zb_datalen);
		sc->sc_ev_zused.ev_count -= zb->zb_datalen;
		zb->zb_data = NULL;
		zb->zb_datalen = 0;
		sc->sc_ev_blkfree.ev_count++;
	}

	if (zb->zb_data != NULL) {
		sc->sc_ev_blkreuse.ev_count++;
		error = 0;
	} else {
		error = vmem_alloc(sc->sc_vmem, len, VM_BESTFIT | VM_NOSLEEP,
		    &va);
		if (error == 0) {
			zb->zb_data = (void *)va;
			zb->zb_datalen = (uint16_t)len;
			sc->sc_ev_zused.ev_count += zb->zb_datalen;
			sc->sc_ev_blkalloc.ev_count++;
		} else {
			sc->sc_ev_blkerr.ev_count++;
		}
	}

	if (error == 0)
		memcpy(zb->zb_data, zp->zp_buffer, zb->zb_datalen);

	curlwp_bindx(bound);

	return error;
}

static int
zd_decompress(struct zd_softc *sc, daddr_t blkno, void *data, size_t datalen)
{
	struct zd_percpu *zp;
	struct zd_block *zb = &sc->sc_blkmap[blkno];
	int bound, error;

	/* Bind LWP while using per-CPU zlib state */
	bound = curlwp_bind();

	if (zb->zb_data == NULL) {
		/* No data has been written here yet, fill with zeros */
		sc->sc_ev_blkzero.ev_count++;
		memset(data, 0, datalen);
		return 0;
	}

	zp = &sc->sc_percpu[cpu_index(curcpu())];

	zp->zp_inflate.next_in = zb->zb_data;
	zp->zp_inflate.avail_in = zb->zb_datalen;
	zp->zp_inflate.total_in = 0;
	zp->zp_inflate.next_out = data;
	zp->zp_inflate.avail_out = datalen;
	zp->zp_inflate.total_out = 0;
	inflateReset(&zp->zp_inflate);
	error = inflate(&zp->zp_inflate, Z_FINISH);
	KASSERT(error == Z_STREAM_END);
	KASSERT(zp->zp_inflate.total_out == datalen);

	curlwp_bindx(bound);

	return 0;
}

static int
zdopen(dev_t dev, int flags, int fmt, lwp_t *l)
{
	struct zd_softc *sc;

	sc = device_lookup_private(&zd_cd, DISKUNIT(dev));
	if (sc == NULL)
		return ENXIO;

	return dk_open(&sc->sc_dk, dev, flags, fmt, l);
}

static int
zdclose(dev_t dev, int flags, int fmt, lwp_t *l)
{
	struct zd_softc *sc;

	sc = device_lookup_private(&zd_cd, DISKUNIT(dev));
	if (sc == NULL)
		return ENXIO;

	return dk_close(&sc->sc_dk, dev, flags, fmt, l);
}

static int
zdread(dev_t dev, struct uio *uio, int flags)
{
	struct zd_softc *sc;

	sc = device_lookup_private(&zd_cd, DISKUNIT(dev));
	if (sc == NULL)
		return ENXIO;

	return physio(zdstrategy, NULL, dev, B_READ, minphys, uio);
}

static int
zdwrite(dev_t dev, struct uio *uio, int flags)
{
	struct zd_softc *sc;

	sc = device_lookup_private(&zd_cd, DISKUNIT(dev));
	if (sc == NULL)
		return ENXIO;

	return physio(zdstrategy, NULL, dev, B_WRITE, minphys, uio);
}

static int
zdioctl(dev_t dev, u_long cmd, void *data, int flags, lwp_t *l)
{
	struct zd_softc *sc;
	int error;

printf("%s: dev=%#lx cmd=%#lx\n", __func__, dev, cmd);

	sc = device_lookup_private(&zd_cd, DISKUNIT(dev));

printf("%s: sc=%p\n", __func__, sc);
	if (sc == NULL)
		return ENXIO;

	switch (cmd) {
	case DIOCGCACHE:
	case DIOCCACHESYNC:
		/* Nothing to do here */
		error = 0;
		break;

	default:
		error = dk_ioctl(&sc->sc_dk, dev, cmd, data, flags, l);
		break;
	}

	return error;
}

static void
zdstrategy(struct buf *bp)
{
	struct zd_softc *sc;

	sc = device_lookup_private(&zd_cd, DISKUNIT(bp->b_dev));
	KASSERT(sc != NULL);

	dk_strategy(&sc->sc_dk, bp);
}

static int
zdsize(dev_t dev)
{
	struct zd_softc *sc;

	sc = device_lookup_private(&zd_cd, DISKUNIT(dev));
	if (sc == NULL)
		return -1;

	return dk_size(&sc->sc_dk, dev);
}

static int
zddiskstart(device_t dev, struct buf *bp)
{
	struct zd_softc * const sc = device_private(dev);
	uint8_t *p;
	int error;
	daddr_t blkno;

	if ((bp->b_bcount & (DEV_BSIZE - 1)) != 0) {
		/* Transfer must be a multiple of DEV_BSIZE */
		return EIO;
	}

	bp->b_resid = bp->b_bcount;
	p = bp->b_data;
	blkno = bp->b_blkno;
	while (bp->b_resid > 0) {
		if (ISSET(bp->b_flags, B_READ))
			error = zd_decompress(sc, blkno, p, DEV_BSIZE);
		else
			error = zd_compress(sc, blkno, p, DEV_BSIZE);
		if (error != 0)
			break;
		blkno++;
		p += DEV_BSIZE;
		bp->b_resid -= DEV_BSIZE;
	}

	if (error == 0)
		biodone(bp);

	return error;
}

static void
zdlabel(device_t dev, struct disklabel *lp)
{
	lp->d_partitions[RAW_PART].p_fstype = FS_SWAP;
}

MODULE(MODULE_CLASS_DRIVER, zd, "zlib");

#ifdef _MODULE
int zd_bmajor = -1, zd_cmajor = -1;

CFDRIVER_DECL(zd, DV_DISK, NULL);
#endif

static int
zd_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_cfdriver_attach(&zd_cd);
		if (error != 0)
			break;
		error = config_cfattach_attach(zd_cd.cd_name, &zd_ca);
		if (error != 0) {
			config_cfdriver_detach(&zd_cd);
			break;
		}
		error = devsw_attach("zd", &zd_bdevsw, &zd_bmajor,
		    &zd_cdevsw, &zd_cmajor);
		if (error != 0) {
			config_cfattach_detach(zd_cd.cd_name, &zd_ca);
			config_cfdriver_detach(&zd_cd);
			break;
		}
		zdattach(0);
#endif

		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		devsw_detach(&zd_bdevsw, &zd_cdevsw);
		error = config_cfattach_detach(zd_cd.cd_name, &zd_ca);
		if (error != 0) {
			devsw_attach("zd", &zd_bdevsw, &zd_bmajor,
			    &zd_cdevsw, &zd_cmajor);
			break;
		}
		error = config_cfdriver_detach(&zd_cd);
		if (error != 0) {
			config_cfattach_attach(zd_cd.cd_name, &zd_ca);
			devsw_attach("zd", &zd_bdevsw, &zd_bmajor,
			    &zd_cdevsw, &zd_cmajor);
			break;
		}

		if (zd_instance != NULL) {
			config_detach(zd_instance, DETACH_FORCE);
			zd_instance = NULL;
		}
#endif
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
}
