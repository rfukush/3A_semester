INC_DIR= ../../include
LIB_DIR= ../../lib
BIN_DIR= ../../bin

LDFLAG=-L/usr/X11R6/lib -L$(LIB_DIR)
LIBS= -lARgsub -lARvideo  -lARMulti -lAR -lglut -lGLU -lGL -lXi -lXmu -lX11 -lm -ljpeg
CFLAG= -O -I/usr/X11R6/include -DUSE_EYETOY -I/usr/src/linux/include -I$(INC_DIR)

OBJS = command_sub.o util.o paddle.o
HEADDERS = command_sub.h util.h paddle.h

all: $(BIN_DIR)/paddleDemo

$(BIN_DIR)/paddleDemo: paddleDemo.o $(OBJS)
	cc -o $(BIN_DIR)/paddleDemo paddleDemo.o $(OBJS) $(LDFLAG) $(LIBS)

paddleDemo.o: paddleDemo.c $(HEADDERS)
	cc -c $(CFLAG) paddleDemo.c

command_sub.o: command_sub.c $(HEADDERS)
	cc -c $(CFLAG) command_sub.c

util.o: util.c $(HEADDERS)
	cc -c $(CFLAG) util.c

paddle.o: paddle.c $(HEADDERS)
	cc -c $(CFLAG) paddle.c

clean:
	rm -f *.o
	rm -f $(BIN_DIR)/paddleDemo

allclean:
	rm -f *.o
	rm -f $(BIN_DIR)/paddleDemo
	rm -f Makefile
