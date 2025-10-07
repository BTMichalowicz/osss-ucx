#!/bin/bash

set -e

cd ../shmembench-secure
./scripts/build.sh
./scripts/test-rma.sh
