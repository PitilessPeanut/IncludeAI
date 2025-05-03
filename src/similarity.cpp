#include "similarity.hpp"
#include <cmath>


/****************************************/
/*                              helpers */
/****************************************/
    float sqrt_runtime(float x)
    {
        return std::sqrt(x);
    }


/****************************************/
/*                                Tests */
/****************************************/
    static_assert([]
                  {
                      float haystack1[8] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
                      float haystack2[8] = {1.0f, 3.0f, 5.0f, 7.0f, 9.0f, 0.0f, 0.0f, 1.0f};
                      const float needle[8] = {1.0f, 3.0f, 5.0f, 7.0f, 9.0f, 0.0f, 0.0f, 0.0f};
                      const float similarity1 = calculateNormalizedDotProduct<float, 8, true>(haystack1, needle);
                      const float similarity2 = calculateNormalizedDotProduct<float, 8, true>(haystack2, needle);
                      bool ok = similarity1 < .6014f && similarity2 > 0.99f;
                      ok = ok && similarity1 < .83f && similarity2 > .82f;
                      return ok;
                  }()
                 );
