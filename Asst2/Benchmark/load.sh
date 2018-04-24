#!/bin/sh
make clean
cd ..
rm libmy_pthread.a my_pthread.o
make
cd Benchmark
make
clear
gdb parallelCal -q
