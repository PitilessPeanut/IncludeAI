#include "neural.hpp"
#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__) || defined(__SSSE3__)
  #include <immintrin.h>
#elif defined(__ARM_NEON)
  #include <arm_neon.h>
#elif defined(__wasm_simd128__)
  #include <wasm_simd128.h>
#else // not "__AVX__" etc.
  #error "unknown arch"
#endif // unknown


/****************************************/
/*                     "Neural network" */
/****************************************/

/****************************************/
/*                                Tests */
/****************************************/








/****************************************/
/*                             Research */
/****************************************/
/*
    https://mohitmishra786.github.io/UnderTheHood/ <- lots of nn stuff!
    https://www.jeremyong.com/cpp/machine-learning/2020/10/23/cpp-neural-network-in-a-weekend/
    https://www.youtube.com/watch?v=AYuyN8vvkAM (not simd...)
    https://www.youtube.com/watch?v=KphmOJnLAdI matrix math


*/
