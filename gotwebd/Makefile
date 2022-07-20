.PATH:${.CURDIR}/../lib

SUBDIR = libexec

.include "../got-version.mk"
.include "Makefile.inc"

PROG =		gotwebd
SRCS =		config.c sockets.c log.c gotwebd.c parse.y proc.c \
		fcgi.c gotweb.c got_operations.c
SRCS +=		blame.c commit_graph.c delta.c diff.c \
		diffreg.c error.c fileindex.c object.c object_cache.c \
		object_idset.c object_parse.c opentemp.c path.c pack.c \
		privsep.c reference.c repository.c sha1.c worktree.c \
		utf8.c inflate.c buf.c rcsutil.c diff3.c \
		lockfile.c deflate.c object_create.c delta_cache.c \
		gotconfig.c diff_main.c diff_atomize_text.c diff_myers.c \
		diff_output.c diff_output_plain.c diff_output_unidiff.c \
		diff_output_edscript.c diff_patience.c bloom.c murmurhash2.c \
		worktree_open.c patch.c sigs.c date.c

MAN =		${PROG}.conf.5 ${PROG}.8

CPPFLAGS +=	-I${.CURDIR}/../include -I${.CURDIR}/../lib -I${.CURDIR}
LDADD +=	-lz -levent -lutil -lm
YFLAGS =
DPADD =		${LIBEVENT} ${LIBUTIL}
#CFLAGS +=	-DGOT_NO_OBJ_CACHE

.if ${GOT_RELEASE} != "Yes"
NOMAN = Yes
.endif

.if defined(PROFILE)
CPPFLAGS += -DPROFILE
DEBUG = -O0 -pg -g -static
.else
DEBUG = -O0 -g
.endif

realinstall:
	if [ ! -d ${DESTDIR}${PUB_REPOS_DIR}/. ]; then \
		${INSTALL} -d -o root -g daemon -m 755 ${DESTDIR}${PUB_REPOS_DIR}; \
	fi
	${INSTALL} -c -o root -g daemon -m 0755 ${PROG} ${BINDIR}/${PROG}
	if [ ! -d ${DESTDIR}${HTTPD_DIR}/. ]; then \
		${INSTALL} -d -o root -g daemon -m 755 ${DESTDIR}${HTTPD_DIR}; \
	fi
	if [ ! -d ${DESTDIR}${PROG_DIR}/. ]; then \
		${INSTALL} -d -o root -g daemon -m 755 ${DESTDIR}${PROG_DIR}; \
	fi
	${INSTALL} -c -o ${WWWUSR} -g ${WWWGRP} -m 0755 \
	    ${.CURDIR}/files/htdocs/${PROG}/* ${DESTDIR}${PROG_DIR}

.include <bsd.prog.mk>