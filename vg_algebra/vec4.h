# ifndef AUX_GEOM_VEC4_H
# define AUX_GEOM_VEC4_H

#include <cmath>
#include <algorithm>

#include "frwd.h"
#include "fequals.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"

template <typename I>
struct vec4 {
 I x,y,z,w;

 /* constructor */
 vec4(I _x, I _y, I _z, I _w) : x(_x),   y(_y),   z(_z),   w(_w)   {}
 vec4(I _x, I _y, I _z)       : x(_x),   y(_y),   z(_z),   w(1)    {}
 vec4(const vec3<I>& r)   : x(r.x), y(r.y), z(r.z), w(1)   {}
 vec4(const vec2<I>& r)   : x(r.x), y(r.y), z(0),   w(1)   {}
 vec4()                   : x(0),   y(0),   z(0),   w(1)   {}

 /* copy constructor */
 vec4(const vec4<double>& r) : x(r.x), y(r.y), z(r.z), w(r.w) {}

 /* convienience constructors */
 vec4(I _x,       const vec3<I>& v)          : x(_x),   y(v.x), z(v.y), w(v.z) {}
 vec4(I _x, I _y, const vec2<I>& v)          : x(_x),   y(_y),  z(v.x), w(v.y) {}
 vec4(const vec3<I>& v, I _w)                : x(v.x), y(v.y), z(v.z),  w(_w)  {}
 vec4(const vec2<I>& v, I _z, I _w)          : x(v.x), y(v.y), z(_z),   w(_w)  {}
 vec4(const vec2<I>& l, const vec2<I>& r)    : x(l.x), y(l.y), z(r.x),  w(r.y) {}

 /* explicit copy assignment operator */
 vec4& operator = (const vec4& r) = default;

 /* scalar */
 const vec4<I>& operator *= (I s);

 /* scalar */
 const vec4<I>& operator /= (I s);

 /* scalar */
 const vec4<I>& operator += (I s);

 /* scalar */
 const vec4<I>& operator -= (I s);

 /* vector */
 const vec4<I>& operator *= (const vec4<I>& r);

 /* vector */
 const vec4<I>& operator /= (const vec4<I>& r);

 /* vector */
 const vec4<I>& operator += (const vec4<I>& r);

 /* vector */
 const vec4<I>& operator -= (const vec4<I>& r);

 /* vector-matrix multiplication. NOTE: vector is a row vector, result is a row vector */
 const vec4<I>& operator *= (const mat4x4<I>& r);

 /* accessor */
 I&          operator [](int r);
 const I&    operator [](int r) const;
 I*          data()            {return (I*)&x;}
};

 /* accessors */
template <typename I>
I&  vec4<I>::operator       [] (int r) {
 return *(&x + r);
}

template <typename I>
const I&  vec4<I>::operator [] (int r) const {
 return *(&x + r);
}

/* unary */
template <typename I>
vec4<I> operator + (const vec4<I>& p);
template <typename I>
vec4<I> operator - (const vec4<I>& p);

/* equality */
template <typename I>
bool operator == (const vec4<I>& l, const vec4<I>& r);
template <typename I>
bool operator != (const vec4<I>& l, const vec4<I>& r);

/* arithmetic */
template <typename I>
vec4<I> operator + (const vec4<I>& x, const vec4<I>& y);
template <typename I>
vec4<I> operator - (const vec4<I>& x, const vec4<I>& y);
template <typename I>
vec4<I> operator * (const vec4<I>& x, const vec4<I>& y);
template <typename I>
vec4<I> operator / (const vec4<I>& x, const vec4<I>& y);

/* scalar */
template <typename I>
vec4<I> operator * (I s,      const vec4<I>& v);
template <typename I>
vec4<I> operator * (const vec4<I>& v, I s);
template <typename I>
vec4<I> operator / (I s,      const vec4<I>& v);
template <typename I>
vec4<I> operator / (const vec4<I>& v, I s);

