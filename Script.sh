#!/bin/bash -login
#PBS -N Compression
#PBS -j oe
#PBS -m ae
#PBS -l nodes=4:ppn=16
#PBS -l walltime=00:00:20
#PBS -l mem=1gb
#PBS -W x=PARTITION:lena

cd Compression

module load intel

make

./main
