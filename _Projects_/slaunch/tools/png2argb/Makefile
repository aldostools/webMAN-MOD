all: png2argb

CFLAGS = -Iext -DSPNG_STATIC
LDFLAGS = -static
LIBS = -lz -lm

OBJ_BIN = png2argb.o ext/spng.o

png2argb: $(OBJ_BIN)
	$(CC) $(LDFLAGS) $(OBJ_BIN) $(LIBS) -o png2argb 

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm png2argb.o png2argb
