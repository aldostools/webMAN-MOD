all: png2argb

png2argb: png2argb.o
	cc  png2argb.o -lspng -o png2argb

png2argb.o: png2argb.c
	cc -c png2argb.c

clean:
	rm png2argb.o png2argb
