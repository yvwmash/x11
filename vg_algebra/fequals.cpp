#include <cmath>

#include "fequals.h"

/* assumption that the functionality will be used for the numbers scaled to {-1, 1} range.
*/

/*  */
bool eq_eps(double l, double r, double eps){
 return (l == r) || (std::abs(l - r) <= eps);
}

/* */
bool eq_eps(float l, float r, float eps){
 return (l == r) || (std::abs(l - r) <= eps);
}

/* https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
   the web page have an example reference implementation. the implementation differs from below one.
   the implementation assumes a knowledge of how floating point numbers work.
*/
bool eq_eps_ulp(double l, double r, int ulp) {
 return (l == r)
        || (std::abs(l - r) <= (std::abs(fabs(l + r)) * DBL_EPSILON * ulp))
        || (std::abs(l - r) <  DBL_MIN);
}

/* https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
   the web page have an example reference implementation. it assumes a knowledge of how floating point numbers work.
*/
bool eq_eps_ulp(float l, float r, int ulp) {
 return (l == r)
        || (std::abs(l - r) <= (std::abs(fabs(l + r)) * FLT_EPSILON * ulp))
        || (std::abs(l - r) <  FLT_MIN);
}
