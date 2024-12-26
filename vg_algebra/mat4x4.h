# ifndef AUX_GEOM_MAT4X4_H
# define AUX_GEOM_MAT4X4_H

#include <cmath>
#include <string.h>

#include <cmath>

#include "vec2.h"
#include "vec3.h"
#include "vec4.h"

template <typename I>
struct mat4x4 {
 vec4<I> m[4];
 /* x.x y.x z.x 0 */
 /* x.y y.y z.y 0 */
 /* x.z y.z z.z 0 */
 /* x.w y.w z.w 1 */

 /* { x.x y.x z.x 0 x.y y.y z.y 0 x.z y.z z.z 0 x.w y.w z.w 1 } */

 /* constructor */
 mat4x4();
 mat4x4(const vec4<I>& c0, const vec4<I>& c1, const vec4<I>& c2, const vec4<I>& c3) : m{c0,c1,c2,c3} {}

 template <typename T>
 explicit mat4x4(const mat4x4<T>& r) : mat4x4(r[0], r[1], r[2], r[3]) {}

 const mat4x4<I>&  operator *=     (const mat4x4<I> &r);
 vec4<I>&          operator []     (int c);       /* column */
 const vec4<I>&    operator []     (int c) const; /* column */
 I*                data() const { return (I*)&m[0].x; }
};

template <typename I>
mat4x4<I>        transpose   (const mat4x4<I> &m);
template <typename I>
vec4<I>          operator  * (const mat4x4<I> &m, const vec4<I>   &r);
template <typename I>
mat4x4<I>        operator  * (const mat4x4<I> &l, const mat4x4<I> &r);

template <typename I>
mat4x4<I>        scale       (const mat4x4<I> &m, const vec4<I> &v);
template <typename I>
mat4x4<I>        translate   (const mat4x4<I> &m, const vec4<I> &v);
template <typename I>
mat4x4<I>        translate   (const mat4x4<I> &m, const vec3<I> &v);
template <typename I>
mat4x4<I>        translate   (const mat4x4<I> &m, const vec2<I> &v);
template <typename I>
mat4x4<I>        rotate_x    (const mat4x4<I> &m, I a);
template <typename I>
mat4x4<I>        rotate_y    (const mat4x4<I> &m, I a);
template <typename I>
mat4x4<I>        rotate_z    (const mat4x4<I> &m, I a);
template <typename I>
mat4x4<I>        rotate_axis (const mat4x4<I> &m, const vec4<I> &v, I a);

/* +++ */

template <typename I>
mat4x4<I>::mat4x4() {
 memset(m, 0, sizeof(m));
 m[0].x = m[1].y = m[2].z = m[3].w = 1.0; /* unit matrix */
}

template <typename I>
vec4<I>&  mat4x4<I>::operator       [] (int c) {
 return m[c];
}

template <typename I>
const vec4<I>&  mat4x4<I>::operator [] (int c) const {
 return m[c];
}

/* A[:,0] x b00 + A[:,1] x b10 + A[:,2] x b20 */
/*              ⎡a₀₀  a₀₁  a₀₂⎤ ⎡b₀₀⎤   ⎡a₀₀⋅b₀₀ + a₀₁⋅b₁₀ + a₀₂⋅b₂₀⎤ */
/*              ⎢             ⎥ ⎢   ⎥   ⎢                           ⎥ */
/* A x B      = ⎢a₁₀  a₁₁  a₁₂⎥ ⎢b₁₀⎥ = ⎢a₁₀⋅b₀₀ + a₁₁⋅b₁₀ + a₁₂⋅b₂₀⎥ */
/*              ⎢             ⎥ ⎢   ⎥   ⎢                           ⎥ */
/*              ⎣a₂₀  a₂₁  a₂₂⎦ ⎣b₂₀⎦   ⎣a₂₀⋅b₀₀ + a₂₁⋅b₁₀ + a₂₂⋅b₂₀⎦ */
template <typename I>
vec4<I> operator * (const mat4x4<I> &m, const vec4<I> &r) {
 vec4<I>   res(0,0,0,0);
 mat4x4<I> lhs = m;

 lhs[0] *= r.x; /* A[:,0] x b00 */
 lhs[1] *= r.y; /* A[:,1] x b10 */
 lhs[2] *= r.z; /* A[:,2] x b20 */
 lhs[3] *= r.w; /* A[:,3] x b30 */

 res += lhs[0]; /* R += A[:,0] x b00 */
 res += lhs[1]; /* R += A[:,1] x b10 */
 res += lhs[2]; /* R += A[:,2] x b20 */
 res += lhs[3]; /* R += A[:,3] x b30 */

 return res;
}

