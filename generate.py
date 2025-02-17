#!/usr/bin/env python

files = [
("src/asmtypes.hpp",  []),
("src/bitalloc.hpp", []),
("src/bitalloc.cpp", []),
("src/micro_math.hpp", []),
("src/micro_math.cpp", []),
("src/ai.hpp", []),
("src/ai.cpp", [])
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

def filter(i,sources):
    new = []
    for elem in sources:
        if elem[0]==i:
            new.append(elem)
    return new

def extr(tup,sources):
    new = []
    for i in tup[1]:
        new += filter(i,sources)
    return new

def arrange(sources):
    new = []
    for elem in reversed(sources):
        new += extr(elem,sources)
    return new

sorted = arrange(files) + files
unique = []
[unique.append(elem) for elem in sorted if elem not in unique]
for c in unique:
    print(c[0]+" - "+" ".join(c[1]))

merge_result = """/*
    You must add '#define INCLUDEAI_IMPLEMENTATION' before '#include'ing this in ONE source file.
    Like this:
        #define INCLUDEAI_IMPLEMENTATION
        #include \"includeai.hpp\"
*/\n\n"""

license = """\n/*
MIT
2025 Shaiden Spreitzer (aka. Pitiless Peanut, Professor Peanut, etc...)
*/\n
"""

merge_result += "#ifndef INCLUDEAI_HPP\n#define INCLUDEAI_HPP\n\n"
merge_result += "#ifdef INCLUDEAI_IMPLEMENTATION\n\n\n"
merge_result += """#include <cmath>
#include <concepts>
#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
  #include <immintrin.h>
#elif defined(__aarch64__)
  #include <arm_neon.h>
#elif defined(__wasm__)
  // nothing?
#else
  #error "unknown arch"
#endif\n\n
namespace include_ai {\n\n\n
"""

remove = ["#include", "_HPP", "double include", "x86_64", "aarch64", "defined(__wasm__)", "nothing", "unknown", "include_ai"]

for filename in unique:
    f = open(filename[0], "r")
    lines = f.readlines()
    for code_line in lines:
        if any(x in code_line for x in remove):
            pass
        else:
            merge_result += code_line
    merge_result += "\n\n"

merge_result += """} // namespace single_header_ai\n\n\n
#endif // INCLUDEAI_IMPLEMENTATION\n\n
#endif // INCLUDEAI_HPP\n
"""
merge_result += license

open("./includeai.hpp", "w+").write(merge_result + "\n")

print("success")
