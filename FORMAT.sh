#!/bin/bash

# --- Format the code using clang-format
find src -name "*.c" -o -name "*.h" | xargs clang-format -i
