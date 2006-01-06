#ifndef _PLATFORM_H
#define _PLATFORM_H

// Platform headers
//
#if defined(_WIN32)
#include <windows.h>
#include <tchar.h>
#else

#include <stddef.h>

char * itoa(int value, char *string, int radix);

unsigned short MultiByteToWideChar(unsigned long code_page, unsigned long, 
        const char * source, size_t src_count, 
        unsigned short * dest, size_t dst_count);

#endif    // _WIN32

#endif    // _PLATFORM_H
