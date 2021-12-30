.POSIX:
.PHONY: clean install run
.SUFFIXES: .c .h .o .ff.bz2 .ff

VERSION = 0.1-rc

# platform dependent params
# defaults should be ok for most systems
CC = cc
PREFIX = /usr/local/bin
INC = -I/usr/local/include
LIB = -I/usr/local/lib
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE \
	${INC} -DVERSION=\"${VERSION}\" -DNEED_STRL
LDFLAGS = ${LIB} -lGL -lglfw -lGLEW -lm

OBJ = log.o render.o ff.o main.o obj.o sched.o
HDR = log.h render.h ff.h obj.h

ASSETS = assets/ibm10x22.ff \
	assets/tux.ff \
	assets/grass.ff \
	assets/sand.ff \
	assets/sword.ff

run: test ${ASSETS}
	./test

test: ${OBJ}
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
