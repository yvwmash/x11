# ifndef AUX_GEOM_H
# define AUX_GEOM_H

#include <cmath>
#include <cfloat>

#include <list>
#include <array>

#define POLY_WINDING_CW   1
#define POLY_WINDING_CCW -1

#include "fequals.h"
#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "pt2.h"
#include "pt3.h"
#include "segment.h"
#include "mat4x4.h"
#include "triangle.h"
#include "cpu_glsl.h"

/* **** */

/* */
template <typename I>
int intersect_seg        (const  segment<I>& s1,
                          const  segment<I>& s2,
                          pt2<I>&            pi);

/* */
template <typename I>
int polygon_winding      (pt2<I>  *va, int n);

/* */
template <typename I>
int polygon_is_diagonal  (pt2<I>   *va,
                          int       o,
                          int       n,
                          int       i0,
                          int       i1,
                          pt2<I>   *outp);

/* */
template <typename I>
int polygon_triangles    (pt2<I>                        *va,
                          int                            o,
						  int                            n,
                          std::list<std::array<int,3>>  &tris);

/* +++ */

#define FLOAT_CMP_SMALL_NUM   1e-8

/* =========================================================================================== */
template <typename I>
int intersect_seg(const segment<I>& s1, const segment<I>& s2, pt2<I>& pi) {
 vec2<I>    u = s1.p1 - s1.p0;
 vec2<I>    v = s2.p1 - s2.p0;
 vec2<I>    w = s1.p0 - s2.p0;
 I D = cross(u,v);

 // test if they are parallel
 if(std::abs(D) < FLOAT_CMP_SMALL_NUM){
  return 0;
 }

 I si = cross(v,w) / D;
 if((si < .0) || (si > 1.0)){
  return 0;
 }
 I ti = cross(u,w) / D;
 if((ti < .0) || (ti > 1.0)){
  return 0;
 }

 pi = s1.p0 + si * u;

 return 1;
}

/* =========================================================================================== */
template <typename I>
int polygon_winding(pt2<I> *va, int n) {
 pt2<I>  *s = NULL;
 pt2<I>  *v = NULL;
 pt2<I>  *bv, *ev;
 int      bi, ei, vi;

 /* find lowest rightmost vertex */
 s = &va[0];
 for(int i = 1; i < n; ++i){
  v = &va[i];

  /* comparing for <less_then>|<greater_then> can fail due to the imprecision in arithmetics.
     comparing for equality, here, done somewhat more precise.
  */
  if((v->y < s->y) || (eq_eps(v->y, s->y) && (v->x > s->x))){
   vi = i;
   s  = v;
  }
 }

 /* take the cross product of the edges fore
    and aft the lowest rightmost vertex */
 bi = vi - 1;
 ei = (vi == n - 1)?0:vi+1;
 bv = &va[bi];
 ev = &va[ei];

 vec2<I> a = *bv - *s;
 vec2<I> b = *ev - *s;
 I       d = cross(a,b);
 int    sn = std::signbit(d);

 if(sn){
  return POLY_WINDING_CCW;
 }else {
  return POLY_WINDING_CW;
 }
}

/* =========================================================================================== */
template <typename I>
int polygon_is_diagonal(pt2<I> *va, int o, int n, int i0, int i1, pt2<I> *outp) {
 pt2<I>      dp0 = va[i0];
 pt2<I>      dp1 = va[i1];
 segment<I>  d(dp0, dp1);
 pt2<I>      pi;
 int         status,b,e;

 /* test for intersection of a requested segment with all edges */
 for(int i = 0; i < n; ++i){
  pt2<I>      bp, ep;
  segment<I>  edge;
  b     = i;
  e     = b == n - 1 ? 0 : b + 1;
  bp    = va[b];
  ep    = va[e];
  edge  = {bp, ep};
  status = intersect_seg(d, edge, pi);
  if(status && (pi != dp0) && (pi != dp1)){
   if(outp)
    *outp = pi;
   return 0;
  }
 }

 { /* no intersection. can it be outer diagonal? */
  int mi = i1 - 1; /* middle vertex of a triangle */
  if(mi < 0)
   mi = n - 1;
  pt2<I> t[3] = {va[i0], va[mi], va[i1]}; /* triangle */
  if(polygon_winding(t, 3) != o){
   return 0; /* the diagonal does not belong to polygon area */
  }
 }

 return 1;
}

/* =========================================================================================== */
#define GEOM_C_ADVANCE(F,L,I,STEP) if(STEP < 0){ \
                                    if(I == F) \
                                     I = L; \
                                    else \
                                     --I; \
								   }else { \
									 if(I == L) \
                                      I = F; \
                                     else \
                                      ++I; \
                                   }

template <typename I>
int polygon_triangles(pt2<I> *va, int o, int n, std::list<std::array<int,3>> &tris) {
 int             i = 0;
 std::list<int>  l;

 /* indices from 0 to n-1 */
 for(; i < n; ++i){
  l.push_back(i);
 }

 std::list<int>::iterator  m = std::begin(l);
 decltype(m)              be;
 decltype(m)              af;
 decltype(m)              la;
 decltype(m)              fi;
 while(l.size() > 3){
  fi = std::begin(l);
  la = std::prev(std::end(l));
  be = af = m;
  GEOM_C_ADVANCE(fi, la, be, -1)
  GEOM_C_ADVANCE(fi, la, af, +1)
  if(polygon_is_diagonal(va, o, n, *be, *af, NULL)){
   tris.push_back({*be,*m,*af});
   l.erase(m);
   m = std::begin(l);
  }else {
   GEOM_C_ADVANCE(fi, la, m, +1)
  }
 }
 be = std::begin(l);
 m  = std::next(be);
 af = std::next(m);
 tris.push_back({*be, *m, *af});

 return 0;
}

# endif
