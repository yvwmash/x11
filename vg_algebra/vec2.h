# ifndef AUX_GEOM_VEC2_H
# define AUX_GEOM_VEC2_H

#include <cmath>
#include <algorithm>

#include "frwd.h"
#include "fequals.h"

template <typename T, typename U>
concept same_type = std::same_as<T, U>;

template <typename I>
struct vec2 {
 I x, y;

 /* constructor */
 vec2() {};
 vec2(I x, I y) : x(x), y(y) {};

 /* explicit copy assignment operator */
 vec2& operator = (const vec2& r) = default;

 /* conversion */
 explicit operator pt2<I>() const;

 /* constructor cast */
 template <typename T>
 explicit vec2(const vec2<T>& v) : x(v.x), y(v.y) {}

  /* arithmetic */
 const vec2<I>& operator += (const vec2<I>& r);
 const vec2<I>& operator -= (const vec2<I>& r);
 const vec2<I>& operator *= (const vec2<I>& r);
 const vec2<I>& operator /= (const vec2<I>& r);
 const vec2<I>& operator %= (const vec2<I>& r);

 template <typename T>
 friend const vec2<I>& operator += (vec2<I>& l, const vec2<T>& r);

 /* accessors */
 I&          operator [](int r);
 const I&    operator [](int r) const;
 I*          data()             {return (I*)&x;};
};

 /* accessors */
template <typename I>
I&  vec2<I>::operator       [] (int r) {
 return *(&x + r);
}

template <typename I>
const I&  vec2<I>::operator [] (int r) const {
 return *(&x + r);
}


/* unary */
template <typename I>
vec2<I> operator + (const vec2<I>& p);
template <typename I>
vec2<I> operator - (const vec2<I>& p);

/* equality */
template <typename I>
bool operator == (const vec2<I>& l, const vec2<I>& r);
template <typename I>
bool operator != (const vec2<I>& l, const vec2<I>& r);

/* arithmetic */
template <typename I>
vec2<I> operator *  (const vec2<I>& x, const vec2<I>& y);
template <typename I>
vec2<I> operator /  (const vec2<I>& x, const vec2<I>& y);
template <typename I>
vec2<I> operator +  (const vec2<I>& x, const vec2<I>& y);
template <typename I>
vec2<I> operator -  (const vec2<I>& x, const vec2<I>& y);

/* vector from points */
template <typename I>
vec2<I> operator - (const pt2<I>& l, const pt2<I>& r);

/* scalar */
template <typename I>
vec2<I> operator * (I s, const vec2<I>&  v);
template <typename I>
vec2<I> operator * (const vec2<I>& v,  I s);
template <typename I>
vec2<I> operator / (I s,  const vec2<I>& v);
template <typename I>
vec2<I> operator / (const vec2<I>& v, I s);

/* cross product */
template <typename I>
I cross(const vec2<I>& l, const vec2<I>& r);

/* dot product */
template <typename I>
I dot(const vec2<I>& l, const vec2<I>& r);
template <typename I>
I dot(const vec2<I>& l, const pt2<I>&  r);


/* length of a vector */
template <typename I>
I length(const vec2<I>& v);

/* unit vector */
template <typename I>
vec2<I> normalize(const vec2<I>& v);

/* clamp to range [min,max] */
template <typename I>
vec2<I> clamp(const vec2<I>& x, I              lo, I              hi);
template <typename I>
vec2<I> clamp(const vec2<I>& x, const vec2<I>& lo, const vec2<I>& hi);

/* +++ */

/* unary */
template <typename I>
vec2<I> operator + (const vec2<I>& p){
 return vec2<I>(p.x, p.y);
}
template <typename I>
vec2<I> operator - (const vec2<I>& p){
 return vec2<I>(-p.x, -p.y);
}

/* equality */
template <>
bool operator == (const vec2<float>& l, const vec2<float>& r){
 return eq_eps(l.x, r.x) && eq_eps(l.y, r.y);
}
template <>
bool operator == (const vec2<double>& l, const vec2<double>& r){
 return eq_eps(l.x, r.x) && eq_eps(l.y, r.y);
}
template <>
bool operator != (const vec2<float>& l, const vec2<float>& r){
 return !eq_eps(l.x, r.x) || !eq_eps(l.y, r.y);
}
template <>
bool operator != (const vec2<double>& l, const vec2<double>& r){
 return !eq_eps(l.x, r.x) || !eq_eps(l.y, r.y);
}

/* explicit conversion */
template <typename I>
vec2<I>::operator pt2<I>() const {
 return *((const pt2<I>*)this);
}

/* arithmetic */
template <typename I>
const vec2<I>& vec2<I>::operator += (const vec2<I>& r){
 x += r.x;
 y += r.y;
 return *this;
}
template <typename I>
const vec2<I>& vec2<I>::operator -= (const vec2<I>& r){
 x -= r.x;
 y -= r.y;
 return *this;
}
template <typename I>
const vec2<I>& vec2<I>::operator *= (const vec2<I>& r){
 x *= r.x;
 y *= r.y;
 return *this;
}
template <typename I>
const vec2<I>& vec2<I>::operator /= (const vec2<I>& r){
 x /= r.x;
 y /= r.y;
 return *this;
}
template <typename I>
const vec2<I>& vec2<I>::operator %= (const vec2<I>& r){
 x %= r.x;
 y %= r.y;
 return *this;
}

