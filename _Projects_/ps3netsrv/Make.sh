#!/bin/bash

if [ -f Makefile.linux ]
then
	make -f Makefile.linux
else
	make
fi
