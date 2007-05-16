/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001, 2005 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: FontStub.cpp
Responsibility: Sharon Correll

Description:
	Stub definition of the Font class, just enough to get rid of link errors.
----------------------------------------------------------------------------------------------*/

#include "GrcRtFileFont.h"
#include <stdio.h>


namespace gr
{

/*----------------------------------------------------------------------------------------------
	Return uniquely identifying information that will be used a the key for this font
	in the font cache. This includes the font face name and the bold and italic flags.
----------------------------------------------------------------------------------------------*/
void Font::UniqueCacheInfo(std::wstring & stuFace, bool & fBold, bool & fItalic)
{
	size_t cbSize;
	const byte * pNameTbl = static_cast<const byte *>(getTable(TtfUtil::TableIdTag(ktiName), &cbSize));
	long lOffset, lSize;
	if (!TtfUtil::Get31EngFamilyInfo(pNameTbl, lOffset, lSize))
	{
		// TODO: try to find any name in any arbitrary language.
		Assert(false);
		return;
	}
	// byte * pvName = (byte *)pNameTbl + lOffset;
	utf16 rgchwFace[128];
	const size_t cchw = min(long(lSize / sizeof(utf16)), long(sizeof rgchwFace - 1));
	const utf16 *src_start = reinterpret_cast<const utf16 *>(pNameTbl+ lOffset);
	std::copy(src_start, src_start + cchw, rgchwFace);
	rgchwFace[cchw] = 0;  // zero terminate
	TtfUtil::SwapWString(rgchwFace, cchw);
	stuFace.assign(rgchwFace, rgchwFace + cchw);

	const void * pOs2Tbl = getTable(TtfUtil::TableIdTag(ktiOs2), &cbSize);
	TtfUtil::FontOs2Style(pOs2Tbl, fBold, fItalic);
	// Do we need to compare the results from the OS2 table with the italic flag in the
	// head table? (There is no requirement that they be consistent!)
}

/*----------------------------------------------------------------------------------------------
	A default unhinted implmentation of getGlyphPoint(..)
----------------------------------------------------------------------------------------------*/
void Font::getGlyphPoint(gid16 glyphID, unsigned int pointNum, Point & pointReturn)
{
	Assert(false);	// should not be called
	
	// Default values 
	pointReturn.x = 0;
	pointReturn.y = 0;

	// this isn't used very often, so don't bother caching
	size_t cbLocaSize = 0;
	const void * pGlyf = getTable(TtfUtil::TableIdTag(ktiGlyf), &cbLocaSize);
	if (pGlyf == 0)	return;

	const void * pHead = getTable(TtfUtil::TableIdTag(ktiHead), &cbLocaSize);
	if (pHead == 0) return;

	const void * pLoca = getTable(TtfUtil::TableIdTag(ktiLoca), &cbLocaSize);
	if (pLoca == 0)	return;

	const size_t MAX_CONTOURS = 32;
	int cnPoints = MAX_CONTOURS;
	bool rgfOnCurve[MAX_CONTOURS];
	int prgnX[MAX_CONTOURS];
	int prgnY[MAX_CONTOURS];
	
	if (TtfUtil::GlyfPoints(glyphID, pGlyf, pLoca, 
		cbLocaSize, pHead, 0, 0, 
		prgnX, prgnY, rgfOnCurve, cnPoints))
	{
		float nPixEmSquare;
		getFontMetrics(0, 0, &nPixEmSquare);

		const float nDesignUnitsPerPixel =  float(TtfUtil::DesignUnits(pHead)) / nPixEmSquare;
		pointReturn.x = prgnX[pointNum] / nDesignUnitsPerPixel;
		pointReturn.y = prgnY[pointNum] / nDesignUnitsPerPixel;
	}
}


/*----------------------------------------------------------------------------------------------
	A default unhinted implmentation of getGlyphMetrics(..)
----------------------------------------------------------------------------------------------*/
void Font::getGlyphMetrics(gid16 glyphID, gr::Rect & boundingBox, gr::Point & advances)
{
	Assert(false);	// should not be called
	
	// Setup default return values in case of failiure.
	boundingBox.left = 0;
	boundingBox.right = 0;
	boundingBox.bottom = 0;
	boundingBox.top = 0;
	advances.x = 0;
	advances.y = 0;

	// get the necessary tables.
	size_t locaSize, hmtxSize;
	const void * pHead = getTable(TtfUtil::TableIdTag(ktiHead), &locaSize);
	if (pHead == 0) return;

	const void * pHmtx = getTable(TtfUtil::TableIdTag(ktiHmtx), &hmtxSize);
	if (pHmtx == 0) return;

	// Calculate the number of design units per pixel.
	float pixelEmSquare;
	getFontMetrics(0, 0, &pixelEmSquare);
	const float designUnitsPerPixel = 
		float(TtfUtil::DesignUnits(pHead)) / pixelEmSquare;

	// Use the Hmtx and Head tables to find the glyph advances.
	int lsb, advance = 0;
	if (TtfUtil::HorMetrics(glyphID, pHmtx, hmtxSize, pHead, 
			lsb, advance))
	{
		advances.x = (advance / designUnitsPerPixel);
		advances.y = 0.0f;		
	}

	const void * pGlyf = getTable(TtfUtil::TableIdTag(ktiGlyf), &locaSize);
	if (pGlyf == 0)	return;

//	const void * pHhea = getTable(TtfUtil::TableIdTag(ktiHhea), &locaSize);
//	if (pHhea == 0)	return;

	const void * pLoca = getTable(TtfUtil::TableIdTag(ktiLoca), &locaSize);
	if (pLoca == 0)	return;

	// Fetch the glyph bounding box, GlyphBox may return false for a 
	// whitespace glyph.
	// Note that using GlyfBox here allows attachment points (ie, points lying outside
	// the glyph's outline) to affect the bounding box, which might not be what we want.
	int xMin, xMax, yMin, yMax;
	if (TtfUtil::GlyfBox(glyphID, pGlyf, pLoca, locaSize, pHead,
			xMin, yMin, xMax, yMax))
	{
		boundingBox.left = (xMin / designUnitsPerPixel);
		boundingBox.bottom = (yMin / designUnitsPerPixel);
		boundingBox.right = (xMax / designUnitsPerPixel);
		boundingBox.top = (yMax / designUnitsPerPixel);
	}
}

/*----------------------------------------------------------------------------------------------
	Copy constructor.
----------------------------------------------------------------------------------------------*/
Font::Font(const Font & fontSrc)
{
	Assert(false);	// not implemented
}

/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
Font::~Font()
{
}

} // namespace gr