# ifndef AUX_GEOM_PT3_H
# define AUX_GEOM_PT3_H

#include <cmath>

#include "frwd.h"
#include "fequals.h"
#include "pt2.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"

template <typename I>
struct pt3 {
 I x, y, z;

 /* constructor */
 pt3()                                           {}
 pt3(const vec3<I>& v)  : x(v.x), y(v.y), z(v.z) {}
 pt3(const pt2<I>&  v)  : x(v.x), y(v.y), z(0)   {}
 pt3(const pt3<I>&  v)  : x(v.x), y(v.y), z(v.z) {}
 pt3(I _x, I _y, I _z)  : x(_x),  y(_y),  z(_z)  {}

 /* translate */
 pt3<I>& operator += (const vec3<I>& v);
 pt3<I>& operator -= (const vec3<I>& v);

 /* accessor */
 I* data() {return (I*)&x;}
};

/* unary */
template <typename I>
pt3<I> operator + (const pt3<I>& p);
template <typename I>
pt3<I> operator - (const pt3<I>& p);

/* equality */
template <typename I>
bool operator == (const pt3<I>& l, const pt3<I>& r);
template <typename I>
bool operator != (const pt3<I>& l, const pt3<I>& r);

/* translate */
template <typename I>
pt3<I>  operator + (const pt3<I>& p, const vec3<I>& v);
template <typename I>
pt3<I>  operator - (const pt3<I>& p, const vec3<I>& v);

/* scalar */
template <typename I>
pt3<I> operator * (I s, const pt3<I>&  v);
template <typename I>
pt3<I> operator * (const pt3<I>& v,  I s);
template <typename I>
pt3<I> operator / (I s, const pt3<I>&  v);
template <typename I>
pt3<I> operator / (const pt3<I>& v, I  s);

/* distance between two points */
template <typename I>
I distance(const pt3<I>& l, const pt3<I>& r);

/* +++ */

/* translate */
template <typename I>
pt3<I>& pt3<I>::operator += (const vec3<I> &v){
 x += v.x;
 y += v.y;
 z += v.z;
 return *this;
}
template <typename I>
pt3<I>& pt3<I>::operator -= (const vec3<I> &v){
 x -= v.x;
 y -= v.y;
 z -= v.z;
 return *this;
}

/* unary */
template <typename I>
pt3<I>operator + (const pt3<I>& p){
 return pt3(p.x, p.y, p.z);
}
template <typename I>
pt3<I>operator - (const pt3<I>& p){
 return pt3(-p.x, -p.y, -p.z);
}

/* equality */
template <>
bool operator == (const pt3<float>& l, const pt3<float>& r){
 return eq_eps(l.x, r.x) && eq_eps(l.y, r.y) && eq_eps(l.z, r.z);
}
template <>
bool operator == (const pt3<double>& l, const pt3<double>& r){
 return eq_eps(l.x, r.x) && eq_eps(l.y, r.y) && eq_eps(l.z, r.z);
}
template <>
bool operator != (const pt3<float>& l, const pt3<float>& r){
  return !eq_eps(l.x, r.x) || !eq_eps(l.y, r.y) || !eq_eps(l.z, r.z);
}
template <>
bool operator != (const pt3<double>& l, const pt3<double>& r){
  return !eq_eps(l.x, r.x) || !eq_eps(l.y, r.y) || !eq_eps(l.z, r.z);
}

/* translate */
template <typename I>
pt3<I>operator + (const pt3<I>& p, const vec3<I>& v){
 return pt3(p.x + v.x, p.y + v.y, p.z + v.z);
}
template <typename I>
pt3<I>operator - (const pt3<I>& p, const vec3<I>& v){
 return pt3(p.x - v.x, p.y - v.y, p.z - v.z);
}

/* scalar */
template <typename I>
pt3<I> operator * (I s, const pt3<I>& v){
 return pt3<I>(s * v.x, s * v.y, s * v.z);
}
template <typename I>
pt3<I> operator * (const pt3<I>& v, I s){
 return pt3<I>(s * v.x, s * v.y, s * v.z);
}
template <typename I>
pt3<I> operator / (I s,      const pt3<I>& v) {
 return pt3<I>(s / v.x, s / v.y, s / v.z);
}
template <typename I>
pt3<I> operator / (const pt3<I>& v, I s) {
 return pt3<I>(v.x / s, v.y / s, v.z / s);
}

/* distance between two points */
template <typename I>
I distance(const pt3<I>& l, const pt3<I>& r){
 return sqrt(pow(r.x - l.x, 2.0) + pow(r.y - l.y, 2.0) + pow(r.z - l.z, 2.0));
}

typedef pt3<float>  pt3f;
typedef pt3<double> pt3d;

#pragma clang diagnostic pop

#endif
