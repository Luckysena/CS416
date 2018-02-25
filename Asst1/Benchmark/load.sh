#!/bin/sh
make clean
cd ..
rm libmy_pthread.a my_pthread.o
make
cd Benchmark
make
clear
echo Running vectorMultiply...
./vectorMultiply 2
echo
echo Running externalCal...
./externalCal 2
echo
echo Running parallelCal...
./parallelCal 2
