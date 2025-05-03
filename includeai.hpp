/*
    You must add '#define INCLUDEAI_IMPLEMENTATION' before '#include'ing this in ONE source file.
    Like this:
        #define INCLUDEAI_IMPLEMENTATION
        #include "includeai.hpp"

    AUTHOR
        Pitiless Peanut (aka. Shaiden Spreitzer, Professor Peanut, etc...) of VECTORPHASE
        
    LICENSE 
        BSD 4-Clause (See end of file)
*/

#ifndef INCLUDEAI_HPP
#define INCLUDEAI_HPP

#ifdef INCLUDEAI_IMPLEMENTATION


#include <cmath>
#include <concepts>
#if defined(AI_DEBUG)
  #include <assert.h>
#endif
#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
  #include <immintrin.h>
#elif defined(__aarch64__)
  #include <arm_neon.h>
#elif defined(__wasm__)
  #include <wasm_simd128.h>
#else
  #error "unknown arch"
#endif


namespace include_ai {




// 32 bit:
typedef decltype([]
                 {
                     constexpr auto u64a = 0ull;
                     constexpr auto u32a = 0ul;
                     constexpr auto u32b = 0u;
                     if constexpr (sizeof(u32a) == (sizeof(u64a) / 2))
                         return u32a;
                     else if constexpr (sizeof(u32b) == (sizeof(u64a) / 2))
                         return u32b;
                 }()
                )
        UDWORD;
typedef decltype([]
                 {
                     constexpr auto i64a = 0ll;
                     constexpr auto i32a = 0l;
                     constexpr auto i32b = 0;
                     if constexpr (sizeof(i32a) == (sizeof(i64a) / 2))
                         return i32a;
                     else if constexpr (sizeof(i32b) == (sizeof(i64a) / 2))
                         return i32b;
                 }()
                )
        SDWORD;
static_assert(sizeof(UDWORD) == sizeof(SDWORD));
static_assert([]{ constexpr UDWORD x=~0; return x>>31; }());
#ifndef BASETYPES
  typedef UDWORD ULONG;
#else
  static_assert(sizeof(UDWORD) == sizeof(ULONG));
#endif
typedef SDWORD SLONG;

// 64 bit:
typedef decltype([]{ constexpr auto u64=0ull; return u64; }()) UQWORD;
typedef decltype([]{ constexpr auto i64=0ll ; return i64; }()) SQWORD;
static_assert(sizeof(UQWORD) == sizeof(SQWORD));
static_assert([]{ constexpr UQWORD x=~0; return x>>63; }());
static_assert(sizeof(UQWORD) == (sizeof(UDWORD)<<1));

// 16 bit:
typedef decltype([]{ constexpr unsigned short u16=0; return u16; }()) UWORD;
typedef decltype([]{ constexpr signed short i16=0; return i16; }()) SWORD;
static_assert(sizeof(UWORD) == sizeof(SWORD));
static_assert([]{ constexpr UWORD x=~0; return x>>15; }());
static_assert(sizeof(UDWORD) == (sizeof(UWORD)<<1));
#ifndef BASETYPES
  typedef UWORD USHORT;
#else
  static_assert(sizeof(UWORD) == sizeof(USHORT));
#endif
typedef SWORD SSHORT;

// 8 bit:
constexpr decltype(sizeof(int)) CHARBITS = []{ return 32u / sizeof( UDWORD ); }();
typedef unsigned char UBYTE;
typedef signed char SBYTE;
static_assert((sizeof(UBYTE)*CHARBITS) == 8);
static_assert(sizeof(UBYTE) == sizeof(SBYTE));

// "real" numbers:
typedef float FLOAT;
typedef FLOAT REAL;
static_assert(sizeof(FLOAT) == sizeof(UDWORD));
typedef double DOUBLE;
typedef DOUBLE LREAL;
static_assert(sizeof(DOUBLE) == sizeof(UQWORD));









/****************************************/
/*                                Tools */
/****************************************/
// ctz
    template <typename Int>
    constexpr int ctz_comptime(Int val)
    {
        Int c=0;  // output: c will count val's trailing zero bits,
                  // so if val is 1101000, then c will be 3
        if (val)
        {
            val = (val ^ (val - 1)) >> 1;  // Set val's trailing 0s to 1s and zero rest
            for (; val; c++)
                val >>= 1;
        }
        else
        {
            c = CHARBITS * sizeof(val);
        }
        return c;
    }

    int ctz_runtime(const SDWORD);
    int ctz_runtime(const UDWORD);
    int ctz_runtime(const UQWORD);
    int ctz_runtime(const  UBYTE);


// rotate
    template <typename Int>
    constexpr Int rotl_comptime(Int x, int amount)
    {
        return ((x << amount) | (x >> ((sizeof(Int)*CHARBITS) - amount)));
    }

    SDWORD rotl_runtime(const SDWORD x, int amount);
    UDWORD rotl_runtime(const UDWORD x, int amount);
    UQWORD rotl_runtime(const UQWORD x, int amount);
    UBYTE  rotl_runtime(const  UBYTE x, int amount);


// min/max
    template <typename T>
    inline constexpr T bitAllocatorMin(T x, T y) { return x < y ? x : y; }
    template <typename T>
    inline constexpr T bitAllocatorMax(T x, T y) { return x > y ? x : y; }


/****************************************/
/*                             Concepts */
/****************************************/
    template<typename T>
    concept BitfieldIntType = std::is_unsigned_v<T> && std::is_integral_v<T>;


/****************************************/
/*                               Config */
/****************************************/
    enum class BitAlloc_Mode { FAST, TIGHT };


/****************************************/
/*                                 Impl */
/****************************************/
    template <int Size, BitfieldIntType BitfieldType, BitAlloc_Mode mode=BitAlloc_Mode::FAST, bool comptime=false>
    struct BitAlloc
    {
    private:
        static constexpr int Intbits = sizeof(BitfieldType)*CHARBITS;
        static constexpr int NumberOfBuckets = (Size%Intbits)==0 ? (Size/Intbits) : (Size/Intbits)+1;
    public:
        BitfieldType bucketPool[NumberOfBuckets] = {0};
    public:
        constexpr auto largestAvailChunk(const int desiredSize)
        {
            struct Pos
            {
                int posOfAvailChunk;
                int length;
            };

            auto ctz = [](auto val)
                       {
                           if constexpr (comptime)
                               return ctz_comptime(val);
                           else
                               return ctz_runtime(val);
                       };

            auto rotl = [](auto x, int amount)
                        {
                            if constexpr (comptime)
                                return rotl_comptime(x, amount);
                            else
                                return rotl_runtime(x, amount);
                        };

            int discoveredSize = bitAllocatorMin(desiredSize, Intbits*NumberOfBuckets);
            while (discoveredSize>0)
            {
                constexpr BitfieldType everyBitSet = ~0;
                if (discoveredSize <= Intbits)
                {
                    for (int bucketNo=0; bucketNo<NumberOfBuckets; ++bucketNo)
                    {
                        const int availTail = ctz(bucketPool[bucketNo]);
                        if (availTail >= discoveredSize)
                        {
                            BitfieldType mask = everyBitSet << discoveredSize; // the 'discoveredSize' here is always <= than Intbits
                            mask = rotl(mask, availTail-discoveredSize);
                            bucketPool[bucketNo] = bucketPool[bucketNo] | ~mask;

                            const BitfieldType posOfAvailChunk = (bucketNo*Intbits) + (Intbits-availTail);
                            return Pos{ .posOfAvailChunk = static_cast<int>(posOfAvailChunk)
                                      , .length = static_cast<int>(discoveredSize)
                                      };
                        }
                    }
                }
                else
                {
                    if constexpr (mode == BitAlloc_Mode::FAST)
                    {
                        const int bucketsRequired = discoveredSize/Intbits;
                        const int remainingBits = discoveredSize-(Intbits*bucketsRequired);
                        const bool needsExactFullBuckets = remainingBits == 0;
                        for (int bucketNo=0; bucketNo<(NumberOfBuckets-bucketsRequired+needsExactFullBuckets); ++bucketNo)
                        {
                            bool avail = true;
                            for (int i=0; i<bucketsRequired; ++i)
                                avail = avail && bucketPool[bucketNo+i] == 0;

                            if (avail)
                            {
                                for (int i=0; i<bucketsRequired; ++i)
                                    bucketPool[bucketNo+i] = everyBitSet;

                                if (remainingBits > 0)
                                {
                                    const BitfieldType mask = everyBitSet << (Intbits - remainingBits);
                                    bucketPool[bucketNo+bucketsRequired-needsExactFullBuckets] = mask;
                                }
                                return Pos{ .posOfAvailChunk = static_cast<int>(bucketNo*Intbits)
                                          , .length = static_cast<int>(discoveredSize)
                                          };
                            }
                        }
                    }
                    else if constexpr (mode == BitAlloc_Mode::TIGHT)
                    {

//                    const int headBitsUsed = ctz(bucketPool[bucketNo]);
//                    int remainingBits = discoveredSize - headBitsUsed;
//                    int emptyBuckets = 0;
//
//                    bool avail = true;
//                    while (remainingBits > 0 && avail)
//                    {
//                        emptyBuckets += 1;
//                        remainingBits -= Intbits;
//                        avail = avail && bucketPool[bucketNo+emptyBuckets] == 0;
//                    }
//
//                    if (avail)
//                    {
//                        constexpr Inttype everyBitSet = -1;
//                        bucketPool[bucketNo] = bucketPool[bucketNo] | ~(everyBitSet << headBitsUsed);
//                        bucketPool[bucketNo+emptyBuckets] = everyBitSet << (Intbits - ((discoveredSize-headBitsUsed)%Intbits));
//                        const int usedBuckets = emptyBuckets+1;
//                        while (emptyBuckets--)
//                            bucketPool[bucketNo+emptyBuckets] = -1;
//                        return Pos{ .pos = ((bucketNo*usedBuckets)*Intbits)
//                                         + ((discoveredSize-headBitsUsed)%Intbits)
//                                  , .len = discoveredSize
//                                  };
//                    }
                    }
                }
                discoveredSize -= 1;
            } // while (discoveredSize>0)

            return Pos{ .posOfAvailChunk = -1, .length = 0 };
        }

