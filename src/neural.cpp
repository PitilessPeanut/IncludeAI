#include "neural.hpp"
#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__) || defined(__SSSE3__)
  #include <immintrin.h>
#elif defined(__ARM_NEON)
  #include <arm_neon.h>
#elif defined(__wasm_simd128__)
  #include <wasm_simd128.h>
#else // not "__AVX__" etc.
  #error "unknown arch! (if compiling for wasm use '-msimd128' to fix)"
#endif // unknown


//extern "C" int is_prime(int n);


/****************************************/
/*                      feed forward 32 */
/****************************************/

/****************************************/
/*           feed forward 16 (IEEE 754) */
/****************************************/

/****************************************/
/*                                Tests */
/****************************************/
    static_assert([]
                  {
                      constexpr int columns = 10;
                      float weights[columns * columns] =
                          { /* From:      0    1       2    3      4     5     6     7      8     9 */
                            /* To: 0 */ 0.0f, 0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
                            /*     1 */ 0.0f, 0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
                            /*     2 */ 0.5f, 0.8f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
                            /*     3 */-0.4f, 0.1f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
                            /*     4 */ 0.9f,-0.2f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
                            /*     5 */ 0.0f, 0.0f, -1.2f, 1.1f, 0.3f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
                            /*     6 */ 0.0f, 0.0f, -0.8f, 0.4f,-0.9f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
                            /*     7 */ 0.0f, 0.0f,  0.2f, 0.7f,-0.8f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
                            /*     8 */ 0.0f, 0.0f,  0.0f, 0.0f, 0.0f,  1.3f,-0.4f, 0.6f,  0.0f, 0.0f,
                            /*     9 */ 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, -0.5f, 0.2f,-0.1f,  0.0f, 0.0f
                          };
                      int i=0;
                      auto rng = [&weights, &i]()
                      {
                          return weights[i];
                      };
                      auto correct0 = Neural<2,2,3,decltype(rng)>::BitArray<columns*columns>{
                                          0b0000000000000000000011000000001100000000110000000000000000000000,
                                          0b0000000000000000000000000000000000000000000000000000000000000000
                                      };
                      auto correct1 = Neural<2,2,3,decltype(rng)>::BitArray<columns*columns>{
                                          0b0000000000000000000000000000000000000000000000000000111000000011,
                                          0b1000000011100000000000000000000000000000000000000000000000000000
                                      };
                      auto correct2 = Neural<2,2,3,decltype(rng)>::BitArray<columns*columns>{
                                          0b0000000000000000000000000000000000000000000000000000000000000000,
                                          0b0000000000000000000001110000000111000000000000000000000000000000
                                      };
                      return true; // todo!!!
                  }()
                 );








/****************************************/
/*                             Research */
/****************************************/
/*
    https://mohitmishra786.github.io/UnderTheHood/ <- lots of nn stuff!
    https://www.jeremyong.com/cpp/machine-learning/2020/10/23/cpp-neural-network-in-a-weekend/
    https://www.youtube.com/watch?v=AYuyN8vvkAM (not simd...)
    https://www.youtube.com/watch?v=KphmOJnLAdI matrix math


*/
