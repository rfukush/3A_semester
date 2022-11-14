#MAKEFILE

CC	= gcc
RM	= rm -f

CFLAGS	= -g -Wall -O2
LDFLAGS	= -lglut -lGL -lGLU -lm

TARGET	= videocapture_gl
OBJS	= videocapture_gl.o videocapture_mmap.o

.c.o:
	${CC} -c ${CFLAGS} $<

TARGET: ${OBJS}
	${CC} -o ${TARGET} ${OBJS} ${LDFLAGS}

clean:
	${RM} ${TARGET} *.o *~

