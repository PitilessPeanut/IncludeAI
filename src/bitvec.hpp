#ifndef BITVEC_HPP
#define BITVEC_HPP

#include "asmtypes.hpp"


/****************************************/
/*                           Bit vector */
/****************************************/
    template <class Int, int Size, int Charbits=CHARBITS>
    class Bitvec
    {
    private:
        static constexpr Int w = sizeof(Int)*Charbits;
        static constexpr Int shift = []{ Int p=0, b=sizeof(Int)*Charbits; while (b>>=1) p++; return p; }();
        static constexpr int N_INTS = (Size + w - 1) / w;
        Int mem[N_INTS] = {};
    public:
        constexpr Bitvec() = default;

        constexpr Bitvec(const Bitvec& other) = default;

        constexpr Bitvec& operator=(const Bitvec& rhs)
        {
            if (this == &rhs)
                return *this;
            for (int i=0; i<N_INTS; ++i)
                mem[i] = rhs.mem[i];
            return *this;
        }

        constexpr bool operator==(const Bitvec& rhs) const
        {
            int same = 0;
            for (int i=0; i<(N_INTS-1); ++i)
                same += mem[i] == rhs.mem[i];
            if (same != (N_INTS-1))
                return false;

            constexpr int remainingBits = Size % w;
            if constexpr (remainingBits == 0)
            {
                return mem[N_INTS - 1] == rhs.mem[N_INTS - 1];
            }
            else
            {
                constexpr Int mask = (Int(1) << remainingBits) - 1;
                return (mem[N_INTS-1] & mask) == (rhs.mem[N_INTS-1] & mask);
            }
        }

        constexpr void set(const int pos)
        {
            const Int bit = Int(1) << (pos&(w-1));
            mem[pos>>shift] |= bit;
        }

        constexpr void set(const int pos, const bool val)
        {
            const Int mask = Int(1) << (pos&(w-1));
            if (val)
                mem[pos>>shift] |= mask;
            else
                mem[pos>>shift] &= ~mask;
        }

        constexpr void setRange(const int startInclusive, const int endInclusive)
        {
            int startPos = startInclusive>>shift;
            const int endPos = endInclusive>>shift;

            Int bitsStart = ~0;
            bitsStart <<= startInclusive&(w-1);
            Int bitsEnd = ~0;
            bitsEnd <<= (endInclusive+1)&(w-1);
            bitsEnd = ~bitsEnd;

            const Int bitsOverlap = bitsStart & bitsEnd;
            const bool overlap = (startPos==endPos) && bitsOverlap;

            mem[startPos] |= (bitsStart * !overlap);

            const int middlePos = endPos - 1;
            constexpr Int allSet = ~0;
            for (startPos+=1; startPos <= middlePos; ++startPos)
                mem[startPos] = allSet;

            mem[endPos] |= (bitsEnd * !overlap) + (bitsOverlap * overlap);
        }

        constexpr void clear(const int pos)
        {
            const Int bit = Int(1) << (pos&(w-1));
            mem[pos>>shift] &= ~bit;
        }

        constexpr void clearRange(const int startInclusive, const int endInclusive)
        {
            int startPos = startInclusive>>shift;
            const int endPos = endInclusive>>shift;

            Int bitsStart = ~0;
            bitsStart <<= startInclusive&(w-1);
            Int bitsEnd = ~0;
            bitsEnd <<= (endInclusive+1)&(w-1);
            bitsEnd = ~bitsEnd;

            const Int bitsOverlap = bitsStart & bitsEnd;
            const bool overlap = (startPos==endPos) && bitsOverlap;

            mem[startPos] &= ~(bitsStart * !overlap);

            const int middlePos = endPos - 1;
            for (startPos+=1; startPos <= middlePos; ++startPos)
                mem[startPos] = 0;

            mem[endPos] &= ~((bitsEnd * !overlap) + (bitsOverlap * overlap));
        }

        constexpr void clearAll()
        {
            for (int i=0; i<N_INTS; ++i)
                mem[i] = 0;
        }

        constexpr Int check(const int pos) const
        {
            const Int bit = mem[pos>>shift];
            return (bit >> (pos&(w-1))) & 1;
        }

        constexpr Int checkRange(const int startInclusive, const int endInclusive) const
        {
            Int maskA = ~0, maskB = ~0;
            maskA <<= startInclusive&(w-1);
            maskB <<= (endInclusive+1)&(w-1);
            maskB = ~maskB;

            int startPos = startInclusive>>shift;
            const int endPos = endInclusive>>shift;
            const Int maskOverlap = maskA & maskB;
            const bool overlap = (startPos==endPos) && maskOverlap;

            bool identical = (mem[startPos]&maskA) == maskA;

            const int middlePos = endPos - 1;
            constexpr Int allSet = ~0;
            for (startPos+=1; startPos <= middlePos; ++startPos)
            {
                identical = identical && ((mem[startPos] & allSet) == allSet);
                const int isNotIdentical = !identical;
                startPos += middlePos & -isNotIdentical;
            }

            identical = identical && ((mem[endPos] & maskB) == maskB);

            identical = identical || (overlap && ((mem[endPos] & maskOverlap) == maskOverlap));

            return (Int)identical;
        }

        constexpr Int operator[](const int pos) const
        {
            return check(pos);
        }

        template <bool Comptime=false>
        constexpr Int popcnt() const
        {
            Int cnt = 0;
            for (int i=0; i<(N_INTS-1); ++i)
            {
                Int val = mem[i];
                if constexpr (Comptime)
                {
                    while (val)
                    {
                        cnt += val & 1;
                        val >>= 1;
                    }
                }
                else
                {
                    if constexpr (sizeof(Int) == sizeof(UQWORD))
                        cnt += __builtin_popcountll( val );
                    else
                        cnt += __builtin_popcount( val );
                }
            }

            // Remaining bits:
            constexpr int remainingBits = Size % w;
            Int last_val = mem[N_INTS-1];
            if constexpr (remainingBits != 0)
            {
                constexpr Int mask = (Int(1) << remainingBits) - 1;
                last_val &= mask;
            }

            if constexpr (Comptime)
            {
                while (last_val)
                {
                    cnt += last_val & 1;
                    last_val >>= 1;
                }
            }
            else
            {
                if constexpr (sizeof(Int) == sizeof(UQWORD))
                    cnt += __builtin_popcountll( last_val );
                else
                    cnt += __builtin_popcount( last_val );
            }
            return cnt;
        }
    };


#else
  #error "double include"
#endif // BITVEC_HPP
