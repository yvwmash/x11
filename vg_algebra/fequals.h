# ifndef AUX_GEOM_FEQUALS_H
# define AUX_GEOM_FEQUALS_H

#include <cfloat>

/* */
bool eq_eps(double  l, 
            double  r, 
            double  eps = DBL_EPSILON);

/* */
bool eq_eps(float  l, 
            float  r, 
            float  eps = FLT_EPSILON);

/* */
bool eq_eps_ulp(double  l, 
                double  r, 
                int     ulp);

/* */
bool eq_eps_ulp(float  l, 
                float  r, 
                int    ulp);


#endif