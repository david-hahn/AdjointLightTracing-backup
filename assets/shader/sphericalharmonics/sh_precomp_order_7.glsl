const float t2 = x*x;
const float t3 = x*x*x;
const float t5 = x*x*x*x*x;
const float t7 = y*y;
const float t8 = y*y*y;
const float t10 = y*y*y*y*y;
const float t12 = z*z;
const float t19 = sqrt(2.0);
const float t4 = t2*t2;
const float t6 = t2*t2*t2;
const float t9 = t7*t7;
const float t11 = t7*t7*t7;
const float t13 = t12*t12;
const float t14 = t12*t12*t12;
const float t17 = t12*3.0;
const float t18 = t12*7.0;
const float t20 = -t3;
const float t21 = -t7;
const float t22 = -t8;
const float t24 = t12*1.1E+1;
const float t25 = t12*1.3E+1;
const float t26 = t12*3.0E+1;
const float t28 = t7*x*3.0;
const float t29 = t2*y*3.0;
const float t33 = t12*1.1E+2;
const float t35 = t2*t7*6.0;
const float t36 = t12*(1.5E+1/2.0);
const float t41 = t2*t7*1.0E+1;
const float t42 = t2*t8*1.0E+1;
const float t43 = t3*t7*1.0E+1;
const float t46 = t12*(1.05E+2/2.0);
const float t47 = t12*(1.05E+2/4.0);
const float t56 = t12*(9.45E+2/2.0);
const float t57 = t12*(9.45E+2/4.0);
const float t60 = t12*(9.45E+2/1.6E+1);
const float t64 = t12*5.1975E+3;
const float t65 = t12*2.59875E+3;
const float t69 = t12*6.75675E+4;
const float t15 = t4*3.0;
const float t16 = t9*3.0;
const float t23 = -t11;
const float t27 = t13*3.3E+1;
const float t30 = t9*x*5.0;
const float t31 = t4*y*5.0;
const float t32 = -t26;
const float t34 = t13*1.43E+2;
const float t37 = -t33;
const float t38 = t17-1.0;
const float t39 = t18-3.0;
const float t40 = -t35;
const float t44 = t2*t9*1.5E+1;
const float t45 = t4*t7*1.5E+1;
const float t48 = t24-3.0;
const float t49 = t25-3.0;
const float t50 = -t41;
const float t51 = -t42;
const float t52 = -t43;
const float t54 = -t47;
const float t55 = t13*(3.15E+2/8.0);
const float t58 = t2+t21;
const float t59 = -t57;
const float t61 = t14*1.876875E+2;
const float t62 = t13*4.33125E+2;
const float t63 = t13*2.165625E+2;
const float t67 = -t65;
const float t68 = t13*5.630625E+3;
const float t70 = t20+t28;
const float t71 = t22+t29;
const float t72 = t36-3.0/2.0;
const float t73 = t46-1.5E+1/2.0;
const float t76 = t56-1.05E+2/2.0;
const float t78 = t64-9.45E+2/2.0;
const float t81 = t69-5.1975E+3;
const float t53 = -t45;
const float t66 = -t63;
const float t74 = t27+t32+5.0;
const float t75 = t4+t9+t40;
const float t77 = t34+t37+1.5E+1;
const float t79 = t5+t30+t52;
const float t80 = t10+t31+t51;
const float t82 = t15+t16+t50;
const float t83 = t54+t55+1.5E+1/8.0;
const float t85 = t59+t62+1.05E+2/8.0;
const float t86 = t67+t68+9.45E+2/8.0;
const float t84 = t6+t23+t44+t53;
const float t87 = t60+t61+t66-3.5E+1/1.6E+1;
const float shWeights[64] = {
    2.820947917738781E-1,
    t19*y*(-3.454941494713355E-1),
    z*4.886025119029199E-1,
    t19*x*(-3.454941494713355E-1),
    t19*x*y*7.725484040463791E-1,
    t19*y*z*(-7.725484040463791E-1),
    t12*9.461746957575601E-1-3.1539156525252E-1,
    t19*x*z*(-7.725484040463791E-1),
    t19*t58*3.862742020231895E-1,
    t19*y*(t2*2.0+t58)*(-4.172238236327841E-1),
    t19*x*y*z*2.043970952866565,
    t19*t72*y*(-2.154534560761004E-1),
    z*(t12*5.0-3.0)*3.731763325901154E-1,
    t19*t72*x*(-2.154534560761004E-1),
    t19*t58*z*1.021985476433282,
    t19*x*(t2-t7*3.0)*(-4.172238236327841E-1),
    t19*t58*x*y*1.77013076977993,
    t19*z*(t8-t29)*1.251671470898352,
    t19*t73*x*y*8.920620580763856E-2,
    t19*t39*y*z*(-4.7308734787878E-1),
    t12*(-3.173566407456129)+t13*3.702494142032151+3.173566407456129E-1,
    t19*t39*x*z*(-4.7308734787878E-1),
    t19*t58*t73*4.460310290381928E-2,
    t19*z*(t3-t28)*(-1.251671470898352),
    t19*(t4*4.425326924449827E-1+t9*4.425326924449827E-1-t2*t7*2.655196154669896),
    t19*y*(t4*5.0+t9+t50)*(-4.641322034408581E-1),
    t19*t58*x*y*z*5.870859593223005,
    t19*t76*(t8-t29)*6.589404174225527E-3,
    t19*t38*x*y*z*3.389542366521799,
    t19*t83*y*(-1.708168792406481E-1),
    z*(t12*-7.0E+1+t13*6.3E+1+1.5E+1)*1.169503224534236E-1,
    t19*t83*x*(-1.708168792406481E-1),
    t19*t38*t58*z*1.694771183260899,
    t19*t76*(t3-t28)*(-6.589404174225528E-3),
    t19*t75*z*1.467714898305751,
    t19*x*(t4+t9*5.0+t50)*(-4.641322034408582E-1),
    t19*t82*x*y*9.661682271601325E-1,
    t19*t80*z*(-1.673452458100098),
    t19*t58*t78*x*y*3.020370479187285E-3,
    t19*t48*z*(t8-t29)*6.513904858677158E-1,
    t19*t85*x*y*4.962975130420691E-2,
    t19*t74*y*z*(-4.119755163011408E-1),
    t12*6.674766238100985-t13*2.002429871430295E+1+t14*1.468448572382217E+1-3.178460113381421E-1,
    t19*t74*x*z*(-4.119755163011408E-1),
    t19*t58*t85*2.481487565210345E-2,
    t19*t48*z*(t3-t28)*(-6.513904858677158E-1),
    t19*t75*t78*7.550926197968211E-4,
    t19*t79*z*(-1.673452458100098),
    t19*t84*4.830841135800663E-1,
    t19*y*(t6*7.0+t23+t2*t9*2.1E+1-t4*t7*3.5E+1)*(-5.000395635705507E-1),
    t19*t82*x*y*z*3.741953453425937,
    t19*t80*t81*(-7.059715720566384E-5),
    t19*t49*t58*x*y*z*2.935429796611502,
    t19*t86*(t8-t29)*2.809731380603065E-3,
    t19*t77*x*y*z*3.129178677245881E-1,
    t19*t87*y*(-1.459979252047546E-1),
    z*(t12*3.15E+2-t13*6.93E+2+t14*4.29E+2-3.5E+1)*6.828427691200493E-2,
    t19*t87*x*(-1.459979252047546E-1),
    t19*t58*t77*z*1.564589338622941E-1,
    t19*t86*(t3-t28)*(-2.809731380603064E-3),
    t19*t49*t75*z*7.338574491528756E-1,
    t19*t79*t81*(-7.059715720566384E-5),
    t19*t84*z*1.870976726712969,
    t19*x*(t6-t11*7.0+t2*t9*3.5E+1-t4*t7*2.1E+1)*(-5.000395635705507E-1)
};
