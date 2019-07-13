OS = windows
BUILD_TYPE = release_static

OUTPUT := ps3netsrv
OBJS = main.o compat.o File.o VIsoFile.o

CFLAGS = -Wall -I. -I./polarssl-1.3.2/include -std=gnu99 -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CPPFLAGS += -Wall -I. -I./polarssl-1.3.2/include -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

#CFLAGS += -DNOSSL
#CPPFLAGS +=-DNOSSL

LDFLAGS = -L. -L./polarssl-1.3.2/library
LIBS = -lstdc++ -lpolarssl


ifeq ($(OS), linux)
LIBS += -lpthread
endif

ifeq ($(OS), windows)
OBJS += scandir.o dirent.o
CC = gcc
CXX = g++
LIBS += -lws2_32
OUTPUT := $(OUTPUT).exe
endif

ifeq ($(OS), cross)
OBJS += scandir.o dirent.o
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
	rm -f *.o

clean:
	rm -f $(OUTPUT) *.o

$(OUTPUT): $(OBJS)
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LIBS)
