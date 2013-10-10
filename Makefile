CC = gcc
CC_OPTS = -g -std=c99 -Wall -Werror -pedantic -c
LD = gcc
LD_OPTS = -Wall -pedantic
LIBRARIES = -lm

tinysynth: tinysynth.o
	${LD} ${LD_OPTS} $^ -o $@ ${LIBRARIES}

tinysynth.o: tinysynth.c
	${CC} ${CC_OPTS} $< -o $@