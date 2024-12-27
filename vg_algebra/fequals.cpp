#include <cmath>
#include <bit>

#include <stdint.h>

#include "fequals.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"

/* assumption that the functionality will be used for the numbers scaled to {-1, 1} range.
*/

/*  */
bool eq_eps(double l, double r, double eps){
 return (std::bit_cast<uint64_t>(l) == std::bit_cast<uint64_t>(r)) || (std::abs(l - r) <= eps);
}

/* */
bool eq_eps(float l, float r, float eps){
 return (std::bit_cast<uint32_t>(l) == std::bit_cast<uint32_t>(r)) || (std::abs(l - r) <= eps);
}

/* https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
   the web page have an example reference implementation. the implementation differs from below one.
   the implementation assumes a knowledge of how floating point numbers work.
*/
bool eq_eps_ulp(double l, double r, int ulp) {
 return (std::bit_cast<uint64_t>(l) == std::bit_cast<uint64_t>(r))
        || (std::abs(l - r) <= (std::abs(fabs(l + r)) * DBL_EPSILON * (double)ulp))
        || (std::abs(l - r) <  DBL_MIN);
}

/* https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
   the web page have an example reference implementation. it assumes a knowledge of how floating point numbers work.
*/
bool eq_eps_ulp(float l, float r, int ulp) {
 return (std::bit_cast<uint32_t>(l) == std::bit_cast<uint32_t>(r))
        || (std::abs(l - r) <= (std::abs(fabs(l + r)) * FLT_EPSILON * (float)ulp))
        || (std::abs(l - r) <  FLT_MIN);
}

#pragma clang diagnostic pop
