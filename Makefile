#!/usr/bin/make -f

TARGET=simplesh

CFLAGS=-ggdb3 -Wall -Werror -Wno-unused -std=c11
LDLIBS=-lreadline

#OBJECTS=$(patsubst %.c,%.o,$(wildcard *.c))

$(TARGET) : simplesh.o

simplesh.o: simplesh.c simplesh_execute.o 

simplesh_execute.o: simplesh_execute.c simplesh_syntactic.o

simplesh_syntactic.o: simplesh_syntactic.c simplesh_structs.o

simplesh_structs.o: simplesh_structs.c

clean:
	#rm -rf *~ $(OBJECTS) $(TARGET) coreç
.PHONY: clean
