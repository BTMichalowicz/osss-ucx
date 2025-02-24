#!/bin/bash

# --- Check if the version is provided and valid
VERSION="$1"
if [ $# -eq 1 ]; then
  if [ "$VERSION" != "1.4" ] && [ "$VERSION" != "1.5" ]; then
    echo "Error: Version must be either 1.4 or 1.5"
    exit 1
  fi
fi

OSSS_DIR="$(pwd)"
HLINE="---------------------------------------------------------"

# --- Function to test version 1.4
test_1_4() {
  cd ../tests-sos-1.4
  echo $HLINE
  echo "Running tests in $(pwd)"
  ./TEST.sh
  cd $OSSS_DIR
}

# --- Function to test version 1.5  
test_1_5() {
  cd ../tests-sos-1.5
  echo $HLINE
  echo "Running tests in $(pwd)"
  ./TEST.sh
  cd $OSSS_DIR
}

# --- Run the appropriate test(s)
if [ $# -eq 0 ]; then
  # No version specified - run both
  test_1_4
  test_1_5
elif [ "$VERSION" == "1.4" ]; then
  test_1_4
elif [ "$VERSION" == "1.5" ]; then
  test_1_5
fi
