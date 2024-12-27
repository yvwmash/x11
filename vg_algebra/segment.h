# ifndef AUX_GEOM_SEG_H
# define AUX_GEOM_SEG_H

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"

template <typename I>
struct segment {
 pt2<I> p0, p1;

 /* construcor */
 segment()                                                        {}
 segment(const pt2<I>& _p0, const pt2<I>& _p1) : p0(_p0), p1(_p1) {}

 /* accessor */
 I* data() {return (I*)&p0;}
};

#pragma clang diagnostic pop

#endif
