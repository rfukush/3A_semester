#MAKEFILE

CC=gcc
RM=rm -f

CFLAGS=-Wall
LDFLAGS=-lglut -lGL -lGLU -ljpeg -lm

TARGET=sample-texbind
OBJS=sample-texbind.o 

.c.o:
	${CC} -c ${CFLAGS} $<

TARGET: ${OBJS}
	${CC} -o ${TARGET} ${OBJS} ${LDFLAGS}

clean:
	${RM} ${TARGET} *.o *~

