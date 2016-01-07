/*
 * This code follows Marsaglia & Tsang 2000, but with a passed-in data
 * structure for mutable variables. (RngData modifications authored by
 * Seb James, August 2014).
 *
 * Example usage:
 *
 * #include <stdio.h>
 * #include "rng.h"
 *
 * int main()
 * {
 *     RngData rd;
 *     float rn;
 *     int i;
 *
 *     rngDataInit (&rd);
 *     zigset(&rd, 11);
 *     rd.seed = 102;
 *
 *     while (i < 10) {
 *         rn = randomNormal ((&rd));
 *         printf ("%f\n", rn);
 *         i++;
 *     }
 *     return i;
 * }
 *
 * g++ -o testrng testrng.cpp -lm
 */
#ifndef _RNG_H_
#define _RNG_H_

#include <cstdlib>
#include <cmath>
#include <climits>
#include <ctime>
#include <sys/time.h>

/*
 * Definition of a data storage class for use with this code. Each
 * thread wishing to call functions from this random number generator
 * must manage an instance of RngData. All functions operate by taking
 * a pointer to an instance of RngData.
 *
 * Prior to using one of the random number generators here, first call
 * rngDataInit to set up your RngData instance (if you're compiling
 * this with a c++-11 compatible compiler, you can move the
 * initialisation into the struct).
 */
struct RngData {
    // Some constants, which could go at global scope, but which I
    // prefer to have in here.
    const static int a_RNG = 1103515245;
    const static int c_RNG = 12345;
    unsigned int seed; int hz;
    unsigned int iz,jz,kn[128],ke[256];
    float wn[128],fn[128], we[256],fe[256];
    float qBinVal,sBinVal,rBinVal,aBinVal;
};

// An initialiser function for RngData
void rngDataInit (RngData* rd);

int getTime(void);

float uniformGCC(RngData* rd);

// RANDOM NUMBER GENERATOR
// returns int (probably - it does <unsigned int> + <int>):
#define SHR3(rd) ((rd)->jz=(rd)->seed,          \
                  (rd)->seed^=((rd)->seed<<13), \
                  (rd)->seed^=((rd)->seed>>17), \
                  (rd)->seed^=((rd)->seed<<5),  \
                  (rd)->jz+(rd)->seed)
// returns float:
#define UNI(rd) uniformGCC(rd)
// returns float:
#define RNOR(rd) ((rd)->hz=SHR3(rd),                                    \
                  (rd)->iz=(rd)->hz&127,                                \
                  ((unsigned int)abs((rd)->hz) < (rd)->kn[(rd)->iz]) ? (rd)->hz*(rd)->wn[(rd)->iz] : nfix(rd))
// returns float:
#define REXP(rd) ((rd)->jz=SHR3(rd),                                    \
                  (rd)->iz=(rd)->jz&255,                                \
                  ((rd)->jz < (rd)->ke[(rd)->iz]) ? (rd)->jz*(rd)->we[(rd)->iz] : efix(rd))
// returns double:
#define RPOIS(rd) -log(1.0-UNI(rd))

float nfix (RngData* rd); /*provides RNOR if #define cannot */

float efix (RngData* rd); /*provides REXP if #define cannot */

// == This procedure sets the seed and creates the tables ==
void zigset (RngData* rd, unsigned int jsrseed);

int slowBinomial(RngData* rd, int N, float p);

int fastBinomial(RngData* rd, int N, float p);

#define _randomUniform(rd) uniformGCC(rd)
#define _randomNormal(rd) RNOR(rd)
#define _randomExponential(rd) REXP(rd)
#define _randomPoisson(rd) RPOIS(rd)
#define HACK_MACRO(rd,N,p) 1;                                           \
    int spks=fastBinomial(rd,N,p);                                      \
    for(unsigned int i=0;i<spks;++i) {DATAOutspike.push_back(num);}

#endif // _RNG_H_