        constexpr void free(const int pos, const int len)
        {
            // two bugs: pos 0, pos 64 len 128!
            constexpr BitfieldType everyBitSet = ~0;
            int startBucket = pos/Intbits;
            int endBucket = (pos+len)/Intbits;
            const BitfieldType maskHead = everyBitSet << (Intbits - (pos%Intbits));
            BitfieldType maskTail = everyBitSet << (Intbits - ((pos+len)%Intbits));

            // fix nasty bug where additional slots get deleted when the delete range reaches the edge:
            if (((pos+len)-(endBucket*Intbits)) == 0) // ((pos+len) % Intbits) == 0
                maskTail = 0;

            if (startBucket == endBucket)
            {
                bucketPool[startBucket] = (maskHead | ~maskTail) & bucketPool[startBucket];
            }
            else
            {
                // Start
                bucketPool[startBucket] = maskHead & bucketPool[startBucket];

                // End
                bucketPool[endBucket] = ~maskTail & bucketPool[endBucket];

                // Middle
                endBucket -= 1;
                while (&bucketPool[startBucket++] != &bucketPool[endBucket])
                    bucketPool[startBucket] = 0;
            }
        }

        constexpr void clearAll()
        {
            for (int i=0; i<NumberOfBuckets; ++i)
                bucketPool[i] = 0;
        }
    };






/****************************************/
/*                                Tools */
/****************************************/
// ctz
    int ctz_runtime(const UBYTE val)
    {
        #ifdef _WIN32
          return _tzcnt_u32( (unsigned int)val );
        #else
          return (val==0) ? (sizeof(UBYTE )*CHARBITS) : __builtin_ctz(val);
        #endif
    }

    int ctz_runtime(const UDWORD val)
    {
        #ifdef _WIN32
          return _tzcnt_u32( val );
        #else
          return (val==0) ? (sizeof(UDWORD)*CHARBITS) : __builtin_ctzl(val);
        #endif
    }

    int ctz_runtime(const UQWORD val)
    {
        #ifdef _WIN32
          return _tzcnt_u64( val );
        #else
          return (val==0) ? (sizeof(UQWORD)*CHARBITS) : __builtin_ctzll(val);
        #endif
    }

    int ctz_runtime(const SDWORD val)
    {
        #ifdef _WIN32
          return _tzcnt_u32( (unsigned int)val );
        #else
          return (val==0) ? (sizeof(SDWORD)*CHARBITS) : __builtin_ctz(val);
        #endif
    }


// rotate
    SDWORD rotl_runtime(const SDWORD x, int amount)
    {
        #ifdef _WIN32
          return _rotl(x, amount);
        #else
          return rotl_comptime(x, amount);
        #endif
    }

    UDWORD rotl_runtime(const UDWORD x, int amount)
    {
        #ifdef _WIN32
          _rotl(x, amount);
        #else
          return rotl_comptime(x, amount);
        #endif
    }

    UQWORD rotl_runtime(const UQWORD x, int amount)
    {
        #ifdef _WIN32
          return _rotl64(x, amount);
        #else
          return rotl_comptime(x, amount);
        #endif
    }

    UBYTE  rotl_runtime(const UBYTE x, int amount)
    {
        #ifdef _WIN32
          return _rotl8(x, amount);
        #else
          return rotl_comptime(x, amount);
        #endif
    }


/****************************************/
/*                                Tests */
/****************************************/
    static_assert([]
                  {
                      constexpr bool comptime = true;
                      BitAlloc<1*CHARBITS, UBYTE, BitAlloc_Mode::FAST, comptime> testAllocator;
                      auto pos = testAllocator.largestAvailChunk(3);
                      bool ok = pos.posOfAvailChunk==0 && pos.length==3;
                      pos = testAllocator.largestAvailChunk(7); // Edge case!
                      ok = ok && pos.posOfAvailChunk==3 && pos.length==5;
                      return ok;
                  }()
                 );

    static_assert([]
                  {
                      constexpr bool comptime = true;
                      BitAlloc<5*CHARBITS, UBYTE, BitAlloc_Mode::FAST, comptime> testAllocator;
                      auto pos = testAllocator.largestAvailChunk(3);
                      bool ok = pos.posOfAvailChunk==0 && pos.length==3;
                      pos = testAllocator.largestAvailChunk(3);
                      ok = ok && pos.posOfAvailChunk==3 && pos.length==3;
                      pos = testAllocator.largestAvailChunk(2);
                      ok = ok && pos.posOfAvailChunk==6 && pos.length==2;
                      ok = ok && testAllocator.bucketPool[0] == 0b11111111;
                      testAllocator.free(3, 3);
                      ok = ok && testAllocator.bucketPool[0] == 0b11100011;
                      pos = testAllocator.largestAvailChunk(5);
                      ok = ok && pos.posOfAvailChunk==8 && pos.length==5;
                      ok = ok && testAllocator.bucketPool[1] == 0b11111000;
                      pos = testAllocator.largestAvailChunk(18);
                      ok = ok && pos.posOfAvailChunk==16 && pos.length==18;
                      ok = ok && testAllocator.bucketPool[0] == 0b11100011;
                      ok = ok && testAllocator.bucketPool[1] == 0b11111000;
                      ok = ok && testAllocator.bucketPool[2] == 255;
                      ok = ok && testAllocator.bucketPool[3] == 255;
                      ok = ok && testAllocator.bucketPool[4] == 0b11000000;
                      pos = testAllocator.largestAvailChunk(7);
                      ok = ok && pos.posOfAvailChunk==34;
                      ok = ok && pos.length==6;
                      testAllocator.free(10, 24);
                   //   ok = ok && testAllocator.bucketPool[1] == 0b11000000;
                  //    ok = ok && testAllocator.bucketPool[2] == 0;
                  //    ok = ok && testAllocator.bucketPool[3] == 0;
                  //    ok = ok && testAllocator.bucketPool[4] == 0b00111111;
                  //    testAllocator.free(1, 1);
                 //     ok = ok && testAllocator.bucketPool[0] == 0b10100011;
                   //   testAllocator.free(12, 5);
                   //   ok = ok && testAllocator.bucketPool[1] == 0b11000000;
                   //   ok = ok && testAllocator.bucketPool[2] == 0;
                   //   testAllocator.free(30, 6);
                   //   ok = ok && testAllocator.bucketPool[3] == 0;
                   //   ok = ok && testAllocator.bucketPool[4] == 0b00001111;

//                      posB = testAllocator.largestAvailChunk(20);
     //   ok = ok && pos.pos==31 ; //&& pos.length<20;
       // ok = ok && testAllocator.bucketPool[4] == 0b00011111;

                      return ok;
                  }()
                 );





/****************************************/
/*                 Fisher-Yates shuffle */
/****************************************/
    template <typename T>
    constexpr void FisherYates(T *container, const int len, const int s)
    {
        for (int k = 0; k < len; ++k)
        {
            const int r = k + s % (len - k);
            T temp = container[k];
            container[k] = container[r];
            container[r] = temp;
        }
    }


/****************************************/
/*                           PCG Random */
/****************************************/
    class pcg32
    {
    private:
        static constexpr auto PCG_DEFAULT_MULTIPLIER = 6364136223846793005ull;
        UQWORD state, inc;
    public:
        explicit constexpr pcg32(const UQWORD seed = 0);

        constexpr UDWORD operator()();
    };

    class pcg16
    {
    private:
        static constexpr auto PCG_DEFAULT_MULTIPLIER = 747796405u;
        UDWORD state, inc;
    public:
        explicit constexpr pcg16(const UDWORD seed = 0);

        constexpr UWORD operator()();
    };

    UDWORD pcg32rand(const UQWORD seed);
    UWORD  pcg16rand(const UDWORD seed);

