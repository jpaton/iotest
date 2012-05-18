OS := $(shell uname -s)
ifeq ($(OS), Darwin)
LFLAGS := 
else
LFLAGS := -lrt
endif

all: iotest.c barrier.c
	gcc -std=c99 -Wall -pthread -o iotest barrier.c iotest.c $(LFLAGS)
clean:
	rm iotest
