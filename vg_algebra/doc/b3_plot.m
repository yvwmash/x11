pkg load matgeom

P0 = [0 0];
P1 = [2 2];
P2 = [2.5 -0.5];
P3 = [0 1];

Vp = [P0;P1;P2;P3];

# translate by -0.5 y
Vp -= [0, 0.5];

# C    = [a;b;c;d]
# P(4) = ax^3 + bx^2 + cx + d

Px = [1,2,4,-16];
# p(x) = x^3 + 2x^2 + 4x - 16
#R = roots(Px);
# disp(R);

# iff discriminant is zero, then cubic has a multiple root.
# More than that, iff its coeficients are real, then all of its roots are real.

# if discriminant > 0, then cubic has three distinct real roots.
# if discriminant < 0, then cubic has one real root and two non-real 
# complex conjugate roots.

# general cubic discriminant
gD = 18 * Px(1) * Px(2)* Px(3)* Px(4) ...
     - 4 * Px(2)^3 * Px(4)            ...
     + Px(2)^2 * Px(3)^2            ...
     - 4 * Px(1) * Px(3)^3            ...
     -27 * Px(1)^2 * Px(4)^2;

# general cubic for ax^3+bx^2+cx+d=0
# d0 = b^2  - 3ac
# d1 = 2b^3 - 9abc + 27a^2d
# C  = cubic_root((d1 ± sqrt(d1^2 - 4d0^3)) / 3)
# where the symbols sqrt and sqrt{3} are interpreted 
# as any square root and any cube root, respectively.
# The sign "±" before the square root is either "+" or "–"; 
# the choice is almost arbitrary, and changing it amounts 
# to choosing a different square root. 
# However, if a choice yields C = 0, then the other sign 
# must be selected instead.

a = Px(1);
b = Px(2);
c = Px(3);
d = Px(4);
d0 = b^2 - 3*a*c;
d1 = 2*b^3 - 9*a*b*c + 27*a^2*d;
C = cbrt( (d1 - sqrt(d1^2 - 4 * d0^3)) / 2 );

# printf("C = %f\n", C);

x1 = (-1/(3*a)) * (b + C + d0/C);

# printf("x1 = %f\n", x1);

Px = [1,-3,-144,432];
# p(x) = x^3 - 3x^2 - 144x + 432
R = roots(Px);

disp(R);

#drawBezierCurve(Vp);