# 과제 02 - 이진트리 프로그램 빌드
#   make          : 세 프로그램을 모두 빌드
#   make run      : 비교 프로그램 실행
#   make clean    : 실행 파일 삭제
CC     ?= gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2
HDRS    = tree_common.h tree_array.h tree_linked.h

all: tree_array tree_linked tree_compare

tree_array: main_array.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ main_array.c

tree_linked: main_linked.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ main_linked.c

tree_compare: compare.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ compare.c

run: tree_compare
	./tree_compare

clean:
	rm -f tree_array tree_linked tree_compare

.PHONY: all run clean
