float t0 = 0.0;
float t1 = 0.0;
float t2 = cos(ia);
const float t3 = cos(oa);
const float t4 = sin(oa);
float t5 = hp1*ld1;
float t6 = hp2*ld2;
float t7 = hp3*ld3;
float t8 = hp1*n1;
float t9 = hp2*n2;
const float t10 = hp3*n3;
const float t11 = ld1*lp1;
const float t12 = ld2*lp2;
const float t13 = ld3*lp3;
const float t14 = lp1*n1;
const float t15 = lp2*n2;
const float t16 = lp3*n3;
const float t17 = hp1*2.0;
const float t18 = hp2*2.0;
const float t19 = hp3*2.0;
const float t20 = lp1*2.0;
const float t21 = lp2*2.0;
const float t22 = lp3*2.0;
const float t23 = -lp1;
const float t25 = -lp2;
const float t27 = -lp3;
const float t24 = -t20;
const float t26 = -t21;
const float t28 = -t22;
const float t29 = -t3;
const float t30 = -t5;
const float t31 = -t6;
const float t32 = -t7;
const float t33 = -t14;
const float t34 = -t15;
const float t35 = -t16;
const float t36 = hp1+t23;
const float t37 = hp2+t25;
const float t38 = hp3+t27;
const float t39 = t2+t29;
const float t40 = ld1*t36;
const float t41 = ld2*t37;
const float t42 = ld3*t38;
const float t43 = t17+t24;
const float t44 = t18+t26;
const float t45 = t19+t28;
const float t46 = t36*t36;
const float t47 = t37*t37;
const float t48 = t38*t38;
const float t56 = t8+t9+t10+t33+t34+t35;
const float t49 = 1.0/t39;
const float t54 = t46+t47+t48;
const float t55 = t40+t41+t42;
const float t50 = t49*t49;
const float t51 = t49*t49*t49;
const float t52 = t2*t49;
const float t53 = t3*t49;
const float t57 = t54+tinyEps;
const float t60 = sqrt(t54);
const float t58 = 1.0/t57;
const float t61 = 1.0/t60;
const float t64 = t3*t60;
const float t59 = t58*t58;
const float t62 = t61*t61*t61;
const float t63 = t61*t61*t61*t61*t61;
const float t65 = t11+t12+t13+t30+t31+t32+t64;
const float t67 = t49*t55*t61;
const float t66 = t65*t65;
const bool t68 = (t52 < t67);
const bool t69 = (t67 < t53);
const bool t70 = (t68 || t69);
if (t67 < t53) {
    t0 = 0.0;
} else if (t52 < t67) {
    t0 = (n1*t61)/t57-t43*t61*1.0/pow(t57,2.0)*t56-(t43*t62*t56)/(t57*2.0);
} else {
    t0 = (n1*t50*t62*t66)/t57-t43*t50*t62*t56*1.0/pow(t57,2.0)*t66-(t43*t50*t63*t56*t66*(3.0/2.0))/t57-(t50*t62*t56*(ld1-(t3*t43*t61)/2.0)*t65*2.0)/t57;
}
if (t67 < t53) {
    t1 = 0.0;
} else if (t52 < t67) {
    t1 = (n2*t61)/t57-t44*t61*1.0/pow(t57,2.0)*t56-(t44*t62*t56)/(t57*2.0);
} else {
    t1 = (n2*t50*t62*t66)/t57-t44*t50*t62*t56*1.0/pow(t57,2.0)*t66-(t44*t50*t63*t56*t66*(3.0/2.0))/t57-(t50*t62*t56*(ld2-(t3*t44*t61)/2.0)*t65*2.0)/t57;
}
if (t67 < t53) {
    t2 = 0.0;
} else if (t52 < t67) {
    t2 = (n3*t61)/t57-t45*t61*1.0/pow(t57,2.0)*t56-(t45*t62*t56)/(t57*2.0);
} else {
    t2 = (n3*t50*t62*t66)/t57-t45*t50*t62*t56*1.0/pow(t57,2.0)*t66-(t45*t50*t63*t56*t66*(3.0/2.0))/t57-(t50*t62*t56*(ld3-(t3*t45*t61)/2.0)*t65*2.0)/t57;
}
if (t70) {
    t5 = 0.0;
} else {
    t5 = (t51*t62*t56*sin(ia)*t66*-2.0)/t57;
}
if (t70) {
    t6 = 0.0;
} else {
    t6 = (t36*t50*t62*t56*t65*2.0)/t57;
}
if (t70) {
    t7 = 0.0;
} else {
    t7 = (t37*t50*t62*t56*t65*2.0)/t57;
}
if (t70) {
    t8 = 0.0;
} else {
    t8 = (t38*t50*t62*t56*t65*2.0)/t57;
}
if (t70) {
    t9 = 0.0;
} else {
    t9 = (t4*t50*(t61*t61)*t56*t65*2.0)/t57+(t4*t51*t62*t56*t66*2.0)/t57;
}
dp[0] = t0;
dp[1] = t1;
dp[2] = t2;
dia = t5;
dn[0] = t6;
dn[1] = t7;
dn[2] = t8;
doa = t9;