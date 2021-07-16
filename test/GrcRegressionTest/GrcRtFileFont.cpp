/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001, 2005 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GrcRtFileFont.cpp
Responsibility: Sharon Correll

Description:
	
----------------------------------------------------------------------------------------------*/

//#include "FontTableCache.h"
#include "GrcRtFileFont.h"
#include <stdio.h>

// TBD do this properly
////#define FUDGE_FACTOR 72
////#define fix26_6(x) (x >> 6) + (x & 32 ? (x > 0 ? 1 : 0) : (x < 0 ? -1 : 0))

/*
GrcRtFileFont::GrcRtFileFont()
	: 
	m_file(NULL),
	m_pTableCache(NULL),
	m_ascent(0),
	m_descent(0),
	m_emSquare(0),
	m_pointSize(0),
	m_dpiX(FUDGE_FACTOR),
	m_dpiY(FUDGE_FACTOR),
	m_isValid(false),
	m_pHeader(NULL),
	m_pTableDir(NULL),
	m_xScale(1.0f),
	m_yScale(1.0f)
{
	m_fItalic = false;
	m_fBold = false;
	// colors are not used
	m_clrFore = (unsigned long)kclrBlack;
	m_clrBack = (unsigned long)kclrTransparent;
}

GrcRtFileFont::GrcRtFileFont(FILE * file, float pointSize, 
				   unsigned int dpiX, unsigned int dpiY)
	: 
	m_file(file),
	m_pTableCache(NULL),
	m_ascent(0),
	m_descent(0),
	m_emSquare(0),
	m_pointSize(pointSize),
	m_dpiX(dpiX),
	m_dpiY(dpiY),
	m_isValid(false),
	m_pHeader(NULL),
	m_pTableDir(NULL),
	m_xScale(1.0f),
	m_yScale(1.0f)
{
	Assert(m_file); // shouldn't be null
	initializeFromFace();
	
}

GrcRtFileFont::GrcRtFileFont(char * fileName, float pointSize, 
				   unsigned int dpiX, unsigned int dpiY)
	: 
	m_file(NULL),
	m_pTableCache(NULL),
	m_ascent(0),
	m_descent(0),
	m_emSquare(0),
	m_pointSize(pointSize),
	m_dpiX(dpiX),
	m_dpiY(dpiY),
	m_isValid(false),
	m_pHeader(NULL),
	m_pTableDir(NULL),
	m_xScale(1.0f),
	m_yScale(1.0f)
{
	Assert(fileName); // shouldn't be null but we play safe anyway
	m_file = fopen(fileName, "rb");

	initializeFromFace();
}
*/

GrcRtFileFont::GrcRtFileFont(std::string fileName, float pointSize, 
				   unsigned int dpiX, unsigned int dpiY)
	: 
	m_file(NULL),
	////m_pTableCache(NULL),
	m_ascent(0),
	m_descent(0),
	m_emSquare(0),
	m_pointSize(pointSize),
	m_dpiX(dpiX),
	m_dpiY(dpiY),
	m_isValid(false),
	m_pHeader(NULL),
	m_pTableDir(NULL),
	m_xScale(1.0f),
	m_yScale(1.0f)
{
	Assert(fileName.length()); // shouldn't be null but we play safe anyway
	m_file = fopen(fileName.c_str(), "rb");

	initializeFromFace();
}


