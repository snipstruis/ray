#pragma once
#include<cmath>

#define IC inline constexpr 
#define I  inline
#define fl float

struct v3{
    fl x,y,z;
    v3() = default;
    IC v3(fl f):x(f),y(f),z(f){};
    IC v3(fl x, fl y, fl z):x(x),y(y),z(z){};
    IC void operator+=(v3 v){x+=v.x; y+=v.y; z+=v.z;};
    IC void operator-=(v3 v){x-=v.x; y-=v.y; z-=v.z;};
    IC void operator+=(fl f){x+=f;   y+=f;   z+=f;};
    IC void operator-=(fl f){x-=f;   y-=f;   z-=f;};
    IC void operator*=(fl f){x*=f;   y*=f;   z*=f;};
    IC void operator/=(fl f){x/=f;   y/=f;   z/=f;};
};

IC v3 operator+(v3 a, v3 b){return v3(a.x+b.x, a.y+b.y, a.z+b.z);}
IC v3 operator-(v3 a, v3 b){return v3(a.x-b.x, a.y-b.y, a.z-b.z);}
IC v3 operator*(v3 a, fl b){return v3(a.x*b, a.y*b, a.z*b);}
IC v3 operator*(fl a, v3 b){return b*a;}
IC v3 operator/(v3 a, fl b){return v3(a.x/b, a.y/b, a.z/b);}
IC fl dot      (v3 a, v3 b){return a.x*b.x + a.y*b.y + a.z*b.z;}
IC v3 cross    (v3 a, v3 b){return v3(a.y*b.z - a.z*b.y,
                                      a.z*b.x - a.x*b.z,
                                      a.x*b.y - a.y*b.x);}
IC fl lengthsq (v3 v)      {return dot(v,v);}
I  fl length   (v3 v)      {return sqrtf(lengthsq(v));}//sqrt: not constextpr! 
I  fl distance (v3 a, v3 b){return length(b-a);}
I  v3 normalize(v3 v)      {return v/length(v);}

#undef IC
#undef I
#undef fl
