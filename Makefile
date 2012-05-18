OS := $(shell uname -s)
SRC :=  iotest.c
ifeq ($(OS), Darwin)
LFLAGS := 
SRC += barrier.c
else
LFLAGS := -lrt
endif

all: $(SRC)
	gcc -std=c99 -Wall -pthread -o iotest $(SRC) $(LFLAGS)
clean:
	rm iotest
