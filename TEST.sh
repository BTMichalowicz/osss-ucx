#!/bin/bash

HLINE="---------------------------------------------------------"

if [ $# -ne 1 ]; then
    echo "Usage: $0 <version>"
    echo "Version must be either 1.4 or 1.5"
    exit 1
fi

if [ "$1" != "1.4" ] && [ "$1" != "1.5" ]; then
    echo "Error: Version must be either 1.4 or 1.5"
    exit 1
fi

VERSION="$1"

export OSSS="$(pwd)/build/build"
export OSHCC="$OSSS/bin/oshcc"
export OSHCXX="$OSSS/bin/oshcxx"
export OSHRUN="$OSSS/bin/oshrun"

# echo "OSHCC: $OSHCC"
# echo "OSHCXX: $OSHCXX"
# echo "OSHRUN: $OSHRUN"

if [ $VERSION == "1.4" ]; then
  cd ../tests-sos-1.4
  echo $HLINE
  echo "Running tests in $(pwd)"
  echo $HLINE
  ./BUILD.sh
elif [ $VERSION == "1.5" ]; then
  cd ../tests-sos-1.5
  echo $HLINE
  echo "Running tests in $(pwd)"
  echo $HLINE
  ./BUILD.sh
fi


