#	$NetBSD$

DTSSUBDIR=	rockchip
DTSMAKEVARS=	CONFIG_ARCH_ROCKCHIP=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

# Extra .dts files from sys/arch/arm/dts
DTS+=		rk3399-pinebook-pro.dts

.include <bsd.dtb.mk>
