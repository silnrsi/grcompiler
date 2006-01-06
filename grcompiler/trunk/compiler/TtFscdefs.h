/*
    Original File:       fscdefs.h

    Copyright:  c 1988-1990 by Apple Computer, Inc., all rights reserved.
	Modified 4/25/2000 by Alan Ward
*/

/*
*/
#define ONEFIX      ( 1L << 16 )
#define ONEFRAC     ( 1L << 30 )
#define ONEHALFFIX  0x8000L

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef long int32;
typedef unsigned long uint32;

typedef short FUnit;
typedef unsigned short uFUnit;

typedef long Fixed;
typedef long Fract;

#ifndef SHORTMUL
#define SHORTMUL(a,b)   (int32)((int32)(a) * (b))
#endif

#ifndef SHORTDIV
#define SHORTDIV(a,b)   (int32)((int32)(a) / (b))
#endif

inline float F2Dot14(short a) {return (a >> 14) + (a & 0x3FFF) / (float)0x4000;}

#ifdef FSCFG_BIG_ENDIAN /* target byte order matches Motorola 68000 */
inline unsigned short swapw(short a) {return a;};
inline unsigned long swapl(long a) {return a;};
inline short swapws(short a) {return a;}
inline long swapls(long a) {return a;};
#else
 /* Portable code to extract a short or a long from a 2- or 4-byte buffer */
 /* which was encoded using Motorola 68000 (TrueType "native") byte order. */
// if p were signed, the last element in the p array would be signed extended 
// use the first two for unsigned fields and the last two for signed fields
inline unsigned short swapw(short a) {unsigned char * p = (unsigned char *)&a; return p[0]<<8 | p[1];};
inline unsigned long swapl(long a) {unsigned char * p = (unsigned char *)&a; return p[0]<<24 | p[1]<<16 | p[2]<<8 | p[3];};
inline short swapws(short a) {unsigned char * p = (unsigned char *)&a; return p[0]<<8 | p[1];};
inline long swapls(long a) {unsigned char * p = (unsigned char *)&a; return p[0]<<24 | p[1]<<16 | p[2]<<8 | p[3];};
#endif

