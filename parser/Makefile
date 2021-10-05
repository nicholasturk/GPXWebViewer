UNAME := $(shell uname)
CC = gcc
CFLAGS = -Wall -std=c11 -g
LDFLAGS= -L.

INC = include/
SRC = src/
BIN = bin/
PARSER_SRC_FILES = $(wildcard src/GPX*.c)
PARSER_OBJ_FILES = $(patsubst src/GPX%.c,bin/GPX%.o,$(PARSER_SRC_FILES))

ifeq ($(UNAME), Linux)
	XML_PATH = /usr/include/libxml2
endif
ifeq ($(UNAME), Darwin)
	XML_PATH = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/libxml2
endif

parser: $(BIN)libgpxparser.so

$(BIN)libgpxparser.so: $(PARSER_OBJ_FILES) $(BIN)LinkedListAPI.o
	gcc -shared -o ../libgpxparser.so $(PARSER_OBJ_FILES) $(BIN)LinkedListAPI.o -lxml2 -lm

#Compiles all files named GPX*.c in src/ into object files, places all coresponding GPX*.o files in bin/
$(BIN)GPX%.o: $(SRC)GPX%.c $(INC)LinkedListAPI.h $(INC)GPX*.h
	gcc $(CFLAGS) -I$(XML_PATH) -I$(INC) -c -fpic $< -o $@

$(BIN)liblist.so: $(BIN)LinkedListAPI.o
	$(CC) -shared -o $(BIN)liblist.so $(BIN)LinkedListAPI.o

$(BIN)LinkedListAPI.o: $(SRC)LinkedListAPI.c $(INC)LinkedListAPI.h
	$(CC) $(CFLAGS) -c -fpic -I$(INC) $(SRC)LinkedListAPI.c -o $(BIN)LinkedListAPI.o

all: $(BIN)GPXParser.o $(BIN)LinkedListAPI.o
	gcc -g -I/usr/include/libxml2 -lxml2 $(BIN)GPXParser.o $(BIN)LinkedListAPI.o -o run

buildRun:
	make clean && make && make all && ./run ../uploads/vic.gpx

mem:
	make clean && make && make all && valgrind --track-origins=yes --show-leak-kinds=all ./run ../uploads/vic.gpx

clean:
	rm -rf $(BIN)StructListDemo $(BIN)xmlExample $(BIN)*.o ../*.so ../*.dylib run

#This is the target for the in-class XML example
xmlExample: $(SRC)libXmlExample.c
	$(CC) $(CFLAGS) -I$(XML_PATH) $(SRC)libXmlExample.c -lxml2 -o $(BIN)xmlExample

#These are sample targets for the list demo code incldued in the class examples.  They will not be used
#for A1, but they can help you figure out who to set up a target for your own test main

StructListDemo: $(BIN)StructListDemo.o $(BIN)liblist.so
	$(CC) $(CFLAGS) $(LDFLAGS) -L$(BIN) -o $(BIN)StructListDemo $(BIN)StructListDemo.o  -llist
	
$(BIN)StructListDemo.o: $(SRC)StructListDemo.c
	$(CC) $(CFLAGS) -I$(INC) -c $(SRC)StructListDemo.c -o $(BIN)StructListDemo.o

###################################################################################################