/* dot product */
template <typename I>
I dot(const vec4<I>& l, const vec4<I>& r);

/* length of a vector */
template <typename I>
I length(const vec4<I>& v);

/* unit vector */
template <typename I>
vec4<I> normalize(const vec4<I>& v);

/* clamp to range [min,max] */
template <typename I>
vec4<I> clamp(const vec4<I>& x, I              lo, I              hi);
template <typename I>
vec4<I> clamp(const vec4<I>& x, const vec4<I>& lo, const vec4<I>& hi);

/* +++ */

/* unary */
template <typename I>
vec4<I> operator + (const vec4<I>& p){
 return vec4<I>(p.x, p.y, p.z);
}
template <typename I>
vec4<I> operator - (const vec4<I>& p){
 return vec4<I>(-p.x, -p.y, -p.z);
}

/* scalar */
template <typename I>
const vec4<I>& vec4<I>::operator *= (I s){
 x *= s;
 y *= s;
 z *= s;
 w *= s;
 return *this;
}

/* scalar */
template <typename I>
const vec4<I>& vec4<I>::operator /= (I s){
 x /= s;
 y /= s;
 z /= s;
 w /= s;
 return *this;
}

/* scalar */
template <typename I>
const vec4<I>& vec4<I>::operator += (I s){
 x += s;
 y += s;
 z += s;
 w += s;
 return *this;
}

/* scalar */
template <typename I>
const vec4<I>& vec4<I>::operator -= (I s){
 x -= s;
 y -= s;
 z -= s;
 w -= s;
 return *this;
}

/* vector */
template <typename I>
const vec4<I>& vec4<I>::operator *= (const vec4<I>& r){
 x *= r.x;
 y *= r.y;
 z *= r.z;
 w *= r.w;
 return *this;
}

/* vector */
template <typename I>
const vec4<I>& vec4<I>::operator /= (const vec4<I>& r){
 x /= r.x;
 y /= r.y;
 z /= r.z;
 w /= r.w;
 return *this;
}

/* vector */
template <typename I>
const vec4<I>& vec4<I>::operator += (const vec4<I>& r){
 x += r.x;
 y += r.y;
 z += r.z;
 w += r.w;
 return *this;
}

/* vector */
template <typename I>
const vec4<I>& vec4<I>::operator -= (const vec4<I>& r){
 x -= r.x;
 y -= r.y;
 z -= r.z;
 w -= r.w;
 return *this;
}


/* equality */
template <>
bool operator == (const vec4<float>& l, const vec4<float>& r){
 return eq_eps(l.x, r.x) && eq_eps(l.y, r.y) && eq_eps(l.z, r.z) && eq_eps(l.w, r.w);
}

template <>
bool operator == (const vec4<double>& l, const vec4<double>& r){
 return eq_eps(l.x, r.x) && eq_eps(l.y, r.y) && eq_eps(l.z, r.z) && eq_eps(l.w, r.w);
}

template <>
bool operator != (const vec4<float>& l, const vec4<float>& r){
 return !eq_eps(l.x, r.x) || !eq_eps(l.y, r.y) || !eq_eps(l.z, r.z) || !eq_eps(l.w, r.w);
}

template <>
bool operator != (const vec4<double>& l, const vec4<double>& r){
 return !eq_eps(l.x, r.x) || !eq_eps(l.y, r.y) || !eq_eps(l.z, r.z) || !eq_eps(l.w, r.w);
}

/* arithmetic */
template <typename I>
vec4<I> operator + (const vec4<I>& x, const vec4<I>& y){
 return vec4<I>(x.x + y.x, x.y + y.y, x.z + y.z, x.w + y.w);
}
template <typename I>
vec4<I> operator - (const vec4<I>& x, const vec4<I>& y){
 return vec4<I>(x.x - y.x, x.y - y.y, x.z - y.z, x.w - y.w);
}
template <typename I>
vec4<I> operator * (const vec4<I>& x, const vec4<I>& y){
 return vec4<I>(x.x * y.x, x.y * y.y, x.z * y.z, x.w * y.w);
}
template <typename I>
vec4<I> operator / (const vec4<I>& x, const vec4<I>& y){
 return vec4<I>(x.x / y.x, x.y / y.y, x.z / y.z, x.w / y.w);
}

