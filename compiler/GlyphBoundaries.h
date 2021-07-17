/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GlyphBoundaries.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    A data structure that approximates the boundaries of the glyph.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef GLYPHBOUND_INCLUDED
#define GLYPHBOUND_INCLUDED


const double gbcUndef = 123456.7;	// undefined value - hopefully this is large enough that
									//it will never be used for real

/*----------------------------------------------------------------------------------------------
Class: GlyphBoundCell
Description: A single cell of the glyph boundaries. 
Hungarian: gbcell
----------------------------------------------------------------------------------------------*/

class GlyphBoundaryCell
{
	friend class GlyphBoundaries;

	enum GbcIndices {
		// indices into enter/exit arrays:
		gbcMin = 0,
		gbcMax = 1,
	};

protected:
	GlyphBoundaryCell();
	void Initialize();
	void AddEntryMinMax(int gbcSide, int mVal);
	void AddExitMinMax( int gbcSide, int mVal);
	bool HasData();
	bool HasEntry(int nSide);
	bool HasExit(int nSide);

protected:
	// Instance variables:
	float m_dValues[8];	// 8: left, right, bottom, top, four diagonals

	// Where the glyph curve enters and exits the cell on each side;
	// these are in em-units
	int m_mEntry[4][2];	// 4: left, right, bottom, top, 2: min, max
	int m_mExit[4][2];
};


/*----------------------------------------------------------------------------------------------
Class: GlyphBoundaries
Description: A data structure that approximates the boundaries of the glyph
Hungarian: gbdy
----------------------------------------------------------------------------------------------*/

class GlyphBoundaries
{
	friend class GlyphBoundaryCell;

	enum {
		gbgridCellsH = 4,	// number of horizontal cells
		gbgridCellsV = 4	// number of vertical cells
	};

	enum GbcIndices {
		gbcLeft = 0,
		gbcRight,
		gbcBottom,
		gbcTop,
		gbcDPMin,	// minimal positively-sloped diagonal
		gbcDPMax,	// maximal positively-sloped diagonal
		gbcDNMin,	// minimal negatively-sloped diagonal
		gbcDNMax,	// maximal negatively-sloped diagonal
	};

public:
	//	Constructors & destructors:
	//GlyphBoundaries(GrcFont * pfont, gid16 wGlyphID)
	//	: m_pfont(pfont)
	//{
	//}

	~GlyphBoundaries()
	{
	}

	void Initialize(gid16 wGlyphID)
	{
		m_wGlyphID = wGlyphID;
	}

	void OverlayGrid(GrcFont * pfont, bool fComplex);
	int CellGridBitmap();

protected:
	// Cell data is in the order: (0,0), (1,0), (2,0), (3,0), (1,0), (1,1), ...
	// where (0,0) is the lower left cell, (3,3) is the upper right.
	int CellIndex(int x, int y) { return (y * gbgridCellsH) + x; }

	void NormalizePoint(int mx, int my, float * pdx, float * pdy);
	void UnnormalizePoint(float dx, float dy, int * pmx, int * pmy);
	void NormalizeSumAndDiff(int mSum, int mDiff, float * pdSum, float * pdDiff);
	void UnnormalizeSumAndDiff(float dSum, float dDiff, int * pmSum, int * pmDiff);
	void AddPoint(int icellX, int icellY, int mx, int my, float dx, float dy, bool fEntire);
	void AddEntryMinMax(int icellX, int icellY, int gbcSide, int mVal);
	void AddExitMinMax( int icellX, int icellY, int gbcSide, int mVal);

	void ClearSubBoxCells();
	void RoundSubBoxCells();

	// Compiler:
public:
	int OutputToGlat(GrcBinaryStream * pbstrm);
	static int OutputToGlatNonexistent(GrcBinaryStream * pbstrm);
	bool ComplexFit();
	void DebugXml(std::ofstream & strmOut);
protected:
	int OutputGlatFullDiagonals(GrcBinaryStream * pbstrm);
	int OutputGlatSubBox(GrcBinaryStream * pbstrm, int icellX, int icellY);

protected:
	//	Instance variables:
	GrcFont * m_pfont;
	utf16 m_wGlyphID;		// the glyph these are the boundaries for
	// Cache glyph metrics:
	int m_mxBbMin;
	int m_mxBbMax;
	int m_myBbMin;
	int m_myBbMax;

	// Values for full glyph
	GlyphBoundaryCell m_gbcellEntire;

	// Sub-box cells
	GlyphBoundaryCell m_rggbcellSub[gbgridCellsH * gbgridCellsV];
};


#endif // GLYPHBOUND_INCLUDED
