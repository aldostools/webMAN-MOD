#!/bin/bash

if [ -f polarssl-1.3.2/bin/libpolarssl-linux.a ]
then
	mv polarssl-1.3.2/bin/libpolarssl-linux.a polarssl-1.3.2/library/libpolarssl.a
fi

if [ -f Makefile.linux ] 
then
	mv Makefile Makefile.win
	mv Makefile.linux Makefile
fi

make
