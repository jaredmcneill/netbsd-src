#	$NetBSD: Makefile.playstation2.inc,v 1.10 2016/03/28 10:31:45 martin Exp $

CFLAGS+= -mabi=32 -march=r5900 -fno-pic -msoft-float
AFLAGS+= -mabi=32 -march=r5900 -fno-pic -msoft-float
