#ifndef BIT_ALLOCATOR_HPP
#define BIT_ALLOCATOR_HPP

#include "asmtypes.hpp"

#include <concepts>
#include <type_traits>

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


#else // BIT_ALLOCATOR_HPP
  #error "double include"
#endif // BIT_ALLOCATOR_HPP
