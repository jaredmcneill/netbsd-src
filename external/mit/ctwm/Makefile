#	$NetBSD: Makefile,v 1.4 2020/09/24 06:52:48 nia Exp $

# Top level Makefile to cross build xorg inside the NetBSD tree.

.include <bsd.own.mk>

SUBDIR=		bin libexec

.if ${MKUPDATE} == "no" && !defined(NOCLEANDIR)
BUILDTARGETS+=  cleandir
.endif
.if ${MKOBJDIRS} != "no"
BUILDTARGETS+=  obj
.endif
BUILDTARGETS+=	do-build

.ORDER: ${BUILDTARGETS}

do-build:
.for targ in dependall install
	${MAKEDIRTARGET} . ${targ}
.endfor

.for dir in	tools lib
do-${dir:S/\//-/g}:
.  for targ in dependall install
	${MAKEDIRTARGET} ${dir} ${targ}
.  endfor
.endfor


DISTRIBTARGETS=	\
		bin/ctwm

.include <bsd.subdir.mk>
