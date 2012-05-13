OS := $(shell uname -s)
ifeq ($(OS), Darwin)
LFLAGS := 
else
LFLAGS := -lrt
endif

all: iotest.c
	gcc -std=c99 -Wall -o iotest iotest.c $(LFLAGS)
clean:
	rm iotest
