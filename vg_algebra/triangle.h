# ifndef AUX_GEOM_VEC2_H
# define AUX_GEOM_VEC2_H

#include <assert.h>

#include <cmath>
#include <vector>
#include <algorithm>

#include "frwd.h"
#include "fequals.h"
#include "pt2.h"
#include "pt3.h"


template <typename I>
struct triangle {
 pt3<I> x,y,z;

 /* constructor */
 triangle()                                                                      {};
 triangle(const pt2<I>& a, const pt2<I>& b, const pt2<I>& c) : x(a), y(b), z(c)  {};
 triangle(const pt3<I>& a, const pt3<I>& b, const pt3<I>& c) : x(a), y(b), z(c)  {};
 triangle(const std::vector<I>::const_iterator b, const std::vector<I>::const_iterator e);

 /* translate */
 triangle<I>& operator += (const vec3<I>& v);
 triangle<I>& operator -= (const vec3<I>& v);

 /* accessor */
 I* data() {return (I*)&x;};
};

/* equality */
template <typename I>
bool operator == (const triangle<I>& l, const triangle<I>& r);
template <typename I>
bool operator != (const triangle<I>& l, const triangle<I>& r);

/* +++ */

 /* constructor */
template <typename I>
triangle::triangle(const std::vector<I>::const_iterator b, const std::vector<I>::const_iterator e)
{
 assert((e - b) == 9);
 x(*(b+0), *(b+1), *(b+2));
 y(*(b+3), *(b+4), *(b+5));
 z(*(b+6), *(b+7), *(b+8));
}

/* equality */
template <typename I>
bool operator == (const triangle<I>& l, const triangle<I>& r)
{
 return (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
}
template <typename I>
bool operator != (const triangle<I>& l, const triangle<I>& r)
{
 return (l.x != r.x) && (l.y != r.y) && (l.z != r.z);
}

 /* translate */
template <typename I>
triangle<I>& triangle::triangle<I>& operator += (const vec3<I>& v)
{
 x += v.x;
 y += v.y;
 z += v.z;
 return *this;
}
template <typename I>
triangle<I>& triangle::triangle<I>& operator -= (const vec3<I>& v)
{
 x -= v.x;
 y -= v.y;
 z -= v.z;
 return *this;
}

# endif
