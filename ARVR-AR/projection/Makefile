#MAKEFILE

CC = gcc
CXX = g++
RM	= rm -f

CFLAGS += -c -g -O3 -Wall
CXXFLAGS += -c -g -O3 -Wall

# リンクするライブラリにmathとOpenCVを設定
LDFLAGS +=  -lglut -lGLU -lGL -lm -lc  -I/usr/local/include/opencv2 $(shell pkg-config --libs opencv)
# インクルードするファイルのパスにOpenCVを追加
INCLUDE +=  -I/usr/local/include/opencv2

TARGET	= projection
OBJS	= projection.o

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) $<
.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDE) $<

TARGET: ${OBJS}
	$(CXX) -o ${TARGET} ${OBJS} $(LDFLAGS)

clean:
	${RM} ${TARGET} *.o *~

