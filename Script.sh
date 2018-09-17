#!/bin/bash -login
#PBS -N Compression
#PBS -j oe
#PBS -m ae
#PBS -l nodes=1:ppn=8
#PBS -l walltime=00:00:02
#PBS -l mem=256mb
#PBS -W x=PARTITION:lena

cd $BIGWORK/Compression

module load intel

lfs setstripe --count -1 .

make clean
make
mpirun ./main
