#!/bin/bash -login
#PBS -N Benchmark_Compression
#PBS -j oe
#PBS -m ae
#PBS -l nodes=1:ppn=1
#PBS -l walltime=00:00:05
#PBS -l mem=128mb
#PBS -W x=PARTITION:lena

cd Compression/Compression_Benchmark

module load intel

make

./main
