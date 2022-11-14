# ディレクトリ内の.cと.cppファイルからディレクトリ名の実行ファイルを作るMakefile

# コンパイラを指定
CC = gcc
CXX = g++
# コンパイラオプション
CFLAGS += -c -g -O3 -Wall
CPPFLAGS += 
CXXFLAGS += -c -g -O3 -Wall
# リンクするライブラリにmathとOpenCVを設定
LDFLAGS += -lm $(shell pkg-config --libs opencv)
# インクルードするファイルのパスにOpenCVを追加
INCLUDE += $(shell pkg-config --cflags opencv)
# ターゲットの名前にディレクトリ名を使う
TARGET = ./$(shell basename `readlink -f .`)
# 全ての.cファイルと.cppファイルをコンパイル
CSOURCES = $(wildcard *.c)
CXXSOURCES = $(wildcard *.cpp)
# 中間オブジェクトを指定
OBJECTS  = $(CSOURCES:.c=.o) $(CXXSOURCES:.cpp=.o)

# .oファイルが最新ならリンクして実行ファイルを生成
$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# .cと.cppファイルから最新の.oファイルを生成（Suffix Rule）
.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $<

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ $<

# 中間オブジェクトを削除して再度実行ファイルを生成
all: clean $(TARGET)

# 実行ファイルと中間オブジェクトを削除
.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TARGET)