    template <typename Int>
    auto pcgRand(const Int seed=0)
    {
        if constexpr (sizeof(Int) == sizeof(UQWORD))
            return pcg32rand(seed);
        else
            return pcg16rand(seed);
    }


/****************************************/
/*                                fibon */
/****************************************/
    template <typename Int>
    constexpr Int fib(const Int i)
    {
        if (i<=1)
            return i;
        return fib(i-1) + fib(i-2);
    }






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
    https://learnopengl.com/Getting-started/Transformations
    for (int y=0; y<4; ++y)
    for (int x=0; x<4; ++x)
    for (int k=0; k<4; ++k)
        product[(y*4)+x] += mat[(y*4)+k] + mat[(k*4)+x];
*/



/****************************************/
/*                              pattern */
/****************************************/
    template <int PatternSize>
    struct Pattern
    {
        static constexpr int patternSize = PatternSize;
        bool pattern[PatternSize] = {0}; // todo: bit array!
    };


/****************************************/
/*                              helpers */
/****************************************/
    float sqrt_runtime(float x);

    template <bool comptime=false>
    constexpr float similarity_sqrt(float x)
    {
        if constexpr (comptime)
        {
            if (x <= 0) { return 0; }
            float guess = x / 2.;
            float prev_guess = 0.;
            // Iterate until the guess stabilizes:
            for (int i = 0; i < 100; ++i)
            {
                prev_guess = guess;
                guess = (guess + x / guess) / 2.;
                // Check for convergence:
                if (prev_guess == guess) { break; }
            }
            return guess;
        }
        else
        {
            return sqrt_runtime(x);
        }
    }

    template <typename ValType, int PatternSize, bool comptime=false>
    constexpr float calculateNormalizedDotProduct(const ValType *hay, const ValType *needle)
    {
        float dotProduct = 0.0f;
        float normA = 0.0f;
        float normB = 0.0f;

        for (int j=0; j<PatternSize; ++j)
        {
            dotProduct += hay[j] * needle[j];
            normA += hay[j] * hay[j];
            normB += needle[j] * needle[j];
        }

        normA = similarity_sqrt<comptime>(normA);
        normB = similarity_sqrt<comptime>(normB);

        float similarity = 0.0f;
        if (normA > 0 && normB > 0) {
            similarity = dotProduct / (normA * normB);
        }
        return similarity;
    }


    template <int PatternSize>
    constexpr float calculateJaccardSimilarity(const bool *hay, const bool *needle)
    {
        int intersection = 0;
        int unionCount = 0;

        for (int i = 0; i < PatternSize; ++i)
        {
            // For binary data, intersection is the count of positions where both are 1
            if (hay[i] && needle[i]) {
                intersection++;
            }

            // Union is the count of positions where at least one is 1
            if (hay[i] || needle[i]) {
                unionCount++;
            }
        }

        // Avoid division by zero
        if (unionCount == 0) {
            return 0.0f; // Both arrays are all zeros
        }

        // Jaccard similarity = size of intersection / size of union
        return static_cast<float>(intersection) / unionCount;
    }


/****************************************/
/*                   similarity scoring */
/*   - Why cosine and not Jaccard? -    */
/* In chess, the overall pattern/       */
/* structure is often more important    */
/* than raw counts. Cosine similarity   */
/* better captures the "shape" or       */
/* configuration of the position        */
/****************************************/
    template <typename Pattern>
    constexpr float diversify(Pattern *dst, const Pattern& input, const int maxPatterns)
    {
        // Find a pair that is MOST similar:
        float maxSimilarity = -1.0f;
        int idx1 = 0, idx2 = 0;
        for (int i=0; i<maxPatterns; ++i)
        {
            for (int j=i+1; j<maxPatterns; ++j)
            {
                const float similarity = calculateNormalizedDotProduct<bool, Pattern::patternSize>(dst[i].pattern, dst[j].pattern);
                if (similarity>maxSimilarity)
                {
                    maxSimilarity = similarity;
                    idx1 = i;
                    idx2 = j;
                }
            }
        }

        // Decide which of the pair to replace
        const float sim1 = calculateNormalizedDotProduct<bool, Pattern::patternSize>(dst[idx1].pattern, input.pattern);
        const float sim2 = calculateNormalizedDotProduct<bool, Pattern::patternSize>(dst[idx2].pattern, input.pattern);

        // Select the pattern that has higher similarity with input (we'll replace the one that's more redundant)
        const int replaceIdx = (sim1 < sim2) ? idx2 : idx1;

        // Replace only if the new pattern would decrease the maximum similarity
        // Compute similarities of replacement with all other patterns
        float maxNewSimilarity = -1.0f;
        for (int i = 0; i < maxPatterns; ++i)
        {
            if (i == replaceIdx) continue;
            const float potentialSim = calculateNormalizedDotProduct<bool, Pattern::patternSize>(dst[i].pattern, input.pattern);
            maxNewSimilarity = potentialSim > maxNewSimilarity ? potentialSim : maxNewSimilarity;
        }

        // Replace if doing so would reduce the maximum similarity in the collection
        if (maxNewSimilarity < maxSimilarity)
        {
            for (int i = 0; i < dst[replaceIdx].patternSize; ++i)
                dst[replaceIdx].pattern[i] = input.pattern[i];

            return maxNewSimilarity; // Return the new maximum similarity
        }
        else
        {
            // Not replacing - new pattern would increase similarity:
            return maxSimilarity; // Keep the current maximum similarity
        }
    }






/****************************************/
/*                              helpers */
/****************************************/
    float sqrt_runtime(float x)
    {
        return std::sqrt(x);
    }


/****************************************/
/*                                Tests */
/****************************************/
    static_assert([]
                  {
                      float haystack1[8] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
                      float haystack2[8] = {1.0f, 3.0f, 5.0f, 7.0f, 9.0f, 0.0f, 0.0f, 1.0f};
                      const float needle[8] = {1.0f, 3.0f, 5.0f, 7.0f, 9.0f, 0.0f, 0.0f, 0.0f};
                      const float similarity1 = calculateNormalizedDotProduct<float, 8, true>(haystack1, needle);
                      const float similarity2 = calculateNormalizedDotProduct<float, 8, true>(haystack2, needle);
                      bool ok = similarity1 < .4014f && similarity2 > .41f;
                      ok = similarity1 < .83f && similarity2 > .82f;
                      return ok;
                  }()
                 );



#if defined(AI_DEBUG)
#endif



/****************************************/
/*       Util functions / configuration */
/****************************************/
// sqrt
    inline constexpr float aiSqrt(auto x) { return std::sqrt(x-0.f); }

// log
    inline constexpr float aiLog(auto x) { return std::log(x-0.f); }

// abs
    inline constexpr auto aiAbs(auto x) { return std::abs(x); }

// min/max
    inline float aiMin(float x, float y) { return std::fminf(x, y); }
    inline float aiMax(float x, float y) { return std::fmaxf(x, y); }
    template <typename T>
    inline constexpr T aiMin(T x, T y) { return x < y ? x : y; }
    template <typename T>
    inline constexpr T aiMax(T x, T y) { return x > y ? x : y; }

// assert/debug
    #if defined(AI_DEBUG)
      #define aiAssert(x) assert(x)
      #define aiDebug(x) x
    #else
      #define aiAssert(x)
      #define aiDebug(x)
    #endif


/****************************************/
/*   A 'move'/'action' by a user/player */
/* within the game context              */
/****************************************/
    template <typename T>
    concept GameMove = std::movable<T> || std::copyable<T>;


/****************************************/
/*            outcome of a single round */
/****************************************/
    enum class Outcome { running, draw, fin, invalid };


/****************************************/
/*          A view of the game from the */
/* perspective of a player as a Concept */
/*                                      */
/* This 'view' can be either the actual */
/* game state (complete information) or */
/* a guess/estimation of the game state */
/* percieved by one player (incomplete  */
/* information)                         */
/****************************************/
  //  template <typename T>
  //  concept convertible_to_ptrFloat =
  //      std::convertible_to<T, HALF *> ||
  //      std::convertible_to<T, DOUBLE *> ||
   //     std::convertible_to<T, FLOAT *>;

