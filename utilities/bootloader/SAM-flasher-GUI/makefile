all: bin bin/sam-flasher

CC = g++

BUILDHOST := $(shell uname -s)
BUILDHOST := $(patsubst MINGW64_%,MINGW64,$(BUILDHOST))

WXCFLAGS := $(shell wx-config --cxxflags)
WXLFLAGS := $(shell wx-config --libs)
WXRC := $(shell wx-config --rescomp)

RELEASE := yes

ifeq ($(BUILDHOST), MINGW64)
  CFLAGS = -mno-ms-bitfields
  LFLAGS = -lsetupapi
else
  CFLAGS =
  LFLAGS =
endif

CFLAGS += -MD -I./include
LFLAGS += $(WXLFLAGS)

ifeq ($(RELEASE), yes)
    CFLAGS += -O3
else
    CFLAGS += -Og -g -DDEVEL
endif

# The source for anything placed here must be in "./src" or
# "./msgpack/cmp/src/".  If not, add another rule.
C_OBJ = \
	bin/mainFrame.o \
	bin/main.o \
	bin/slip.o \
	bin/serial_posix.o \
	bin/serial_win32.o \
	bin/flash_cmd.o \
	bin/uart_enum.o \
	bin/uart_enum_linux.o \
	bin/uart_enum_win.o \
	bin/util.o

./bin/%.o : ./src/%.cpp
	$(CC) $(CFLAGS) -DUNICODE $(WXCFLAGS) -c ./src/$*.cpp -o $@

./bin/%.o : ./src/%.c
	gcc $(CFLAGS) -c ./src/$*.c -o $@

bin/sam-flasher: $(C_OBJ) mingw/sam-flasher.res
	$(CC) -o $@ $^ $(LFLAGS)

mingw-pkg: bin/mingw-pkg bin/sam-flasher
	cp bin/sam-flasher.exe bin/mingw-pkg
	mingw/mingw-bundledlls --copy bin/mingw-pkg/sam-flasher.exe

clean:
	rm -rf bin/*

bin:
	mkdir -p bin

bin/mingw-pkg:
	mkdir -p bin/mingw-pkg

mingw/sam-flasher.res: mingw/sam-flasher.rc
	$(WXRC) mingw/sam-flasher.rc -O coff -o mingw/sam-flasher.res

.PHONY: clean bindir mingw-pkg

# pull in and check dependencies
-include $(C_OBJ:.o=.d)
