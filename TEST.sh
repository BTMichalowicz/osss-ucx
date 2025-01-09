#!/bin/bash

export OSSS="$(pwd)/build/build"
export OSHCC="$OSSS/bin/oshcc"
export OSHCXX="$OSSS/bin/oshcxx"
export OSHRUN="$OSSS/bin/oshrun"

echo "OSSS: $OSSS"
echo "OSHCC: $OSHCC"
echo "OSHCXX: $OSHCXX"
echo "OSHRUN: $OSHRUN"

cd test/tests-sos
make clean
./autogen.sh
mkdir -p build
cd build
../configure CC=$OSHCC CXX=$OSHCXX
make
make check NPROCS=4 TESTS=broadcast
