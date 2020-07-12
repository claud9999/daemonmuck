# Generated automatically from Makefile.in by configure.
# Makefile for DaemonMUCK 0.14
#
# Search for the text 'CONFIGURATION SECTION' and make any changes needed
# there.

SHELL=/bin/sh
srcdir = .
VPATH = .
CC = gcc
# CPP = ${CC-cc} -E	# This is broken in autoconf.  Sigh.
CPP = $(CC) -E
LIBS =  -lresolv -lcrypt
LIBOBJS = 
CCFLAGS= -g 

MV= -mv -f
RM= -rm -f
ROOT= ..
MAKE= make
LINT= lint
MKTAGS= ctags
MKDEPEND= makedepend
INCLUDE= ./
SRC_PREFIX=
DEFS=
CFLAGS= ${CCFLAGS} ${DEFS} -I${INCLUDE}

# Destination for binaries
DEST=${ROOT}/game

# ===================== CONFIGURATION SECTION ====================
#
# Select the correct C compiler.  Whatever you choose, it must be able
# to grok ANSI C (function prototypes)
#
#-----CC (must be able to grok function prototypes)
#
#DEFS =
#
#-----GCC if the libraries were built for a pcc-derived cc compiler
#     (most systems)
#
#DEFS = -fpcc-struct-return -Wall
#
#-----GCC with GCC-compatible libraries
#
#DEFS = -Wreturn-type -Wunused -Wswitch -Wcomment -Wformat -Wparentheses
DEFS = -W -Wall -Wno-implicit
#
#-----HP-UX C compiler
#
#DEFS = -w +Obb800 -Aa -D_INCLUDE_POSIX_SOURCE -D_INCLUDE_HPUX_SOURCE -D_INCLUDE_XOPEN_SOURCE
#
#-----MIPS C compiler (also DEC 3xxx, Prime EXL7xxx)
#
#DEFS = -signed

# Libraries.  Use the second line if you want to use the resolver to get
# hostnames and your libc doesn't use it already.  If you use it, you'd
# better have your nameserver working or things may hang for a while when
# people try to login from distant sites.  Use the third line if you're running
# on a SysV-ish system and BSD support isn't built in to the standard libc.
#
MORELIBS	= -lm 
#MORELIBS	= -lm -lnsl -lsocket -L/usr/ucblib -lucb	# Mips
#MORELIBS       = -lresolve

# ================== END OF CONFIGURATION SECTION =================

# paths is likely to remain broken
MISCSRC= Makefile ChangeLog COPYING

CSRC= boolexp.c case.c compile.c compress.c create.c db.c disassem.c edit.c \
	interface.c game.c hashtab.c help.c inst.c interp.c log.c look.c \
	match.c move.c player.c predicates.c property.c rob.c set.c speech.c \
	stringutil.c unparse.c utils.c wiz.c p_conversions.c \
	p_interaction.c p_logic.c p_operands.c p_create.c p_objects.c \
	p_property.c p_stack.c p_strings.c p_tests.c p_time.c p_descriptor.c \
	p_for.c dbreflist.c wild.c eval.c \
        strftime.c smatch.c flags.c \
        config.c
LSRC= copyright.c
MSRC= decompress.c sanity.c

SRC= ${MISCSRC} ${CSRC} ${LSRC} ${MSRC}

COBJ= boolexp.o case.o compile.o compress.o create.o db.o disassem.o edit.o \
	interface.o game.o hashtab.o help.o inst.o interp.o log.o look.o \
	match.o move.o player.o predicates.o property.o rob.o set.o speech.o \
	stringutil.o unparse.o utils.o wiz.o p_conversions.o \
	p_interaction.o p_logic.o p_operands.o p_create.o p_objects.o \
	p_property.o p_stack.o p_strings.o p_tests.o p_time.o p_descriptor.o \
	p_for.o dbreflist.o wild.o \
        strftime.o smatch.o flags.o \
        config.o
LOBJ= copyright.o
MOBJ= decompress.o sanity.o

OBJ= ${COBJ} ${LOBJ} ${MOBJ}
TARGETS= netmuck decompress sanity

.c.o:
	${CC} ${CFLAGS} -c $*.c

netmuck: ${P} ${COBJ} ${LOBJ}
	${RM} netmuck netmuck~
	${CC} ${CFLAGS} -o netmuck ${COBJ} ${LOBJ} ${LIBS} ${MORELIBS}

all:    ${TARGETS} tags

sanity: ${P} sanity.o unparse.o utils.o db.o compress.o log.o property.o ${LOBJ}
	${RM} sanity
	${CC} ${CFLAGS} -o sanity sanity.o unparse.o utils.o db.o compress.o \
		log.o property.o dbreflist.o stringutil.o case.o ${LOBJ} \
                ${LIBS} ${MORELIBS}

decompress: decompress.o compress.o ${LOBJ}
	${RM} decompress
	${CC} ${CFLAGS} -o decompress decompress.o compress.o ${LOBJ} ${LIBS} \
        ${MORELIBS}

saber_src:
	#load ${CFLAGS} ${CSRC}

saber_obj: ${CSRC} interface.o
	#load ${OBJ} interface.o

tags: ${CSRC} ${MSRC}
	${MKTAGS} ${CSRC} ${MSRC}

lint:   ${CSRC} ${MSRC}
	${LINT} ${LINTFLAGS} ${CSRC} ${MSRC}

sources: ${SRC}

names:
	@for i in ${SRC}; do echo ${SRC_PREFIX}$$i; done

depend:
	${MKDEPEND} -- ${CFLAGS} -- ${CSRC} ${MSRC} ${LSRC}
	echo chmod -w Makefile
	chmod -w Makefile

clean: Makefile
	${RM} ${OBJ} core tags netmuck netmuck~ t tt

clobber:
	${RM} ${OBJ} core
	${RM} ${TARGETS} tags *~ *.bak *.orig

nuke:
	${RM} ${OBJ} core
	${RM} ${TARGETS} tags *~ *.bak *.orig
	-${CLEAN} *
	-${GET} Makefile

install: all
	${MV} ${TARGETS} ${DEST}


# DO NOT DELETE THIS LINE -- make depend depends on it.
