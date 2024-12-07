printf("\n");

function label(p, c)
 sp_x = num2str(p(1));
 sp_y = num2str(p(2));
 x    = p(1);
 y    = p(2);
 s    = strcat("(", sp_x, ",", sp_y, ")");
 text(x + 1,y, s);
 h = plot(p(1), p(2), ".*");
 set(h, 'color', c);
endfunction

p0 = [ 5;-10];
p1 = [18; 18];
p2 = [38; -5];
p3 = [45; 15];

hold on;

xlim([0 50])
axis equal

label(p0, 'green');
label(p1, 'green');
label(p2, 'green');
label(p3, 'green');

#  b3 spline
pts = kron((1-t).^3,p0)      ...
    + kron(3*(1-t).^2.*t,p1) ...
    + kron(3*(1-t).*t.^2,p2) ...
    + kron(t.^3,p3);
p = plot(pts(1,:),pts(2,:));
set(p, 'color', 'red');

# tangents
a = -3*t.^2 +  6*t - 3;
b =  9*t.^2 - 12*t + 3;
c = -9*t.^2 +  6*t;
d =  3*t.^2;

tvec = kron(a,p0) + kron(b,p1) + kron(c,p2) + kron(d,p3);
for i=1:10:101
    l = line([pts(1,i), pts(1,i)+tvec(1,i)/6], ...
             [pts(2,i), pts(2,i)+tvec(2,i)/6]);
    set(l,'color', 'green');
end

syms a b c d 
