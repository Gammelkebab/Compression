#!/bin/bash -login
#PBS -N Compression
#PBS -j oe
#PBS -m ae
#PBS -l nodes=1:ppn=8
#PBS -l walltime=00:00:05
#PBS -l mem=128mb
#PBS -W x=PARTITION:tane

cd Compression

module load foss

make

mpirun -np 1 ./main
mpirun ./main
