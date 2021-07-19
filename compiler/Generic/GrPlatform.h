#ifndef _GR_PLATFORM_H
#define _GR_PLATFORM_H

/***********************************************************************************************
	Turn off the goofy warnings for MS VisualStudio.
***********************************************************************************************/
#ifdef _MSC_VER
#pragma warning(disable: 4065) // Switch statement contains default but no case.
#pragma warning(disable: 4097) // typedef-name 'xxx' used as synonym for class-name 'yyy'.
#pragma warning(disable: 4100) // unreferenced formal parameter.
#pragma warning(disable: 4192) // automatically excluding while importing.
#pragma warning(disable: 4201) // nonstandard extension used : nameless struct/union.
#pragma warning(disable: 4290) // exception specification ignored.
#pragma warning(disable: 4310) // cast truncates constant value.
#pragma warning(disable: 4355) // 'this' used in base member initializer list.
#pragma warning(disable: 4505) // unreferenced local function has been removed.
#pragma warning(disable: 4510) // default constructor could not be generated - caused by
                               // applying ComSmartPtr to a non-interface class.
#pragma warning(disable: 4511) // copy constructor could not be generated.
#pragma warning(disable: 4512) // assignment operator could not be generated.
#pragma warning(disable: 4610) // class 'xxx' can never be instantiated - user defined
                               // constructor required - caused by applying ComSmartPtr to a non-interface class.
#pragma warning(disable: 4660) // template-class specialization is already instantiated.
#pragma warning(disable: 4701) // local variable 'xxx' may be used without being initialized.
                               // We would like to keep this warning (4701) enabled but the compiler applies it in
                               // places that are obviously OK.
#pragma warning(disable: 4702) // unreachable code. We would like to keep this warning (4702)
                               // enabled but the compiler applies it in places that are obviously OK.
#pragma warning(disable: 4710) // not inlined.
#pragma warning(disable: 4786) // identifier truncated in debug info.
#pragma warning(disable: 4800) // forcing value to bool 'true' or 'false' (performance warning).

#endif


// Standard headers
#include <cstddef>

// Platform headers
//
#if defined(_WIN32)
#include <windows.h>
#include <tchar.h>

#ifdef SIZEOF_WCHAR_T
#define SIZEOF_WCHAR_T 2
#endif

#endif

// Project headers
#ifndef _WIN32
#include "GrMstypes.h"
#else
#include <algorithm>
#endif
#include <iostream>

#if defined(_WIN32)
//  TSE: We need to define std::max and std::min, because VC6.0 seems to omit
//  these from their version of <algorithm> and rely on the macro versions
//  to silently take their place.  This breaks the use of the name max in any
//  other scope e.g. as in std::numeric_limits<..>::max().

#undef min
#undef max

//	SJC: Only do this for version 6, otherwise the definitions conflict with the
//	built-in STL definitions, and the compiler really struggles.
#if _MSC_VER == 1200 // VC version 6
namespace std 
{
	template<typename T> inline T max(const T a, const T b) { return a < b ? b : a; }
	template<typename T> inline T min(const T a, const T b) { return a > b ? b : a; }
}
#endif // _MSC_VER == 1200
//////using std::max; // SJC moved to GrCommon.h
//////using std::min;

#else // _WIN32

#if !HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME
// These are GNU extensions to libc and not available on Mac or BSD
extern char* program_invocation_name;
extern char* program_invocation_short_name;
#endif

#endif // _WIN32


namespace gr
{
// Typedefs
// These are correct for the x86_64 architecture too, on both Windows and Unix
typedef unsigned char byte;
typedef unsigned int utf32;	// UTF32 encoded Unicode codepoints
typedef unsigned short int utf16;	// UTF16 encoded Unicode codepoints
typedef unsigned char utf8;			// UTF-8 encoded Unicode codepoints
typedef unsigned short int gid16;	// glyph ID
typedef unsigned int fontTableId32;	// ID to pass to getTable()

typedef unsigned char      data8;
typedef unsigned short int data16;	// generic 16-bit data
typedef unsigned int       data32;	// generic 32-bit data
typedef signed char      sdata8;
typedef signed short int sdata16;	// generic 16-bit data
typedef signed int       sdata32;	// generic 32-bit data

using offset_t = std::streamoff;

#ifndef NO_ASM
#define NO_ASM
#endif

#ifndef NULL
#define NULL	0
#endif

typedef std::ios_base::openmode openmode_t;


inline bool GrIsBadStringPtrW(const utf16 *const pszw, const long)
{
	return !pszw;
}

inline bool GrIsBadStringPtrA(const char *const psza, const long)
{
	return !psza;
}

inline bool GrIsBadReadPtr(const void *const, const size_t)
{
	return false;
}

inline bool GrIsBadWritePtr(const void *const, const size_t)
{
	return false;
}


size_t Platform_UnicodeToANSI(const utf16 * prgchwSrc, size_t cchwSrc, char * prgchsDst, size_t cchsDst);
size_t Platform_AnsiToUnicode(const char * prgchsSrc, size_t cchsSrc, utf16 * prgchwDst, size_t cchwDst);
size_t Platform_8bitToUnicode(int nCodePage, const char * prgchsSrc, size_t cchsSrc, utf16 * prgchwDst, size_t cchwDst);

utf16 *utf16cpy(utf16 *dest, const utf16 *src);
utf16 *utf16ncpy(utf16 *dest, const utf16 *src, size_t n);

#ifdef UTF16DEBUG
void utf16Output(const utf16 *input);
#endif

size_t utf16len(const utf16 *s);
size_t utf16len(const utf16 *s, size_t n);
size_t utf8len(const char *s);


int utf16cmp(const utf16 *s1, const utf16 *s2);
int utf16ncmp(const utf16 *s1, const utf16 *s2, size_t n);
int utf16cmp(const utf16 *s1, const char *s2);
int utf16ncmp(const utf16 *s1, const char *s2, size_t n);

unsigned short MultiByteToWideChar(unsigned long code_page, unsigned long, 
        const char * source, size_t src_count, 
        unsigned short * dest, size_t dst_count);


} // namespace gr

#if !defined(GR_NAMESPACE)
using namespace gr;
#endif

#endif
