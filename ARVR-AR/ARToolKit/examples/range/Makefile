INC_DIR= ../../include
LIB_DIR= ../../lib
BIN_DIR= ../../bin

LDFLAG=-L/usr/X11R6/lib -L$(LIB_DIR)
LIBS= -lARgsub -lARvideo -lAR -lglut -lGLU -lGL -lXi -lXmu -lX11 -lm -ljpeg
CFLAG= -O -I/usr/X11R6/include -DUSE_EYETOY -I/usr/src/linux/include -I$(INC_DIR)

OBJS =
HEADDERS =

all: $(BIN_DIR)/rangeTest

$(BIN_DIR)/rangeTest: rangeTest.o $(OBJS)
	cc -o $(BIN_DIR)/rangeTest rangeTest.o $(OBJS) $(LDFLAG) $(LIBS)

rangeTest.o: rangeTest.c $(HEADDERS)
	cc -c $(CFLAG) rangeTest.c

clean:
	rm -f *.o
	rm -f $(BIN_DIR)/rangeTest

allclean:
	rm -f *.o
	rm -f $(BIN_DIR)/rangeTest
	rm -f Makefile
