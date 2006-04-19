/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: UtilMem.h
Responsibility: Shon Katzenberger
Last reviewed:

	Memory management utilities.
----------------------------------------------------------------------------------------------*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef UtilMem_H
#define UtilMem_H 1

//#include <crtdbg.h>
#include <new>
#include "GrCommon.h"
#include "Throwable.h"

namespace gr
{

/***********************************************************************************************
	Memmory moving and setting functions.
***********************************************************************************************/


/*----------------------------------------------------------------------------------------------
	This clears (sets to zero) cb bytes starting at pv.
----------------------------------------------------------------------------------------------*/
inline void ClearBytes(void * pv, int cb)
{
	memset(pv, 0, cb);
}


/*----------------------------------------------------------------------------------------------
	This clears (sets to 0) cn ints starting at pv.
----------------------------------------------------------------------------------------------*/
inline void ClearInts(void * pv, int cn)
{
	memset(pv, 0, cn * isizeof(int));
}


/*----------------------------------------------------------------------------------------------
	This clears (sets to 0) cv * isizeof(T) bytes starting at pv. This does not invoke any
	destructors - it just blasts zeros into the memory.
----------------------------------------------------------------------------------------------*/
template<typename T>
	inline void ClearItems(T * pv, int cv)
{
	memset(pv, 0, cv * isizeof(T));
}


/*----------------------------------------------------------------------------------------------
	This fills cb bytes starting at pv with the value b.
----------------------------------------------------------------------------------------------*/
inline void FillBytes(void * pv, byte b, int cb)
{
	memset(pv, b, cb);
}


/*----------------------------------------------------------------------------------------------
	This fills csn shorts starting at pv with the value sn.
----------------------------------------------------------------------------------------------*/
void FillShorts(void * pv, short sn, int csn);


/*----------------------------------------------------------------------------------------------
	This fills cn ints starting at pv with the value n.
----------------------------------------------------------------------------------------------*/
void FillInts(void * pv, int n, int cn);


/*----------------------------------------------------------------------------------------------
	Fill an array of characters with ch.
----------------------------------------------------------------------------------------------*/
inline void FillChars(utf16 * prgch, utf16 ch, int cch)
{
	FillShorts(prgch, ch, cch);
}


inline void FillChars(schar * prgch, schar ch, int cch)
{
	FillBytes(prgch, ch, cch);
}



/*----------------------------------------------------------------------------------------------
	This fills cv items of type T starting at pv with the element t. It doesn't invoke
	assignment operators - it just blasts bytes. To invoke assignment operators, use
	AssignItems instead.
----------------------------------------------------------------------------------------------*/
template<typename T>
	inline void FillItems(T * pv, T t, int cv)
{
	AssertPtrSize(pv, cv * isizeof(T));

	while (--cv >= 0)
	{
		memcpy(pv, &t, isizeof(T));
		pv++;
	}
}


/*----------------------------------------------------------------------------------------------
	This assigns the item t to cv items starting at pv.
----------------------------------------------------------------------------------------------*/
template<typename T>
	inline void AssignItems(T * pv, T & t, int cv)
{
	AssertPtrSize(pv, cv * isizeof(T));
	T * pt = (T *)pv;
	T * ptLim = pt + cv;

	while (pt < ptLim)
		*pt++ = t;
}


/*----------------------------------------------------------------------------------------------
	This moves cb bytes from pvSrc to pvDst. It handles overlapping blocks.
----------------------------------------------------------------------------------------------*/
inline void MoveBytes(const void * pvSrc, void * pvDst, int cb)
{
	memmove(pvDst, pvSrc, cb);
}


/*----------------------------------------------------------------------------------------------
	This moves cn ints from pvSrc to pvDst. It handles overlapping blocks.
----------------------------------------------------------------------------------------------*/
inline void MoveInts(const void * pvSrc, void * pvDst, int cn)
{
	memmove(pvDst, pvSrc, cn * isizeof(int));
}


/*----------------------------------------------------------------------------------------------
	This moves cv items from pvSrc to pvDst. It handles overlapping blocks. It doesn't invoke
	assignment operators - it just moves bytes.
----------------------------------------------------------------------------------------------*/
template<typename T>
	inline void MoveItems(const T * pvSrc, T * pvDst, int cv)
{
	memmove(pvDst, pvSrc, cv * isizeof(T));
}


/*----------------------------------------------------------------------------------------------
	This copies cb bytes from pvSrc to pvDst. It _doesn't_ handle overlapping blocks.
----------------------------------------------------------------------------------------------*/
inline void CopyBytes(const void * pvSrc, void * pvDst, int cb)
{
	Assert((byte *)pvSrc + cb <= (byte *)pvDst || (byte *)pvDst + cb <= (byte *)pvSrc);
	memcpy(pvDst, pvSrc, cb);
}


/*----------------------------------------------------------------------------------------------
	This copies cn ints from pvSrc to pvDst. It _doesn't_ handle overlapping blocks.
----------------------------------------------------------------------------------------------*/
inline void CopyInts(const void * pvSrc, void * pvDst, int cn)
{
	Assert((int *)pvSrc + cn <= (int *)pvDst || (int *)pvDst + cn <= (int *)pvSrc);
	memcpy(pvDst, pvSrc, cn * isizeof(int));
}


/*----------------------------------------------------------------------------------------------
	This copies cv items from pvSrc to pvDst. It _doesn't_ handle overlapping blocks. It
	doesn't invoke assignment operators - it just moves bytes.
----------------------------------------------------------------------------------------------*/
template<typename T>
	inline void CopyItems(const T * pvSrc, T * pvDst, int cv)
{
	Assert(pvSrc + cv <= pvDst || pvDst + cv <= pvSrc);
	memcpy(pvDst, pvSrc, cv * isizeof(T));
}


/*----------------------------------------------------------------------------------------------
	This swaps the values of two variables of type T. It invokes assignment operators.
----------------------------------------------------------------------------------------------*/
template<typename T>
	inline void SwapVars(T & t1, T & t2)
{
	T tT = t1;
	t1 = t2;
	t2 = tT;
}


/*----------------------------------------------------------------------------------------------
	Other memory moving functions.
----------------------------------------------------------------------------------------------*/
void ReverseBytes(void * pv, int cb);
void ReverseInts(void * pv, int cn);
void SwapBlocks(void * pv, int cb1, int cb2);
void SwapBytes(void * pv1, void * pv2, int cb);
void MoveElement(void * pv, int cbElement, int ivSrc, int ivTarget);

}

