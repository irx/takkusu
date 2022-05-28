.POSIX:
.PHONY: all clean install run
.SUFFIXES: .c .h .o .ff.bz2 .ff .ff.h

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

EXTRA_OBJ =
EXTRA_HDR =
OBJ = log.o render.o ff.o main.o obj.o sched.o ${EXTRA_OBJ}
HDR = log.h render.h ff.h obj.h ${EXTRA_HDR}

-include config.mk

ASSETS = assets/ibm10x22.ff \
	assets/tux.ff \
	assets/grass.ff \
	assets/sand.ff \
	assets/sword.ff
HASSETS = ${ASSETS:.ff=.ff.h}

all: takkusu ${ASSETS}

takkusu: ${HDR} ${OBJ}
	@echo LD $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

.ff.bz2.ff:
	bzip2 -dc $< > $@

.ff.ff.h:
	./bin2hdr "$<" > $@

assets_data.gen.h: bin2hdr embed_assets.sh ${HASSETS}
	./embed_assets.sh ${ASSETS}

vfs.o: assets_data.gen.h

clean:
	rm -f ${OBJ}
	rm -f assets_data.gen.h ${HASSETS}

install: test
	@echo not implemented yet
