const float t2 = n1*wi1;
const float t3 = n2*wi2;
const float t4 = n3*wi3;
const float t5 = 1.0/3.141592653589793;
const float t6 = alpha2-1.0;
const float t7 = -lp1;
const float t8 = -lp2;
const float t9 = -lp3;
const float t10 = hp1+t7;
const float t11 = hp2+t8;
const float t12 = hp3+t9;
const float t22 = t2+t3+t4;
const float t13 = (t10/abs(t10));
const float t14 = (t11/abs(t11));
const float t15 = (t12/abs(t12));
const float t16 = abs(t10);
const float t17 = abs(t11);
const float t18 = abs(t12);
const float t23 = abs(t22);
const float t24 = t22*t22;
const float t19 = t16*t16;
const float t20 = t17*t17;
const float t21 = t18*t18;
const float t25 = t24+tinyEps;
const float t26 = t24-1.0;
const float t27 = 1.0/t25;
const float t28 = t19+t20+t21;
const float t29 = 1.0/sqrt(t28);
const float t31 = alpha2*t26*t27;
const float t30 = t29*t29*t29;
const float t32 = n1*t29;
const float t33 = n2*t29;
const float t34 = n3*t29;
const float t35 = -t31;
const float t40 = t10*t29;
const float t41 = t11*t29;
const float t42 = t12*t29;
const float t36 = -t32;
const float t37 = -t33;
const float t38 = -t34;
const float t39 = t35+1.0;
const float t43 = t10*t32;
const float t44 = t11*t33;
const float t45 = t12*t34;
const float t47 = -t40;
const float t48 = -t41;
const float t49 = -t42;
const float t64 = t10*t13*t16*t30;
const float t65 = t11*t14*t17*t30;
const float t66 = t12*t15*t18*t30;
const float t68 = n2*t11*t13*t16*t30;
const float t69 = n1*t10*t14*t17*t30;
const float t70 = n3*t12*t13*t16*t30;
const float t72 = n1*t10*t15*t18*t30;
const float t73 = n3*t12*t14*t17*t30;
const float t74 = n2*t11*t15*t18*t30;
const float t46 = sqrt(t39);
const float t50 = t47+wi1;
const float t51 = t48+wi2;
const float t52 = t49+wi3;
const float t67 = n1*t64;
const float t71 = n2*t65;
const float t75 = n3*t66;
const float t76 = -t64;
const float t77 = -t65;
const float t78 = -t66;
const float t82 = t43+t44+t45;
const float t53 = t46+1.0;
const float t54 = abs(t50);
const float t55 = abs(t51);
const float t56 = abs(t52);
const float t57 = (t50/abs(t50));
const float t58 = (t51/abs(t51));
const float t59 = (t52/abs(t52));
const float t79 = t29+t76;
const float t80 = t29+t77;
const float t81 = t29+t78;
const float t83 = abs(t82);
const float t84 = (t82/abs(t82));
const float t85 = t82*t82;
const float t112 = t36+t67+t68+t70;
const float t113 = t37+t69+t71+t73;
const float t114 = t38+t72+t74+t75;
const float t60 = t54*t54;
const float t61 = t55*t55;
const float t62 = t56*t56;
const float t63 = 1.0/t53;
const float t86 = t85+tinyEps;
const float t87 = t85-1.0;
const float t90 = t23*t83*4.0;
const float t95 = t10*t14*t17*t30*t54*t57*2.0;
const float t96 = t11*t13*t16*t30*t55*t58*2.0;
const float t97 = t10*t15*t18*t30*t54*t57*2.0;
const float t98 = t12*t13*t16*t30*t56*t59*2.0;
const float t99 = t11*t15*t18*t30*t55*t58*2.0;
const float t100 = t12*t14*t17*t30*t56*t59*2.0;
const float t103 = t54*t57*t79*2.0;
const float t104 = t55*t58*t80*2.0;
const float t105 = t56*t59*t81*2.0;
const float t88 = 1.0/t86;
const float t91 = t90+tinyEps;
const float t92 = t60+t61+t62;
const float t106 = -t103;
const float t107 = -t104;
const float t108 = -t105;
const float t89 = t88*t88;
const float t93 = 1.0/t91;
const float t101 = 1.0/sqrt(t92);
const float t115 = alpha2*t87*t88;
const float t123 = t96+t98+t106;
const float t124 = t95+t100+t107;
const float t125 = t97+t99+t108;
const float t94 = t93*t93;
const float t102 = t101*t101*t101;
const float t109 = -n1*t101*(t40-wi1);
const float t110 = -n2*t101*(t41-wi2);
const float t111 = -n3*t101*(t42-wi3);
const float t116 = -t115;
const float t127 = pow(n1*t101*(t40-wi1)+n2*t101*(t41-wi2)+n3*t101*(t42-wi3),2.0);
const float t117 = t116+1.0;
const float t126 = t109+t110+t111;
const float t128 = t6*t127;
const float t118 = sqrt(t117);
const float t129 = t128+1.0;
const float t119 = 1.0/t118;
const float t120 = t118+1.0;
const float t130 = 1.0/(t129*t129);
const float t131 = 1.0/(t129*t129*t129);
const float t121 = 1.0/t120;
const float t122 = t121*t121;
gradSpecular[0] = alpha2*t5*t63*t93*t119*t122*t130*(alpha2*t82*t88*t112*2.0-alpha2*t82*t87*t89*t112*2.0)*2.0-alpha2*t5*t6*t63*t93*t121*t131*(n1*t101*(t40-wi1)+n2*t101*(t41-wi2)+n3*t101*(t42-wi3))*(t68*t101+t70*t101-n1*t79*t101+(n1*t102*t123*(t40-wi1))/2.0+(n2*t102*t123*(t41-wi2))/2.0+(n3*t102*t123*(t42-wi3))/2.0)*1.6E+1-alpha2*t5*t23*t63*t84*t94*t112*t121*t130*1.6E+1;
gradSpecular[1] = alpha2*t5*t63*t93*t119*t122*t130*(alpha2*t82*t88*t113*2.0-alpha2*t82*t87*t89*t113*2.0)*2.0-alpha2*t5*t6*t63*t93*t121*t131*(n1*t101*(t40-wi1)+n2*t101*(t41-wi2)+n3*t101*(t42-wi3))*(t69*t101+t73*t101-n2*t80*t101+(n1*t102*t124*(t40-wi1))/2.0+(n2*t102*t124*(t41-wi2))/2.0+(n3*t102*t124*(t42-wi3))/2.0)*1.6E+1-alpha2*t5*t23*t63*t84*t94*t113*t121*t130*1.6E+1;
gradSpecular[2] = alpha2*t5*t63*t93*t119*t122*t130*(alpha2*t82*t88*t114*2.0-alpha2*t82*t87*t89*t114*2.0)*2.0-alpha2*t5*t6*t63*t93*t121*t131*(n1*t101*(t40-wi1)+n2*t101*(t41-wi2)+n3*t101*(t42-wi3))*(t72*t101+t74*t101-n3*t81*t101+(n1*t102*t125*(t40-wi1))/2.0+(n2*t102*t125*(t41-wi2))/2.0+(n3*t102*t125*(t42-wi3))/2.0)*1.6E+1-alpha2*t5*t23*t63*t84*t94*t114*t121*t130*1.6E+1;
