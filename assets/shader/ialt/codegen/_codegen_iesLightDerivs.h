float t0 = 0.0;
float t1 = 0.0;
float t2 = hp1*lb1;
float t3 = hp2*lb2;
float t4 = hp3*lb3;
float t5 = hp1*ld1;
float t6 = hp2*ld2;
float t7 = hp3*ld3;
float t8 = hp1*lt1;
float t9 = hp2*lt2;
float t10 = hp3*lt3;
float t11 = lb1*lp1;
const float t12 = lb2*lp2;
const float t13 = lb3*lp3;
const float t14 = ld1*lp1;
const float t15 = ld2*lp2;
const float t16 = ld3*lp3;
const float t17 = lp1*lt1;
const float t18 = lp2*lt2;
const float t19 = lp3*lt3;
const float t20 = minHAngle+tinyEps;
const float t21 = minVAngle+tinyEps;
const float t22 = 1.0/3.141592653589793;
const float t23 = -lp1;
const float t24 = -lp2;
const float t25 = -lp3;
const float t26 = -minHAngle;
const float t27 = -minVAngle;
const bool t28 = (maxHAngle < t20);
const bool t29 = (maxVAngle < t21);
const float t30 = -t11;
const float t31 = -t12;
const float t32 = -t13;
const float t33 = -t14;
const float t34 = -t15;
const float t35 = -t16;
const float t36 = -t17;
const float t37 = -t18;
const float t38 = -t19;
const float t39 = hp1+t23;
const float t40 = hp2+t24;
const float t41 = hp3+t25;
const float t42 = maxHAngle+t26;
const float t43 = maxVAngle+t27;
const float t44 = t39*t39;
const float t45 = t40*t40;
const float t46 = t41*t41;
const float t47 = 1.0/t42;
const float t48 = 1.0/t43;
const float t50 = t2+t3+t4+t30+t31+t32;
const float t51 = t5+t6+t7+t33+t34+t35;
const float t52 = t8+t9+t10+t36+t37+t38;
const float t49 = t44+t45+t46;
const float t53 = t50*t50;
const float t54 = t51*t51;
const float t55 = t52*t52;
const float t56 = 1.0/t52;
const float t57 = 1.0/t55;
const float t58 = 1.0/t49;
const float t59 = 1.0/sqrt(t49);
const float t60 = t59*t59*t59;
const float t61 = t53*t58;
const float t62 = t54*t58;
const float t63 = t55*t58;
const float t64 = -t62;
const float t67 = t61+t63;
const float t65 = t64+1.0;
const float t68 = 1.0/t67;
if (t29) {
  t0 = 0.0;
} else {
  t0 = rangeUV1*t22*t48*1.0/sqrt(t64+1.0)*(ld1*t59-((hp1*2.0-lp1*2.0)*t60*t51)/2.0)*1.8E+2;
}
if (t28) {
  t1 = 0.0;
} else {
  t1 = (rangeUV2*t22*t47*(lb1*t56-lt1*t50*t57)*-1.8E+2*t63)/(t61+t63);
}
if (t29) {
  t2 = 0.0;
} else {
  t2 = rangeUV1*t22*t48*1.0/sqrt(t64+1.0)*(ld2*t59-((hp2*2.0-lp2*2.0)*t60*t51)/2.0)*1.8E+2;
}
if (t28) {
  t3 = 0.0;
} else {
  t3 = (rangeUV2*t22*t47*(lb2*t56-lt2*t50*t57)*-1.8E+2*t63)/(t61+t63);
}
if (t29) {
  t4 = 0.0;
} else {
  t4 = rangeUV1*t22*t48*1.0/sqrt(t64+1.0)*(ld3*t59-((hp3*2.0-lp3*2.0)*t60*t51)/2.0)*1.8E+2;
}
if (t28) {
  t5 = 0.0;
} else {
  t5 = (rangeUV2*t22*t47*(lb3*t56-lt3*t50*t57)*-1.8E+2*t63)/(t61+t63);
}
if (t29) {
  t6 = 0.0;
} else {
  t6 = rangeUV1*t22*t39*t48*t59*1.0/sqrt(t64+1.0)*-1.8E+2;
}
if (t29) {
  t7 = 0.0;
} else {
  t7 = rangeUV1*t22*t40*t48*t59*1.0/sqrt(t64+1.0)*-1.8E+2;
}
if (t29) {
  t8 = 0.0;
} else {
  t8 = rangeUV1*t22*t41*t48*t59*1.0/sqrt(t64+1.0)*-1.8E+2;
}
if (t28) {
  t9 = 0.0;
} else {
  t9 = (rangeUV2*t22*t39*t47*t58*t50*-1.8E+2)/(t61+t63);
}
if (t28) {
  t10 = 0.0;
} else {
  t10 = (rangeUV2*t22*t40*t47*t58*t50*-1.8E+2)/(t61+t63);
}
if (t28) {
  t11 = 0.0;
} else {
  t11 = (rangeUV2*t22*t41*t47*t58*t50*-1.8E+2)/(t61+t63);
}
duvdp[0][0] = t0;
duvdp[0][1] = t1;
duvdp[1][0] = t2;
duvdp[1][1] = t3;
duvdp[2][0] = t4;
duvdp[2][1] = t5;
duvdn[0][0] = t6;
duvdn[1][0] = t7;
duvdn[2][0] = t8;
duvdt[0][1] = t9;
duvdt[1][1] = t10;
duvdt[2][1] = t11;
