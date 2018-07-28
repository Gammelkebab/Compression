#!/bin/bash -login
#PBS -N Compression
#PBS -j oe
#PBS -m ae
#PBS -l nodes=4:ppn=4
#PBS -l walltime=00:00:05
#PBS -l mem=256mb
#PBS -W x=PARTITION:tane

cd Compression

module load foss

make full
