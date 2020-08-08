/* $NetBSD$ */

/*-
 * Copyright (c) 2019 Jason R Thorpe
 * All rights reserved.
 */

/*
 * Driver for the MediaTek MT2701-compatible Ethernet controller.
 *
 * This Ethernet core is very similar to the Ralink MIPS-based SoC
 * Ethernet block due to MediaTek's acquisition of Ralink and
 * subsequent use of those functional blocks in thedir ARM-based SoCs.
 *
 * The interface is broken up into two distinct functional blocks:
 *
 * - The "frame engine", which controls most of the logic, interfacing
 *   with host memory, padding short frames, VLAN encapsulation, checksum
 *   and segmentation offload, etc.
 *
 * - The individual GMACs that represent the actual physical ports.  There
 *   may be more than one GMAC, and they all share a frame engine.  On the
 *   MT2701 type interfaces, there are 2 GMACs.
 *
 * Because of this arrangement, we have the following device hiererchy:
 *
 *	mt2701fe			"frame engine" layer
 *		mtge			ifnet layer
 *			mii/phy		MII PHY / switch handling
 *		mtge
 *			mii/phy
 *
 * "mtge" provides the ifnet interface logic and queueing for that
 * interface, but call into the "mt2701fe" layer to put those packets
 * onto the wire.
 *
 * Despite the fact that the GMACs share a frame engine, we are not entirely
 * bound to sharing everything, because the frame engine has 4 transmit and
 * receive rings.  We can thus give each GMAC its own set of descriptor rings.
 *
 * The MT2701's frame engine can do a numnber of nifty things, including:
 *
 * - TCP segmentation offload
 * - TCP / UDP / IP checksum offload
 * - VLAN tagging / untagging
 * - PPPoE encapsulation
 *
 * Of these, we support:
 *
 * - XXX NADA
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net/bpf.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/mii/miivar.h>

#include <arm/mediatek/mt2701_ethreg.h>

#define	NTGE_TXQUEUELEN		256
#define	MTGE_NTXSEGS		1	/* XXX Double-check Linux driver */
#define	MTGE_NTXDESC		(NTGE_TXQUEUELEN * MTGE_NTXSEGS)
#define	MTGE_NTXDESC_MASK	(MTGE_NTXDESC - 1)
#define	MTGE_NEXTTX(x)		(((x) + 1) & MTGE_NTXDESC_MASK)

#define	MTGE_NRXDESC		256
#define	MTGE_NRXDESC_MASK	(MTGE_NRXDESC - 1)
#define	MTGE_NEXTRX(x)		(((x) + 1) & MTGE_NRXDESC_MASK)

/*
 * Control structures DMA'd to the frame engine.  Allocate them in a
 * single clump that maps to a single DMA segment to make several things
 * easier, and allows us to control alignment.
 */
struct mtge_control_data {
	struct fe_tx_desc	cd_txdescs[MTGE_NTXDESC];
	struct fe_rx_desc	cd_rxdescs[MTGE_NRXDESC];
};

#define	MTGE_CDOFF(x)		offsetof(struct stge_control_data, x)
#define	MTGE_CDTXOFF(x)		MTGE_CDOFF(cd_txdescs[(x)])
#define	MTGE_CDRXOFF(x)		MTGE_CDOFF(cd_rxdescs[(x)])

/*
 * Software state for transmit job.
 */
struct mtge_txstate {
	struct mbuf *txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
	int txs_firstdesc;		/* first descriptor in packet */
	int txs_lastdesc;		/* last descriptor in packet */
	SIMPLEQ_ENTRY(sip_txsoft) txs_q;
};

SIMPLEQ_HEAD(mtge_txsq, mtge_txstate);

/*
 * Software state for receive jobs.
 */
struct mtge_rxstate {
	struct mbuf *rxs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t rxs_dmamap;	/* our DMA map */
};

/*
 * Frame Engine software state.
 */
struct fe_softc {
	device_t	sc_dev;

	bus_space_tag_t sc_st;
	bus_space_handle_t sc_sh;

	bus_dma_tag_t sc_dmat;

};
