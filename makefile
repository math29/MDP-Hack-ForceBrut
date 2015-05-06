##############################################################################

ALL=$(shell for i in decrypt*.c ; do basename $$i .c ; done)

all : ${ALL}

CC=gcc
CFLAGS=-std=c99 -U__STRICT_ANSI__ \
       -W -Wall -Wc++-compat -Wno-long-long -pedantic \
       -O3 -DNDEBUG -ffast-math -s -fomit-frame-pointer \
       -pthread
LDFLAGS=-pthread -lm -lcrypt

.SUFFIXES:
.SECONDARY:

% : %.o
	${CC} -o $@ $^ ${LDFLAGS}
	@echo

%.o : %.c crsUtils.h
	${CC} -o $@ -c $< ${CFLAGS}
	@echo

clean :
	rm -f ${ALL} *.o a.out core

##############################################################################