void GrcRtFileFont::initializeFromFace()
{
	if (m_dpiY == 0)
		m_dpiY = m_dpiX;
	//m_fItalic = false;
	//m_fBold = false;
	//// colors are not used
	//m_clrFore = (unsigned long)kclrBlack;
	//m_clrBack = (unsigned long)kclrTransparent;

	if (m_file)
	{
		size_t lOffset;
		size_t lSize;
		TtfUtil::GetHeaderInfo(lOffset, lSize);
		m_pHeader = new gr::byte [lSize];
		m_isValid = true;
		if (!m_pHeader) 
		{
			m_isValid = false;
			return;
		}
		m_isValid = (fseek(m_file, lOffset, SEEK_SET) == 0);
		size_t bytesRead = fread(m_pHeader, 1, lSize, m_file);
		Assert(static_cast<int>(bytesRead) == lSize);
		m_isValid = TtfUtil::CheckHeader(m_pHeader);

		if (!m_isValid) return;
		m_isValid = TtfUtil::GetTableDirInfo(m_pHeader, lOffset, lSize);
		m_pTableDir = new gr::byte[lSize];
		if (!m_pTableDir) 
		{
			m_isValid = false;
			return;
		}
		// if lOffset hasn't changed this isn't needed
		fseek(m_file, lOffset, SEEK_SET);
		bytesRead = fread(m_pTableDir, 1, lSize, m_file);
		Assert(static_cast<int>(bytesRead) == lSize);
		
		// now read head tables
		m_isValid = TtfUtil::GetTableInfo(ktiOs2, m_pHeader, m_pTableDir, 
										  lOffset, lSize);
		if (!m_isValid) 
		{
			return;
		}
		gr::byte * pTable = readTable(ktiOs2, lSize);
		// get ascent, descent, style while we have the OS2 table loaded
		if (!m_isValid || !pTable) 
		{
			return;
		}
		bool fBold, fItalic; // temp
		m_isValid = TtfUtil::FontOs2Style(pTable, fBold, fItalic);
		m_ascent = static_cast<float>(TtfUtil::FontAscent(pTable));
		m_descent = static_cast<float>(TtfUtil::FontDescent(pTable));
		delete[] pTable; // OS2
		
		pTable = readTable(ktiName, lSize);
		if (!m_isValid || !pTable) 
		{
			return;
		}
		if (!TtfUtil::Get31EngFamilyInfo(pTable, lOffset, lSize))
		{
			// not English name
			m_isValid = false;
			return;
		}
		Assert(lSize %2 == 0);// should be utf16
		int cchw = (lSize / sizeof(utf16)) + 1;
		cchw = min(cchw, 128);
		utf16 * pTable16 = reinterpret_cast<utf16*>(pTable + lOffset);
		utf16 rgchwFace[128];
		std::copy(pTable16, pTable16 + cchw - 1, rgchwFace);
		rgchwFace[cchw - 1] = 0;  // zero terminate
		TtfUtil::SwapWString(rgchwFace, cchw - 1);
// We could use something like "if (sizeof(std::wstring::value_type) == 4)" here,
// but a compile-time switch is preferable.
////#if SIZEOF_WCHAR_T == 4
		// Quick and dirty utf16 -> wchar_t:
		for (int c16 = 0; c16 < cchw; c16++)
		{
			m_stu32FaceName.push_back((wchar_t)rgchwFace[c16]);
		}
////#else -- for some reason this won't build on Windows, but the above approach works okay
////		m_stu32FaceName.assign(const_cast<utf16 *>(rgchwFace));
////#endif
		delete[] pTable; // name

		pTable = readTable(ktiHead, lSize);
		if (!m_isValid || !pTable) 
		{
			return;
		}
		m_emSquare = static_cast<float>(TtfUtil::DesignUnits(pTable));
		// can now set the scale
		m_xScale = scaleFromDpi(m_dpiX);
		m_yScale = scaleFromDpi(m_dpiY);
		delete[] pTable; // head
	}
	else
	{
		m_isValid = false;
	}
}

// Unlike original (FileFont) version, caller is responsible for deleting the allocated buffer.
gr::byte * GrcRtFileFont::readTable(int /*TableId*/ tid, size_t & size)
{
	TableId tableId = static_cast<TableId>(tid);
	bool isValid = true;
	size_t lOffset = 0, lSize = 0;
	//if (!m_pTableCache)
	//{
	//	m_pTableCache = new FontTableCache();		
	//}
	//if (!m_pTableCache) return NULL;
	
	gr::byte * pTable = NULL; // m_pTableCache->getTable(tableId);
	size = 0; // m_pTableCache->getTableSize(tableId);
	// check whether it is already in the cache
	//if (pTable) return pTable;
	isValid &= TtfUtil::GetTableInfo(tableId, m_pHeader, m_pTableDir, 
									  lOffset, lSize);
	if (!isValid) 
		return NULL;
	fseek(m_file, lOffset, SEEK_SET);

	//pTable = m_pTableCache->allocateTable(tableId, lSize);
	pTable = new gr::byte[lSize]; 
	if (!pTable) 
	{
		isValid = false;
		return NULL;
	}
	size_t bytesRead = fread(pTable, 1, lSize, m_file);
	isValid = (static_cast<int>(bytesRead) == lSize);
	if (isValid)
	{
		isValid &= TtfUtil::CheckTable(tableId, pTable, lSize);
	}
	if (!isValid) 
	{
		return NULL;
	}
	size = lSize;
	return pTable;
}


GrcRtFileFont::~GrcRtFileFont()
{
	
	// if this is the last copy of the font sharing these tables
	// delete them
	//if (m_pTableCache)
	//{
	//	m_pTableCache->decrementFontCount();
	//	if (m_pTableCache->getFontCount() == 0)
	//	{
	//		delete [] m_pHeader;
	//		delete [] m_pTableDir;
	//		delete m_pTableCache;
	//		m_pTableCache = NULL;
	//		if (m_file) fclose(m_file);
	//	}
	//}
	//else
	//{
		delete [] m_pHeader;
		delete [] m_pTableDir;
		if (m_file) fclose(m_file);
	//}
	// note the DecFontCount(); is done in the Font base class
}

