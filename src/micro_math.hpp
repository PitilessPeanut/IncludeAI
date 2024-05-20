#ifndef MICRO_MATH_HPP
#define MICRO_MATH_HPP

#include "asmtypes.hpp"


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

    
#else
  #error double include
#endif // MICRO_MATH_HPP
