/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GrcFont.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    A class to access the font.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef GRC_FONT_INCLUDED
#define GRC_FONT_INCLUDED

#ifdef UINT_MAX
#define GRCFONT_END UINT_MAX
#else
#define GRCFONT_END -1
#endif 

/*----------------------------------------------------------------------------------------------
Class: GrcFont
Description: A class representing the font file and used to access its information.
				None of the methods with wGlyphID as an argument can handle pseudo glyph ids.
Hungarian: font
----------------------------------------------------------------------------------------------*/

class GrcFont
{
public:
	GrcFont(char * pchFileName);
	GrcFont(bool fDebug);

	~GrcFont(); //Review: should this be virtual?
	int Init(GrcManager *); // must call before using any of the below methods; clean up handled by dtor

	void GetFontFamilyName(utf16 * rgchwName, size_t cchMax);

	utf16 FirstFreeGlyph();
	size_t AutoPseudos(std::vector<unsigned int> & vnUnicode, std::vector<utf16> & vwGlyphID);

	void GetGlyphsFromCmap(utf16 * rgchwUniToGlyphID);
	unsigned int UnicodeFromCodePage(utf16 wCodePage, utf16 wCodePoint, GdlObject * pgdlobj);
	utf16 GlyphFromCmap(unsigned int nUnicode, GdlObject * pgdlobj);
	utf16 GlyphFromPostscript(std::string staPostscriptName, GdlObject * pgdlobj, bool fError);

	int ConvertGPathToGPoint(gid16 wGlyphID, int nPathNumber, GdlObject * pgdlobj);

	int ScaledToAbsolute(int nValue, int mScale);
	int DesignUnits();

	int GetGlyphMetric(gid16 wGlyphID, GlyphMetric gmet, GdlObject * pgdlobj);

	bool IsPointAlone(gid16 wGlyphID, int nPointNumber, GdlObject * pgdlobj);
	int GetXYAtPoint(gid16 wGlyphID, int nPointNumber, int * mX, int * mY, GdlObject * pgdlobj);
	int GetPointAtXY(gid16 wGlyphID, int mX, int mY, int mPointRadius, GdlObject * pgdlobj);

	bool IsSpace(gid16 wGlyphID)
	{
		return TtfUtil::IsSpace(wGlyphID, m_pLoca, m_cLoca, m_pHead);
	}

	// Class for iterating over the potentially wide range of Unicode codepoints in the cmap.
	class iterator
	{
		friend class GrcFont;
	public:
		iterator() // default iterator
		{}

		iterator(GrcFont * pfont, bool fAtEnd = false)
		{
			m_pfont = pfont;
			if (fAtEnd)
			{
				m_iBlock = static_cast<unsigned int>(m_pfont->CBlocks());
				m_nUni = GRCFONT_END;
			}
			else
			{
				m_nUni = m_pfont->m_vnMinUnicode[0];
				m_iBlock = 0;
			}
		}

		iterator & operator ++()
		{
			Assert(m_nUni != GRCFONT_END);
			Assert(m_iBlock < m_pfont->CBlocks() || m_nUni < m_pfont->m_vnLimUnicode[m_iBlock]);
			Assert(m_nUni < m_pfont->m_vnLimUnicode[m_iBlock]);

			m_nUni++;
			if (m_nUni >= m_pfont->m_vnLimUnicode[m_iBlock])
			{
				m_iBlock++;
				if (m_iBlock >= m_pfont->CBlocks())
					m_nUni = GRCFONT_END; // at end
				else
					m_nUni = m_pfont->m_vnMinUnicode[m_iBlock];
			}

			return *this;
		}

		bool operator == (const iterator & fit)
		{
			return (this->m_nUni == fit.m_nUni);
		}
		bool operator != (const iterator & fit)
		{
			return (this->m_nUni != fit.m_nUni);
		}

		unsigned int operator *()
		{
			return m_nUni;
		}

	protected:
		GrcFont * m_pfont;
		unsigned int m_nUni;   // current unicode codepoint
		unsigned int m_iBlock; // which block of unicode is current
	};

	friend class iterator;

	// iterators
	iterator Begin()
	{
		iterator fit(this, false);
		return fit;
	}
	iterator End()
	{
		iterator fit(this, true);
		return fit;
	}

	int NumUnicode()
	{
		return m_cnUnicode;
	}

	bool AnySupplementaryPlaneChars()
	{
		return (m_vnLimUnicode[m_vnLimUnicode.size() - 1] > 0xFFFF);
	}

protected:
	int OpenFile(void);
	int CloseFile(void);
	int ReadData(gr::byte ** ppData, ptrdiff_t lnOffset, size_t lnSize);
	int ReadTable(TableId ktiTableId, void * pHdr, void * pTableDir, gr::byte ** ppTable, size_t * plnSize);
	int ReadTable(gr::byte*& pTable);

	bool IsGraphiteFont(void * pHdr, void * pTableDir);
	int ScanGlyfIds(void);
	int GetGlyfContours(gid16 wGlyphID, std::vector<int> * pvnEndPt);

public:
	int GetGlyfPts(gid16 wGlyphID, std::vector<int> * pvnEndPt, 
		std::vector<int> * pvnX, std::vector<int> * pvnY, std::vector<bool> * pvfOnCurve);

protected:
	//	Member variables:

	char *m_pchFileName; // Review: should this use a string class
	FILE *m_pFile;
	
	gr::byte * m_pCmap;
	size_t m_cCmap;
	gr::byte * m_pGlyf;
	size_t m_cGlyf;
	gr::byte * m_pHead;
	size_t m_cHead;
	gr::byte * m_pHhea;
	size_t m_cHhea;
	gr::byte * m_pHmtx;
	size_t m_cHmtx;
	gr::byte * m_pLoca;
	size_t m_cLoca;
	gr::byte * m_pMaxp;
	size_t m_cMaxp;
	gr::byte * m_pOs2;
	size_t m_cOs2;
	gr::byte * m_pPost;
	size_t m_cPost;
	gr::byte * m_pName;
	size_t m_cName;

	// point to MS cmap subtables within m_pCmap for MS data
	// try to use the 3-10 pointer first. this is for MS UCS-4 encoding (UTF-32)
	void * m_pCmap_3_10;
	// the 3_1 pointer is for MS Unicode encoding (UTF-16)
	// it should be present even if the 3-10 subtable is also present
	// this could point to a 3-0 table instead of a 3-1 table though 3-1 is attempted first
	void * m_pCmap_3_1; 
	int m_nMaxGlyfId;

	// ranges of unicode codepoints in the cmap
	std::vector<unsigned int> m_vnMinUnicode;
	std::vector<unsigned int> m_vnLimUnicode;
	int m_cnUnicode;

	std::vector<unsigned int> m_vnCollisions; // Unicode ids with colliding glyph ids

	bool m_fDebug;

	// for interator
	size_t CBlocks()
	{
		Assert(m_vnMinUnicode.size() == m_vnLimUnicode.size());
		return m_vnMinUnicode.size();
	}
};

#endif // GRC_FONT_INCLUDED