GrcRtFileFont * GrcRtFileFont::copyThis()
{
	GrcRtFileFont * copy = new GrcRtFileFont(*this);
	return copy;
}


/**
* A copy constructor that allows a change of point size or dpi
* The underlying table cache will be shared between the fonts, so
* this should have a low overhead.
*/
/****
GrcRtFileFont::GrcRtFileFont(const FileFont & font, float pointSize, 
				   unsigned int dpiX, unsigned int dpiY)
: Font(font),
	m_file(font.m_file),
	m_ascent(font.m_ascent),
	m_descent(font.m_descent),
	m_emSquare(font.m_emSquare),
	m_pointSize(font.m_pointSize),
	m_dpiX(font.m_dpiX),
	m_dpiY(font.m_dpiY),
	m_isValid(font.m_isValid),
	m_pHeader(font.m_pHeader),
	m_pTableDir(font.m_pTableDir),
	m_xScale(font.m_xScale),
	m_yScale(font.m_yScale)
{
	if (pointSize > 0)
	{
		m_pointSize = pointSize;
		if (dpiX > 0)
		{
			m_dpiX = dpiX;

			if (dpiY > 0) m_dpiY = dpiY;
			else dpiY = dpiX;
		}
		m_xScale = scaleFromDpi(m_dpiX);
		m_yScale = scaleFromDpi(m_dpiY);
	}
	m_fItalic = font.m_fItalic;
	m_fBold = font.m_fBold;
	// colors are not used
	m_clrFore = font.m_clrFore;
	m_clrBack = font.m_clrBack;
	m_faceName.assign(font.m_faceName);
	
	m_pTableCache = font.m_pTableCache;
	// use the same table cache between instances
	if (m_pTableCache)
		m_pTableCache->incrementFontCount();
	
	
	// I dont' see why we need to reget the face, but WinFont does
	//m_pfface = FontFace::GetFontFace(this, m_fpropDef.szFaceName,
	//			m_fpropDef.fBold, m_fpropDef.fItalic);
}

****/

//virtual FontErrorCode isValidForGraphite(int * pnVersion = NULL, int * pnSubVersion = NULL);

/*----------------------------------------------------------------------------------------------
	Read a table from the font.
----------------------------------------------------------------------------------------------*/

const void * GrcRtFileFont::getTable(fontTableId32 tableID, size_t * pcbSize)
{
	*pcbSize = 0;
	// use a cache to reduce the number of times tables have to be reloaded
	//if (m_pTableCache == NULL)
	//{
	//	// constructor automatically sets the font count to 1
	//	m_pTableCache = new FontTableCache();
	//} 
	//TableId tid;
	//for (int i = 0; i<ktiLast; i++)
	//{
	//	tid = static_cast<TableId>(i);
	//	if (tableID == TtfUtil::TableIdTag(tid))
	//	{
	//		if (m_pTableCache->getTable(tid))
	//		{
	//			*pcbSize = m_pTableCache->getTableSize(tid);
	//			return m_pTableCache->getTable(tid);
	//		}
	//		break;	
	//	}
	//}

	// Map backwards from the fontTableId32 to the kti enum (convoluted, but this is based
	// on the way FileFont was originally implemented):
	TableId tid;
	for (int i = 0; i < ktiLast; i++)
	{
		tid = static_cast<TableId>(i);
		if (tableID == TtfUtil::TableIdTag(tid))
			break;
	}
	Assert(tid < ktiLast);

	size_t tableSize = 0;
	void * pTable = readTable(tid, tableSize);
	*pcbSize = static_cast<int>(tableSize);
	return pTable;
}


void GrcRtFileFont::getFontMetrics(float * pAscent, float * pDescent,
		float * pEmSquare)
{
	if (pEmSquare)
		*pEmSquare = m_emSquare * m_xScale;
	if (pAscent)
		*pAscent = m_ascent * m_yScale;
	if (pDescent)
		*pDescent = m_descent * m_yScale;
}

/****
bool GrcRtFileFont::FontHasGraphiteTables(FILE * file)
{
	FileFont testFont(file, 1.0f, FUDGE_FACTOR);
	return testFont.fontHasGraphiteTables();
}

bool GrcRtFileFont::FontHasGraphiteTables(char * fileName)
{
	FileFont testFont(fileName, 1.0f, FUDGE_FACTOR);
	return testFont.fontHasGraphiteTables();
}

bool GrcRtFileFont::fontHasGraphiteTables()
{
	long tableSize;
	bool isGraphiteFont = m_isValid;
	isGraphiteFont &= (readTable(ktiSilf, tableSize) != NULL);
	return isGraphiteFont;
}
****/
