.POSIX:
.PHONY: all clean install run test
.SUFFIXES: .c .h .o .rc .res .ff.bz2 .ff .ff.h .snd.bz2 .snd .snd.h

VERSION = 0.1-rc
BUILD_INFO =
OUTBIN = takkusu

# platform dependent params
# defaults should be ok for most systems
CC = cc
HOSTCC = ${CC}
BIN2HDR = xxd -i
WINDRES = windres
PREFIX = /usr/local/bin
INC = -I/usr/local/include
LIB = -L/usr/local/lib
CFLAGS = -std=c99 -pedantic -Wall -D_POSIX_C_SOURCE=200112L -D_DEFAULT_SOURCE -D_BSD_SOURCE \
	${INC} -DVERSION=\"${VERSION}\" -DBUILD_INFO="\"${BUILD_INFO}\"" -DGLEW_STATIC -g
LDFLAGS = ${LIB} -lGL -lglfw -lGLEW -lm -lportaudio

EXTRA_OBJ =
EXTRA_HDR =
OBJ = \
	src/main.o \
	src/render.o \
	src/ff.o \
	src/entity.o \
	src/sched.o \
	src/audio.o \
	src/vfs.o \
	src/fs.o \
	src/io.o \
	src/dict.o \
	src/log.o \
	${EXTRA_OBJ}
HDR = \
	src/render.h \
	src/ff.h \
	src/entity.h \
	src/audio.h \
	src/vfs.h \
	src/fs.h \
	src/io.h \
	src/dict.h \
	src/log.h \
	${EXTRA_HDR}

-include config.mk

IMG_ASSETS = \
	assets/ibm10x22.ff \
	assets/tux.ff \
	assets/grass.ff \
	assets/sand.ff \
	assets/sword.ff
SND_ASSETS = \
	assets/blip.snd
ASSETS = ${IMG_ASSETS} ${SND_ASSETS}
IMG_ASSETS_H = ${IMG_ASSETS:.ff=.ff.h}
SND_ASSETS_H = ${SND_ASSETS:.snd=.snd.h}
ASSETS_H = ${IMG_ASSETS_H} ${SND_ASSETS_H}

all: ${OUTBIN} ${ASSETS}

takkusu: ${HDR} ${OBJ}
	@echo LD $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $< -o $@

.ff.bz2.ff:
	bzip2 -dc $< > $@

.ff.ff.h:
	${BIN2HDR} "$<" > $@

.snd.bz2.snd:
	bzip2 -dc $< > $@

.snd.snd.h:
	${BIN2HDR} "$<" > $@

bin2hdr: src/bin2hdr.c
	@echo HOSTCC $<
	@${HOSTCC} -o $@ $<

src/assets_data.gen.h: embed_assets.sh ${ASSETS_H}
	./embed_assets.sh ${ASSETS}

.rc.res:
	@echo WINDRES $<
	@${WINDRES} $< -O coff -o $@

clean:
	rm -f ${OBJ}
	rm -f src/assets_data.gen.h ${ASSETS_H}
	rm -f test/*.o *.test

install: test
	@echo not implemented yet



## TESTS ##

TESTS = \
	dict.test \
	entity.test

test: ${TESTS}
	for t in ${TESTS} ; do "./$$t" ; done

dict.test: test/dict.o src/dict.o src/log.o
	@echo LD $@
	@${CC} -o $@ test/dict.o src/dict.o src/log.o ${LDFLAGS}

entity.test: test/entity.o src/entity.o src/dict.o src/ff.o src/render.o src/log.o
	@echo LD $@
	@${CC} -o $@ test/entity.o src/entity.o src/dict.o src/ff.o src/render.o src/log.o ${LDFLAGS}

fs.test: test/fs.o src/log.o src/io.o src/fs.o
	@echo LD $@
	@${CC} -o $@ test/fs.o src/fs.o src/io.o src/log.o ${LDFLAGS}

test/dict.o: src/dict.h src/log.h
test/entity.o: src/entity.h src/dict.h src/ff.h src/render.h src/log.h
test/fs.o: src/log.h src/io.h src/fs.h
