#include "bitalloc.hpp"
#include "asmtypes.hpp"

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
