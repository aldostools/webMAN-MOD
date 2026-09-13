OS = windows
BUILD_TYPE = release_static

BUILD_DATE ?= $(shell date +%Y%m%d 2>/dev/null || echo unknown)

OUTPUT := ps3netsrv
OBJS = src/main.o src/padlock.o src/aes.o src/compat.o src/mem.o src/File.o src/VIsoFile2.o

CFLAGS = -Wall -Wno-format -I./include -std=gnu99 -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DPOLARSSL -DBUILD_DATE=\"$(BUILD_DATE)\"
CPPFLAGS += -Wall -Wno-format -I./include -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DPOLARSSL -DBUILD_DATE=\"$(BUILD_DATE)\"

#CFLAGS += -Doff64_t=off_t
#CPPFLAGS += -Doff64_t=off_t

#CFLAGS += -DOPT_FILE_SEEK
#CPPFLAGS += -DOPT_FILE_SEEK

#OUTPUT := ps3netsrv_ro
#CFLAGS += -DREAD_ONLY
#CPPFLAGS += -DREAD_ONLY

#OUTPUT := makeiso
#CFLAGS += -DMAKEISO
#CPPFLAGS += -DMAKEISO

#CFLAGS += -DNO_UPDATE
#CPPFLAGS += -DNO_UPDATE
#OUTPUT := makeiso_rip

#CFLAGS += -DNOSSL
#CPPFLAGS +=-DNOSSL
#OBJS = src/main.o src/compat.o src/mem.o src/File.o src/VIsoFile2.o

LDFLAGS = -L.
LIBS = -lstdc++

ifeq ($(OS), linux)
LIBS += -lpthread
endif

ifeq ($(OS), windows)
CFLAGS += -D_OS_WINDOWS
CPPFLAGS += -D_OS_WINDOWS
OBJS += src/scandir.o src/dirent.o
CC = gcc
CXX = g++
LIBS += -lws2_32
OUTPUT := $(OUTPUT).exe
endif

ifeq ($(OS), cross)
OBJS += src/scandir.o src/dirent.o
CC = i586-pc-mingw32-gcc
CXX = i586-pc-mingw32-g++
LIBS += -lws2_32
OUTPUT := $(OUTPUT).exe
endif

ifeq ($(BUILD_TYPE), debug)
CFLAGS += -O0 -g3 -DDEBUG
CPPFLAGS += -O0 -g3 -DDEBUG
endif

ifeq ($(BUILD_TYPE), debug_static)
CFLAGS += -O0 -static -g3 -DDEBUG
CPPFLAGS += -O0 -static -g3 -DDEBUG
endif

ifeq ($(BUILD_TYPE), release)
CFLAGS += -O3 -s -DNDEBUG
CPPFLAGS += -O3 -s -DNDEBUG
endif

ifeq ($(BUILD_TYPE), release_static)
CFLAGS += -static -O3 -s -DNDEBUG
CPPFLAGS += -static -O3 -s -DNDEBUG
endif

all: $(OUTPUT)
	rm -r -f src/*.o

clean:
	rm -r -f $(OUTPUT) src/*.o

$(OUTPUT): $(OBJS)
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LIBS)
