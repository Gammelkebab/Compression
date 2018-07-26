#!/bin/bash -login
#PBS -N Benchmark_Compression
#PBS -j oe
#PBS -m ae
#PBS -l nodes=1:ppn=1
#PBS -l walltime=00:00:20
#PBS -l mem=1gb
#PBS -W x=PARTITION:lena

cd Compression

module load intel

make

./main
