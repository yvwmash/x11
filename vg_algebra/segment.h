# ifndef AUX_GEOM_SEG_H
# define AUX_GEOM_SEG_H

template <typename I>
struct segment {
 pt2<I> p0, p1;

 /* construcor */
 segment()                                                    {};
 segment(const pt2<I>& p0, const pt2<I>& p1) : p0(p0), p1(p1) {};

 /* accessor */
 I* data() {return (I*)&p0;};
};

#endif
