#include "bitvec.hpp"
#include "asmtypes.hpp"

using Bitvec64 = Bitvec<UQWORD, CHARBITS>;
using Bitvec32 = Bitvec<UDWORD, CHARBITS>;


/****************************************/
/*                                Tests */
/****************************************/
static_assert([]
              {
                  UDWORD mem32[128/32] = {0};
                  Bitvec32 testVecA(mem32, 128/32);
                  testVecA.set(32);
                  testVecA.set(63);
                  testVecA.set(127);

                  bool ok = testVecA.check(32);
                  ok = ok && testVecA.check(63);
                  ok = ok && testVecA.check(127);
                  ok = ok && (testVecA.check(1) == 0);

                  UQWORD mem64[256/64] = {0};
                  Bitvec64 testVecB(mem64, 256/64);
                  ok = ok && (testVecB.checkRange(64, 127) == 0);
                  testVecB.set(32);
                  ok = ok && testVecB.check(32);
                  testVecB.setRange(100, 101);
                  ok = ok && (testVecB.check(99) == 0);
                  ok = ok && testVecB.check(100);
                  ok = ok && testVecB.check(101);
                  ok = ok && (testVecB.check(102) == 0);
                  ok = ok && testVecB.checkRange(100, 101);
                  ok = ok && (testVecB.checkRange(100, 200) == 0);
                  testVecB.clear(100);
                  ok = ok && (testVecB.check(100) == 0);
                  testVecB.set(255);
                  ok = ok && testVecB.check(255);
                  testVecB.setRange(104, 194);
                  ok = ok && (testVecB.check(103) == 0);
                  ok = ok && testVecB.check(104);
                  ok = ok && testVecB.check(194);
                  ok = ok && (testVecB.check(195) == 0);

                  UBYTE mem8[16/8] = {0};
                  Bitvec<UBYTE, CHARBITS> testVecC(mem8, 16/8);
                  testVecC.setRange(7, 10);
                  ok = ok && (testVecC.check(6) == 0);
                  ok = ok && testVecC.check(7);
                  ok = ok && testVecC.check(8);
                  ok = ok && testVecC.check(9);
                  ok = ok && testVecC.check(10);
                  ok = ok && (testVecC.check(11) == 0);
                  ok = ok && (testVecC.checkRange(6, 8) == 0);
                  ok = ok && testVecC.checkRange(7, 10);
                  ok = ok && (testVecC.checkRange(8, 11) == 0);
                  testVecC.clearRange(6, 10);
                  ok = ok && (testVecC.checkRange(6, 10) == 0);
    
                  return ok;
              }()
             );
