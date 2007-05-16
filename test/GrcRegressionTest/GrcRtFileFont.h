/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 2007 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GrcRtFileFont.h
Responsibility: Sharon Correll

Description:
    The GrcRtFileFont class is based roughly on the FileFont class, but there is no table cache.
-------------------------------------------------------------------------------*//*:End Ignore*/
#include "GrCommon.h"
#include "GrData.h"
#ifndef _WIN32
#include "GrMstypes.h"
#endif
#include "GrDebug.h"

#include <fstream>
#include <iostream>

#include <vector>
#include <string>

// gAssert should be used for any kind of assertions that can be caused by a corrupted font,
// particularly those that won't be caught when loading the tables.
#define gAssert(x) Assert(x)

#include "GrResult.h"
#include "GrAppData.h"
#include "TtfUtil.h"
#include "Font.h"

#ifdef _MSC_VER
#pragma once
#endif
#ifndef RTFILEFONT_H
#define RTFILEFONT_H 1

#define NO_EXCEPTIONS 1

using namespace gr;

class GrcRtFileFont : public Font
{
public:
	GrcRtFileFont(std::string fileName, float pointSize,
		unsigned int dpiX, unsigned int dpiY);
	~GrcRtFileFont();

	Font * GrcRtFileFont::copyThis();
	virtual bool bold()		{ return false; }
	virtual bool italic()	{ return false; }
	virtual float ascent()	{ return m_ascent; }
	virtual float descent()	{ return m_descent; }
	virtual float height()	{ return m_ascent + m_descent; }
	virtual unsigned int getDPIx()	{ return m_dpiX; }
	virtual unsigned int getDPIy()	{ return m_dpiY; }

	const void * getTable(fontTableId32 tableID, size_t * pcbSize);
	void getFontMetrics(float * pAscent, float * pDescent, float * pEmSquare);

protected:
	void initializeFromFace();
	gr::byte * readTable(int /*TableId*/ tid, long & size);

	float scaleFromDpi(int dpi)
	{
		return (dpi * m_pointSize) / (72.0f * m_emSquare);
	}

	// Member variables:
	FILE *m_file;

	// KRS: I think these should be cached otherwise Segment::LineContextSegment doesn't work
	float m_ascent;
	float m_descent;
	float m_emSquare;
	float m_pointSize;
	int m_dpiX;
	int m_dpiY;
	bool m_isValid;
	std::wstring m_faceName;
	gr::byte * m_pHeader;
	gr::byte * m_pTableDir;
	float m_xScale;
	float m_yScale;
};


#endif // !RTFILEFONT_H

