#MAKEFILE

CC=gcc
RM=rm -f

CFLAGS=-Wall
LDFLAGS=-lglut -lGLU -lGL -lm

TARGET=planet
OBJS=planet_ex.o get_clock_now.o draw_function.o

.c.o:
	${CC} -c ${CFLAGS} $<

TARGET: ${OBJS}
	${CC} -o ${TARGET} ${OBJS} ${LDFLAGS}

clean:
	${RM} ${TARGET} *.o *~

