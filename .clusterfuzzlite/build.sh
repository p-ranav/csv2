#!/bin/bash -eu

# Copy all fuzzer executables to $OUT/
$CXX $CFLAGS $LIB_FUZZING_ENGINE \
  $SRC/csv2/.clusterfuzzlite/reader_fuzzer.cpp \
  -o $OUT/reader_fuzzer \
  -I$SRC/csv2/include
