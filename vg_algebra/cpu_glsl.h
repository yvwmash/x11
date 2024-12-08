# ifndef AUX_GEOM_CPU_GLSL_H
# define AUX_GEOM_CPU_GLSL_H

# ifndef ARGB_UI_4F
#define ARGB_UI_4F(a,r,g,b) (((uint8_t)(a * 255.0)) << 24) |\
                            (((uint8_t)(r * 255.0)) << 16) |\
                            (((uint8_t)(g * 255.0)) << 8)  |\
                            (((uint8_t)(b * 255.0)) << 0)
#endif

#include <cstdint>
#include <cmath>

#include "vec2.h"
#include "vec3.h"
#include "vec4.h"

/* */
template <typename I>
inline I smoothstep(I edge0, I edge1, I x);

/* */
template <typename I>
vec2<I> mix(const vec2<I>& a, const vec2<I>& b, I t);

/* */
template <typename I, typename VEC >
vec3<I> mix(const vec3<I>& a, const vec3<I>& b, I t);

/* */
template <typename I, typename VEC>
vec4<I> mix(const vec4<I>& a, const vec4<I>& b, I t);

/* */
template <typename I>
uint32_t ui_argb(I a, I r, I g, I b);
template <typename I>
uint32_t ui_argb(I a, const vec3<I>& v);
template <typename I>
uint32_t ui_argb(const vec4<I>& v);

/* */
template <typename I>
uint32_t ui_rgb(I r, I g, I b);
template <typename I>
uint32_t ui_rgb(const vec3<I>& v);
template <typename I>
uint32_t ui_rgb(const vec4<I>& v);

/* +++ */

/* */
template <>
double smoothstep(double edge0, double edge1, double x)
{
 /* scale, bias and saturate x to 0..1 range */
 x = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0); 
 /* evaluate polynomial */
 return x * x * (3.0 - 2.0 * x);
}

/* */
template <>
float smoothstep(float edge0, float edge1, float x)
{
 /* scale, bias and saturate x to 0..1 range */
 x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f); 
 /* evaluate polynomial */
 return x * x * (3.0f - 2.0f * x);
}


/* */
template <typename I>
vec2<I> mix(const vec2<I>& a, const vec2<I>& b, I t)
{
 return vec2<I>(std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t));
}

/* */
template <typename I>
vec3<I> mix(const vec3<I>& a, const vec3<I>& b, I t)
{
 return vec3<I>(std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t));
}

/* */
template <typename I>
vec4<I> mix(const vec4<I>& a, const vec4<I>& b, I t)
{
 return vec4<I>(std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t), std::lerp(a.w, b.w, t));
}

/* */
template <typename I>
uint32_t ui_argb(I a, I r, I g, I b){
 return (((uint8_t)(a * 255.0))   << 24)
		| (((uint8_t)(r * 255.0)) << 16) 
        | (((uint8_t)(g * 255.0)) << 8)
        | (((uint8_t)(b * 255.0)) << 0);
}

/* */
template <typename I>
uint32_t ui_argb(I a, const vec3<I>& v){
 return (((uint8_t)(a * 255.0))   << 24)
		| (((uint8_t)(v.x * 255.0)) << 16) 
        | (((uint8_t)(v.y * 255.0)) << 8)
        | (((uint8_t)(v.z * 255.0)) << 0);
}

/* */
template <typename I>
uint32_t ui_argb(const vec4<I>& v){
 return (((uint8_t)(v.x * 255.0))   << 24)
		| (((uint8_t)(v.y * 255.0)) << 16) 
        | (((uint8_t)(v.z * 255.0)) << 8)
        | (((uint8_t)(v.w * 255.0)) << 0);
}

/* */
template <typename I>
uint32_t ui_rgb(I r, I g, I b){
 return (((uint8_t)(r * 255.0))   << 16) 
        | (((uint8_t)(g * 255.0)) << 8)
        | (((uint8_t)(b * 255.0)) << 0);
}

/* */
template <typename I>
uint32_t ui_rgb(const vec3<I>& v){
 return (((uint8_t)(v.x * 255.0))   << 16) 
        | (((uint8_t)(v.y * 255.0)) << 8)
        | (((uint8_t)(v.z * 255.0)) << 0);
}

/* */
template <typename I>
uint32_t ui_rgb(const vec4<I>& v){
 return (((uint8_t)(v.x * 255.0))   << 16) 
        | (((uint8_t)(v.y * 255.0)) << 8)
        | (((uint8_t)(v.z * 255.0)) << 0);
}

#endif