    template <typename T>
    concept Gameview =
        !std::copy_constructible<T> &&
        !std::is_copy_assignable<T>::value &&
        !std::copyable<T> &&
        requires (T obj, const T cobj)
        {
            {cobj.clone()} -> std::same_as<T>;
            {obj.generateMovesAndGetCnt(nullptr)} -> std::convertible_to<int>;
            {obj.doMove(int())} -> std::same_as<Outcome>;
            {obj.switchPlayer()};
            {cobj.getCurrentPlayer()} -> std::equality_comparable;
            {cobj.getWinner()} -> std::equality_comparable;
           // {obj.getNetworkInputs()} -> convertible_to_ptrFloat;
        };


/****************************************/
/*       Node (should be converted from */
/*        an 'array of structs' into a  */
/*       'struct of arrays'...)         */
/* Attn: The Node does *not* store a    */
/* copy of the Board, instead the game  */
/* state must be kept in sync during    */
/* tree expansion (HEADACHE ATTACK!!!)  */
/****************************************/
    template <GameMove Move>
    struct Node
    {
        static constexpr SWORD never_expanded = -1;
        static constexpr SWORD removed = -2; // Debug
        SWORD    activeBranches = never_expanded; // Must be signed!
        SWORD    createdBranches = 0;             // Must be signed!
        Node    *parent = nullptr;
        Node    *branches = nullptr;  // Must be initialized to NULL
        FLOAT    score = 0.f; //0.01f;    // Start with some positive value to prevent divide by zero error
        Move     moveHere;
        SWORD    visits = 1; // Must be '1' to stop 'nan' // todo if changed also chage in ucbselect()!
        FLOAT    UCBscore;         // Placeholder used during UCB calc.
        #ifdef INCLUDEAI__ADD_SCORE_FOR_TERMINAL_NODES
          FLOAT    terminalScore = 0.f; // Keeping another separate score for terminal nodes may not make sense,
                                        // since a terminal condition may pop up at any level of the tree,
                                        // which means that by the time we are back at the root, a wrong node
                                        // could end up having a higher score than the correct one...
        #endif
SWORD branchScore = 0;
int shallowestTerminalDepth = 9999;

        constexpr Node()
          : moveHere(0)
        {}

        constexpr Node(Node *newParent, Move move)
          : activeBranches(never_expanded),
            parent(newParent),
            // Ownership is implicit: the 'owner' is the player who makes this move!
            moveHere(move)
        {}

        Node(const Node&)            = delete;
        Node& operator=(const Node&) = delete;
        Node(Node&&)                 = delete;
        Node& operator=(Node&& other)
        {
            if (this != &other)
            {
                activeBranches  = other.activeBranches;
                createdBranches = other.createdBranches;
                parent          = other.parent;
                branches        = other.branches;
                visits          = other.visits;
                score           = other.score;
                #if 0
                  UCBscore = other.UCBscore;
                #endif
                moveHere        = other.moveHere;
            //    branchScore = other.branchScore;
                shallowestTerminalDepth = other.shallowestTerminalDepth;
                for (int i=0; i<other.createdBranches; ++i)
                    branches[i].parent = this;
            }
            return *this;
        }
    };

    static_assert(sizeof(Node<SQWORD>) <= 64);


/****************************************/
/*                           Ai context */
/****************************************/
    template <int NumNodes, GameMove MoveType, BitfieldIntType BitfieldType, class Pattern, int MaxPatterns>
    struct Ai_ctx
    {
        static constexpr int numNodes = NumNodes;
        BitAlloc<NumNodes, BitfieldType> bitalloc;
        Node<MoveType> nodePool[NumNodes];

        static constexpr int maxPatterns = MaxPatterns;
        int storedPatterns = 0;
        Pattern patterns[maxPatterns];
        float maxPatternSimilarity = 1.f;

        Ai_ctx() {}
        Ai_ctx(const Ai_ctx&) = delete;
        Ai_ctx& operator=(const Ai_ctx&) = delete;

        template <Gameview Board>
        void collectPattern(Board& current)
        {
            const Pattern *ptrInputs = current.getNextPattern();
            while (ptrInputs != nullptr)
            {
                if (storedPatterns < maxPatterns)
                {
                    Pattern& target = patterns[storedPatterns];
                    for (int i=0; i<target.patternSize; ++i)
                        target.pattern[i] = ptrInputs->pattern[i];
                    storedPatterns += 1;
                }
                else
                {
                    maxPatternSimilarity = diversify(patterns, *ptrInputs, maxPatterns);
                }
                ptrInputs = current.getNextPattern();
            }
        }

        template <Gameview Board>
        void train(Board& current)
        {
            const Pattern *ptrInputs = current.getNextPattern();
            // todo
        }

        template <Gameview Board>
        float query(Board& current) const
        {
            // todo
            const float win  = 1;// inputs[0];
            const float lose = 1; //-inputs[1];
            return (win+lose) / 2.f;
        }
    };


/****************************************/
/*    Search tree node helper functions */
/****************************************/
    template <int NumNodes, GameMove MoveType, BitfieldIntType BitfieldType, class Pattern, int NumPatterns>
    inline Node<MoveType> *disconnectBranch(Ai_ctx<NumNodes, MoveType, BitfieldType, Pattern, NumPatterns>& ai_ctx,
                                            Node<MoveType> *parent,
                                            const Node<MoveType> *removeMe
                                           )
    {
        aiAssert(parent);
        aiAssert(parent->activeBranches > 0);
        aiAssert(parent->branches);
        aiAssert(parent == removeMe->parent);
        aiAssert((removeMe-ai_ctx.nodePool)>=0 && (removeMe-ai_ctx.nodePool)<ai_ctx.numNodes);
        aiAssert((parent->parent==nullptr) || (removeMe->activeBranches <= 0));

        std::printf("disc pos: %d len: %d parnode: %p \n", removeMe-ai_ctx.nodePool, 1, parent-ai_ctx.nodePool);


        const auto posOfChild = removeMe - parent->branches;
        Node<MoveType>& swapDst = parent->branches[posOfChild]; // Bypass the 'const'

        // Discard all child branches of each of the child nodes of the node
        // we want to remove (recursive):
        //for (int i=0; i<swapDst.createdBranches; ++i)
        //    discard(ai_ctx, swapDst.branches[i]);

        // 'parent->parent' is correct:
        const bool branchIsChildOfRoot = parent->parent == nullptr; // Keep full set of moves for root
        if (removeMe->createdBranches && !branchIsChildOfRoot)
        {
            std::printf("rem: %d \n", removeMe->createdBranches);
            ai_ctx.bitalloc.free(removeMe->branches - ai_ctx.nodePool, removeMe->createdBranches);
        }

        // But don't discard() the child node, that is the slot we will swap into:
        //discard(ai_ctx, swapDst); // incorrect!! // todo test!!


        const MoveType removedMoveHere = swapDst.moveHere;
        const auto     removedVisits   = swapDst.visits;
        const auto     removedScore    = swapDst.score;
        const auto     removedBranchScore = swapDst.branchScore;
        const auto    removedShallowestTerminalDepth = swapDst.shallowestTerminalDepth;

        Node<MoveType>& swapSrc = parent->branches[parent->activeBranches-1];

        // Don't swap w/ itself if the to-be-removed node is last:
        if (&swapDst == &parent->branches[parent->activeBranches-1])
        {
            parent->activeBranches -= 1;
            return &swapSrc;
        }

        swapDst.activeBranches  = swapSrc.activeBranches;
        swapDst.createdBranches = swapSrc.createdBranches;
        swapDst.moveHere        = swapSrc.moveHere;
        swapDst.visits          = swapSrc.visits;
        swapDst.score           = swapSrc.score;
        swapDst.branches        = swapSrc.branches;


        swapDst.branchScore = swapSrc.branchScore;
        swapDst.shallowestTerminalDepth = swapSrc.shallowestTerminalDepth;


        // Establish new "parent" for each branch node after swap
        // (the "parent" was prev. &swapSrc):
        //for (int i=0; i<swapDst.createdBranches; ++i)
        for (int i=0; i<swapDst.activeBranches; ++i)
            swapDst.branches[i].parent = &swapDst;

        #ifdef INCLUDEAI__PREVENT_ROOTNODES_FROM_GETTING_UPDATED_CORRECTLY
        #else
          swapSrc.activeBranches = Node<MoveType>::removed;
          swapSrc.moveHere       = removedMoveHere;
          swapSrc.visits         = removedVisits;
          swapSrc.score          = removedScore;
          swapSrc.branches       = nullptr;
          swapSrc.branchScore = removedBranchScore;
          swapSrc.shallowestTerminalDepth = removedShallowestTerminalDepth; // Important!
        #endif

          //for (int i=0; i<swapSrc.createdBranches; ++i)
            //swapSrc.branches[i].parent = &swapSrc;


        parent->activeBranches -= 1;
        aiAssert(swapSrc.parent == parent);
        return &swapSrc;
    }


/****************************************/
/*                               Memory */
/****************************************/
    template <typename Ctx, GameMove MoveType>
    inline constexpr auto& insertNodeIntoPool(Ctx& ctx, const int pos, Node<MoveType> *node, MoveType move)
    {
        aiAssert(pos < ctx.numNodes);
        return ctx.nodePool[pos] = Node(node, move);
    }


/****************************************/
/*                            Simulator */
/* MaxRandSims should be '%3 != 0'      */
/****************************************/
    template <int MaxRandSims, Gameview Board, typename Rndfunc>
    constexpr float simulate(const Board& original, Rndfunc rand)
    {
        int simWins = 0;
        // Run simulations:
        for (int i=0; i<MaxRandSims; ++i)
        {
            Board boardSim = original.clone();
            // Start single sim, run until end:
            auto outcome = Outcome::running;
            do
            {
                typename Board::StorageForMoves storageForMoves;
                const int nAvailMovesForThisTurn = boardSim.generateMovesAndGetCnt(storageForMoves);
                if (nAvailMovesForThisTurn == 0)
                    break;
                const int idx = rand() % nAvailMovesForThisTurn;
                outcome = boardSim.doMove( storageForMoves[idx] );
                boardSim.switchPlayer();
            } while (outcome==Outcome::running);
            // Count winner/loser:
            if (outcome != Outcome::draw)
            {
                const bool weWon = boardSim.getWinner() == original.getCurrentPlayer();
                simWins += weWon;
                simWins -= !weWon;
            }
        }
        const float winRatio = (simWins-0.f) / (MaxRandSims-0.f);
        return winRatio; // range from [-1,1]
    }


/****************************************/
/*                              Minimax */
/****************************************/
    constexpr SWORD MinimaxWin              =   1;
    constexpr SWORD MinimaxDraw             =   0;
    constexpr SWORD MinimaxLose             =  -1;
    constexpr SWORD MinimaxInit             =  -2;
    // Both "undeterminable" and "indeterminable" are correct and can be used interchangeably in many
    // contexts. However, "indeterminable" is more commonly used in formal or academic writing. (chatgpt)
    constexpr SWORD MinimaxIndeterminable99 = -99; // Debug
    constexpr SWORD MinimaxIndeterminable   =   0;

