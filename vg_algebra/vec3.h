# ifndef AUX_GEOM_VEC3_H
# define AUX_GEOM_VEC3_H

#include <cmath>
#include <algorithm>

#include "frwd.h"
#include "fequals.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"

template <typename I>
struct vec3 {
 I x, y, z;

 /* constructor */
 vec3() {}
 vec3(I _x, I _y, I _z) : x(_x), y(_y), z(_z) {}

 /* explicit copy assignment operator */
 vec3& operator = (const vec3& r) = default;

 /* convienience constructors */
 vec3(const vec2<I> &v, I _z) : x(v.x),  y(v.y), z(_z)  {}
 vec3(I _x, const vec2<I> &v) : x(_x),   y(v.x), z(v.y) {}

 /* arithmetic */
 const vec3<I>& operator += (const vec3<I>& r);
 const vec3<I>& operator -= (const vec3<I>& r);
 const vec3<I>& operator *= (const vec3<I>& r);
 const vec3<I>& operator /= (const vec3<I>& r);

 /* accessor */
 I* data() {return (I*)&x;}
};

/* unary */
template <typename I>
vec2<I> operator + (const vec2<I>& p);
template <typename I>
vec2<I> operator - (const vec2<I>& p);

/* equality */
template <typename I>
bool operator == (const vec3<I>& l, const vec3<I>& r);
template <typename I>
bool operator != (const vec3<I>& l, const vec3<I>& r);

/* arithmetic */
template <typename I>
vec3<I> operator +  (const vec3<I>& x, const vec3<I>& y);
template <typename I>
vec3<I> operator -  (const vec3<I>& x, const vec3<I>& y);
template <typename I>
vec3<I> operator *  (const vec3<I>& x, const vec3<I>& y);
template <typename I>
vec3<I> operator /  (const vec3<I>& x, const vec3<I>& y);

/* vector from points */
template <typename I>
vec3<I> operator - (const pt3<I>& l, const pt3<I>& r);

/* scalar */
template <typename I>
vec3<I> operator * (I s, const vec3<I>&  v);
template <typename I>
vec3<I> operator * (const vec3<I>& v,  I s);
template <typename I>
vec3<I> operator / (I s,  const vec3<I>& v);
template <typename I>
vec3<I> operator / (const vec3<I>& v,  I s);

/* cross product */
template <typename I>
vec3<I> cross(const vec3<I>& l, const vec3<I>& r);

/* dot product */
template <typename I>
I dot(const vec3<I>& l, const vec3<I>& r);

/* length of a vector */
template <typename I>
I length(const vec3<I>& v);

/* unit vector */
template <typename I>
vec3<I> normalize(const vec3<I>& v);

/* clamp to range [min,max] */
template <typename I>
vec3<I> clamp(const vec3<I>& x, I              lo, I              hi);
template <typename I>
vec3<I> clamp(const vec3<I>& x, const vec3<I>& lo, const vec3<I>& hi);

/* +++ */

/* unary */
template <typename I>
vec3<I> operator + (const vec3<I>& p){
 return vec3<I>(p.x, p.y, p.z);
}
template <typename I>
vec3<I> operator - (const vec3<I>& p){
 return vec3<I>(-p.x, -p.y, -p.z);
}


/* equality */
template <>
bool operator == (const vec3<float>& l, const vec3<float>& r){
 return eq_eps(l.x, r.x) && eq_eps(l.y, r.y) && eq_eps(l.z, r.z);
}
template <>
bool operator == (const vec3<double>& l, const vec3<double>& r){
 return eq_eps(l.x, r.x) && eq_eps(l.y, r.y) && eq_eps(l.z, r.z);
}
template <>
bool operator != (const vec3<float>& l, const vec3<float>& r){
 return !eq_eps(l.x, r.x) || !eq_eps(l.y, r.y) || !eq_eps(l.z, r.z);
}
template <>
bool operator != (const vec3<double>& l, const vec3<double>& r){
 return !eq_eps(l.x, r.x) || !eq_eps(l.y, r.y) || !eq_eps(l.z, r.z);
}


/* arithmetic */
template <typename I>
const vec3<I>& vec3<I>::operator += (const vec3<I>& r){
 x += r.x;
 y += r.y;
 z += r.z;
 return *this;
}
template <typename I>
const vec3<I>& vec3<I>::operator -= (const vec3<I>& r){
 x -= r.x;
 y -= r.y;
 z -= r.z;
 return *this;
}
template <typename I>
const vec3<I>& vec3<I>::operator *= (const vec3<I>& r){
 x *= r.x;
 y *= r.y;
 z *= r.z;
 return *this;
}
template <typename I>
const vec3<I>& vec3<I>::operator /= (const vec3<I>& r){
 x /= r.x;
 y /= r.y;
 z /= r.z;
 return *this;
}

/* arithmetic */
template <typename I>
vec3<I> operator + (const vec3<I>& x, const vec3<I>& y){
 return vec3(x.x + y.x, x.y + y.y, x.z + y.z);
}
template <typename I>
vec3<I> operator - (const vec3<I>& x, const vec3<I>& y){
 return vec3(x.x - y.x, x.y - y.y, x.z - y.z);
}
template <typename I>
vec3<I> operator * (const vec3<I>& x, const vec3<I>& y){
 return vec3(x.x * y.x, x.y * y.y, x.z * y.z);
}
template <typename I>
vec3<I> operator / (const vec3<I>& x, const vec3<I>& y){
 return vec3(x.x / y.x, x.y / y.y, x.z / y.z);
}

/* vector from points */
template <typename I>
vec3<I> operator - (const pt3<I>& l, const pt3<I>& r){
 return vec3(l.x - r.x, l.y - r.y, l.z - r.z);
}

/* scalar */
template <typename I>
vec3<I> operator * (I s, const vec3<I>& v){
 return vec3(s * v.x, s * v.y, s * v.z);
}
template <typename I>
vec3<I> operator * (const vec3<I>& v, I s){
 return vec3(s * v.x, s * v.y, s * v.z);
}
template <typename I>
vec3<I> operator / (I s,      const vec3<I>& v) {
 return vec3(s / v.x, s / v.y, s / v.z);
}
template <typename I>
vec3<I> operator / (const vec3<I>& v, I s) {
 return vec3(v.x / s, v.y / s, v.z / s);
}

/* cross product */
template <typename I>
vec3<I> cross(const vec3<I>& l, const vec3<I>& r){
 return vec3(l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x);
}

/* dot product */
template <typename I>
I dot(const vec3<I>& l, const vec3<I>& r){
 return l.x * r.x + l.y * r.y + l.z * r.z;
}

/* length of a vector */
template <typename I>
I length(const vec3<I>& v){
 return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

/* unit vector */
template <typename I>
vec3<I> normalize(const vec3<I>& v){
 I inv_mag = 1.0 / std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
 return vec3<I>(v.x * inv_mag, v.y * inv_mag, v.z * inv_mag);
}

/* clamp to range [min,max] */
template <typename I>
vec3<I> clamp(const vec3<I>& x, I lo, I hi){
 return vec3(std::clamp(x.x, lo, hi), std::clamp(x.y, lo, hi), std::clamp(x.z, lo, hi));
}
template <typename I>
vec3<I> clamp(const vec3<I>& x, const vec3<I>& lo, const vec3<I>& hi){
 return vec3(std::clamp(x.x, lo.x, hi.x), std::clamp(x.y, lo.y, hi.y), std::clamp(x.z, lo.z, hi.z));
}

typedef vec3<float>  vec3f;
typedef vec3<double> vec3d;

#pragma clang diagnostic pop

#endif
