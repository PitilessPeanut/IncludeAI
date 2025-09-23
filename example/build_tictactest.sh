#!/bin/bash
set -euo pipefail

# -ffinite-math-only
# -fsanitize=address # dynamic bounds check "undefined reference"
# -finput-charset=UTF-8 -fexec-charset=UTF-8 -fextended-identifiers
# -Wfatal-errors # make gcc output bearable
# -Wshadow-compatible-local # not working on clang
# -Wimplicit-fallthrough # warn missing [[fallthrough]]
# -Wundef # Macros must be defined

clang++ -I../src -std=c++20 -fno-exceptions -fno-rtti \
        -pedantic -ffast-math -Wno-unknown-warning-option \
        -Wno-misleading-indentation -Wno-different-indent \
        -finput-charset=UTF-8 -Wall -Wextra -Oz -flto \
        ./tictac_test.cpp ../src/bitalloc.cpp ../src/micro_math.cpp ../src/similarity.cpp -o tictac_test
