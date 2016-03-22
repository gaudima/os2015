#!/bin/bash

if [ $# -ne 1 ]; then
	exit
fi

find -L $@ -mtime +7 -type l -print