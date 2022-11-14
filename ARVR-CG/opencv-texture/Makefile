#MAKEFILE

CC = gcc
CXX = g++
RM	= rm -f

CFLAGS += -c -g -O3 -Wall
CXXFLAGS += -c -g -O3 -Wall

# リンクするライブラリにmathとOpenCVを設定
LDFLAGS +=  -lglut -lGLU -lGL -lm -lc $(shell pkg-config --libs opencv)
# インクルードするファイルのパスにOpenCVを追加
INCLUDE += $(shell pkg-config --cflags opencv)

TARGET	= videotexture
OBJS	= videotexture.o

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) $<
.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDE) $<

TARGET: ${OBJS}
	$(CXX) -o ${TARGET} ${OBJS} $(LDFLAGS)

clean:
	${RM} ${TARGET} *.o *~