/* scalar */
template <typename I>
vec4<I> operator * (I s, const vec4<I>& v) {
  return vec4<I>(s * v.x, s * v.y, s * v.z, s * v.w);
}
template <typename I>
vec4<I> operator * (const vec4<I>& v, I s) {
  return vec4<I>(s * v.x, s * v.y, s * v.z, s * v.w);
}
template <typename I>
vec4<I> operator / (I s,      const vec4<I>& v) {
 return vec4<I>(s / v.x, s / v.y, s / v.z, s / v.w);
}
template <typename I>
vec4<I> operator / (const vec4<I>& v, I s) {
 return vec4<I>(v.x / s, v.y / s, v.z / s, v.w / s);
}


/* dot product */
template <typename I>
I dot(const vec4<I>& l, const vec4<I>& r) {
 return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w;
}

/* length of a vector */
template <typename I>
I length(const vec4<I>& v){
 return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

/* unit vector */
template <typename I>
vec4<I> normalize(const vec4<I>& v){
 I inv_mag = 1.0 / std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
 return vec4<I>(v.x * inv_mag, v.y * inv_mag, v.z * inv_mag, v.w * inv_mag);
}

/* clamp to range [min,max] */
template <typename I>
vec4<I> clamp(const vec4<I>& x, I      lo, I      hi){
 return vec4<I>(std::clamp(x.x, lo, hi), std::clamp(x.y, lo, hi), std::clamp(x.z, lo, hi), std::clamp(x.w, lo, hi));
}
template <typename I>
vec4<I> clamp(const vec4<I>& x, const vec4<I>& lo, const vec4<I>& hi){
 return vec4<I>(std::clamp(x.x, lo.x, hi.x), std::clamp(x.y, lo.y, hi.y), std::clamp(x.z, lo.z, hi.z), std::clamp(x.w, lo.w, hi.w));
}

/* vector-matrix multiplication. NOTE: vector is a row vector, result is a row vector */
/* A x B =
                     ⎡a₀₀  a₀₁  a₀₂  a₀₃⎤
                     ⎢                  ⎥
                     ⎢a₁₀  a₁₁  a₁₂  a₁₃⎥
  [v₀  v₁  v₂  v₃] x ⎢                  ⎥ =   [a₀₀⋅v₀ + a₁₀⋅v₁ + a₂₀⋅v₂ + a₃₀⋅v₃,
					 ⎢a₂₀  a₂₁  a₂₂  a₂₃⎥      a₀₁⋅v₀ + a₁₁⋅v₁ + a₂₁⋅v₂ + a₃₁⋅v₃,
                     ⎢                  ⎥      a₀₂⋅v₀ + a₁₂⋅v₁ + a₂₂⋅v₂ + a₃₂⋅v₃,
                     ⎣a₃₀  a₃₁  a₃₂  a₃₃⎦      a₀₃⋅v₀ + a₁₃⋅v₁ + a₂₃⋅v₂ + a₃₃⋅v₃]
*/

template <typename I>
const vec4<I>& vec4<I>::operator *= (const mat4x4<I>& r){
 auto      &m = r.m;
 auto       v = *this;

 *this  = m[0] * v.x;
 *this += m[1] * v.y;
 *this += m[2] * v.z;
 *this += m[3] * v.w;

 return *this;
}

typedef vec4<float>    vec4f;
typedef vec4<double>   vec4d;
typedef vec4<unsigned> uvec4;
typedef vec4<int>      ivec4;

#pragma clang diagnostic pop

# endif
