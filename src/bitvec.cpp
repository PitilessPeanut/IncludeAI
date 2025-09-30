#include "bitvec.hpp"

/****************************************/
/*                                Tests */
/****************************************/
    static_assert([]
                  {
                      Bitvec<UDWORD, 128, CHARBITS> testVecA;
                      testVecA.set(32);
                      testVecA.set(63);
                      testVecA.set(127);

                      bool ok = testVecA.check(32);
                      ok = ok && testVecA.check(63);
                      ok = ok && testVecA.check(127);
                      ok = ok && (testVecA.check(1) == 0);
                      ok = ok && (testVecA.template popcnt<true>() == 3);

                      Bitvec<UQWORD, 256, CHARBITS> testVecB;
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

                      Bitvec<UBYTE, 16, CHARBITS> testVecC;
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

    static_assert([]
                  {
                      Bitvec<UQWORD, 128, CHARBITS> a;
                      a.set(64);
                      Bitvec<UQWORD, 128, CHARBITS> b;
                      b = a;
                      return b.check(64);
                  }()
                 );
