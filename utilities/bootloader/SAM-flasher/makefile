all: bin bin/sam-flasher

CC = gcc
CXX = g++

BUILDHOST := $(shell uname -s)
BUILDHOST := $(patsubst MINGW64_%,MINGW64,$(BUILDHOST))

RELEASE := yes

ifeq ($(BUILDHOST), MINGW64)
  CFLAGS = -mno-ms-bitfields
  LFLAGS = -lsetupapi
else
  CFLAGS =
  LFLAGS =
endif

CFLAGS += -MD -I./include
LFLAGS +=

ifeq ($(RELEASE), yes)
    CFLAGS += -O3
else
    CFLAGS += -Og -g -DDEVEL
endif

# The source for anything placed here must be in "./src" or
# "./msgpack/cmp/src/".  If not, add another rule.
C_OBJ = \
	bin/main.o \
	bin/slip.o \
	bin/serial_posix.o \
	bin/serial_win32.o \
	bin/flash_cmd.o \
	bin/util.o

./bin/%.o : ./src/%.cpp
	$(CXX) $(CFLAGS) -c ./src/$*.cpp -o $@

./bin/%.o : ./src/%.c
	$(CC) $(CFLAGS) -c ./src/$*.c -o $@

bin/sam-flasher: $(C_OBJ)
	$(CXX) -o $@ $^ $(LFLAGS)

clean:
	rm -rf bin/*

bin:
	mkdir -p bin

.PHONY: clean bindir

# pull in and check dependencies
-include $(C_OBJ:.o=.d)
