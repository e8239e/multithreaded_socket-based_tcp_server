COMMON_FLAGS = -Wall -Werror -pedantic -O -g -flto=auto -pipe
CXXFLAGS = $(COMMON_FLAGS) -std=c++20

all: build

build: server client

clean:
	rm -f server client *.o
