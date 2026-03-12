#include <cmath>
#include "MathUtils.h"

void perspective(float fovy,float aspect,float znear,float zfar,float* m)
{
    float f=1.0f/tan(fovy/2.0f);

    m[0]=f/aspect; m[1]=0; m[2]=0; m[3]=0;
    m[4]=0; m[5]=f; m[6]=0; m[7]=0;
    m[8]=0; m[9]=0; m[10]=(zfar+znear)/(znear-zfar); m[11]=-1;
    m[12]=0; m[13]=0; m[14]=(2*zfar*znear)/(znear-zfar); m[15]=0;
}