    template <Gameview Board, GameMove MoveType>
    constexpr SWORD minimax(const Board& current, const MoveType move, SWORD alpha, SWORD beta, const int depth)
    {
        if (depth==0) { return MinimaxIndeterminable; }

        Board clone = current.clone();
        clone.switchPlayer();
        const Outcome outcome = clone.doMove(move);
        if (outcome != Outcome::running)
        {
            if (outcome == Outcome::draw)
                return MinimaxDraw;
            else if (clone.getWinner() != current.getCurrentPlayer())
                return MinimaxLose;
            else
                return MinimaxWin;
        }
        typename Board::StorageForMoves storageForMoves;
        int nMoves = clone.generateMovesAndGetCnt(storageForMoves);
        nMoves -= 1;
        SWORD minimaxScore = MinimaxInit;
        while (nMoves >= 0)
        {
            const MoveType moveHere = storageForMoves[nMoves];
            const SWORD mnx = -minimax(clone, moveHere, -beta, -alpha, depth-1);
            minimaxScore = aiMax(minimaxScore, mnx);
            alpha        = aiMax(alpha, mnx);
            // Alpha-Beta Pruning (Thank you AI!!!!):
            if (beta <= alpha) {
                break; // Cut off the search
            }
            nMoves -= 1;
        }
        return minimaxScore==MinimaxInit ? MinimaxDraw : minimaxScore;
    }

