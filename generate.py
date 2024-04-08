#!/usr/bin/env python

files = [
("node_allocator.hpp", ["src/asmtypes.hpp"]),
("node_allocator.cpp", ["node_allocator.hpp"]),
("src/ai.hpp", ["src/asmtypes.hpp", "src/node_allocator.hpp"]),
("src/ai.cpp", ["src/ai.hpp", "src/micro_math.hpp"])
]

# for testing only
mix = [
("g", ["d","f","e"]),
("a", []),
("f", ["b","c"]),
("b", []),
("c", []),
("d", ["e","c"]),
("e", ["a","b"])
]

merge_result = """/*
    You must add '#define INCLUDEAI_IMPLEMENTATION' before #include'ing this in ONE source file.
    Like this:
        #define INCLUDEAI_IMPLEMENTATION
        #include \"includeai.hpp\"
*/\n\n"""

license = """/*
*/
"""

merge_result += "#ifndef INCLUDEAI_HPP\n#define INCLUDEAI_HPP\n\n"
merge_result += "#ifdef INCLUDEAI_IMPLEMENTATION\n\n\n"
merge_result += """#include <cmath>
#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
  #include <immintrin.h>
#endif
"""

merge_result += "#endif // INCLUDEAI_IMPLEMENTATION\n\n\n#endif // INCLUDEAI_HPP\n"
merge_result += license

open("./includeai.hpp", "w+").write(merge_result + "\n")

print("success")

