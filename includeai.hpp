/*
    You must add '#define INCLUDEAI_IMPLEMENTATION' before '#include'ing this in ONE source file.
    Like this:
        #define INCLUDEAI_IMPLEMENTATION
        #include "includeai.hpp"

    AUTHOR
        Pitiless Peanut (aka. Shaiden Spreitzer, Professor Peanut, etc...) of VECTORPHASE

    LICENSE
        BSD 4-Clause (See end of file)

    HISTORY
        0.00.0    - 2025-11-30 - pre-alpha. Don't use
*/

#ifndef INCLUDEAI_HPP
#define INCLUDEAI_HPP

#ifdef INCLUDEAI_IMPLEMENTATION


#include <cmath>
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
  #error "unknown arch! (if compiling for wasm try using '-msimd128', on intel '-march=native')"
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


/*
  // original asmtypes.h types:
  typedef unsigned short BOOL;
  typedef unsigned char BYTE;
  typedef unsigned long DWORD;
  typedef signed char SBYTE;
  typedef signed char SCHAR;
  typedef signed long SDWORD;
  typedef signed int SINT;
  typedef signed long SLONG;
  typedef signed short SSHORT;
  typedef signed short SWORD;
  typedef unsigned char UBYTE;
  typedef unsigned char UCHAR;
  typedef unsigned long UDWORD;
  typedef unsigned int UINT;
  typedef unsigned long ULONG;
  typedef unsigned short USHORT;
  typedef unsigned short UWORD;
  typedef unsigned short WORD;
*/






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
        constexpr int bits = sizeof(Int) * CHARBITS;
        const int effective_amount = amount & (bits - 1);
        if (effective_amount == 0) return x;
        return ((x << effective_amount) | (x >> (bits - effective_amount)));
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
                if (discoveredSize < Intbits)
                {
                    for (int bucketNo=0; bucketNo<NumberOfBuckets; ++bucketNo)
                    {
                        const int availTail = ctz(bucketPool[bucketNo]);
                        if (availTail >= discoveredSize)
                        {
                            BitfieldType mask = everyBitSet << discoveredSize;
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
                                if (remainingBits > 0)
                                {
                                    const BitfieldType mask = everyBitSet << (Intbits - remainingBits);
                                    //bucketPool[bucketNo+bucketsRequired-needsExactFullBuckets] |= mask;
                                    if ((bucketPool[bucketNo+bucketsRequired]&mask) == 0)
                                    {
                                        bucketPool[bucketNo+bucketsRequired] |= mask;
                                    }
                                    else
                                        break;
                                }

                                for (int i=0; i<bucketsRequired; ++i)
                                    bucketPool[bucketNo+i] = everyBitSet;

                                return Pos{ .posOfAvailChunk = static_cast<int>(bucketNo*Intbits),
                                            .length = static_cast<int>(discoveredSize)
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
            // todo: this current free() implementation mirrors the "BitAlloc_Mode::TIGHT" version of largestAvailChunk()
            //       there should be an optimized version for "BitAlloc_Mode::FAST" too! A TIGHT mode free() is
            //       compatible with FAST mode largestAvailChunk() but not vice versa.
            if constexpr (false && mode == BitAlloc_Mode::FAST)
            {
            }
            else if constexpr (true || mode == BitAlloc_Mode::TIGHT) // todo remove 'true ||'
            {
                constexpr BitfieldType everyBitSet = ~0;
                int startBucket = pos/Intbits;
                int endBucket = (pos+len-1)/Intbits;
                const BitfieldType maskHead = (pos % Intbits == 0) ? 0 : (everyBitSet << (Intbits - (pos%Intbits)));
                const BitfieldType maskTail = ((pos+len) % Intbits == 0) ? everyBitSet /*everyBitSet (todo test!) */: (everyBitSet << (Intbits - ((pos+len)%Intbits)));

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
                    while (++startBucket < endBucket)
                        bucketPool[startBucket] = 0;
                }
            } // else if constexpr (mode == BitAlloc_Mode::TIGHT)
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
          return _rotl(x, amount);
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
                      BitAlloc<256, UQWORD, BitAlloc_Mode::FAST, comptime> testAllocator1;
                      testAllocator1.largestAvailChunk(63);
                      testAllocator1.largestAvailChunk(65);
                      testAllocator1.largestAvailChunk(64);
                      testAllocator1.free(1, 191);
                      testAllocator1.free(0, 1);
                      bool ok = testAllocator1.bucketPool[0] == 0;
                      ok = ok && (testAllocator1.bucketPool[0]+testAllocator1.bucketPool[1]+testAllocator1.bucketPool[2]) == 0;
                      ok = ok && testAllocator1.bucketPool[3] == 0xffffffffffffffff;
                      auto pos1 = testAllocator1.largestAvailChunk(257);
                      ok = ok && pos1.posOfAvailChunk==0 && pos1.length==192; // Last bucket already full
                      testAllocator1.free(64, 3);
                      ok = ok && testAllocator1.bucketPool[1] == 0b0001111111111111111111111111111111111111111111111111111111111111;
                      testAllocator1.free(0, 64);
                      ok = ok && testAllocator1.bucketPool[0] == 0;
                      pos1 = testAllocator1.largestAvailChunk(65);
                      ok = ok && testAllocator1.bucketPool[1] == 0b1001111111111111111111111111111111111111111111111111111111111111;
                      ok = ok && pos1.posOfAvailChunk==0 && pos1.length==65;
                      testAllocator1.free(63, 1);
                      ok = ok && testAllocator1.bucketPool[0] == 0b1111111111111111111111111111111111111111111111111111111111111110;
                      pos1 = testAllocator1.largestAvailChunk(1);
                      ok = ok && testAllocator1.bucketPool[0] == 0xFFFFFFFFFFFFFFFF;
                      ok = ok && pos1.posOfAvailChunk==63 && pos1.length==1;
                      ok = ok && testAllocator1.bucketPool[1] == 0b1001111111111111111111111111111111111111111111111111111111111111;
                      pos1 = testAllocator1.largestAvailChunk(2);
                      ok = ok && pos1.length==0 && pos1.posOfAvailChunk==-1;
                      testAllocator1.free(64, 1);
                      ok = ok && testAllocator1.bucketPool[1] == 0b0001111111111111111111111111111111111111111111111111111111111111;
                      testAllocator1.free(64, 64);
                      pos1 = testAllocator1.largestAvailChunk(64);
                      ok = ok && pos1.posOfAvailChunk==64 && pos1.length==64;
                      testAllocator1.free(0, 128);
                      ok = ok && testAllocator1.bucketPool[0] == 0;
                      ok = ok && testAllocator1.bucketPool[1] == 0;

                      BitAlloc<5*CHARBITS, UBYTE, BitAlloc_Mode::FAST, comptime> testAllocator2;
                      auto pos = testAllocator2.largestAvailChunk(3);
                      ok = ok && pos.posOfAvailChunk==0 && pos.length==3;
                      pos = testAllocator2.largestAvailChunk(3);
                      ok = ok && pos.posOfAvailChunk==3 && pos.length==3;
                      pos = testAllocator2.largestAvailChunk(2);
                      ok = ok && pos.posOfAvailChunk==6 && pos.length==2;
                      ok = ok && testAllocator2.bucketPool[0] == 0b11111111;
                      testAllocator2.free(3, 3);
                      ok = ok && testAllocator2.bucketPool[0] == 0b11100011;
                      pos = testAllocator2.largestAvailChunk(5);
                      ok = ok && pos.posOfAvailChunk==8 && pos.length==5;
                      ok = ok && testAllocator2.bucketPool[1] == 0b11111000;
                      pos = testAllocator2.largestAvailChunk(18);
                      ok = ok && pos.posOfAvailChunk==16 && pos.length==18;
                      ok = ok && testAllocator2.bucketPool[0] == 0b11100011;
                      ok = ok && testAllocator2.bucketPool[1] == 0b11111000;
                      ok = ok && testAllocator2.bucketPool[2] == 255;
                      ok = ok && testAllocator2.bucketPool[3] == 255;
                      ok = ok && testAllocator2.bucketPool[4] == 0b11000000;
                      pos = testAllocator2.largestAvailChunk(7);
                      ok = ok && pos.posOfAvailChunk==34;
                      ok = ok && pos.length==6;
                      testAllocator2.free(10, 24);
                      ok = ok && testAllocator2.bucketPool[1] == 0b11000000;
                      ok = ok && testAllocator2.bucketPool[2] == 0;
                      ok = ok && testAllocator2.bucketPool[3] == 0;
                      ok = ok && testAllocator2.bucketPool[4] == 0b00111111;
                      testAllocator2.free(1, 1);
                      ok = ok && testAllocator2.bucketPool[0] == 0b10100011;
                      testAllocator2.free(12, 5); // free already free space
                      ok = ok && testAllocator2.bucketPool[1] == 0b11000000; // should remain unchanged
                      ok = ok && testAllocator2.bucketPool[2] == 0;          // unchanged
                      testAllocator2.free(30, 6);
                      ok = ok && testAllocator2.bucketPool[3] == 0;
                      ok = ok && testAllocator2.bucketPool[4] == 0b00001111;
                      pos = testAllocator2.largestAvailChunk(20);
                      ok = ok && pos.posOfAvailChunk==16 && pos.length==20;
                      ok = ok && testAllocator2.bucketPool[4] == 0b11111111; // should be 4 existing bits + 4 new bits
                      return ok;
                  }()
                 );

    static_assert([]
                  {
                      constexpr bool comptime = true;
                      BitAlloc<4*CHARBITS, UBYTE, BitAlloc_Mode::FAST, comptime> testAlloc;
                      testAlloc.largestAvailChunk(17);
                      testAlloc.free(8, 8);
                      bool ok = testAlloc.bucketPool[0] == 0xff;
                      ok = ok && testAlloc.bucketPool[1] == 0b00000000;
                      ok = ok && testAlloc.bucketPool[2] == 0b10000000;
                      const auto pos = testAlloc.largestAvailChunk(10); // not enough space anywhere
                      ok = ok && testAlloc.bucketPool[1] == 0b11111111; // 8 of ten placed here
                      ok = ok && pos.posOfAvailChunk==8 && pos.length==8; // This check is critical!
                      ok = ok && testAlloc.bucketPool[2] == 0b10000000; // This one should remain unchanged in the BitAlloc_Mode::FAST mode.
                                                                        // In TIGHT mode, it would have been 0b11111111 with 7 bits placed here
                      ok = ok && testAlloc.bucketPool[3] == 0;          // and the remaining 3 bits placed here
                      return ok;
                  }()
                 );





/****************************************/
/*                 Fisher-Yates shuffle */
/****************************************/
    template <typename Container>
    constexpr void FisherYates(Container& container, const int len, const int s)
    {
        for (int k = 0; k < len; ++k)
        {
            const int r = k + s % (len - k);
            auto temp = container[k];
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

    UDWORD pcg32rand(const UQWORD seed=0);
    UWORD  pcg16rand(const UDWORD seed=0);

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
        if (i<=1) return i;
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
        static bool initialized = false;
        static pcg32 generator;
        if (seed != 0 || !initialized) {
            generator = pcg32(seed);
            initialized = true;
        }
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
        static bool initialized = false;
        static pcg16 generator;
        if (seed != 0 || !initialized) {
            generator = pcg16(seed);
            initialized = true;
        }
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





inline float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }
inline float sigmoid_derivative(float x) { return x * (1.0f - x); }
inline float relu(float x) { return x > 0 ? x : 0.0001*x; }
//inline float relu_derivative(float x) { return x > 0 ? 1 : 0.0001*x; }
inline float relu_derivative(float x) { return x > 0 ? 1.0f : 0.0001f; }
using std::tanh;
inline float tanh_derivative(float y) { return 1.0f - y * y; }


/****************************************/
/*  feed forward 32 (this one is worse) */
/****************************************/
    template <int InputSize, int OutputSize, int Max_layers, int HiddenWidth, typename Rng, int batchSize = 1>
    class FeedForward32
    {
    public:
        static constexpr int columns = InputSize+((Max_layers-1)*HiddenWidth)+OutputSize;
        template <int Size>
        struct BitArray
        {
            UQWORD data[(Size+63) / 64] = {0};
            bool operator[](int idx) const
            {
                const int word = idx / 64;
                const int bit  = idx % 64;
                return (data[word] >> (63 - bit)) & 1;
            }
            void set(int idx, bool value)
            {
                const int word = idx / 64;
                const int bit  = idx % 64;
                if (value)
                    data[word] |= (UQWORD(1) << (63 - bit));
                else
                    data[word] &= ~(UQWORD(1) << (63 - bit));
            }
            bool operator==(const BitArray& other) const
            {
                for (int i = 0; i < (Size+63) / 64; ++i)
                {
                    if (data[i] != other.data[i])
                        return false;
                }
                return true;
            }
        };
    private:
        static constexpr int nLayers = Max_layers;
        FLOAT weights[columns * columns];
        FLOAT biases[columns * Max_layers];
        BitArray<columns*columns> topologies[ Max_layers ];
        FLOAT activations[columns * Max_layers];
    private:
        static FLOAT u64_to_float(const UQWORD i)
        {
            union { UQWORD i; DOUBLE d; } u;
            // This generates a double in [1.0, 2.0) by setting the exponent bits
            // and using the top 52 bits of the UQWORD for the mantissa.
            u.i = (0x3FFULL << 52) | (i >> 12);
            return static_cast<FLOAT>(u.d - 1.0);
        }
    private:
        template <FLOAT (*Act)(FLOAT)>
        void forward567(const int layer, const FLOAT *input, int inputSize)
        {
            #if 0
            FLOAT tops[columns*columns];
            for (int dst = 0; dst < columns; ++dst)
            {
                for (int src = 0; src < inputSize; ++src)
                {
                    const int i = dst * columns + src;
                    tops[dst * columns + src] = topologies[layer][i];
                }
            }

            for (int dst = 0; dst < columns; ++dst)
                {
                    __m256 v_sum = _mm256_setzero_ps();
                    int src = 0;
                    const int weight_row_start = dst * columns;


                    //__m256 v_connected[inputSize/8];
                    //int pos = 0;
                    //for (int src2=src; src2 <= inputSize - 8; src2 += 8)
                    //{
                    //    const float connected_mask[8] = {
                    //        (float)topologies[layer][weight_row_start + src2 + 0],
                    //        (float)topologies[layer][weight_row_start + src2 + 1],
                    //        (float)topologies[layer][weight_row_start + src2 + 2],
                    //        (float)topologies[layer][weight_row_start + src2 + 3],
                    //        (float)topologies[layer][weight_row_start + src2 + 4],
                    //        (float)topologies[layer][weight_row_start + src2 + 5],
                    //        (float)topologies[layer][weight_row_start + src2 + 6],
                    //        (float)topologies[layer][weight_row_start + src2 + 7]
                    //    };
                    //    v_connected[pos++] = _mm256_loadu_ps(connected_mask);
                    //}
                    //pos = 0;

                    for (; src <= inputSize - 8; src += 8)
                    {
                        // Build the connection mask for 8 elements
                        //const float connected_mask[8] = {
                        //    (float)topologies[layer][weight_row_start + src + 0],
                        //    (float)topologies[layer][weight_row_start + src + 1],
                        //    (float)topologies[layer][weight_row_start + src + 2],
                        //    (float)topologies[layer][weight_row_start + src + 3],
                        //    (float)topologies[layer][weight_row_start + src + 4],
                        //    (float)topologies[layer][weight_row_start + src + 5],
                        //    (float)topologies[layer][weight_row_start + src + 6],
                        //    (float)topologies[layer][weight_row_start + src + 7]
                        //};

                        const __m256 v_connected = _mm256_loadu_ps(&tops[weight_row_start+src]);
                        //const __m256 v_connected = _mm256_loadu_ps(connected_mask);

                        // Load 8 contiguous inputs and weights (use 'loadu' for unaligned access)
                        const __m256 v_input = _mm256_loadu_ps(&input[src]);
                        const __m256 v_weight = _mm256_loadu_ps(&weights[weight_row_start + src]);

                        __m256 v_prod = _mm256_mul_ps(v_connected, v_weight);
                        v_sum = _mm256_fmadd_ps(v_prod, v_input, v_sum);
                    }

                    // Horizontal sum of the 8 floats in the v_sum vector
                    __m128 v_low = _mm256_castps256_ps128(v_sum);
                    __m128 v_high = _mm256_extractf128_ps(v_sum, 1);
                    v_low = _mm_add_ps(v_low, v_high);
                    __m128 h_sum = _mm_hadd_ps(v_low, v_low);
                    h_sum = _mm_hadd_ps(h_sum, h_sum);
                    float sum = _mm_cvtss_f32(h_sum);

                    // Handle any remaining elements (if inputSize is not a multiple of 8)
                    for (; src < inputSize; ++src)
                    {
                        const int i = weight_row_start + src;
                        if (topologies[layer][i])
                        {
                            sum += input[src] * weights[i];
                        }
                    }

                    // Apply bias and activation function
                    activations[(layer * columns) + dst] = Act(sum + biases[(layer * columns) + dst]);
                }
            #endif
        }


        template <FLOAT (*Act)(FLOAT)>
        void forward3455(const int layer, const FLOAT *input, int inputSize)
        {
            int cntW = 0;
            float sparseWeights[columns+columns];
            float sparseSrc[columns+columns];
            float sparseDst[columns+columns];
            for (int dst = 0; dst < columns; ++dst)
            {
                for (int src = 0; src < inputSize; ++src)
                {
                    const int i = dst * columns + src;
                    if (topologies[layer][i] != 0)
                    {
                        sparseWeights[cntW] = weights[i];
                      //  sparseSrc[cntSrc++] = src;
                    }
                }
            }





            for (int dst = 0; dst < columns; ++dst)
            {
                float sum = 0.0f;
                for (int src = 0; src < inputSize; ++src)
                {
                    const int i = dst * columns + src;
                    const float connected = topologies[layer][i];
                    sum += input[src] * weights[i] * connected;
                }
                activations[(layer*columns) + dst] = Act(sum + biases[(layer*columns) + dst]);
            }
        }



        template <FLOAT (*Act)(FLOAT)>
        void forward(const int layer, const FLOAT *input, int inputSize)
        {
            for (int dst = 0; dst < columns; ++dst)
            {
                FLOAT tops[columns];
                for (int src = 0; src < inputSize; ++src)
                {
                    const int i = dst * columns + src;
                    tops[src] = topologies[layer][i];
                }

                FLOAT sum = 0.0f;
                for (int src = 0; src < inputSize; ++src)
                {
                    const int i = dst * columns + src;
                    sum += input[src] * weights[i] * tops[src];
                }
                activations[(layer*columns) + dst] = Act(sum + biases[(layer*columns) + dst]);
            }
        }



            //for (int dst = 0; dst < columns; ++dst)
            //    {
            //        // Vector accumulator for the dot product.
            //        float32x4_t v_sum = vdupq_n_f32(0.0f);
            //        int src = 0;
            //        const int weight_row_start = dst * columns;
            //
            //        for (; src <= inputSize - 4; src += 4)
            //        {
            //            // 1. Load 4 contiguous inputs. (FAST)
            //            const float32x4_t v_input = vld1q_f32(&input[src]);
            //
            //            // 2. Load 4 contiguous weights. (FAST)
            //            const float32x4_t v_weight = vld1q_f32(&weights[weight_row_start + src]);
            //
            //            // 3. Handle the 'connected' bits. This remains a bottleneck.
            //            //    We still need to build the mask on the fly.
            //            const float connected_mask[4] = {
            //                (float)topologies[layer][weight_row_start + src + 0],
            //                (float)topologies[layer][weight_row_start + src + 1],
            //                (float)topologies[layer][weight_row_start + src + 2],
            //                (float)topologies[layer][weight_row_start + src + 3]
            //            };
            //            const float32x4_t v_connected = vld1q_f32(connected_mask);
            //
            //            // 4. Multiply weights and inputs.
            //            float32x4_t v_prod = vmulq_f32(v_input, v_weight);
            //            v_sum = vmlaq_f32(v_sum, v_prod, v_connected);
            //        }
            //
            //        // 7. Horizontally add the 4 lanes of the sum vector to get a single float.
            //        float sum = vgetq_lane_f32(v_sum, 0) + vgetq_lane_f32(v_sum, 1) +
            //                    vgetq_lane_f32(v_sum, 2) + vgetq_lane_f32(v_sum, 3);
            //
            //        // 8. Handle any remaining elements (if inputSize is not a multiple of 4).
            //        for (; src < inputSize; ++src)
            //        {
            //            const int i = weight_row_start + src;
            //            if (topologies[layer][i])
            //            {
            //                sum += input[src] * weights[i];
            //            }
            //        }
            //
            //        // 9. Apply bias and activation function.
            //        activations[(layer * columns) + dst] = Act(sum + biases[(layer * columns) + dst]);
            //    }


        template <FLOAT (*Deriv)(FLOAT)>
        void backward(const int topologyIdx, FLOAT *deltas, const FLOAT *inputs, const FLOAT *activations, FLOAT learning_rate)
        {
            FLOAT hidden_error[columns] = {0.0f};
            for (int input_idx = 0; input_idx < columns; ++input_idx)
            {
                const FLOAT common = inputs[input_idx] * learning_rate;
                for (int hidden_idx = 0; hidden_idx < columns; ++hidden_idx)
                {
                    const int weight_idx = input_idx * columns + hidden_idx;
                    const auto connected = topologies[topologyIdx][weight_idx];
                    hidden_error[hidden_idx] += inputs[input_idx] * weights[weight_idx] * connected;
                    weights[weight_idx] += activations[hidden_idx] * common * connected;
                }
            }
            for (int h = 0; h < columns; ++h)
            {
                deltas[h] = hidden_error[h] * Deriv(activations[h]);
                const int biasIdx = (topologyIdx - 1) * columns + h;
                biases[biasIdx] += deltas[h] * learning_rate;
            }
        }

    public:
        explicit FeedForward32(Rng& rng, bool randomizeTopology = true)
        {
            // Initialize weights/biases with random values in range [-1.0, 1.0]
            // should be ~symmetric around zero
            //for (int i = 0; i < columns * columns; ++i)
            //    weights[i] = 2.0f * u64_to_float(rng()) - 1.0f;
            //for (int i = 0; i < columns * Max_layers; ++i)
            //    biases[i] = 2.0f * u64_to_float(rng()) - 1.0f;







            // 1. Calculate hidden width to determine weight scaling
                  const int hiddenSizex = (columns-(InputSize+OutputSize)) / (nLayers-1);

                  // 2. He/Xavier Initialization: scale weights down so massive summations
                  // don't blow out the Tanh Output derivative into 0.0!
                  const FLOAT weight_scale = std::sqrt(2.0f / (FLOAT)(columns));

                  for (int i = 0; i < columns * columns; ++i)
                      weights[i] = (2.0f * u64_to_float(rng()) - 1.0f) * weight_scale;

                  // 3. Initialize biases slightly positive to ensure ReLUs aren't born "dead"
                  for (int i = 0; i < columns * Max_layers; ++i)
                      biases[i] = 0.01f;










            // Initialize DAG:
            int from = InputSize*columns;
            const int hiddenSize = (columns-(InputSize+OutputSize)) / (nLayers-1);
            for (int i=0; i<hiddenSize; ++i)
            {
                for (int j=0; j<InputSize; ++j)
                    topologies[0].set(from + j, true);
                from += columns;
            }
            from += InputSize;
            for (int layer=1; layer<nLayers-1; ++layer)
            {
                for (int i=0; i<hiddenSize; ++i)
                {
                    for (int j=0; j<hiddenSize; ++j)
                        topologies[layer].set(from + j, true);
                    from += columns;
                }
                from += hiddenSize;
            }
            const int current_src = from % columns; // Preserve previous layer's src offset
            from = (columns - OutputSize) * columns + current_src; // Hard-align the dst
            for (int i=0; i<OutputSize; ++i)
            {
                for (int j=0; j<hiddenSize; ++j)
                    topologies[nLayers-1].set(from + j, true);
                from += columns;
            }

            // Randomize connections a bit:
            for (int layer=0; randomizeTopology && layer<nLayers; ++layer)
            {
                for (int i=0; i<columns*columns; ++i)
                {
                    if (rng() % 100 < 25) // 25% chance to flip
                    {
                        const bool current = topologies[layer][i];
                        topologies[layer].set(i, !current);
                    }
                }
            }
        }

        FLOAT *evaluate(const FLOAT *inputs)
        {
            forward<relu>(0, inputs, InputSize);
            for (int i=1; i<nLayers-1; ++i)
                forward<relu>(i, &activations[(i-1)*columns], columns);
            // todo: tahn or softmax?
            forward<tanh>(nLayers-1, &activations[(nLayers-2)*columns], columns);
            return &activations[(nLayers-1)*columns];
        }

        FLOAT evaluateBatch()
        {
            // Not implemented yet, but the idea is to process multiple inputs in parallel using SIMD.
            // This would require reorganizing the data layout to be more cache-friendly and SIMD-friendly.
            return 0.0f;
        }

        FLOAT train(const FLOAT *inputs, const FLOAT *targets, FLOAT learning_rate)
        {
            forward<relu>(0, inputs, InputSize);
            for (int i=1; i<nLayers-1; ++i)
                forward<relu>(i, &activations[(i-1)*columns], columns);
            // last layer:
            // todo: tahn or softmax?
            forward<tanh>(nLayers-1, &activations[(nLayers-2)*columns], columns);

            // Output to last hidden (sigmoid):
            FLOAT squared_error_sum = 0.0f;
            FLOAT output_delta[columns] = {0.0f};
            const int lastLayerIdx = (nLayers-1) * columns;
            for (int j=0; j<OutputSize; ++j)
            {
                const int output_neuron_idx = (columns - OutputSize) + j;
                const FLOAT output_error = targets[j] - activations[lastLayerIdx + output_neuron_idx];
                output_delta[output_neuron_idx] = output_error * tanh_derivative(activations[lastLayerIdx + output_neuron_idx]);
                biases[lastLayerIdx + output_neuron_idx] += output_delta[output_neuron_idx] * learning_rate;

                // this line has nothing to do with anything, just for the return at the end:
                squared_error_sum += output_error * output_error;
            }


            // Hidden (relu):
            FLOAT *next_layer_deltas = output_delta;
            FLOAT delta_buffer[columns];
            for (int l = nLayers-1; l > 1; --l)
            {
                backward<relu_derivative>(l, delta_buffer, next_layer_deltas, &activations[(l-1)*columns], learning_rate);
                next_layer_deltas = delta_buffer;
            }


            // Hidden to input (sigmoid):
            backward<relu_derivative>(1, delta_buffer, next_layer_deltas, activations, learning_rate);


            // Update weights (hidden to input):
            for (int hidden_idx = 0; hidden_idx < columns; ++hidden_idx)
            {
                const FLOAT common = delta_buffer[hidden_idx] * learning_rate;
                for (int input_idx = 0; input_idx < InputSize; ++input_idx)
                {
                    const int weight_idx = hidden_idx * columns + input_idx;
                    const auto connected = topologies[0][weight_idx];
                    weights[weight_idx] += inputs[input_idx] * common * connected;
                }
            }
            return squared_error_sum / OutputSize; // mean squared error
        }
    };


/****************************************/
/*           feed forward 16 (IEEE 754) */
/****************************************/
template <int InputSize, int OutputSize, int Max_layers, int HiddenWidth, typename Rng>
using FeedForward16 = FeedForward32<InputSize, OutputSize, Max_layers, HiddenWidth, Rng>;
// FLOAT masterCopy


/****************************************/
/*              "Neural Network" 🙄🙄🙄 */
/****************************************/
#if defined(__AVX512FP16__) || defined(__ARM_NEON) || defined(__wasm_simd128__)
  template <int InputSize, int OutputSize, int Max_layers, int HiddenWidth, typename Rng>
  using Neural = FeedForward16<InputSize, OutputSize, Max_layers, HiddenWidth, Rng>;
#else
  // Of course not... 🙄
  template <int InputSize, int OutputSize, int Max_layers, int HiddenWidth, typename Rng>
  using Neural = FeedForward32<InputSize, OutputSize, Max_layers, HiddenWidth, Rng>;
#endif







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
                      auto correct0 = Neural<2,2,3,16,decltype(rng)>::BitArray<columns*columns>{
                                          0b0000000000000000000011000000001100000000110000000000000000000000,
                                          0b0000000000000000000000000000000000000000000000000000000000000000
                                      };
                      auto correct1 = Neural<2,2,3,16,decltype(rng)>::BitArray<columns*columns>{
                                          0b0000000000000000000000000000000000000000000000000000111000000011,
                                          0b1000000011100000000000000000000000000000000000000000000000000000
                                      };
                      auto correct2 = Neural<2,2,3,16,decltype(rng)>::BitArray<columns*columns>{
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



//#if defined(AI_DEBUG)
//#endif



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
  //  #if defined(AI_DEBUG)
      #define aiAssert(x) assert(x)
 //     #define aiDebug(x) x
  //  #else
  //    #define aiAssert(x)
   //   #define aiDebug(x)
    //#endif


/****************************************/
/*   A 'move'/'action' by a user/player */
/* within the game context              */
/****************************************/
    template <typename T>
    concept GameMove = std::movable<T> || std::copyable<T>;


/****************************************/
/*            outcome of a single round */
/****************************************/
    enum class Outcome { running, draw, fin };


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
    template <typename T>
    concept convertible_to_ptrFloat = std::convertible_to<T, DOUBLE *> || std::convertible_to<T, FLOAT *>;

    template <typename T>
    concept Gameview =
        !std::copy_constructible<T> &&
        !std::is_copy_assignable<T>::value &&
        !std::copyable<T> &&
        requires (T obj, const T cobj)
        {
            // todo: how to require noexcept?
            {cobj.clone()} -> std::same_as<T>;
            {cobj.generateMovesAndGetCnt(nullptr)} -> std::convertible_to<int>;
            {obj.doMove(typename T::Move{})} -> std::same_as<Outcome>;
            {obj.switchPlayer()};
            {cobj.getCurrentPlayer()} -> std::equality_comparable;
            {cobj.getWinner()} -> std::equality_comparable;
            {cobj.getBoardScore()} -> std::convertible_to<FLOAT>;
            {obj.getNetworkInputs()} -> convertible_to_ptrFloat;
            {obj.randomize()};
            //{ T::MaxNetworkInputs } -> std::convertible_to<std::size_t>; // todo
            //requires std::bool_constant<T::MaxNetworkInputs >= 0>::value;
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
    struct Node // todo: convert this from an 'array of structs' into a 'struct of arrays'
    {
        static constexpr SWORD never_expanded = -1;
        static constexpr SWORD removed = -2; // Debug
        SWORD    activeBranches = never_expanded; // Must be signed!
        SWORD    createdBranches = 0;             // Must be signed!
        Node    *parent = nullptr;
        Node    *branches = nullptr;  // Must be initialized to NULL
        FLOAT    score = 0.f; //0.01f;    // Start with some positive value to prevent divide by zero error
        Move     moveHere;
        SWORD    visits = 1; // Must be '1' to stop 'x/0' // todo if changed also chage in ucbselect()!
        FLOAT    UCBscore;         // Placeholder used during UCB calc.
        FLOAT    nnScore;
        #ifdef INCLUDEAI__SEPARATE_SCORE_FOR_TERMINAL_NODES
          FLOAT    terminalScore = 0.f; // Irrelevant. "cutoff" ensures that branches are pruned beyond terminal depth
        #endif
SWORD branchScore = 0;
int shallowestTerminalDepth = 9999;

        constexpr Node() noexcept
          : moveHere{}
        {}

        constexpr Node(Node *newParent, Move move) noexcept
          : activeBranches(never_expanded),
            parent(newParent),
            // Ownership is implicit: the 'owner' is the player who makes this move!
            moveHere(move)
        {}

        Node(const Node&)            = delete;
        Node& operator=(const Node&) = delete;
        Node(Node&&)                 = delete;
        Node& operator=(Node&& other) noexcept
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

    //static_assert(sizeof(Node<SQWORD>) <= 64); todo: del this line after a-o-s -> s-o-a conversion


/****************************************/
/*                           Ai context */
/****************************************/
    template <int NumNodes, GameMove MoveType, BitfieldIntType BitfieldType>
    struct Ai_ctx
    {
        static constexpr int numNodes = NumNodes;
        BitAlloc<NumNodes, BitfieldType> bitalloc;
        Node<MoveType> nodePool[NumNodes];

        Ai_ctx() {}
        Ai_ctx(const Ai_ctx&) = delete;
        Ai_ctx& operator=(const Ai_ctx&) = delete;
    };


/****************************************/
/*    Search tree node helper functions */
/****************************************/
    template <int NumNodes, GameMove MoveType, BitfieldIntType BitfieldType>
    inline void disconnectBranch(Ai_ctx<NumNodes, MoveType, BitfieldType>& ai_ctx,
                                 Node<MoveType> *parent,
                                 const Node<MoveType> *removeMe)
    {
        aiAssert(parent);
        aiAssert(parent->activeBranches > 0);
        aiAssert(parent->branches);
        aiAssert(parent == removeMe->parent);
        aiAssert((removeMe-ai_ctx.nodePool)>=0 && (removeMe-ai_ctx.nodePool)<ai_ctx.numNodes);
        aiAssert((parent->parent==nullptr) || (removeMe->activeBranches <= 0));

        //std::printf("disc pos: %d len: %d parnode: %p \n", removeMe-ai_ctx.nodePool, 1, parent-ai_ctx.nodePool);


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
            //std::printf("rem: %d \n", removeMe->createdBranches);
            ai_ctx.bitalloc.free(removeMe->branches - ai_ctx.nodePool, removeMe->createdBranches);
        }

        // But don't discard() the child node, that is the slot we will swap into:
        //discard(ai_ctx, swapDst); // incorrect!! // todo test!!


        const MoveType removedMoveHere = swapDst.moveHere;
        const auto     removedVisits   = swapDst.visits;
        const auto     removedScore    = swapDst.score;
        const auto     removedBranchScore = swapDst.branchScore;
        const auto     removedShallowestTerminalDepth = swapDst.shallowestTerminalDepth;

        Node<MoveType>& swapSrc = parent->branches[parent->activeBranches-1];

        // Don't swap w/ itself if the to-be-removed node is last:
        if (&swapDst == &parent->branches[parent->activeBranches-1])
            parent->activeBranches -= 1;

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
        for (int i=0; i<swapDst.activeBranches; ++i)
            swapDst.branches[i].parent = &swapDst;

        parent->activeBranches -= 1;
        aiAssert(swapDst.parent == parent);
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
            Board boardSim = original.clone(); // todo: randomize?
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
/* A board.clone() arriving here is     */
/* probably already randomized.         */
/* Randomization does not work here     */
/****************************************/
    constexpr SWORD MinimaxWin            =    1;
    constexpr SWORD MinimaxDraw           =    0;
    constexpr SWORD MinimaxLose           =   -1;
    constexpr SWORD MinimaxInit           =   -2;
    // Both "undeterminable" and "indeterminable" are correct and can be used interchangeably in many
    // contexts. However, "indeterminable" is more commonly used in formal or academic writing. (ai)
    constexpr SWORD MinimaxIndeterminable = -999;

    template <Gameview Board, GameMove MoveType>
    constexpr SWORD minimax(const Board& current, const MoveType move, SWORD alpha, SWORD beta, const int depth)
    {
        Board clone = current.clone();
        const Outcome outcome = clone.doMove(move);
        clone.switchPlayer();
        if (outcome != Outcome::running)
        {
            if (outcome == Outcome::draw)
                return MinimaxDraw;
            else if (clone.getWinner() != current.getCurrentPlayer())
                return MinimaxLose;
            else
                return MinimaxWin;
        }
        if (depth<=0) { return MinimaxIndeterminable; }
        typename Board::StorageForMoves storageForMoves;
        int nMoves = clone.generateMovesAndGetCnt(storageForMoves);
        nMoves -= 1;
        if (clone.getCurrentPlayer() == current.getCurrentPlayer())
        {
            SWORD bestScore = MinimaxInit;
            bool encounteredIndeterminable = false;
            while (nMoves >= 0)
            {
                const MoveType moveHere = storageForMoves[nMoves];
                const SWORD returnedScore = minimax(clone, moveHere, alpha, beta, depth-1);
                if (returnedScore == MinimaxIndeterminable)
                {
                    encounteredIndeterminable = true;
                    // We cannot use this branch for scoring (yet),
                    // but we must continue searching in case we find a Win elsewhere.
                    nMoves -= 1;
                    continue;
                }
                bestScore = aiMax(bestScore, returnedScore);
                alpha     = aiMax(alpha, bestScore);
                if (bestScore == MinimaxWin) // Can't do better than "win"
                    break;
                if (beta <= alpha)
                    break;
                nMoves -= 1;
            }
            if (encounteredIndeterminable && bestScore != MinimaxWin) // rhs: Never let indeterminable paths overwrite a proven win.
                // hit a depth limit on one of the branches:
                return MinimaxIndeterminable;
            return bestScore == MinimaxInit ? MinimaxDraw : bestScore;
        }
        else
        {
            SWORD bestOpponentScore = MinimaxInit; // Start at -2 (Opponent Loss)
            bool encounteredIndeterminable = false;
            // Alpha/Beta from Opponent's perspective:
            SWORD oppAlpha = -beta;
            const SWORD oppBeta  = -alpha;
            while (nMoves >= 0)
            {
                const MoveType moveHere = storageForMoves[nMoves];
                const SWORD returnedScore = minimax(clone, moveHere, -beta, -alpha, depth-1);
                if (returnedScore == MinimaxIndeterminable)
                {
                    // See above for explanation ^
                    encounteredIndeterminable = true;
                    nMoves -= 1;
                    continue;
                }
                bestOpponentScore = aiMax(bestOpponentScore, returnedScore);
                oppAlpha = aiMax(oppAlpha, returnedScore);
                if (bestOpponentScore == MinimaxWin)
                    break;
                if (oppBeta <= oppAlpha)
                    break;
                nMoves -= 1;
            }
            if (encounteredIndeterminable && bestOpponentScore != MinimaxWin)
                return MinimaxIndeterminable; // see above^ for explanation
            return bestOpponentScore == MinimaxInit ? MinimaxDraw : -bestOpponentScore;
        }
    }

    template <Gameview Board, GameMove MoveType>
    inline constexpr SWORD minimax(const Board& current, const int MaxDepth)
    {
        Board clone = current.clone();
        typename Board::StorageForMoves storageForMoves;
        int nMoves = clone.generateMovesAndGetCnt(storageForMoves);
        nMoves -= 1;
        SWORD best = MinimaxInit;
        bool encounteredIndeterminable = false;
        while (nMoves >= 0)
        {
            const MoveType moveHere = storageForMoves[nMoves];
            const SWORD mnx = minimax(clone, moveHere, MinimaxLose, MinimaxWin, MaxDepth);
            if (mnx == MinimaxIndeterminable)
            {
                encounteredIndeterminable = true;
                nMoves -= 1;
                continue;
            }
            if (mnx > best)
                best = mnx;
            if (best == MinimaxWin)
                break;
            nMoves -= 1;
        }
        if (encounteredIndeterminable && best != MinimaxWin)
            return MinimaxIndeterminable;
        return best==MinimaxInit ? MinimaxDraw : best;
    }


/****************************************/
/*                               result */
/****************************************/
    template <GameMove MoveType>
    struct MCTS_result
    {
        enum { simulations, minimaxes, thresholdLevel, networkEvaluated, terminalReached,
               score, visits,
               end
             };
        float statistics[end] = {0};
        MoveType best;
        bool errorOutOfMem = false;
    };

    template <GameMove MoveType>
    class MCTS_Future
    {
    public:
        bool ready() const
        {
            return false;
        }

        MoveType getResult() const
        {
        }
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
              typename NN,
              typename Rndfunc
             >
             // todo: 'rand' is only used in the simulator, so maybe it should be moved there instead of being passed all the way down here?
    constexpr MCTS_result<MoveType> mcts(const Board& boardOriginal, AiCtx& ai_ctx, NN& nn, Rndfunc rand) noexcept
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
                    constexpr float exploration_C = 1.618f;// * 2; // useless?
                    constexpr float Hoeffdings_multiplier = 1.f; //2.f; // (http://www.incompleteideas.net/609%20dropbox/other%20readings%20and%20resources/MCTS-survey.pdf)
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

        [[maybe_unused]] auto PUCT =
            [](const Node<MoveType>& node) -> Node<MoveType> *
            {
                return nullptr;
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
        FLOAT threshold = 1.1f; // 'threshold' above which the result of .evaluate() is used, not minimax or randroll
        SWORD rootMovesRemaining;
        MCTS_result<MoveType> mcts_result;
        for (int iterations=0; root->activeBranches!=0 && iterations<MaxIterations; ++iterations)
        {
            Node<MoveType> *selectedNode = root;
            Board boardClone = boardOriginal.clone();
            boardClone.randomize(); // Hidden information is simulated by creating a "plausibe" random game state
            typename Board::StorageForMoves storageForMoves;
            Outcome outcome = Outcome::running;
            int depth = 1;




            // 1. Traverse tree and select leaf:
            Node<MoveType> *parentOfSelected;
            bool is_desynchronized = false;
            while ((selectedNode->activeBranches > 0) && (rootMovesRemaining == 0))
            {
                parentOfSelected = selectedNode;
                aiAssert(selectedNode->branches);
                selectedNode = UCBselectBranch(*selectedNode);
                //aiAssert(selectedNode->branchScore == 0);
                aiAssert(selectedNode->parent == parentOfSelected);
                const MoveType moveHere = selectedNode->moveHere;
                const int nMoves = boardClone.generateMovesAndGetCnt(storageForMoves);
                // desyncs can happen due to the call to randomize() above ^^
                bool moveIsValid = false;
                for (int i=0; i<nMoves; ++i)
                {
                    if (moveHere == storageForMoves[i])
                    {
                        moveIsValid = true;
                        break;
                    }
                }
                if (!moveIsValid)
                {
                    is_desynchronized = true;
                   // assert(false);
                    break;
                }


                outcome = boardClone.doMove(moveHere);
                boardClone.switchPlayer();
                depth += 1;
                if (outcome != Outcome::running)
                {
                    cutoffDepth = cutoffDepth < depth ? cutoffDepth : depth;
                    break;
                }
            }

            if (is_desynchronized)
                continue;

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

                //std::printf("np:%d, avl:%d \n", nodePos, nValidMoves);

                if ((nodePos+nValidMoves) >= ai_ctx.numNodes) [[unlikely]] // This can happen at the end
                {
                    mcts_result.errorOutOfMem = true;
                    std::printf("\033[1;35mexceeded! %d vs  %d \n\033[0m", ai_ctx.numNodes, nodePos+nValidMoves);
                    // Can't be salvaged. Once we are out of nodes we can't release already
                    // alloc'd branches (cuz we must reach a terminal node for that). We are stuck:
                    break; // out-of-mem is stopping condition
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
                //std::printf("\033[1;37mnew branch:%p brch:%p \033[0m \n", selectedNode-ai_ctx.nodePool, (&selectedNode->branches[0])-ai_ctx.nodePool);
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
                depth += 1;
            }
            // 3b. Pick (select) a node for analysis:
            else if (selectedNode->activeBranches > 0)
            {
                aiAssert(selectedNode->branches);
                FLOAT batchNNInputs[64*Board::MaxNetworkInputs] = {0};
                for (int i=0; i<aiMin(selectedNode->activeBranches, 64); ++i)
                {
                    Board boardForNN = boardClone.clone();
                    boardForNN.doMove( selectedNode->branches[i].moveHere );
                    boardForNN.switchPlayer();
                    for (int j=0; j<Board::MaxNetworkInputs; ++j)
                        batchNNInputs[i*Board::MaxNetworkInputs + j] = boardForNN.getNetworkInputs()[j];
                }
                // Should be rand to ensure fair distribution:
                const auto sample = rand() % selectedNode->activeBranches;
                // ^^^ What that means is that if we bias towards a specific kind of node: good,
                // bad or something else, then we will fail to discover unexpected possibilities!
                selectedNode = &selectedNode->branches[sample];
                //aiAssert(selectedNode->score < 1.f);
                outcome = boardClone.doMove( selectedNode->moveHere );
                boardClone.switchPlayer();
                depth += 1;
            }


            //  bool stop = false;
            float score = 0.f;
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
                // todo: remove this ifdef
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

                const FLOAT *pValues = nn.evaluate(boardClone.getNetworkInputs(), boardClone.getBoardScore());
                //const FLOAT *pValues = selectedNode->nnEvaluationResult;
                const FLOAT confidence = pValues[0];
                aiAssert(confidence<1.1f && confidence>-1.1f);
                if (aiAbs(confidence) < threshold)
                {
                    const SWORD branchscore = minimax<Board, MoveType>(boardClone, MinimaxDepth);
                    if (branchscore == MinimaxIndeterminable) // Fallback if minimax fails
                    {
                        // Simulate to get an estimation of the quality of this position:
                        score = simulate<SimDepth>(boardClone, rand);
                        score *= polarity-0.f;
                        mcts_result.statistics[MCTS_result<MoveType>::simulations] += 1;
                        // worst case: no clear result. Adjust threshold:
                        if (aiAbs(score) <= 0.1f)
                        {
                            //threshold = 0.f;
                            //mcts_result.statistics[MCTS_result<MoveType>::thresholdReset] += 1;
                        }
                    }
                    else
                    {
                        score = branchscore-0.f;
                        score *= polarity;
                        const bool sameSign = (score * confidence) > 0;
                        if (sameSign)
                        {
                            threshold = aiMin(confidence, threshold);
                            mcts_result.statistics[MCTS_result<MoveType>::thresholdLevel] = threshold;
                        }
                        #ifndef INCLUDEAI__INSTANT_LOBOTOMY
                          //selectedNode->shallowestTerminalDepth = depth + MinimaxDepth; // todo: we dont know what level the termination happend!
                          //disconnect = true;
                        #endif
                        mcts_result.statistics[MCTS_result<MoveType>::minimaxes] += 1;
                    }
                }
                else
                {
                    score = confidence * (polarity-0.f);
                    mcts_result.statistics[MCTS_result<MoveType>::networkEvaluated] += 1;
                }
            }
            else // This is a terminal node (game ended here)
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

            Node<MoveType> *leafNodeForPruning = selectedNode;






            // Floyd's Cycle. Ensure the graph remains acyclic (debug only!):
            #if defined(INCLUDEAI__CHECK_FOR_CYCLES)
              aiAssert([selectedNode]
                       {
                           Node<MoveType> *fast = selectedNode;
                           Node<MoveType> *slow = selectedNode;
                           while (fast)
                           {
                               if (fast)
                               fast = fast->parent;
                               if (fast == slow)
                                   return false;
                               if (fast)
                                   fast = fast->parent;
                               slow = slow->parent;
                           }
                           return true;
                       }()
                      );
            #endif






            // 5. Backprop/update tree:
            int child_dpt = 9999;
            while (selectedNode != root)
            {
                int dpt_temp = selectedNode->shallowestTerminalDepth;
                selectedNode->shallowestTerminalDepth = child_dpt<dpt_temp ? child_dpt : dpt_temp;
                child_dpt = dpt_temp;

                //selectedNode->visits += 1.f ;//- ((cur-0.f)/(depth-0.f)); // Games that have more than two possible outcomes (example:
                //selectedNode->visits += aiLog( 100 * ((cur-0.f)/(depth-0.f)) );

                // This 'if' is used to (optionally) stop counting scores for branches that are deeper
                // than the most immediate node where a turn ends:
                //if ([&]{ if constexpr ((cutoff_scoring&hyperparams)==cutoff_scoring) return branchDepth<cutoff; else return true; }())
               //////////
                if (depth <= cutoffDepth) [[likely]] // <- This line dramatically improves the "ai"
                {
                    // 'visits' does not tell us if a position is a winner or not. It tells us
                    // how -interesting- a position is:
                    selectedNode->visits += 1; // todo: if being forced to defend, ai won't "see" strong plays beyod the cutoff!!!!
                    selectedNode->score += score;

                    //depth += 1;
                    //selectedNode->weight += 1; //(fib(cur)-0.f) / (fib(depth)-0.f); //(aiLog(cur* 10)/2) / (depth-0.f); //((cur-0.f)/(depth-0.f)) ; weight
                //    cur += 1.f;
                    //    if (fib(cur+1) <= yyy)
                    //    cur += 1;

                }
                depth -= 1;

                //std::printf("sel:%p score: %1.3f act:%d crt:%d  \n", selectedNode-ai_ctx.nodePool, selectedNode->score, selectedNode->activeBranches, selectedNode->createdBranches);


                selectedNode = selectedNode->parent;
                //  if constexpr ((cutoff_scoring&hyperparams)==cutoff_scoring) { branchDepth -= 1; }


            }






            // 6. Cleanup:
            if (disconnect)
            {
                mcts_result.statistics[MCTS_result<MoveType>::terminalReached] += 1;

                Node<MoveType> *child = leafNodeForPruning;
                Node<MoveType> *parent = leafNodeForPruning->parent;
                aiAssert(parent);
                aiAssert(leafNodeForPruning != root);                  // Ensure 'selectedNode' not root
                //aiAssert([&]{ return outcome==Board::Outcome::running ? parent->branches!=nullptr : true; }());
                //aiAssert([&]{ return outcome==Board::Outcome::running ? parent->activeBranches>0 : true; }());
                aiAssert(parent->branches!=nullptr);
                // terminal nodes have no branches:
                aiAssert(parent->activeBranches!=0); //>0 || parent->activeBranches==Node::never_expanded);

                while (parent)
                {
                    //std::printf("/// active:%d *brnch-start:%p root:%p parent:%p dep:%d sel:%p scr:%2.2f\n", parent->activeBranches, parent->branches-ai_ctx.nodePool, root-ai_ctx.nodePool, parent-ai_ctx.nodePool, 0, selectedNode-ai_ctx.nodePool, score);

                    //std::printf("still active 'siblings': ");
                    for (int i=0; false&&i<parent->activeBranches; ++i)
                    {
                   //     std::printf("\033[0;33m%p scr:%2.2f  \033[0m", (&parent->branches[i])-ai_ctx.nodePool, parent->branches[i].score);
                    }
                  //  std::printf("%d \n", parent->activeBranches);


                    disconnectBranch(ai_ctx, parent, child);
                    if (parent->activeBranches != 0)
                    {
                        break; // Stop
                    }
                    else
                    {
                        child = parent;
                        aiAssert(parent != parent->parent);
                        parent = parent->parent; // Lolz
                    }
                }
            }
        } // iterations


        // Max child  vs Robust child vs Robust-max child vs Secure child "Progressive Strategies for Monte-Carlo Tree Searc"

        // The following "shallowTest" code is absolutely VITAL and must not be removed or "disabled"!!!:
        int shallowestTerminal = 9999;
        for (int i=0; i<root->createdBranches; ++i)
        {
            Node<MoveType>& branch = root->branches[i];
            if (branch.shallowestTerminalDepth < shallowestTerminal)
                shallowestTerminal = branch.shallowestTerminalDepth;
        }
        if (shallowestTerminal != 9999)
        {
            bool hasShallowWin = false;
            for (int i=0; i < root->createdBranches; ++i)
            {
                Node<MoveType>& branch = root->branches[i];
                if (branch.shallowestTerminalDepth == shallowestTerminal && branch.score > 0.f)
                {
                    hasShallowWin = true;
                    break;
                }
            }

            if (hasShallowWin)
            {
                // We found a forced win!
                // Nuke _everything_ that isn't this fast win to guarantee we play it:
                for (int i=0; i < root->createdBranches; ++i)
                {
                    Node<MoveType>& branch = root->branches[i];
                    if (branch.shallowestTerminalDepth != shallowestTerminal || branch.score <= 0.f)
                        branch.score = -9999.f;
                }
            }
            else
            {
                // The shallowest terminal is a forced LOSS.
                // We want to avoid it! Nuke ONLY the fast-losing branch(es) so we pick a survival path:
                for (int i=0; i < root->createdBranches; ++i)
                {
                    Node<MoveType>& branch = root->branches[i];
                    if (branch.shallowestTerminalDepth == shallowestTerminal)
                        root->branches[i].score = -9999.f;
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










            for (int i=0; i<root->createdBranches; ++i)
            {
                // todo assert(root->branches[i].score != nan);

                if (i==/*bestScore*/posScore) std::printf("\033[1;36m");
                //std::printf("mv: %c %d  bs:%d   ", root->branches[i].moveHere, root->branches[i].moveHere, root->branches[i].branchScore);
                // v/s == exploration E!
               // std::printf("s:%4.3f v:%d  dp: %d   act:%d  s/v:%2.3f v/s:%2.5f \033[0m \n", root->branches[i].score, root->branches[i].visits, root->branches[i].shallowestTerminalDepth, root->branches[i].activeBranches, root->branches[i].score/ root->branches[i].visits,root->branches[i].visits/root->branches[i].score);
            }
            std::printf("threshold: %.2f mv: %d \033[1;31mcutoff: %d\033[0m ACT:%d \n",
                          threshold, root->branches[posScore].moveHere, cutoffDepth, root->activeBranches ); //, ai_ctx.maxPatternSimilarity);




            mcts_result.statistics[MCTS_result<MoveType>::score]  = bestScore;
            mcts_result.statistics[MCTS_result<MoveType>::visits] = bestVisits;
            mcts_result.best = [root, posVisits, posScore, bestScore]
                               {
                                   if (bestScore > 0.f)
                                       return root->branches[posScore].moveHere;
                                   return root->branches[posVisits].moveHere;
                               }();
            return mcts_result;

    }

    template <int MaxIterations,
              int SimDepth,
              int MinimaxDepth,
              GameMove MoveType,
              BitfieldIntType BitfieldType,
              Gameview Board,
              typename AiCtx,
              typename Rndfunc
             >
    MCTS_Future<MoveType> mcts_async(const Board& boardOriginal, AiCtx& ai_ctx, Rndfunc rand) noexcept
    {
        // https://github.com/cdwfs/cds_sync/blob/master/cds_sync.h
        using atomic = int;
        static atomic rootMovesRemaining;
        if (rootMovesRemaining == 0)
        {
            typename Board::StorageForMoves storageForMoves;
            Board boardClone = boardOriginal.clone();
            rootMovesRemaining = boardClone.generateMovesAndGetCnt(storageForMoves);
        }
        while (rootMovesRemaining)
        {
            typename Board::StorageForMoves storageForMoves;
            Board boardClone = boardOriginal.clone();
          //  ??? = boardClone.generateMovesAndGetCnt(storageForMoves);
            // rootMovesRemaining -= 1;
        }
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
        TicTacTest& operator=(TicTacTest&&) = delete;

        constexpr TicTacTest clone() const
        {
            TicTacTest dst;
            for (int i=0; i<9; ++i) { dst.pos[i] = pos[i]; }
            dst.currentPlayer = currentPlayer;
            return dst;
        }

        constexpr int generateMovesAndGetCnt(TicTacTest::Move *availMoves) const
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

        constexpr float getBoardScore() const { return 0.f; }

        constexpr float *getNetworkInputs() { return &neuralInputs[0]; }

        constexpr void randomize() {}
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



/*
    NOTE: This file is auto-generated by generate.py from isprime.S.
    Do not edit directly.
*/

// Only enable for GCC/Clang on x86-64 where this syntax is valid.
#if defined(__GNUC__) && defined(__x86_64__)


// The 'naked' attribute tells GCC not to generate a function prologue/epilogue,
// as we provide it ourselves in the assembly code.
__attribute__((naked))
int is_prime(int n) {
    // The 'volatile' keyword prevents the compiler from optimizing this block away.
    __asm__ __volatile__(
                ".intel_syntax noprefix\n\t"
        "is_prime:\n\t"
        "push rbp\n\t"
        "mov rbp, rsp\n\t"
        "mov DWORD PTR [rbp-4], edi        ; store n into local var\n\t"
        "cmp DWORD PTR [rbp-4], 2\n\t"
        "jl .not_prime\n\t"
        "mov DWORD PTR [rbp-8], 2\n\t"
        ".loop:\n\t"
        "mov eax, DWORD PTR [rbp-8]        ; eax = i\n\t"
        "imul eax, eax                     ; eax = i * i\n\t"
        "cmp eax, DWORD PTR [rbp-4]        ; if (i*i > n) break\n\t"
        "jg .is_prime\n\t"
        "mov eax, DWORD PTR [rbp-4]        ; eax = n\n\t"
        "xor edx, edx\n\t"
        "mov ecx, DWORD PTR [rbp-8]        ; ecx = i\n\t"
        "idiv ecx                          ; eax=n/i, edx=n%i\n\t"
        "cmp edx, 0                        ; if (n % i == 0)\n\t"
        "je .not_prime\n\t"
        "add DWORD PTR [rbp-8], 1          ; i++\n\t"
        "jmp .loop\n\t"
        ".is_prime:\n\t"
        "mov eax, 1                        ; return 1 (true)\n\t"
        "pop rbp\n\t"
        "ret\n\t"
        ".not_prime:\n\t"
        "mov eax, 0                        ; return 0 (false)\n\t"
        "pop rbp\n\t"
        "ret\n\t"
        ".att_syntax prefix"
    );
}


#endif // __GNUC__ && __x86_64__



} // namespace include_ai



#endif // INCLUDEAI_IMPLEMENTATION


#endif // INCLUDEAI_HPP


/*
Copyright (c) 2025-2026, VECTORPHASE Systems
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


/* Whatever we conceive well we express clearly, and words flow with ease. (Nicolas Boileau-Despreaux) */



