/*
 * rng implementation
 */

#include <cstdlib>
#include <cmath>
#include <ctime>
#include <sys/time.h>

#include "rng.h"

// An initialiser function for RngData
void rngDataInit (RngData* rd)
{
    rd->seed = 0;
    rd->qBinVal = -1;
}

int getTime(void)
{
    struct timeval currTime;
    gettimeofday(&currTime, NULL);
    return time(0) | currTime.tv_usec;
}

float uniformGCC(RngData* rd)
{
    rd->seed = (unsigned int)std::abs(static_cast<int>(rd->seed) * rd->a_RNG + rd->c_RNG);
    float seed2 = rd->seed/2147483648.0;
    return seed2;
}

float nfix (RngData* rd) /*provides RNOR if #define cannot */
{
    const float r = 3.442620f;
    static float x, y;
    for (;;) {
        x=rd->hz*rd->wn[rd->iz];
        if (rd->iz==0) {
            do {
                x = -std::log(UNI(rd))*0.2904764;
                y = -std::log(UNI(rd));
            } while (y+y<x*x);
            return (rd->hz>0) ? r+x : -r-x;
        }

        if (rd->fn[rd->iz]+UNI(rd)*(rd->fn[rd->iz-1]-rd->fn[rd->iz]) < std::exp(-.5*x*x)) {
            return x;
        }

        rd->hz=SHR3(rd);
        rd->iz=rd->hz&127;
        if (std::abs(rd->hz)<(int)rd->kn[rd->iz]) {
            return (rd->hz*rd->wn[rd->iz]);
        }
    }
}

float efix (RngData* rd) /*provides REXP if #define cannot */
{
    float x;
    for (;;) {
        if (rd->iz==0) {
            return (7.69711-std::log(UNI(rd)));
        }
        x=rd->jz*rd->we[rd->iz];
        if (rd->fe[rd->iz]+UNI(rd)*(rd->fe[rd->iz-1]-rd->fe[rd->iz]) < std::exp(-x)) {
            return (x);
        }
        rd->jz=SHR3(rd);
        rd->iz=(rd->jz&255);
        if (rd->jz<rd->ke[rd->iz]) {
            return (rd->jz*rd->we[rd->iz]);
        }
    }
}

// == This procedure sets the seed and creates the tables ==
void zigset (RngData* rd, unsigned int jsrseed)
{
    clock();

    const double m1 = 2147483648.0, m2 = 4294967296.;
    double dn=3.442619855899,tn=dn,vn=9.91256303526217e-3, q;
    double de=7.697117470131487, te=de, ve=3.949659822581572e-3;
    int i;

    /* Tables for RNOR: */
    q=vn/std::exp(-.5*dn*dn);
    rd->kn[0]=(dn/q)*m1; rd->kn[1]=0;
    rd->wn[0]=q/m1; rd->wn[127]=dn/m1;
    rd->fn[0]=1.; rd->fn[127]=std::exp(-.5*dn*dn);
    for (i=126;i>=1;i--) {
        dn=sqrt(-2.*std::log(vn/dn+std::exp(-.5*dn*dn)));
        rd->kn[i+1]=(dn/tn)*m1; tn=dn;
        rd->fn[i]=std::exp(-.5*dn*dn); rd->wn[i]=dn/m1;
    }
    /* Tables for REXP */
    q = ve/std::exp(-de);
    rd->ke[0]=(de/q)*m2; rd->ke[1]=0;
    rd->we[0]=q/m2; rd->we[255]=de/m2;
    rd->fe[0]=1.; rd->fe[255]=std::exp(-de);
    for (i=254;i>=1;i--) {
        de=-std::log(ve/de+std::exp(-de));
        rd->ke[i+1]= (de/te)*m2; te=de;
        rd->fe[i]=std::exp(-de); rd->we[i]=de/m2;
    }
}

int slowBinomial(RngData* rd, int N, float p)
{
    int num = 0;
    for (int i = 0; i < N; ++i) {
        if (UNI(rd) < p) {
            ++num;
        }
    }
    return num;
}

int fastBinomial(RngData* rd, int N, float p)
{
    // setup the computationally intensive vals
    if (rd->qBinVal == -1) {
        rd->qBinVal = 1-p;
        rd->sBinVal = p/rd->qBinVal;
        rd->aBinVal = (N+1)*rd->sBinVal;
        rd->rBinVal = pow(rd->qBinVal,N);
    }

    float r = rd->rBinVal;
    float u = UNI(rd);
    int x = 0;

    while (u>r) {
        u=u-rd->rBinVal;
        x=x+1;
        r=((rd->aBinVal/float(x))-rd->sBinVal)*r;
    }

    return x;
}
