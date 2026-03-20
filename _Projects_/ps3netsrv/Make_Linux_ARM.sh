#!/bin/bash

if [ -f Makefile.linux ]
then
	make -f Makefile.linux.arm
else
	make
fi