/* arithmetic */
template <typename I>
vec2<I> operator + (const vec2<I>& x, const vec2<I>& y){
 return vec2<I>(x.x + y.x, x.y + y.y);
}
template <typename I>
vec2<I> operator - (const vec2<I>& x, const vec2<I>& y){
 return vec2<I>(x.x - y.x, x.y - y.y);
}
template <typename I>
vec2<I> operator *  (const vec2<I>& x, const vec2<I>& y){
 return vec2<I>(x.x * y.x, x.y * y.y);
}
template <typename I>
vec2<I> operator /  (const vec2<I>& x, const vec2<I>& y){
 return vec2<I>(x.x / y.x, x.y / y.y);
}
template <typename I>
vec2<I> operator %  (const vec2<I>& x, const vec2<I>& y){
 return vec2<I>(x.x % y.x, x.y % y.y);
}
template <typename I, typename U>
vec2<I> operator + (const vec2<I>& l, const vec2<U>& r) {
 return vec2<I>(l.x + r.x, l.y + r.y);
}
template <typename I, typename U>
vec2<I> operator - (const vec2<I>& l, const vec2<U>& r) {
 return vec2<I>(l.x - r.x, l.y - r.y);
}
template <typename I, typename U>
vec2<I> operator * (const vec2<I>& l, const vec2<U>& r) {
 return vec2<I>(l.x * r.x, l.y * r.y);
}
template <typename I, typename U>
vec2<I> operator / (const vec2<I>& l, const vec2<U>& r) {
 return vec2<I>(l.x / r.x, l.y / r.y);
}

template <typename I, typename U>
const vec2<I>& operator %= (vec2<I>& l, const vec2<U>& r) {
 l.x %= r.x;
 l.y %= r.y;
}

/* vector from points */
template <typename I>
vec2<I> operator - (const pt2<I>& l, const pt2<I>& r){
 return vec2<I>(l.x - r.x, l.y - r.y);
}

/* scalar */
template <typename I>
vec2<I> operator * (I s, const vec2<I>& v){
 return vec2<I>(s * v.x, s * v.y);
}
template <typename I>
vec2<I> operator * (const vec2<I>& v, I s){
 return vec2<I>(s * v.x, s * v.y);
}
template <typename I>
vec2<I> operator / (I s,      const vec2<I>& v) {
 return vec2<I>(s / v.x, s / v.y);
}
template <typename I>
vec2<I> operator / (const vec2<I>& v, I s) {
 return vec2<I>(v.x / s, v.y / s);
}
/* different types */
template <typename I, typename U>
vec2<I> operator + (const vec2<I>& l, U s) {
 return vec2<I>(l.x + s, l.y + s);
}
template <typename I, typename U>
vec2<I> operator - (const vec2<I>& l, U s) {
 return vec2<I>(l.x - s, l.y - s);
}
template <typename I, typename U>
vec2<I> operator * (const vec2<I>& l, U s) {
 return vec2<I>(s * l.x, s * l.y);
}
template <typename I, typename U>
vec2<I> operator * (U s, const vec2<I>& r) {
 return vec2<I>(s * r.x, s * r.y);
}
template <typename I, typename U>
vec2<I> operator / (const vec2<I>& l, U s) {
 return vec2<I>(l.x / s, l.y / s);
}

/* cross product */
template <typename I>
I cross(const vec2<I>& l, const vec2<I>& r){
 return (l.x * r.y - l.y * r.x);
}

/* dot product */
template <typename I>
I dot(const vec2<I>& l, const vec2<I>& r){
 return l.x * r.x + l.y * r.y;
}

/* dot product */
template <typename I>
I dot(const vec2<I>& l, const pt2<I>& r){
 return l.x * r.x + l.y * r.y;
}

/* length of a vector */
template <typename I>
I length(const vec2<I>& v){
 return std::sqrt(v.x * v.x + v.y * v.y);
}

/* unit vector */
template <typename I>
vec2<I> normalize(const vec2<I>& v){
 I inv_mag = 1.0 / std::sqrt(v.x * v.x + v.y * v.y);
 return vec2<I>(v.x * inv_mag, v.y * inv_mag);
}

/* clamp to range [min,max] */
template <typename I>
vec2<I> clamp(const vec2<I>& x, I lo, I hi){
 return vec2<I>(std::clamp(x.x, lo, hi), std::clamp(x.y, lo, hi));
}
template <typename I>
vec2<I> clamp(const vec2<I>& x, const vec2<I>& lo, const vec2<I>& hi){
 return vec2<I>(std::clamp(x.x, lo.x, hi.x), std::clamp(x.y, lo.y, hi.y));
}

/* *** */

typedef vec2<float>    vec2f;
typedef vec2<double>   vec2d;
typedef vec2<int>      ivec2;
typedef vec2<unsigned> uvec2;

#endif
