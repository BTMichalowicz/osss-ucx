#!/bin/bash

#
# This script needs to be run from the project root directory.
#
# Example:
#   cd /path/to/osss-ucx
#   ./scripts/BUILD.sh
#
# This script will clean the build directory, run autogen, configure, and compile the project.
#

echo "SWHOME=$SWHOME"
echo "PMIX_DIR=$PMIX_DIR"
echo "UCX_DIR=$UCX_DIR"
echo "OMPI_DIR=$OMPI_DIR"

HLINE="--------------------------------------------"
pwd=$(pwd)

# --- Clean up generated files
echo $HLINE
echo "            CLEANING BUILD"
echo $HLINE
./scripts/clean.sh
echo ; echo

# --- Run autogen and configure
echo $HLINE
echo "            RUNNING AUTOGEN"
echo $HLINE
mkdir build
./autogen.sh
cd build
echo ; echo

# --- Configure build
echo $HLINE
echo "            CONFIGURING"
echo $HLINE
PREFIX="$(pwd)/install"
# PREFIX=$OSSS_DIR

export SHMEM_LAUNCHER="$OMPI_BIN/mpiexec"
#export SHMEM_LAUNCHER="$OMPI_BIN/mpirun"
#export SHMEM_LAUNCHER="$MPICH_BIN/mpirun"

../configure              \
  --prefix=$PREFIX        \
  --with-pmix=$PMIX_DIR   \
  --with-ucx=$UCX_DIR     \
  --with-heap-size=128M   \
  --enable-debug --enable-pshmem

# ---  Compile
echo $HLINE
echo "            COMPILING"
echo $HLINE
make -j $(( $(nproc) - 1 )) install 
