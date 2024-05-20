#include "micro_math.hpp"


/****************************************/
/*                           PCG Random */
/****************************************/
    constexpr pcg32::pcg32(const UQWORD seed)
      : state(0), inc(0)
    {
        const UQWORD initstate = seed << 31 | seed;
        const UQWORD initseq   = seed << 31 | seed;
        inc = (initseq << 1) | 1;
        state = initstate + inc;
        state = state * PCG_DEFAULT_MULTIPLIER + inc;
    }

    constexpr UDWORD pcg32::operator()()
    {
        // PCG Random number generation (http://www.pcg-random.org):
        const UQWORD oldstate = state;
        // Advance internal state:
        state = oldstate * PCG_DEFAULT_MULTIPLIER + (inc | 1);
        // Calculate output function (XSH RR), uses old state for max ILP:
        const UDWORD xorshifted = UDWORD(((oldstate >> 18u) ^ oldstate) >> 27u);
        const UDWORD rot = oldstate >> 59u;
        return (xorshifted >> rot) | (xorshifted << ((~rot) & 31));
    }

    UDWORD pcg32rand(const UQWORD seed)
    {
        static pcg32 generator(seed);
        return generator();
    }


    constexpr pcg16::pcg16(const UDWORD seed)
      : state(0), inc(0)
    {
        const UDWORD initstate = seed << 15 | seed;
        const UDWORD initseq   = seed << 15 | seed;
        inc = (initseq << 1) | 1;
        state = initstate + inc;
        state = state * PCG_DEFAULT_MULTIPLIER + inc;
    }

    constexpr UWORD pcg16::operator()()
    {
        // PCG Random number generation (http://www.pcg-random.org):
        const UDWORD oldstate = state;
        // Advance internal state:
        state = oldstate * PCG_DEFAULT_MULTIPLIER + (inc | 1);
        // Calculate output function (XSH RR), uses old state for max ILP:
        const UWORD xorshifted = UWORD(((oldstate >> 10u) ^ oldstate) >> 12u);
        const UWORD rot = oldstate >> 28u;
        return (xorshifted >> rot) | (xorshifted << ((~rot) & 15));
    }

    UWORD pcg16rand(const UDWORD seed)
    {
        static pcg16 generator(seed);
        return generator();
    }


/****************************************/
/*                                Tests */
/****************************************/
    static_assert([]
                  {
                      char srcAlphaA[8] = {'a','b','c','d','e','f','g','h'};
                      FisherYates(srcAlphaA, 8, 1337);
                      bool ok = srcAlphaA[0] == 'b';
                      ok = ok && srcAlphaA[1] == 'a';
                      ok = ok && srcAlphaA[2] == 'h';
                      ok = ok && srcAlphaA[3] == 'f';
                      ok = ok && srcAlphaA[4] == 'd';
                      ok = ok && srcAlphaA[5] == 'c';
                      ok = ok && srcAlphaA[6] == 'e';
                      ok = ok && srcAlphaA[7] == 'g';

                      char srcAlphaB[8] = {'a','b','c','d','e','f','g','h'};
                      FisherYates(srcAlphaB, 8, 696963);
                      ok = ok && srcAlphaB[0] == 'd';
                      ok = ok && srcAlphaB[1] == 'c';
                      ok = ok && srcAlphaB[2] == 'f';
                      ok = ok && srcAlphaB[3] == 'g';
                      ok = ok && srcAlphaB[4] == 'h';
                      ok = ok && srcAlphaB[5] == 'b';
                      ok = ok && srcAlphaB[6] == 'e';
                      ok = ok && srcAlphaB[7] == 'a';

                      return ok;
                  }()
                 );






/****************************************/
/*                             Research */
/****************************************/
/*
    GCD, LCM
    greatest common divisor  24,60 ⇒ 12
    lowest common multiple    8,10 ⇒ 40


    matmul:
    for (int y=0; y<4; ++y)
    for (int x=0; x<4; ++x)
    for (int k=0; k<4; ++k)
        product[(y*4)+x] += mat[(y*4)+k] + mat[(k*4)+x];
*/