/***********************************************************************************************
	New operators.
***********************************************************************************************/

#if 0
//#ifdef DEBUG

	/*__declspec(dllimport)*/ bool WINAPI CanAllocate();
	__declspec(dllimport) int WINAPI DisableNew(bool f);
	__declspec(dllimport) int WINAPI DisableNewAfter(int cnew);

	inline void * __cdecl operator new(size_t cb, bool fClear, int cbExtra,
		const char * pszFile, int nLine)
	{
		if (!CanAllocate() || cb + cbExtra < cb)
			return NULL;
		void * pv = ::operator new(cb + cbExtra, _NORMAL_BLOCK, pszFile, nLine);
		if (!pv)
			ThrowHr(WarnHr(E_OUTOFMEMORY));
		if (fClear)
			ClearBytes(pv, cb + cbExtra);
		return pv;
	}
	inline void __cdecl operator delete(void * pv, bool fClear, int cbExtra,
		const char * pszFile, int nLine)
	{
		::operator delete(pv);
	}
	inline void * __cdecl operator new(size_t cb, bool fClear, const char * pszFile, int nLine)
	{
		if (!CanAllocate())
			return NULL;
		void * pv = ::operator new(cb, _NORMAL_BLOCK, pszFile, nLine);
		if (!pv)
			ThrowHr(WarnHr(E_OUTOFMEMORY));
		if (fClear)
			ClearBytes(pv, cb);
		return pv;
	}
	inline void __cdecl operator delete(void * pv, bool fClear, const char * pszFile, int nLine)
	{
		::operator delete(pv);
	}
	inline void * __cdecl DebugRealloc(void * pv, size_t cb)
	{
		return CanAllocate() ? realloc(pv, cb) : NULL;
	}
	inline void * __cdecl DebugCalloc(size_t cv, size_t cb)
	{
		return CanAllocate() ? calloc(cv, cb) : NULL;
	}
	inline void * __cdecl DebugMalloc(size_t cb)
	{
		return CanAllocate() ? malloc(cb) : NULL;
	}
	inline void * __cdecl DebugCoTaskMemAlloc(size_t cb)
	{
		return CanAllocate() ? CoTaskMemAlloc(cb) : NULL;
	}
	inline void * __cdecl DebugCoTaskMemRealloc(void * pv, size_t cb)
	{
		return CanAllocate() ? CoTaskMemRealloc(pv, cb) : NULL;
	}
	inline BSTR __cdecl DebugSysAllocString(const OLECHAR * pwsz)
	{
		return CanAllocate() ? SysAllocString(pwsz) : NULL;
	}
	inline BSTR __cdecl DebugSysAllocStringLen(const OLECHAR * prgwch, size_t cch)
	{
		return CanAllocate() ? SysAllocStringLen(prgwch, cch) : NULL;
	}
	inline BSTR __cdecl DebugSysAllocStringByteLen(const char * prgch, size_t cb)
	{
		return CanAllocate() ? SysAllocStringByteLen(prgch, cb) : NULL;
	}
	inline int __cdecl DebugSysReAllocString(BSTR * pbstr, const OLECHAR * pwsz)
	{
		return CanAllocate() ? SysReAllocString(pbstr, pwsz) : FALSE;
	}
	inline int __cdecl DebugSysReAllocStringLen(BSTR * pbstr, const OLECHAR * prgwch,
		size_t cch)
	{
		return CanAllocate() ? SysReAllocStringLen(pbstr, prgwch, cch) : FALSE;
	}

	#define NewObj 			new(true, THIS_FILE, __LINE__)
	#define NewObjExtra(cb)	new(true, cb, THIS_FILE, __LINE__)
	#define NewObjNoClea	new(false, THIS_FILE, __LINE__)

	#define realloc(pv, cb)				DebugRealloc(pv, cb)
	#define calloc(cv, cb)				DebugCalloc(cv, cb)
	#define malloc(cb)					DebugMalloc(cb)

	#define CoTaskMemAlloc(cb)			DebugCoTaskMemAlloc(cb)
	#define CoTaskMemRealloc(pv, cb)	DebugCoTaskMemRealloc(pv, cb)

	#define SysAllocString(pwsz)					DebugSysAllocString(pwsz)
	#define SysAllocStringLen(prgwch, cch)			DebugSysAllocStringLen(prgwch, cch)
	#define SysAllocStringByteLen(prgch, cb)		DebugSysAllocStringByteLen(prgch, cb)
	#define SysReAllocString(pbstr, pwsz)			DebugSysReAllocString(pbstr, pwsz)
	#define SysReAllocStringLen(pbstr, prgwch, cch)	DebugSysReAllocStringLen(pbstr, prgwch, cch)

	struct MemoryBlocker
	{
		MemoryBlocker()
			{ DisableNew(true); }
		~MemoryBlocker()
			{ DisableNew(false); }
	};

	struct MemoryLimit
	{
		MemoryLimit(int cnew)
			{ DisableNewAfter(cnew); }
		~MemoryLimit()
			{ DisableNewAfter(-1); }
	};


