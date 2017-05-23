#!/bin/sh

make clean
make mips

cat ./test_m.ko | nc -l -p 1111

