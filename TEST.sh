#!/bin/bash

# --- Check if the test is provided and valid
TEST="$1"
if [ $# -eq 1 ]; then
  if [ "$TEST" != "sos1.4" ] && [ "$TEST" != "sos1.5" ] && [ "$TEST" != "vv1.4" ] && [ "$TEST" != "vv1.5" ] && [ "$TEST" != "shmembench_1.4" ] && [ "$TEST" != "shmembench_1.5" ]; then
    echo "Error: Version must be either sos1.4 or sos1.5 or vv1.5 or shmembench_1.4 or shmembench_1.5"
    exit 1
  fi
fi

OSSS_DIR="$(pwd)"
HLINE="---------------------------------------------------------"

# --- Run SOS 1.4 tests
test_sos_1_4() {
  cd ../tests-sos-1.4
  echo $HLINE
  echo "Running tests in $(pwd)"
  ./TEST.sh
  cd $OSSS_DIR
}

# --- Run SOS 1.5 tests
test_sos_1_5() {
  cd ../tests-sos-1.5.2
  echo $HLINE
  echo "Running tests in $(pwd)"
  ./TEST.sh
  cd $OSSS_DIR
}

# --- Run SHMEMVV 1.4 tests
test_vv_1_4() {
  cd ../shmemvv_1.4
  echo $HLINE
  echo "Running tests in $(pwd)"
  ./TEST.sh
  cd $OSSS_DIR
}

# --- Run SHMEMVV 1.5 tests
test_vv_1_5() {
  cd ../shmemvv_1.5
  echo $HLINE
  echo "Running tests in $(pwd)"
  ./TEST.sh
  cd $OSSS_DIR
}

# --- Run SHMEMBENCH 1.4 tests
test_shmembench_1_4() {
  cd ../shmembench_1.4
  echo $HLINE
  echo "Running tests in $(pwd)"
  ./TEST.sh
  cd $OSSS_DIR
}

# --- Run SHMEMBENCH 1.5 tests
test_shmembench_1_5() {
  cd ../shmembench_1.5
  echo $HLINE
  echo "Running tests in $(pwd)"
  ./TEST.sh
  cd $OSSS_DIR
}

# --- Run the appropriate test(s)
if [ $# -eq 0 ]; then
  # No version specified - run both
  test_sos_1_4
  test_sos_1_5
elif [ "$TEST" == "sos1.4" ]; then
  test_sos_1_4
elif [ "$TEST" == "sos1.5" ]; then
  test_sos_1_5
elif [ "$TEST" == "vv1.4" ]; then
  test_vv_1_4
elif [ "$TEST" == "vv1.5" ]; then
  test_vv_1_5
elif [ "$TEST" == "shmembench_1.4" ]; then
  test_shmembench_1_4
elif [ "$TEST" == "shmembench_1.5" ]; then
  test_shmembench_1_5
fi
