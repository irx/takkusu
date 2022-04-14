.POSIX:
.PHONY: all clean install run
.SUFFIXES: .c .h .o .ff.bz2 .ff

VERSION = 0.1-rc
BUILD_INFO =

# platform dependent params
# defaults should be ok for most systems
CC = cc
PREFIX = /usr/local/bin
INC = -I/usr/local/include
LIB = -L/usr/local/lib
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE \
	${INC} -DVERSION=\"${VERSION}\" -DBUILD_INFO="\"${BUILD_INFO}\""
LDFLAGS = ${LIB} -lGL -lglfw -lGLEW -lm

OBJ = log.o render.o ff.o main.o obj.o sched.o
HDR = log.h render.h ff.h obj.h

-include config.mk

ASSETS = assets/ibm10x22.ff \
	assets/tux.ff \
	assets/grass.ff \
	assets/sand.ff \
	assets/sword.ff

all: takkusu ${ASSETS}

takkusu: ${OBJ}
	@echo LD $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

.c.o: ${HDR}
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

.ff.bz2.ff:
	bzip2 -dc $< > $@

clean:
	rm ${OBJ}

install: test
	@echo not implemented yet
