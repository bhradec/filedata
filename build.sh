#!/bin/sh

mkdir -p ./build/
gcc -ansi -Wall -pedantic-errors -g filedata.c -o ./build/filedata
