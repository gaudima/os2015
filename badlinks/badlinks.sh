#!/bin/sh

if [ $# -ne 1 ]; then
	echo "Expected 1 argument"
	exit
fi

find -L $@ -mtime +7 -type l -print