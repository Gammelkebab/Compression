#!/bin/bash -login
#PBS -N Compression
#PBS -j oe
#PBS -m ae
#PBS -l nodes=1:ppn=16
#PBS -l walltime=00:00:02
#PBS -l mem=256mb
#PBS -W x=PARTITION:lena

cd Compression

module load intel

make full