#else // !DEBUG
#if 1
	inline void * __cdecl operator new(size_t cb, bool fClear, int cbExtra)
	{
		if (cb + cbExtra < cb)
			gr::ThrowHr(WarnHr(E_INVALIDARG));
		void * pv = ::operator new(cb + cbExtra, std::nothrow);
		if (!pv)
			gr::ThrowHr(WarnHr(E_OUTOFMEMORY));
		if (fClear)
			gr::ClearBytes(pv, cb + cbExtra);
		return pv;
	}
	inline void __cdecl operator delete(void * pv, bool fClear, int cbExtra)
	{
		::operator delete(pv);
	}
#ifndef _MSC_VER
	inline void * __cdecl operator new [](size_t cb, bool fClear, int cbExtra)
	{
		if (cb + cbExtra < cb)
			gr::ThrowHr(WarnHr(E_INVALIDARG));
		void * pv = ::operator new [](cb + cbExtra, std::nothrow);
		if (!pv)
			gr::ThrowHr(WarnHr(E_OUTOFMEMORY));
		if (fClear)
			gr::ClearBytes(pv, cb + cbExtra);
		return pv;
	}
	inline void __cdecl operator delete [](void * pv, bool fClear, int cbExtra)
	{
		::operator delete [](pv);
	}
#endif
	inline void * __cdecl operator new(size_t cb, bool fClear)
	{
		void * pv = ::operator new(cb, std::nothrow);
		if (!pv)
			gr::ThrowHr(WarnHr(E_OUTOFMEMORY));
		if (fClear)
			gr::ClearBytes(pv, cb);
		return pv;
	}
	inline void __cdecl operator delete(void * pv, bool fClear)
	{
		::operator delete(pv);
	}
#ifndef _MSC_VER

	inline void * __cdecl operator new[](size_t cb, bool fClear)
	{
		void * pv = ::operator new [](cb, std::nothrow);
		if (!pv)
			gr::ThrowHr(WarnHr(E_OUTOFMEMORY));
		if (fClear)
			gr::ClearBytes(pv, cb);
		return pv;
	}
	inline void __cdecl operator delete[](void * pv, bool fClear)
	{
		::operator delete [](pv);
	}
#endif
	#define NewObj			new(true)
	#define NewObjExtra(cb)	new(true, cb)
	#define NewObjNoClear	new(false)
#else
	#define NewObj new
	#define NewObjNoClear new
#endif
#endif // !DEBUG

#if !defined (GR_NAMESPACE)
using namespace gr;
#endif

#endif // !UtilMem_H

