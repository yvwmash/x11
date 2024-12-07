#include <cmath>

#include <yma/yma_fequals.h>

/* */
bool eq_eps(double l, double r, double eps){
 return (l == r) || (std::abs(l - r) <= eps);
}

/* */
bool eq_eps(float l, float r, float eps){
 return (l == r) || (std::abs(l - r) <= eps);
}

/* */
bool eq_eps_ulp(double l, double r, int ulp){
 return (l == r) 
        || (std::abs(l - r) <= (std::abs(fabs(l + r)) * DBL_EPSILON * ulp))
        || (std::abs(l - r) <  DBL_MIN);
}

/* */
bool eq_eps_ulp(float l, float r, int ulp){
 return (l == r) 
        || (std::abs(l - r) <= (std::abs(fabs(l + r)) * FLT_EPSILON * ulp))
        || (std::abs(l - r) <  FLT_MIN);
}
