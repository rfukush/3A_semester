CC 		= g++

RM 	= rm -f
TARGET 	= cvglcameraview
OBJS 	= cvglcameraview.o

CFLAGS  = -O3 `pkg-config --cflags opencv` 
LDFLAGS = `pkg-config --libs opencv`
LDLIBS  = -lm -lpthread -lglut -lGL -lGLU

.cpp.o:
	${CC} -c ${CFLAGS} $<

${TARGET}: ${OBJS}
	${CC} -o ${TARGET} ${OBJS} ${LDLIBS} ${LDFLAGS}

clean:
	${RM} ${TARGET} *.o *~

