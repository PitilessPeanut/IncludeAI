#!/bin/bash
set -euo pipefail

# -ffinite-math-only
# -fsanitize=address # dynamic bounds check "undefined reference"
# -finput-charset=UTF-8 -fexec-charset=UTF-8 -fextended-identifiers
# -Wfatal-errors # make gcc output bearable
# -Wshadow-compatible-local # not working on clang
# -Wimplicit-fallthrough # warn missing [[fallthrough]]
# -Wundef # Macros must be defined

rm -f tictac_test

g++ -I../src -std=c++20 -fno-exceptions -fno-rtti \
        -march=native -g \
        -pedantic -ffast-math -Wno-unknown-warning-option \
        -Wno-misleading-indentation -Wno-different-indent \
        -finput-charset=UTF-8 -Wall -Wextra -O0 -flto \
        ./tictac_test.cpp ../src/bitalloc.cpp ../src/micro_math.cpp ../src/neural.cpp ../src/ai.cpp -o tictac_test