/* A x B = ⎡ A x B[:,0] ⎤ ⎡A x B[:,1]⎤ ⎡A x B[:,2]⎤ = */
/*         ⎣            ⎦ ⎣          ⎦ ⎣          ⎦   */
/*                                                    */
/*   ⎡a₀₀⋅b₀₀ + a₀₁⋅b₁₀ + a₀₂⋅b₂₀⎤ ⎡a₀₀⋅b₀₁ + a₀₁⋅b₁₁ + a₀₂⋅b₂₁⎤ ⎡a₀₀⋅b₀₂ + a₀₁⋅b₁₂ + a₀₂⋅b₂₂⎤ */
/*   ⎢                           ⎥ ⎢                           ⎥ ⎢                           ⎥ */
/*   ⎢a₁₀⋅b₀₀ + a₁₁⋅b₁₀ + a₁₂⋅b₂₀⎥ ⎢a₁₀⋅b₀₁ + a₁₁⋅b₁₁ + a₁₂⋅b₂₁⎥ ⎢a₁₀⋅b₀₂ + a₁₁⋅b₁₂ + a₁₂⋅b₂₂⎥ */
/*   ⎢                           ⎥ ⎢                           ⎥ ⎢                           ⎥ */
/*   ⎣a₂₀⋅b₀₀ + a₂₁⋅b₁₀ + a₂₂⋅b₂₀⎦ ⎣a₂₀⋅b₀₁ + a₂₁⋅b₁₁ + a₂₂⋅b₂₁⎦ ⎣a₂₀⋅b₀₂ + a₂₁⋅b₁₂ + a₂₂⋅b₂₂⎦ */
template <typename I>
const mat4x4<I>&  mat4x4<I>::operator *= (const mat4x4<I> &r) {
 mat4x4<I> lhs = *this;

 m[0] = lhs * r[0]; /* A x B[:,0] */
 m[1] = lhs * r[1]; /* A x B[:,1] */
 m[2] = lhs * r[2]; /* A x B[:,2] */
 m[3] = lhs * r[3]; /* A x B[:,3] */

 return *this;
}

template <typename I>
mat4x4<I> scale(const mat4x4<I> &m, const vec4<I> &v) {
 mat4x4<I> s;

 s.m[0].x = v.x;
 s.m[1].y = v.y;
 s.m[2].z = v.z;
 s.m[3].w = 1.0;

 return m * s;
}

template <typename I>
mat4x4<I>  translate(const mat4x4<I> &m, const vec4<I> &v) {
 mat4x4<I> t;

 t.m[3].x = v.x;
 t.m[3].y = v.y;
 t.m[3].z = v.z;
 t.m[3].w = 1.0;

 return m * t;
}

template <typename I>
mat4x4<I>  translate(const mat4x4<I> &m, const vec3<I> &v) {
 mat4x4<I> t;

 t.m[3].x = v.x;
 t.m[3].y = v.y;
 t.m[3].z = v.z;
 t.m[3].w = 1.0;

 return m * t;
}

template <typename I>
mat4x4<I>  translate(const mat4x4<I> &m, const vec2<I> &v) {
 mat4x4<I> t;

 t.m[3].x = v.x;
 t.m[3].y = v.y;
 t.m[3].w = 1.0;

 return m * t;
}

template <typename I>
mat4x4<I> rotate_x    (const mat4x4<I> &m, I a) {
 mat4x4<I> rx;
 I c = cos(a);
 I s = sin(a);

 rx[1].y =  c;
 rx[1].z =  s;
 rx[2].y = -s;
 rx[2].z =  c;

 return m * rx;
}

