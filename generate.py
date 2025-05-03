#!/usr/bin/env python

files = [
("src/asmtypes.hpp",  []),
("src/bitalloc.hpp", []),
("src/bitalloc.cpp", []),
("src/micro_math.hpp", []),
("src/micro_math.cpp", []),
("src/similarity.hpp", []),
("src/similarity.cpp", []),
# ("src/neural.hpp", []),
# ("src/neural.cpp", []),
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

    AUTHOR
        Pitiless Peanut (aka. Shaiden Spreitzer, Professor Peanut, etc...) of VECTORPHASE
        
    LICENSE 
        BSD 4-Clause (See end of file)
*/\n\n"""

license = """\n/*
Copyright (c) 2025, VECTORPHASE Systems
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. All advertising materials mentioning features or use of this software must
   display the following acknowledgement:
     This product includes software developed by VECTORPHASE

4. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/\n
"""

merge_result += "#ifndef INCLUDEAI_HPP\n#define INCLUDEAI_HPP\n\n"
merge_result += "#ifdef INCLUDEAI_IMPLEMENTATION\n\n\n"
merge_result += """#include <cmath>
#include <concepts>
#if defined(AI_DEBUG)
  #include <assert.h>
#endif
#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__) || defined(__SSSE3__)
  #include <immintrin.h>
#elif defined(__ARM_NEON)
  #include <arm_neon.h>
#elif defined(__wasm_simd128__)
  #include <wasm_simd128.h>
#else
  #error "unknown arch"
#endif\n\n
namespace include_ai {\n\n\n
"""

remove = ["#include", "_HPP", "double include", "x86_64", "aarch64", "defined(__wasm__)", "nothing?", "unknown", "namespace include_ai"]

for filename in unique:
    f = open(filename[0], "r")
    lines = f.readlines()
    for code_line in lines:
        if any(x in code_line for x in remove):
            pass
        else:
            merge_result += code_line
    merge_result += "\n\n"

merge_result += """} // namespace include_ai\n\n\n
#endif // INCLUDEAI_IMPLEMENTATION\n\n
#endif // INCLUDEAI_HPP\n
"""
merge_result += license

open("./includeai.hpp", "w+").write(merge_result + "\n")

print("success")
