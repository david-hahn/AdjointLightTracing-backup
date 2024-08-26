const float t2 = pixelUV1-1.0;
dbfduv[0] = -pixelUV2*(cd1-cd2)-(cd3-cd4)*(pixelUV2-1.0);
dbfduv[1] = cd2*pixelUV1-cd3*pixelUV1-cd1*t2+cd4*t2;