template <typename I>
mat4x4<I> rotate_y    (const mat4x4<I> &m, I a) {
 mat4x4<I> ry;
 I c = cos(a);
 I s = sin(a);

 ry[0].x  =  c;
 ry[0].z  = -s;
 ry[2].x  =  s;
 ry[2].z  =  c;

 return m * ry;
}

template <typename I>
mat4x4<I> rotate_z    (const mat4x4<I> &m, I a) {
 mat4x4<I> rz;
 I c = cos(a);
 I s = sin(a);

 rz[0].x  =  c;
 rz[0].y  =  s;
 rz[1].x  = -s;
 rz[1].y  =  c;

 return m * rz;
}

template <typename I>
mat4x4<I> rotate_axis (const mat4x4<I> &m, const vec4<I> &v, I a) {
 mat4x4<I> r;
 I c           = cos(a);
 I s           = sin(a);
 I one_minus_c = 1.0 - c;
 I x           = v.x;
 I y           = v.y;
 I z           = v.z;

 r[0].x  =  c + x * x * one_minus_c;
 r[0].y  =  y * x * one_minus_c + z * s;
 r[0].z  =  z * x * one_minus_c - y * s;
 r[1].x  =  x * y * one_minus_c - z * s;
 r[1].y  =  c + y * y * one_minus_c;
 r[1].z  =  z * y * one_minus_c - x * s;
 r[2].x  =  x * z * one_minus_c - y * s;
 r[2].y  =  y * z * one_minus_c + x * s;
 r[2].z  =  c + z * z * one_minus_c;

 return m * r;
}

template <typename I>
mat4x4<I>  transpose (const mat4x4<I> &m) {
 mat4x4<I>  res = m;

 /* C,R */
 /* 0x 1x 2x 3x */          /* 0x 0y 0z 0w */
 /* 0y 1y 2y 3y */ /* => */ /* 1x 1y 1z 1w */
 /* 0z 1z 2z 3z */          /* 2x 2y 2z 2w */
 /* 0w 1w 2w 3w */          /* 3x 3y 3z 3w */

 /* write rows of [m] as columns of [res] */
 std::swap(res[0].y, res[1].x);
 std::swap(res[0].z, res[2].x);
 std::swap(res[0].w, res[3].x);
 std::swap(res[1].z, res[2].y);
 std::swap(res[1].w, res[3].y);
 std::swap(res[2].w, res[3].z);

 return res;
}

template <typename I>
mat4x4<I>  operator  *  (const mat4x4<I> &l, const mat4x4<I> &r) {
 mat4x4<I> res = l;
 return res *= r;
}

template <typename I>
mat4x4<I> ortho(I left, I right, I bottom, I top, I zNear, I zFar)
{
 mat4x4<I> res;
 res[0][0] =   static_cast<I>(2) / (right - left);
 res[1][1] =   static_cast<I>(2) / (top - bottom);
 res[2][2] = - static_cast<I>(2) / (zFar - zNear);
 res[3][0] = - (right + left) / (right - left);
 res[3][1] = - (top + bottom) / (top - bottom);
 res[3][2] = - (zFar + zNear) / (zFar - zNear);
 return res;
}

template<typename I>
mat4x4<I> look_at(vec3<I> const& eye, vec3<I> const& center, vec3<I> const& up)
{
 vec3<I> const f(normalize(center - eye));
 vec3<I> const s(normalize(cross(f, up)));
 vec3<I> const u(cross(s, f));

 mat4x4<I> res;
 res[0][0] = s.x;
 res[1][0] = s.y;
 res[2][0] = s.z;
 res[0][1] = u.x;
 res[1][1] = u.y;
 res[2][1] = u.z;
 res[0][2] =-f.x;
 res[1][2] =-f.y;
 res[2][2] =-f.z;
 res[3][0] =-dot(s, eye);
 res[3][1] =-dot(u, eye);
 res[3][2] = dot(f, eye);
 return res;
}

typedef mat4x4<float>  mat4x4f;
typedef mat4x4<double> mat4x4d;

#endif
