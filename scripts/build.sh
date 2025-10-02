#!/bin/bash
rm -rf build
mkdir build && ./autogen.sh && cd build
PREFIX="$PWD/install"

export PMIX_DIR="${PMIX_DIR:?set this}"
export UCX_DIR="${UCX_DIR:?set this}"

# 🔹 NEW: remove stale libtool archives that can drag in dead deps like libev.la
find "$PMIX_DIR" "$UCX_DIR" -name '*.la' -print -delete 2>/dev/null

# Make your PMIx pkgconfig discoverable
export PKG_CONFIG_PATH="$PMIX_DIR/lib/pkgconfig:$PKG_CONFIG_PATH"

# Hard-pin include/lib paths and link to pmix explicitly
export CPPFLAGS="-I$PMIX_DIR/include ${CPPFLAGS}"
export LDFLAGS="-L$PMIX_DIR/lib ${LDFLAGS}"
export LIBS="-lpmix ${LIBS}"

# (Optional but helpful): make the pmix hints explicit too
PMIX_CPPFLAGS="-I$PMIX_DIR/include" \
PMIX_LDFLAGS="-L$PMIX_DIR/lib" \
PMIX_LIBS="-lpmix" \
../configure \
  CFLAGS="-Wall -pipe -g -O0" \
  --prefix="$PREFIX" \
  --with-pmix="$PMIX_DIR" \
  --with-ucx="$UCX_DIR" \
  --with-heap-size=64M \
  --enable-experimental \
  --enable-encryption \
  --enable-debug \
  --enable-logging

make -j $(( $(nproc) - 1 )) install


