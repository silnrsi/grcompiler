/*  Copyright (c) 2012, Siyuan Fu <fusiyuan2010@gmail.com>
    Copyright (c) 2015, SIL International
    All rights reserved.
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    
    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
    
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    
    3. Neither the name of the copyright holder nor the names of its
       contributors may be used to endorse or promote products derived from
       this software without specific prior written permission.
    
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
    POSSIBILITY OF SUCH DAMAGE.
*/
//#include <cassert>
//#include <cstddef>
#include <cstdlib>
//#include <cstring>

#include "Compressor.h"
#include "Shrinker.h"

using namespace shrinker;

namespace
{
unsigned int const  HASH_BITS = 15;

inline
u32 hash(u32 a) {
    return (a*0x14c7649) >> (32 - HASH_BITS);
}

template<typename T>
inline 
T unaligned_load(void const * ptr) {
  T t; unaligned_copy<sizeof t>(&t, ptr); return t;
}

}

int shrinker::compress(void const *in, void *out, size_t size)
{
    u32* ht = static_cast<u32*>(calloc(1 << HASH_BITS, sizeof(u32)));
    u8 const  * src = static_cast<u8 const *>(in);
    u8        * dst = static_cast<u8*>(out);
    u8 const  * src_end = src + size - MINMATCH - 8;
    u8        * dst_end = dst + size - MINMATCH - 8;
    u8 const  * p_last_lit = src;
    u32 cur_hash, len, match_dist;
    u8 flag, *pflag, cache;
    
    if (size < 32 || size > (1 << 27) - 1)
    {
        free(ht);
        return -1;
    }
    
    while(likely(src < src_end) && likely(dst < dst_end))
    {
        u32 u32val, distance = src - (u8*)in;
        u8 const *pfind, *pcur;
        pcur = src;
        u32val = unaligned_load<u32>(pcur);
        cur_hash = hash(u32val);
        cache = ht[cur_hash] >> 27;
        pfind = (u8*)in + (ht[cur_hash] & 0x07ffffff);
        ht[cur_hash] = distance|(*src<<27);

        if (unlikely(cache == (*pcur & 0x1f))
            && pfind + 0xffff >= (u8*)pcur
            && pfind < pcur
            && unaligned_load<u32>(pfind) == unaligned_load<u32>(pcur))
        {  
            pfind += 4; pcur += 4;
            while(likely(pcur < src_end) && unaligned_load<u32>(pfind) == unaligned_load<u32>(pcur))
            { pfind += 4; pcur += 4;}
            if (likely(pcur < src_end))
            if (unaligned_load<u16>(pfind) == unaligned_load<u16>(pcur)) {pfind += 2; pcur += 2;}
            if (*pfind == *pcur) {pfind++; pcur++;}

            pflag = dst++;
            len = src - p_last_lit;
            if (likely(len < 7)) flag = len << 5;
            else {
                len -= 7;flag = (7<<5);
                while (len >= 255) { *dst++ = 255;len-= 255;}
                *dst++ = len;
            }

            len = pcur - src  - MINMATCH;
            if (likely(len < 15))  flag |= len;
            else {
                len -= 15; flag |= 15;
                while (len >= 255) { *dst++ = 255;len -= 255;}
                *dst++ = len;
            }
            match_dist = pcur - pfind - 1;
            *pflag = flag;
            *dst++ = match_dist & 0xff;
            if (match_dist > 0xff) {
                *pflag |= 0x10;
                *dst++ = match_dist >> 8;
            }
            memcpy_nooverlap(dst, p_last_lit, src - p_last_lit);

            u32val = unaligned_load<u32>(src+1); ht[hash(u32val)] = (src - (u8*)in + 1)|(*(src+1)<<27);
            u32val = unaligned_load<u32>(src+3); ht[hash(u32val)] = (src - (u8*)in + 3)|(*(src+3)<<27);
            p_last_lit = src = pcur;
            continue;
        }
        src++;
    }

    if (dst - (u8*)out + 3 >= src - (u8*)in)
    {
        free(ht);
        return -1;
    }
    src = (u8*)in + size;
    pflag = dst++;
    len = src - p_last_lit;
    if (likely(len < 7)) 
        flag = len << 5;
    else {
        len -= 7; flag = (7<<5);
        while (len >= 255) { *dst++ = 255; len -= 255;}
        *dst++ = len;
    }

    flag |= 0x10; // any number
    *pflag = flag;
    *dst++ = 0xff; *dst++ = 0xff;
    memcpy_nooverlap_surpass(dst, p_last_lit, src - p_last_lit);

    free(ht);
    if (dst > dst_end) return -1;
    else return dst - (u8*)out;
}

