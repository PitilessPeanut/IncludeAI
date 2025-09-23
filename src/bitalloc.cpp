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
                      BitAlloc<256, UQWORD, BitAlloc_Mode::FAST, comptime> testAllocator1;
                      testAllocator1.largestAvailChunk(63);
                      testAllocator1.largestAvailChunk(65);
                      testAllocator1.largestAvailChunk(64);
                      testAllocator1.free(1, 191);
                      testAllocator1.free(0, 1);
                      bool ok = testAllocator1.bucketPool[0] == 0;
                      auto pos1 = testAllocator1.largestAvailChunk(257);
                      ok = ok && pos1.posOfAvailChunk==0 && pos1.length==255; // Max size
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
                     // ok = ok && testAlloc.bucketPool[2] == 0b10000000;
                      const auto pos = testAlloc.largestAvailChunk(10); // not enough space anywhere
                      ok = ok && testAlloc.bucketPool[1] == 0b11111111; // 8 of ten placed here
                      ok = ok && pos.posOfAvailChunk==8 ; //&& pos.length==8;
                      ok = ok && testAlloc.bucketPool[2] == 0b10000000; // This one should remain unchanged in the BitAlloc_Mode::FAST mode.
                                                                        // In TIGHT mode, it would have been 0b11111111 with 7 bits placed here
                      ok = ok && testAlloc.bucketPool[3] == 0;          // and the remaining 3 bits placed here
                      return ok;
                  }()
                 );
