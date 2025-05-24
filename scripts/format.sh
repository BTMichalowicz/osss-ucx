#!/bin/bash

#
# This script needs to be run from the project root directory.
# Example:
#   cd /path/to/osss-ucx
#   ./scripts/FORMAT.sh
#
# This script will format the code using clang-format.
#

# --- Format the code using clang-format
find src -name "*.c" -o -name "*.h" | xargs clang-format -i 