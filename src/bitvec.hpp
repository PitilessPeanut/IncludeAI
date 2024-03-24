#ifndef BITVEC_HPP
#define BITVEC_HPP


/****************************************/
/*                           Bit vector */
/****************************************/
    template <class Int, int Charbits=8>
    class Bitvec
    {
    public:
        using type = Int;
        
    private:
        Int *bitvec;
        static constexpr Int w = sizeof(Int)*Charbits;
        static constexpr Int shift = []{ Int p=0, b=sizeof(Int)*Charbits; while (b>>=1) p++; return p; }();
        const int size;
        
    public:
        explicit constexpr Bitvec(Int *mem, const int nBits)
          : bitvec(mem)
          , size(nBits)
        {}
        
        constexpr bool operator==(const Bitvec<Int, Charbits>& rhs) const
        {
            bool same = true;
            const auto nInts = size / (sizeof(Int) * Charbits);
            for (int i=0; i<nInts; ++i)
                same = same && (bitvec[i] == rhs[i]);
            if (!same)
                return false;
            
            const auto remainingBits = size - (nInts * sizeof(Int) * Charbits);
            Int mask = ~0;
            mask <<= remainingBits;
            mask = ~mask;
            return (bitvec[nInts]&mask) == (rhs[nInts]&mask);
        }
        
        constexpr void set(const int pos)
        {
            Int bit = 1;
            bit <<= pos&(w-1);
            bitvec[pos>>shift] |= bit;
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
            
            bitvec[startPos] |= (bitsStart * !overlap);
            
            const int middlePos = endPos - 1;
            constexpr Int allSet = ~0;
            for (startPos+=1; startPos <= middlePos; ++startPos)
                bitvec[startPos] = allSet;

            bitvec[endPos] |= (bitsEnd * !overlap) + (bitsOverlap * overlap);
        }
        
        constexpr void clear(const int pos)
        {
            Int bit = 1;
            bit <<= pos&(w-1);
            bitvec[pos>>shift] &= ~bit;
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
            
            bitvec[startPos] &= ~(bitsStart * !overlap);
            
            const int middlePos = endPos - 1;
            for (startPos+=1; startPos <= middlePos; ++startPos)
                bitvec[startPos] = 0;

            bitvec[endPos] &= ~((bitsEnd * !overlap) + (bitsOverlap * overlap));
        }
        
        constexpr Int check(const int pos) const
        {
            const Int bit = bitvec[pos>>shift];
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
            
            bool identical = (bitvec[startPos]&maskA) == maskA;
            
            const int middlePos = endPos - 1;
            constexpr Int allSet = ~0;
            for (startPos+=1; startPos <= middlePos; ++startPos)
            {
                identical = identical && ((bitvec[startPos] & allSet) == allSet);
                const int isNotIdentical = !identical;
                startPos += middlePos & -isNotIdentical;
            }

            identical = identical && ((bitvec[endPos] & maskB) == maskB);

            identical = identical || (overlap && ((bitvec[endPos] & maskOverlap) == maskOverlap));

            return (Int)identical;
        }
        
        constexpr Int operator[](const int pos) const
        {
            return check(pos);
        }
    };


#else
  #error "double include"
#endif // BITVEC_HPP