    template <Gameview Board, GameMove MoveType>
    inline constexpr SWORD minimax(const Board& current, const int MaxDepth)
    {
        Board clone = current.clone();
        clone.switchPlayer();
        typename Board::StorageForMoves storageForMoves;
        int nMoves = clone.generateMovesAndGetCnt(storageForMoves);
        nMoves -= 1;
        SWORD best = MinimaxInit;
        while (nMoves >= 0)
        {
            const MoveType moveHere = storageForMoves[nMoves];
            const SWORD mnx = -minimax(clone, moveHere, MinimaxLose-1, MinimaxWin+1, MaxDepth);
            if (mnx > best)
                best = mnx;
            nMoves -= 1;
        }
        return best==MinimaxInit ? MinimaxDraw : best;
    }


/****************************************/
/*                               result */
/****************************************/
    template <GameMove MoveType>
    struct MCTS_result
    {
        enum { simulations, minimaxes, maxPatternSimilarity, end };
        int statistics[end] = {0};
        MoveType best;
    };


/****************************************/
/*                                 mcts */
/****************************************/
    template <int MaxIterations,
              int SimDepth,
              int MinimaxDepth,
              GameMove MoveType,
              BitfieldIntType BitfieldType,
              Gameview Board,
              typename AiCtx,
              typename Rndfunc
             >
    constexpr auto mcts(const Board& boardOriginal, AiCtx& ai_ctx, Rndfunc rand)
    {
        [[maybe_unused]] auto UCBselectBranch =
            [](const Node<MoveType>& node) -> Node<MoveType> *
            {
                for (int i=0; i<node.activeBranches; ++i)
                {
                    aiAssert(node.branches);
                    Node<MoveType>& arm = node.branches[i];
                    if (arm.visits == 1)
                    {
                        // UCB requires each slot-machine 'arm' to be tried at least once (https://u.cs.biu.ac.il/~sarit/advai2018/MCTS.pdf):
                        return &arm; // Prevent x/0
                    }

                    // todo: epl/expl shld be adjusted to utilize max node use: check rate of node use vs nodes released and adjust based on that!
                    // todo2: score prefer if > 0?? (same like at the end????)
                    const float exploit = arm.score / (arm.visits-0.f); // <- This
                    #if 0
                      const float exploit = std::abs(arm.score) / (arm.visits-0.f); // <- Not this
                    #endif
                    constexpr float exploration_C = 1.414f; // useless
                    constexpr float Hoeffdings_multiplier = 2.f; // (http://www.incompleteideas.net/609%20dropbox/other%20readings%20and%20resources/MCTS-survey.pdf)
                    const float explore = exploration_C * aiSqrt((Hoeffdings_multiplier * aiLog(node.visits)) / arm.visits);
                    /*

                        def Q(self):  # returns float
        return self.total_value / (1 + self.number_visits)

    def U(self):  # returns float
        return (math.sqrt(self.parent.number_visits)
                * self.prior / (1 + self.number_visits))

    def best_child(self, C):
        return max(self.children.values(),
                   key=lambda node: node.Q() + C*node.U())

                    */
                    arm.UCBscore = exploit + explore;
                    #ifdef INCLUDEAI__FORCE_REVISIT_IDENTICAL_SCORE
                      for (int j=0; j<node.activeBranches; ++j)
                      {
                        if (&node.branches[j]!=&arm && node.branches[j].UCBscore==arm.UCBscore && node.branches[j].visits==arm.visits)
                            return &arm;
                      }
                    #endif
                }
                int pos = node.activeBranches - 1;
                float best = node.branches[pos].UCBscore;
                for (int i=pos-1; i>=0; --i)
                {
                    if (node.branches[i].UCBscore > best)
                    {
                        best = node.branches[i].UCBscore;
                        pos = i;
                    }
                }
                return &node.branches[pos];
            };

        [[maybe_unused]] auto pickUnexplored =
            [](const Node<MoveType>& node) -> Node<MoveType> *
            {
                for (int i=0; i<node.activeBranches; ++i)
                {
                    Node<MoveType>& branch = node.branches[i];
                    if (branch.visits == 0)
                        return &branch;
                }
                return &node.branches[0];
            };

        [[maybe_unused]] auto pickRandom =
            [](const Node<MoveType>& node) -> Node<MoveType> *
            {
                return nullptr; // todo!!
            };


        Node<MoveType> *placeholder = nullptr; // Prevent gcc from deducting the wrong type... 🙄
        Node<MoveType> *root = &insertNodeIntoPool(ai_ctx, 0, placeholder, MoveType{});
        ai_ctx.bitalloc.clearAll();
        [[maybe_unused]] const auto throwaway = ai_ctx.bitalloc.largestAvailChunk(1);
        for (int i=0; i<AiCtx::numNodes; ++i)
        {
            ai_ctx.nodePool[i].activeBranches = -1;
            ai_ctx.nodePool[i].createdBranches = 0;
        }

        int cutoffDepth = 9999;
        FLOAT threshold = 1.f; // 'threshold' above which the result of the neuralnet is used, not minimax or randroll
        SWORD rootMovesRemaining;
        MCTS_result<MoveType> mcts_result;
        for (int iterations=0; root->activeBranches!=0 && iterations<MaxIterations; ++iterations)
        {
            Node<MoveType> *selectedNode = root;
            Board boardClone = boardOriginal.clone();
            Outcome outcome = Outcome::running;
            int depth = 1;




            // 1. Traverse tree and select leaf:
            Node<MoveType> *parentOfSelected;
            while ((selectedNode->activeBranches > 0) && (rootMovesRemaining == 0))
            {
                parentOfSelected = selectedNode;
                aiAssert(selectedNode->branches);
                selectedNode = UCBselectBranch(*selectedNode);
                //aiAssert(selectedNode->branchScore == 0);
                aiAssert(selectedNode->parent == parentOfSelected);
                const MoveType moveHere = selectedNode->moveHere;
                outcome = boardClone.doMove(moveHere);
                boardClone.switchPlayer();
                depth += 1;
                if (outcome != Outcome::running)
                {
                    //cutoffDepth = cutoffDepth < depth ? cutoffDepth : depth;
                    //  selectedNode = selectedNode->parent;
                }
                if (depth == cutoffDepth)
                {
                    //std::printf("\033[1;36m %d \033[0m \n", depth);
                    //assert(false);
                    //break;
                }
            }

            //if (selectedNode->activeBranches != Node::never_expanded) { std::printf("------never exp. ---%d \n", selectedNode==root); break; } // insuff nodes!
            //if (selectedNode->parent && selectedNode->parent->activeBranches>0) aiAssert(selectedNode->parent->branches);




            //aiAssert(selectedNode->activeBranches <= 0);
            //aiAssert(selectedNode->score < 1);




            // 2. Add (allocate/expand) branch/child nodes to leaf:
            if (selectedNode->activeBranches==Node<MoveType>::never_expanded && outcome==Outcome::running)
            {
                typename Board::StorageForMoves storageForMoves;
                int nValidMoves = boardClone.generateMovesAndGetCnt(storageForMoves);
                const auto availNodes = ai_ctx.bitalloc.largestAvailChunk(nValidMoves);
                nValidMoves = availNodes.length; // This line is critical!
                int nodePos = availNodes.posOfAvailChunk;
                if (nodePos == -1) [[unlikely]]
                {
                    // No more nodes available, stopping condition!
                    // todo: record this in the result!
                    break;
                }
                if (selectedNode == root)
                    rootMovesRemaining = nValidMoves;

                std::printf("np:%d, avl:%d \n", nodePos, nValidMoves);

                if ((nodePos+nValidMoves) >= ai_ctx.numNodes) [[unlikely]] // This can happen at the end
                {
                    // todo: out of mem is stopping condition!
                    std::printf("\033[1;35mexceeded! %d vs  %d \n\033[0m", ai_ctx.numNodes, nodePos+nValidMoves);
                    // Can't be salvaged. Once we are out of nodes we can't release already
                    // alloc'd branches (cuz we must reach a terminal node for that). We are stuck:
                    aiAssert(false);
                    break;
                    //nValidMoves = nValidMoves - ((nodePos+nValidMoves)-nValidMoves);
                    //nValidMoves -= 1;
                }

                // If this gets triggered there is a bug in the game. There can NEVER
                // be a situation where a player can't move but game is still running!!!!:
                aiAssert(nValidMoves > 0); // In one example checking for draw condition was missing, leading to this getting triggered...

                //aiAssert((nodePos+nValidMoves) >= 0); &&
                //aiAssert((nodePos+nValidMoves) <= ai_ctx.numNodes);
                //if ((nodePos+nValidMoves) < 0 || (nodePos+nValidMoves) >= ai_ctx.numNodes) [[unlikely]]
                //    continue;
                selectedNode->activeBranches  = nValidMoves;
                selectedNode->createdBranches = nValidMoves;
                nValidMoves -= 1;

                aiAssert(ai_ctx.nodePool[nodePos].activeBranches <= 0);
                MoveType move = storageForMoves[nValidMoves];
                selectedNode->branches = &insertNodeIntoPool(ai_ctx, nodePos, selectedNode, move);


                while (nValidMoves--)
                {
                    nodePos += 1;
                    move = storageForMoves[nValidMoves];
                    [[maybe_unused]] const auto& unusedNode =
                        insertNodeIntoPool(ai_ctx, nodePos, selectedNode, move);
                }
                std::printf("\033[1;37mnew branch:%p brch:%p \033[0m \n", selectedNode-ai_ctx.nodePool, (&selectedNode->branches[0])-ai_ctx.nodePool);
                for (int i=0; false && i<selectedNode->createdBranches; ++i) // todo put this loop into the assert
                {
                    std::printf("\033[1;36m%p (%d) exp:%d mv:%d \033[0m \n", (&selectedNode->branches[i])-ai_ctx.nodePool, selectedNode->branches[i].moveHere, 0, selectedNode->branches[i].moveHere);
                    aiAssert(selectedNode != &selectedNode->branches[i]);
                    aiAssert(selectedNode->branches && selectedNode->activeBranches>0);
                    aiAssert(selectedNode->branches[i].parent == selectedNode);
                }
            }






            // 3a. Ensure all moves on root node are visited once:
            if (rootMovesRemaining > 0)
            {
                rootMovesRemaining -= 1;
                selectedNode = &root->branches[rootMovesRemaining];
                outcome = boardClone.doMove( selectedNode->moveHere );
                boardClone.switchPlayer();
                if (selectedNode->moveHere == 59)
                {
                    std::printf("root move: %d \n", selectedNode->moveHere);
                }
            }
            // 3b. Pick (select) a node for analysis:
            else if (selectedNode->activeBranches > 0)
            {
                aiAssert(selectedNode->branches);
                // Should be rand to ensure fair distribution:
                const auto sample = rand() % selectedNode->activeBranches;
                // ^^^ What that means is that if we bias towards a specific kind of node: good,
                // bad or something else, then we will fail to discover unexpected possibilities!
                selectedNode = &selectedNode->branches[sample];
                //aiAssert(selectedNode->score < 1.f);
                outcome = boardClone.doMove( selectedNode->moveHere );
                boardClone.switchPlayer();
                //depth += 1;
            }


            //  bool stop = false;
            float score = 0.f; // -1: loss, 1: win, 0: draw // 0 means loss, 1 means draw, 2 means win!!!
            /* if (parent->activeBranches <= 0) {
            std::printf("\033[1;35m act:%d created:%d , fin?: %d \033[0m \n", parent->activeBranches, parent->createdBranches,  outcome!=Board::Outcome::running);
            // break;
            // continue;
            stop = true;
            if (outcome == Board::Outcome::draw) //[[likely]]
            // draw:
            score += 1.f;
            else if (boardOriginal->getCurrentPlayer() != boardClone->getCurrentPlayer())
            // win:
            score += 2.f;
            selectedNode = ppp;
            }*/
            //aiAssert(parent != selectedNode);   // Ensure the graph remains acyclic
            //aiAssert(parent->activeBranches>0); // Ensure parent node has at least one




            // 4. Determine branch score ("rollout"):
            bool disconnect = false;
            if (outcome != Outcome::fin) // <- This one
            #if INCLUDEAI__BRANCH_ON_WRONG_TERMINAL_CONDITION
              if (outcome == Outcome::running) // <- Not this
            #endif
            {
                #ifndef INCLUDEAI__CONFUSE_CURRENT_PLAYER_WITH_WINNER
                  // correct:
                  const SWORD polarity = boardClone.getCurrentPlayer()!=boardOriginal.getCurrentPlayer() ? -1 : 1;
                #else
                  // incorrect:
                  const SWORD polarity = boardClone.getWinner()!=boardOriginal.getCurrentPlayer() ? -1 : 1;
                #endif

                const FLOAT neuroscore = ai_ctx.query(boardClone);
                //if (true && aiAbs(neuroscore) < threshold) // todo!
                {
                    const SWORD branchscore = minimax<Board, MoveType>(boardClone, MinimaxDepth) * polarity;
                    aiAssert(-abs(branchscore) != MinimaxIndeterminable99);
                    if (branchscore == MinimaxIndeterminable)
                    {
                        // Simulate to get an estimation of the quality of this position:
                        score = simulate<SimDepth>(boardClone, rand);
                        score *= polarity-0.f;
                        mcts_result.statistics[MCTS_result<MoveType>::simulations] += 1;
                    }
                    else
                    {
                        score = branchscore-0.f;
                        const bool sameSign = (score * neuroscore) > 0;
                        if (sameSign)
                            threshold = aiMin(neuroscore, threshold);
                        #ifdef INCLUDEAI__INSTANT_LOBOTOMY
                          disconnect = true;
                        #endif
                        mcts_result.statistics[MCTS_result<MoveType>::minimaxes] += 1;
                    }
                }
                //else
                {
                  //  score = neuroscore * (polarity-0.f);
                }
            }
            else
            {
                cutoffDepth = cutoffDepth < depth ? cutoffDepth : depth;
                selectedNode->shallowestTerminalDepth = depth<selectedNode->shallowestTerminalDepth ? depth : selectedNode->shallowestTerminalDepth;

                // A terminal node is equivalent to a 100% simulation score:
                if (outcome != Outcome::draw)
                {
                    constexpr float win = 1.f, lose = -1.f;
                    #ifndef INCLUDEAI__CONFUSE_LAST_PLAYER_WITH_WINNER
                      // This one:
                      score = boardOriginal.getCurrentPlayer() == boardClone.getWinner() ? win : lose;
                    #else
                      // Not this:
                      score = boardOriginal.getCurrentPlayer() != boardClone.getCurrentPlayer() ? win : lose;
                    #endif
                }
                disconnect = true;
            }


            if (disconnect)
            {


                Node<MoveType> *child = selectedNode;
                Node<MoveType> *parent = selectedNode->parent;
                aiAssert(parent);
                aiAssert(selectedNode != root);                  // Ensure 'selectedNode' not root
                //aiAssert([&]{ return outcome==Board::Outcome::running ? parent->branches!=nullptr : true; }());
                //aiAssert([&]{ return outcome==Board::Outcome::running ? parent->activeBranches>0 : true; }());
                aiAssert([&]{ return parent->branches!=nullptr; }());
                // terminal nodes have no branches:
                aiAssert(parent->activeBranches!=0); //>0 || parent->activeBranches==Node::never_expanded);

                while (parent)
                {
                    std::printf("/// active:%d *brnch-start:%p root:%p parent:%p dep:%d sel:%p scr:%2.2f\n", parent->activeBranches, parent->branches-ai_ctx.nodePool, root-ai_ctx.nodePool, parent-ai_ctx.nodePool, 0, selectedNode-ai_ctx.nodePool, score);

                    std::printf("still active 'siblings': ");
                    for (int i=0; false&&i<parent->activeBranches; ++i)
                    {
                        std::printf("\033[0;33m%p scr:%2.2f  \033[0m", (&parent->branches[i])-ai_ctx.nodePool, parent->branches[i].score);
                    }
                    std::printf("%d \n", parent->activeBranches);


                    Node<MoveType> *todo = selectedNode->parent;
                    selectedNode = disconnectBranch(ai_ctx, parent, child);
                    aiAssert(selectedNode->parent == parent);
                    //aiAssert(todo == selectedNode->parent); // 100% fail!!!
                    //selectedNode->parent = todo; // fix this goes in  disconnectBranch()




                    if (parent->activeBranches != 0)
                    {
                        break; // Stop
                    }
                    else
                    {
                        //__builtin_trap(); // todo remove

                        child = parent;
                        aiAssert(parent != parent->parent);
                        parent = parent->parent; // Lolz
                    }
                }
            }

            // Floyd's Cycle. Ensure the graph remains acyclic (debug only!):
            #if defined(AI_DEBUG)
              aiAssert([selectedNode]
                       {
                           Node<MoveType> *fast = selectedNode;
                           Node<MoveType> *slow = selectedNode;
                           while (fast)
                           {
                               if (fast)
                               fast = fast->parent;
                               if (fast == slow)
                               {
                                   fast = nullptr; // 'break'
                                   return false;
                               }
                               if (fast)
                               fast = fast->parent;
                               slow = slow->parent;
                           }
                           return true;
                       }()
                      );
            #endif

            float cur = 1.f;
            //int yyy = fib(depth);

            SWORD depthScore = 1;
           // for (Node *tmp=selectedNode; tmp; ) { tmp=tmp->parent; branchDepth+=1; cur=cur+0.002f; }
            std::printf("scre sel:%p score: %1.3f %d max:%d \n", selectedNode-ai_ctx.nodePool, score, iterations, MaxIterations);

            int child_dp = 9999;






            // 5. Backprop/update tree:
            while (selectedNode != root)
            {
                int dp_temp = selectedNode->shallowestTerminalDepth;
                selectedNode->shallowestTerminalDepth = child_dp<dp_temp ? child_dp : dp_temp;
                child_dp = dp_temp;

                //selectedNode->visits += 1.f ;//- ((cur-0.f)/(depth-0.f)); // Games that have more than two possible outcomes (example:
                //selectedNode->visits += aiLog( 100 * ((cur-0.f)/(depth-0.f)) );

                // This 'if' is used to (optionally) stop counting scores for branches that are deeper
                // than the most immediate node where a turn ends:
                //if ([&]{ if constexpr ((cutoff_scoring&hyperparams)==cutoff_scoring) return branchDepth<cutoff; else return true; }())
                //if (depth <= cutoffDepth)
                {
                    // 'visits' does not tell us if a position is a winner or not. It tells us
                    // how -interesting- a position is:
                    selectedNode->visits += 1;
                    selectedNode->score += score;
                    depthScore += 1;
                    //depth += 1;
                    //selectedNode->weight += 1; //(fib(cur)-0.f) / (fib(depth)-0.f); //(aiLog(cur* 10)/2) / (depth-0.f); //((cur-0.f)/(depth-0.f)) ; weight
                //    cur += 1.f;
                    //    if (fib(cur+1) <= yyy)
                    //    cur += 1;

                }
                depth -= 1;

                std::printf("sel:%p score: %1.3f act:%d crt:%d  \n", selectedNode-ai_ctx.nodePool, selectedNode->score, selectedNode->activeBranches, selectedNode->createdBranches);


                selectedNode = selectedNode->parent;
                //  if constexpr ((cutoff_scoring&hyperparams)==cutoff_scoring) { branchDepth -= 1; }


            }
        } // iterations

        
        // This is the part NOT discussed in the AlphaZero paper (or ANY mcts paper for that matter):
        // What to do when terminal and non-terminal nodes appear in the tree mixed together????
        int shallowestTerminal = 9999;
        for (int i=0; i<root->createdBranches; ++i)
        {
            Node<MoveType>& branch = root->branches[i];
            if (branch.shallowestTerminalDepth < shallowestTerminal)
                shallowestTerminal = branch.shallowestTerminalDepth;
        }

        if (shallowestTerminal != 9999)
        {
            for (int i=0; i < root->createdBranches; ++i)
            {
                Node<MoveType>& branch = root->branches[i];
                if (branch.shallowestTerminalDepth == shallowestTerminal)
                {
                    if (branch.score > 0.f)
                        continue;
                }
                else 
                {
                    branch.score = -999.f;
                }
            }
        } // (shallowestTerminal != 9999)

        // Yes, scoring is complex!!!:
        auto bestScore = root->branches[0].score;
        auto bestVisits = root->branches[0].visits;
        int posScore=0, posVisits=0;
        for (int i=1; i<root->createdBranches; ++i)
        {
            auto expectedScore = root->branches[i].score;
            auto expectedVisits = root->branches[i].visits;
            if (expectedScore > bestScore)
            {
                bestScore = expectedScore;
                posScore = i;
            }
            else if (expectedScore == bestScore)
            {
                if (expectedVisits > root->branches[posScore].visits)
                    posScore = i;
            }
    
            if (expectedVisits > bestVisits)
            {
                bestVisits = expectedVisits;
                posVisits = i;
            }
            else if (expectedVisits == bestVisits)
            {
                if (expectedScore > root->branches[posVisits].score)
                    posVisits = i;
            }
        }









            unsigned char yavpos[11*11] =
                    {0, 0,  0,  0,  0,  0,  0,  0,  0,  0, 0,
                     0, 0,  0,  0,  0, 'a','b','c','d','e',0,
                     0, 0,  0,  0, 'f','g','h','i','j','k',0,
                     0, 0,  0, 'l','m','n','o','p','q','r',0,
                     0, 0, 's','t','u','v','w','x','y','z',0,
                     0,'A','B','C','D','E','F','G','H','I',0,
                     0,'J','K','L','M','N','O','P','Q', 0, 0,
                     0,'R','S','T','U','V','W','X', 0,  0, 0,
                     0,'Y','Z','1','2','3','4', 0,  0,  0, 0,
                     0,'5','6','7','8','9', 0,  0,  0,  0, 0,
                     0, 0,  0,  0,  0,  0,  0,  0,  0,  0, 0
                    };



            for (int i=0; i<root->createdBranches; ++i)
            {
    
                if (i==bestScore) std::printf("\033[1;36m");
                std::printf("mv: %c %d  bs:%d   ", yavpos[root->branches[i].moveHere], root->branches[i].moveHere
                           , root->branches[i].branchScore);
                std::printf("s:%4.3f v:%d  dp: %d   act:%d  s/v:%2.3f v/s:%2.5f \033[0m \n", root->branches[i].score, root->branches[i].visits, root->branches[i].shallowestTerminalDepth, root->branches[i].activeBranches,
                    root->branches[i].score/ root->branches[i].visits,root->branches[i].visits/root->branches[i].score);
            }
            std::printf("threshold: %f mv: %d \033[1;31mcutoff: %d\033[0m ACT:%d maxPatternsim %f \n",
                          threshold, root->branches[posScore].moveHere, cutoffDepth, root->activeBranches, ai_ctx.maxPatternSimilarity);
    
            mcts_result.statistics[MCTS_result<MoveType>::maxPatternSimilarity] = ai_ctx.maxPatternSimilarity*100;
            mcts_result.best = [root, posVisits, posScore, bestScore]
                               {
                                   if (bestScore > 0.f)
                                       return root->branches[posScore].moveHere;
                                   return root->branches[posVisits].moveHere;
                               }();
            return mcts_result;

    }







/****************************************/
/*                                Tests */
/****************************************/
    struct TicTacTest
    {
        using Move = int;
        using StorageForMoves = Move[9];
        unsigned char pos[9+1] = {0,0,0,
                                  0,0,0,
                                  0,0,0
                                 };
        int currentPlayer=1, winner=0;
        float neuralInputs[9+9] = {0};

