#!/bin/bash
mpic++ -o mpi_exp.mpi mpi_exp.cpp
mpirun -np 4 mpi_exp.mpi 50000
