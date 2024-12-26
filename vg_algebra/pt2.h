# ifndef AUX_GEOM_PT2_H
# define AUX_GEOM_PT2_H

#include <cmath>

#include "frwd.h"
#include "fequals.h"

template <typename I>
struct pt2 {
 I x, y;

 /* constructor */
 pt2()                                    {}
 pt2(const vec2<I>& v)   : x(v.x), y(v.y) {}
 pt2(I x, I y)           : x(x),   y(y)   {}

 /* conversion */
 explicit operator vec2<I>() const;

 /* translate */
 pt2<I>& operator += (const vec2<I>& v);
 pt2<I>& operator -= (const vec2<I>& v);

 /* arithmetic */
 pt2<I>& operator *= (const vec2<I>& v);
 pt2<I>& operator /= (const vec2<I>& v);

 /* accessor */
 I* data() {return (I*)&x;}
};

/* unary */
template <typename I>
pt2<I> operator + (const pt2<I>& p);
template <typename I>
pt2<I> operator - (const pt2<I>& p);

/* equality */
template <typename I>
bool operator == (const pt2<I>& l, const pt2<I>& r);
template <typename I>
bool operator != (const pt2<I>& l, const pt2<I>& r);

/* translate */
template <typename I>
pt2<I>   operator + (const pt2<I>& p, const vec2<I>& v);
template <typename I>
pt2<I>   operator - (const pt2<I>& p, const vec2<I>& v);

/* scalar */
template <typename I>
pt2<I> operator * (I s, const pt2<I>&  v);
template <typename I>
pt2<I> operator * (const pt2<I>& v,  I s);
template <typename I>
pt2<I> operator / (I s, const pt2<I>&  v);
template <typename I>
pt2<I> operator / (const pt2<I>& v, I  s);

/* distance between two points */
template <typename I>
I distance(const pt2<I>& l, const pt2<I>& r);

/* +++ */

/* explicit conversion */
template <typename I>
pt2<I>::operator vec2<I>() const {
 return *((const vec2<I>*)this);
}

/* translate */
template <typename I>
pt2<I>& pt2<I>::operator += (const vec2<I>& v){
 x += v.x;
 y += v.y;
 return *this;
}
template <typename I>
pt2<I>& pt2<I>::operator -= (const vec2<I>& v){
 x -= v.x;
 y -= v.y;
 return *this;
}

/* scalar */
template <typename I>
pt2<I> operator * (I s, const pt2<I>& v){
 return pt2<I>(s * v.x, s * v.y);
}
template <typename I>
pt2<I> operator * (const pt2<I>& v, I s){
 return pt2<I>(s * v.x, s * v.y);
}
template <typename I>
pt2<I> operator / (I s,      const pt2<I>& v) {
 return pt2<I>(s / v.x, s / v.y);
}
template <typename I>
pt2<I> operator / (const pt2<I>& v, I s) {
 return pt2<I>(v.x / s, v.y / s);
}

 /* arithmetic */
template <typename I>
pt2<I>& pt2<I>::operator *= (const vec2<I>& v){
 x *= v.x;
 y *= v.y;
 return *this;
}
template <typename I>
pt2<I>& pt2<I>::operator /= (const vec2<I>& v){
 x /= v.x;
 y /= v.y;
 return *this;
}

/* unary */
template <typename I>
pt2<I> operator + (const pt2<I>& p){
 return pt2<I>(p.x, p.y);
}
template <typename I>
pt2<I> operator - (const pt2<I>& p){
 return pt2<I>(-p.x, -p.y);
}

/* equality */
template <>
bool operator == (const pt2<float>& l, const pt2<float>& r){
 return eq_eps(l.x, r.x) && eq_eps(l.y, r.y);
}
template <>
bool operator == (const pt2<double>& l, const pt2<double>& r){
 return eq_eps(l.x, r.x) && eq_eps(l.y, r.y);
}
template <>
bool operator != (const pt2<float>& l, const pt2<float>& r){
  return !eq_eps(l.x, r.x) || !eq_eps(l.y, r.y);
}
template <>
bool operator != (const pt2<double>& l, const pt2<double>& r){
  return !eq_eps(l.x, r.x) || !eq_eps(l.y, r.y);
}

/* translate */
template <typename I>
pt2<I> operator + (const pt2<I>& p, const vec2<I>& v){
 return pt2<I>(p.x + v.x, p.y + v.y);
}
template <typename I>
pt2<I> operator - (const pt2<I>& p, const vec2<I>& v){
 return pt2<I>(p.x - v.x, p.y - v.y);
}

/* distance between two points */
template <typename I>
I distance(const pt2<I>& l, const pt2<I>& r){
 return sqrt(pow(r.x - l.x, 2.0) + pow(r.y - l.y, 2.0));
}

/* *** */

typedef pt2<float>  pt2f;
typedef pt2<double> pt2d;

#endif