        constexpr TicTacTest() {}
        TicTacTest(const TicTacTest&) = delete;
        TicTacTest& operator=(const TicTacTest&) = delete;
        TicTacTest(TicTacTest&&) = default;

        constexpr TicTacTest clone() const
        {
            TicTacTest dst;
            for (int i=0; i<9; ++i) { dst.pos[i] = pos[i]; }
            dst.currentPlayer = currentPlayer;
            return dst;
        }

        constexpr int generateMovesAndGetCnt(TicTacTest::Move *availMoves)
        {
            int availMovesCtr = 0;
            for (int i=0; i<9; ++i)
            {
                // find valid moves:
                if (pos[i]==0) availMoves[availMovesCtr++] = i;
            }
            // Don't do this: "availMovesCtr -= 1"
            return availMovesCtr;
        }

        constexpr include_ai::Outcome doMove(const TicTacTest::Move mv)
        {
            // checking if valid input only needed when running on a server:
            if (pos[mv]) [[unlikely]]
                return include_ai::Outcome::invalid; // <- Therefore this is for demo only!

            pos[mv] = currentPlayer;
            int win = pos[0] && (pos[0]==pos[1]) && (pos[0]==pos[2]);
            win += pos[3] && (pos[3]==pos[4]) && (pos[3]==pos[5]);
            win += pos[6] && (pos[6]==pos[7]) && (pos[6]==pos[8]);

            win += pos[0] && (pos[0]==pos[3]) && (pos[0]==pos[6]);
            win += pos[1] && (pos[1]==pos[4]) && (pos[1]==pos[7]);
            win += pos[2] && (pos[2]==pos[5]) && (pos[2]==pos[8]);

            win += pos[0] && (pos[0]==pos[4]) && (pos[0]==pos[8]);
            win += pos[2] && (pos[2]==pos[4]) && (pos[2]==pos[6]);

            if (win >= 1)
            {
                winner = currentPlayer;
                return include_ai::Outcome::fin;
            }
            // Counting turns can NOT be used here for testing:
            if (pos[0]&&pos[1]&&pos[2]&&pos[3]&&pos[4]&&pos[5]&&pos[6]&&pos[7]&&pos[8])
                return include_ai::Outcome::draw;
            return include_ai::Outcome::running;
        }

