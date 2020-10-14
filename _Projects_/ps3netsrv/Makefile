OS = windows
BUILD_TYPE = release_static

OUTPUT := ps3netsrv
OBJS = src/main.o src/padlock.o src/aes.o src/compat.o src/File.o src/VIsoFile.o

CFLAGS = -Wall -Wno-format -I./include -std=gnu99 -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DPOLARSSL
CXXFLAGS += -Wall -Wno-format -I./include -std=c++0x -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DPOLARSSL

#CFLAGS += -Doff64_t=off_t
#CXXFLAGS += -Doff64_t=off_t

#CFLAGS += -DOPT_FILE_SEEK
#CXXFLAGS += -DOPT_FILE_SEEK

#CFLAGS += -DNOSSL
#CXXFLAGS +=-DNOSSL
#OBJS = src/main.o src/compat.o src/File.o src/VIsoFile.o

LDFLAGS = -L.
LIBS = -lstdc++

# detect if rm is available
ifeq ($(wildcard $(addsuffix /rm,$(subst :, ,$(PATH)))),)
RM = del /Q /F
SUBST = $(subst /,\,$1)
ERRIGNORE = 2>NUL || true
else
RM = rm -f
SUBST = $1
ERRIGNORE = 2>/dev/null
endif

ifeq ($(OS), linux)
LIBS += -lpthread
endif

ifeq ($(OS), windows)
CFLAGS += -D_OS_WINDOWS -DWIN32
CXXFLAGS += -D_OS_WINDOWS -DWIN32
OBJS += src/scandir.o src/dirent.o
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
CXXFLAGS += -O0 -g3 -DDEBUG
endif

ifeq ($(BUILD_TYPE), debug_static)
CFLAGS += -O0 -static -g3 -DDEBUG
CXXFLAGS += -O0 -static -g3 -DDEBUG
endif

ifeq ($(BUILD_TYPE), release)
CFLAGS += -O3 -s -DNDEBUG
CXXFLAGS += -O3 -s -DNDEBUG
endif

ifeq ($(BUILD_TYPE), release_static)
CFLAGS += -static -O3 -s -DNDEBUG
CXXFLAGS += -static -O3 -s -DNDEBUG
endif

all: $(OUTPUT)
	$(RM) $(call SUBST,src/*.o) $(ERRIGNORE)

clean:
	$(RM) $(OUTPUT) $(call SUBST,src/*.o) $(ERRIGNORE)

$(OUTPUT): $(OBJS)
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LIBS)
