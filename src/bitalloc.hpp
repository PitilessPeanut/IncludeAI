#ifndef BIT_ALLOCATOR_HPP
#define BIT_ALLOCATOR_HPP

#include "asmtypes.hpp"


/****************************************/
/*                                Tools */
/****************************************/
// ctz
    template <typename Int>
    consteval int ctz_comptime(Int val)
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
    constexpr T nodeAllocatorMin(T x, T y) { return x < y ? x : y; }
    template <typename T>
    constexpr T nodeAllocatorMax(T x, T y) { return x > y ? x : y; }


/****************************************/
/*                               Config */
/****************************************/
    enum class Mode { FAST, TIGHT };


/****************************************/
/*                                 Impl */
/****************************************/
    template <int Size, class Inttype, Mode mode=Mode::FAST, bool comptime=false>
    struct BitAlloc
    {
    private:
        static constexpr int Intbits = sizeof(Inttype)*CHARBITS;
        static constexpr int NumberOfBuckets = (Size/Intbits) + 1;
    public:
        Inttype bucketPool[NumberOfBuckets] = {0};
    public:
        constexpr auto largestAvailChunk(const int desiredSize)
        {
            struct Pos
            {
                int posOfAvailChunk;
                int len;
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

            int discoveredSize = nodeAllocatorMin(desiredSize, Intbits*NumberOfBuckets);
            while (discoveredSize>0)
            {
                constexpr Inttype everyBitSet = ~0;
                if (discoveredSize < Intbits)
                {
                    for (int bucketNo=0; bucketNo<NumberOfBuckets; ++bucketNo)
                    {
                        if (ctz(bucketPool[bucketNo]) >= discoveredSize)
                        {
                            const int availTail = ctz(bucketPool[bucketNo]);
                            Inttype mask = everyBitSet << discoveredSize; // the 'discoveredSize' here is always <= than Intbits
                            mask = rotl(mask, availTail-discoveredSize);
                            bucketPool[bucketNo] = bucketPool[bucketNo] | ~mask;

                            return Pos{ .posOfAvailChunk = static_cast<int>((bucketNo*Intbits) + (Intbits-availTail))
                                      , .len = static_cast<int>(discoveredSize)
                                      };
                        }
                    }
                }

                if constexpr (mode == Mode::FAST)
                {
                    const int bucketsRequired = discoveredSize/Intbits;
                    const bool noRemainder = discoveredSize-(Intbits*bucketsRequired) == 0;
                    for (int bucketNo=0; bucketNo<(NumberOfBuckets-bucketsRequired); ++bucketNo)
                    {
                        Inttype *iter = &bucketPool[bucketNo + bucketsRequired - noRemainder];
                        Inttype *iter2 = iter;
                        bool avail = true;
                        while (iter != &bucketPool[bucketNo])
                        {
                            iter--;
                            avail = avail && *iter == 0;
                        }

                        if (avail)
                        {
                            while (iter2 != &bucketPool[bucketNo])
                            {
                                iter2--;
                                *iter2 = everyBitSet;
                            }
                            constexpr Inttype everyBitSet = ~0;
                            bucketPool[bucketNo+bucketsRequired-noRemainder] = everyBitSet << (Intbits - (discoveredSize%Intbits));
                            return Pos{ .posOfAvailChunk = static_cast<int>(bucketNo*Intbits)
                                      , .len = static_cast<int>(discoveredSize)
                                      };
                        }
                    }
                }
                else if constexpr (mode == Mode::TIGHT)
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
                discoveredSize -= 1;
            } // while (discoveredSize>0)

            return Pos{ .posOfAvailChunk = 0, .len = 0 };
        }

        constexpr void free(const int pos, const int len)
        {
            // two bugs: pos 0, pos 64 len 128!
            constexpr Inttype everyBitSet = ~0;
            int startBucket = pos/Intbits;
            int endBucket = (pos+len)/Intbits;
            const Inttype maskHead = everyBitSet << (Intbits - (pos%Intbits));
            Inttype maskTail = everyBitSet << (Intbits - ((pos+len)%Intbits));

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


#else // BIT_ALLOCATOR_HPP
  #error "double include"
#endif // BIT_ALLOCATOR_HPP