        constexpr void switchPlayer() { currentPlayer=3-currentPlayer; }

        constexpr int getCurrentPlayer() const { return currentPlayer; }

        constexpr int getWinner() const { return winner; }

        float *getNetworkInputs()
        {
            for (int i=0; i<9; ++i)
            {
                neuralInputs[i  ] = pos[i] == currentPlayer;
                neuralInputs[i+9] = pos[i] != currentPlayer;
            }
            return neuralInputs;
        }
    };

    static_assert([]
                  {
                      UQWORD xoroshiro128plus_state[2] = {0x9E3779B97f4A7C15ull,0xBF58476D1CE4E5B9ull};

                      //   Written in 2016-2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)
                      // To the extent possible under law, the author has dedicated all copyright
                      // and related and neighboring rights to this software to the public domain
                      // worldwide. This software is distributed without any warranty.
                      // See <http://creativecommons.org/publicdomain/zero/1.0/>.
                      auto xoroshiro128plus = [](UQWORD (&state)[2]) -> UQWORD
                      {
                          // The generators ending with + have weak low bits, so they
                          // are recommended for floating point number generation
                          const UQWORD s0 = state[0];
                          UQWORD s1 = state[1];
                          const UQWORD result = s0 + s1;

                          s1 ^= s0;
                          state[0] = ((s0 << 55) | (s0 >> (64 - 55))) ^ s1 ^ (s1 << 14); // a, b
                          state[1] = (s1 << 36) | (s1 >> (64 - 36)); // c
                          return result;
                      };

                      TicTacTest t1;
                      t1.pos[0]=2; t1.pos[1]=1; t1.pos[2]=2;
                      t1.pos[3]=1; t1.pos[4]=1; t1.pos[5]=2;
                      t1.pos[6]=0; t1.pos[7]=2; t1.pos[8]=1;
                      t1.currentPlayer = 1;
                      float res = simulate<1>(t1, [&xoroshiro128plus_state, xoroshiro128plus]{ return xoroshiro128plus(xoroshiro128plus_state); });
                      bool ok = (res < .1f) && (res > -.1f); // draw
                      TicTacTest t2;
                      t2.pos[0]=1; t2.pos[1]=1; t2.pos[2]=0;
                      t2.pos[3]=2; t2.pos[4]=0; t2.pos[5]=0;
                      t2.pos[6]=0; t2.pos[7]=2; t2.pos[8]=0;
                      t2.currentPlayer = 1;
                      res = simulate<1>(t2, [&xoroshiro128plus_state, xoroshiro128plus]{ return xoroshiro128plus(xoroshiro128plus_state); });
                      return ok && (res>.5f); // Player 1 wins
                  }()
                 );

    static_assert([]
                  {
                      TicTacTest t;
                      t.pos[0]=1; t.pos[1]=0; t.pos[2]=2;
                      t.pos[3]=0; t.pos[4]=1; t.pos[5]=0;
                      t.pos[6]=0; t.pos[7]=0; t.pos[8]=0;
                      t.currentPlayer = 2;
                      SWORD res = minimax<TicTacTest, TicTacTest::Move>(t, 10);
                      bool ok = res == MinimaxDraw; // Player 2 can still play position 8!
                      t.pos[5] = 2; // ... but plays  5 instead
                      t.currentPlayer = 1;
                      res = minimax<TicTacTest, TicTacTest::Move>(t, 9);
                      return ok && (res == MinimaxWin); // Player 2 lost!
                  }()
                 );

    static_assert([]
                  {
                      TicTacTest t;
                      t.pos[0]=2; t.pos[1]=0; t.pos[2]=0;
                      t.pos[3]=0; t.pos[4]=1; t.pos[5]=0;
                      t.pos[6]=0; t.pos[7]=2; t.pos[8]=1;
                      t.currentPlayer = 1;
                      const SWORD res = minimax<TicTacTest, TicTacTest::Move>(t, 9);
                      // This position is a guaranteed win for '1':
                      return res == MinimaxWin;
                  }()
                 );

    static_assert([]
                  {
                      TicTacTest t;
                      t.pos[0]=2; t.pos[1]=0; t.pos[2]=0;
                      t.pos[3]=0; t.pos[4]=1; t.pos[5]=0;
                      t.pos[6]=0; t.pos[7]=0; t.pos[8]=0;
                      t.currentPlayer = 1;
                      const SWORD res = minimax<TicTacTest, TicTacTest::Move>(t, 9);
                      // winner not yet determined:
                      return res == MinimaxDraw; // 0
                  }()
                 );

    static_assert([]
                  {
                      TicTacTest t;
                      t.pos[0]=2; t.pos[1]=1; t.pos[2]=2;
                      t.pos[3]=1; t.pos[4]=1; t.pos[5]=2;
                      t.pos[6]=0; t.pos[7]=2; t.pos[8]=1;
                      t.currentPlayer = 1;
                      const SWORD res = minimax<TicTacTest, TicTacTest::Move>(t, 9);
                      // draw:
                      return res == MinimaxDraw; // 0
                  }()
                 );

    static_assert([]
                  {
                      TicTacTest t;
                      t.pos[0]=0; t.pos[1]=0; t.pos[2]=1;
                      t.pos[3]=2; t.pos[4]=0; t.pos[5]=0;
                      t.pos[6]=2; t.pos[7]=0; t.pos[8]=1;
                      t.currentPlayer = 1;
                      const SWORD res = minimax<TicTacTest, TicTacTest::Move>(t, 9);
                      return res == MinimaxWin; // 1
                  }()
                 );

   /* static_assert([]
                  {
                      TicTacTest t;
                      t.pos[0]=1; t.pos[1]=2; t.pos[2]=0;
                      t.pos[3]=0; t.pos[4]=0; t.pos[5]=0;
                      t.pos[6]=0; t.pos[7]=0; t.pos[8]=0;
                      t.currentPlayer = 1;
                      HALF *networkInputs = t.getNetworkInputs();
                      bool ok = networkInputs[0] < 1.f;
                      return ok;
                  }()
                 );*/






/****************************************/
/*                             Research */
/****************************************/
/*
    https://u.cs.biu.ac.il/~sarit/advai2018/MCTS.pdf
    https://drum.lib.umd.edu/items/07cb7ac2-115a-443c-9c9e-fe6dad2a6b41 "MONTE CARLO TREE SEARCH AND MINIMAX COMBINATION – APPLICATION OF SOLVING PROBLEMS IN THE GAME OF GO" (minimax+mcts+code)
    https://www.ijcai.org/proceedings/2018/0782.pdf "MCTS-Minimax Hybrids with State Evaluations"
    http://www.talkchess.com/forum3/viewtopic.php?t=67235 MCTS beginner questions
    http://www.talkchess.com/forum3/viewtopic.php?f=7&t=70694
    http://www.talkchess.com/forum3/viewtopic.php?f=7&t=69993 <- Tic-Tac-Toe






    todo: http://www.incompleteideas.net/609%20dropbox/other%20readings%20and%20resources/MCTS-survey.pdf
*/

/* Whatever we conceive well we express clearly, and words flow with ease. (Nicolas Boileau-Despreaux) */


} // namespace include_ai



#endif // INCLUDEAI_IMPLEMENTATION


#endif // INCLUDEAI_HPP


/*
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
*/


