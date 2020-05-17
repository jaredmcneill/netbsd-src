#	$NetBSD$

TARGETS+=	dtblist

.include <bsd.own.mk>

.if !defined(TARGET_ARCH)
.  if !empty(MACHINE_ARCH:Mearmv[67]*)
TARGET_ARCH=	arm
.  endif
.  if ${MACHINE_ARCH} == "aarch64"
TARGET_ARCH=	arm64
.  endif
.endif

SUBDIR+=	${TARGET_ARCH}

.include <bsd.subdir.mk>

# Regenerate MD set lists in distrib/sets/lists/dtb. Run this whenever
# new dts files are imported.
update-sets:
	${MAKE} TARGET_ARCH=arm dtblist | grep '^[\.#]' | column -t \
	    >${.CURDIR}/../../distrib/sets/lists/dtb/ad.arm
	${MAKE} TARGET_ARCH=arm64 dtblist | grep '^[\.#]' | column -t \
	    >${.CURDIR}/../../distrib/sets/lists/dtb/ad.aarch64
