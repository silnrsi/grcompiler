/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GlyphBoundaries.cpp
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description
    A data structure that approximates the boundaries of the glyph.
-------------------------------------------------------------------------------*//*End Ignore*/

/***********************************************************************************************
	Include files
***********************************************************************************************/
#include "main.h"
#include <iomanip>

#ifdef _MSC_VER
#pragma hdrstop
#endif
#undef THIS_FILE
DEFINE_THIS_FILE

/***********************************************************************************************
	Forward declarations
***********************************************************************************************/

/***********************************************************************************************
	Local constants and static variables
***********************************************************************************************/

//	None


/***********************************************************************************************
	Methods: Constructor
***********************************************************************************************/
//GlyphBoundaries::GlyphBoundaries(GrcFont * pfont, utf16 wGlyphID) : m_pfont(pfont)
//{
//	// Cache some metrics?
//}

GlyphBoundaryCell::GlyphBoundaryCell()
{
	Initialize();
}

void GlyphBoundaryCell::Initialize()
{
	// Initialize min to something large:
	m_dValues[GlyphBoundaries::gbcLeft] = 2.0;
	m_dValues[GlyphBoundaries::gbcBottom] = 2.0;
	m_dValues[GlyphBoundaries::gbcDPMin] = 100.0;
	m_dValues[GlyphBoundaries::gbcDNMin] = 100.0;
	// Initialize max to something small:
	m_dValues[GlyphBoundaries::gbcRight] = 0;
	m_dValues[GlyphBoundaries::gbcTop] = 0;
	m_dValues[GlyphBoundaries::gbcDPMax] = -100.0;		// -1 should be good enough
	m_dValues[GlyphBoundaries::gbcDNMax] = -100.0;		// 0 should be good enough

	// Initialize intersections
	m_mEntry[GlyphBoundaries::gbcLeft][gbcMin] =   99999;
	m_mEntry[GlyphBoundaries::gbcRight][gbcMin] =  99999;
	m_mEntry[GlyphBoundaries::gbcBottom][gbcMin] = 99999;
	m_mEntry[GlyphBoundaries::gbcTop][gbcMin] =    99999;

	m_mExit[GlyphBoundaries::gbcLeft][gbcMin] =    99999;
	m_mExit[GlyphBoundaries::gbcRight][gbcMin] =   99999;
	m_mExit[GlyphBoundaries::gbcBottom][gbcMin] =  99999;
	m_mExit[GlyphBoundaries::gbcTop][gbcMin] =     99999;

	m_mEntry[GlyphBoundaries::gbcLeft][gbcMax] =   -99999;
	m_mEntry[GlyphBoundaries::gbcRight][gbcMax] =  -99999;
	m_mEntry[GlyphBoundaries::gbcBottom][gbcMax] = -99999;
	m_mEntry[GlyphBoundaries::gbcTop][gbcMax] =    -99999;

	m_mExit[GlyphBoundaries::gbcLeft][gbcMax] =    -99999;
	m_mExit[GlyphBoundaries::gbcRight][gbcMax] =   -99999;
	m_mExit[GlyphBoundaries::gbcBottom][gbcMax] =  -99999;
	m_mExit[GlyphBoundaries::gbcTop][gbcMax] =     -99999;

}


/***********************************************************************************************
	Methods: Pre-compiler
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Convert em-units to normalized values between 0 and 1.0, where 0 and 1 are the bounding
	box of the glyph (not the em-square).
----------------------------------------------------------------------------------------------*/
void GlyphBoundaries::NormalizePoint(int mx, int my, float * pdx, float * pdy)
{
	*pdx = float(mx - m_mxBbMin) / float(m_mxBbMax - m_mxBbMin);
	*pdy = float(my - m_myBbMin) / float(m_myBbMax - m_myBbMin);
}

/*----------------------------------------------------------------------------------------------
	Convert normalized points to em-units.
----------------------------------------------------------------------------------------------*/
void GlyphBoundaries::UnnormalizePoint(float dx, float dy, int * pmx, int * pmy)
{
	*pmx = int((dx * m_mxBbMax) + ((1 - dx) * m_mxBbMin));
    *pmy = int((dy * m_myBbMax) + ((1 - dy) * m_myBbMin));
}

/*----------------------------------------------------------------------------------------------
	Convert sums and differences that represent diagonal lines to normalized values.
	The coordinate system for each is the range of maximum and mimimum possible diagonals.
	For the sum, maximum and minimum diagonals are defined by the upper-right and lower-left
	points on the bounding box; for the diff, the range is defined by the lower-right and
	upper-left points.
----------------------------------------------------------------------------------------------*/
void GlyphBoundaries::NormalizeSumAndDiff(int mSum, int mDiff, float * pdSum, float * pdDiff)
{
	// Calculate the scaling factor for normalizing the diagonals.
	int mMinSum = m_mxBbMin + m_myBbMin;	// lower-left point
	int mMaxSum = m_mxBbMax + m_myBbMax;	// upper-right point
	int mMinDiff = m_mxBbMin - m_myBbMax;	// upper-left point
	int mMaxDiff = m_mxBbMax - m_myBbMin;	// lower-right point

	int mSumScale = mMaxSum - mMinSum;
	int mDiffScale = mMaxDiff - mMinDiff;
	Assert(mSumScale == mDiffScale);

	*pdSum = (float)(mSum - mMinSum) / mSumScale;
	*pdDiff = (float)(mDiff - mMinDiff) / mDiffScale;
}

/*----------------------------------------------------------------------------------------------
	Given an sum (x+y) and difference (x-y) in normalised space (x,y between 0 and 1),
	return the corresponding addition and subtraction in normal em space coordinate space.
----------------------------------------------------------------------------------------------*/
void GlyphBoundaries::UnnormalizeSumAndDiff(float dSum, float dDiff, int * pmSum, int * pmDiff)
{
	int mMinSum = m_mxBbMin + m_myBbMin;	// lower-left point
	int mMaxSum = m_mxBbMax + m_myBbMax;	// upper-right point
	int mMinDiff = m_mxBbMin - m_myBbMax;	// upper-left point
	int mMaxDiff = m_mxBbMax - m_myBbMin;	// lower-right point

	int mSumScale = mMaxSum - mMinSum;
	int mDiffScale = mMaxDiff - mMinDiff;
	Assert(mSumScale == mDiffScale);

	*pmSum = (int)(dSum * mSumScale) + mMinSum;
	*pmDiff = (int)(dDiff * mDiffScale) + mMinDiff;
}


// Version from Martin's original code:

//void GlyphBoundaries::UnnormalizeSumAndDiff(float dSum, float dDiff, int * pmSum, int * pmDiff)
//{
//    double mx = ((dSum + dDiff) * 0.5 * (m_mxBbMax - m_mxBbMin)) + m_mxBbMin;
//    double my = ((dSum - dDiff) * 0.5 * (m_myBbMax - m_myBbMin)) + m_myBbMin;
//	*pmSum = int(mx + my);
//	*pmDiff = int(mx - my);
//}

/*----------------------------------------------------------------------------------------------
	 Accumulates a point into a glyph or subglyph structure. We store the actual points, but
	 never use them. But we do use the accumulated maximal bounding box (bbox) and maximal
	 diamond box (dbox). If we are working with subboxes, we will also want to accumulate
	 for the whole glyph. Notice there is no need to accumulate a whole glyph bbox
	 because that's already defined for a glyph.
----------------------------------------------------------------------------------------------*/
void GlyphBoundaries::AddPoint(int icellX, int icellY, int mx, int my, float dx, float dy,
	bool fEntire)
{
	GlyphBoundaryCell * pgbcell = m_rggbcellSub + CellIndex(icellX, icellY);

    //pgbdy->m_vdx.push_back(dx);
	//pgbdy->m_vdy.push_back(dy);

	int mSum = mx + my;		// negative sloped lines at -45 degree angle
	int mDiff = mx - my;	// positive sloped lines at 45 degree angle
	float dSum, dDiff;
	NormalizeSumAndDiff(mSum, mDiff, &dSum, &dDiff);

	// Calculate mins and maxes.
    if (pgbcell->m_dValues[gbcLeft] == gbcUndef || dx < pgbcell->m_dValues[gbcLeft])
		pgbcell->m_dValues[gbcLeft] = dx;
    if (pgbcell->m_dValues[gbcRight] == gbcUndef || dx > pgbcell->m_dValues[gbcRight])
		pgbcell->m_dValues[gbcRight] = dx;
    if (pgbcell->m_dValues[gbcBottom] == gbcUndef || dy < pgbcell->m_dValues[gbcBottom])
		pgbcell->m_dValues[gbcBottom] = dy;
    if (pgbcell->m_dValues[gbcTop] == gbcUndef || dy > pgbcell->m_dValues[gbcTop])
		pgbcell->m_dValues[gbcTop] = dy;

    if (pgbcell->m_dValues[gbcDNMin] == gbcUndef || dSum < pgbcell->m_dValues[gbcDNMin])
		pgbcell->m_dValues[gbcDNMin] = dSum;
    if (pgbcell->m_dValues[gbcDNMax] == gbcUndef || dSum > pgbcell->m_dValues[gbcDNMax])
		pgbcell->m_dValues[gbcDNMax] = dSum;
    if (pgbcell->m_dValues[gbcDPMin] == gbcUndef || dDiff < pgbcell->m_dValues[gbcDPMin])
		pgbcell->m_dValues[gbcDPMin] = dDiff;
    if (pgbcell->m_dValues[gbcDPMax] == gbcUndef || dDiff > pgbcell->m_dValues[gbcDPMax])
		pgbcell->m_dValues[gbcDPMax] = dDiff;

    if (fEntire)
    {
		// Also add to the diamond for the entire glyph.
		if (m_gbcellEntire.m_dValues[gbcDNMin] == gbcUndef || dSum < m_gbcellEntire.m_dValues[gbcDNMin])
			m_gbcellEntire.m_dValues[gbcDNMin] = dSum;
		if (m_gbcellEntire.m_dValues[gbcDNMax] == gbcUndef || dSum > m_gbcellEntire.m_dValues[gbcDNMax])
			m_gbcellEntire.m_dValues[gbcDNMax] = dSum;
		if (m_gbcellEntire.m_dValues[gbcDPMin] == gbcUndef || dDiff < m_gbcellEntire.m_dValues[gbcDPMin])
			m_gbcellEntire.m_dValues[gbcDPMin] = dDiff;
		if (m_gbcellEntire.m_dValues[gbcDPMax] == gbcUndef || dDiff > m_gbcellEntire.m_dValues[gbcDPMax])
			m_gbcellEntire.m_dValues[gbcDPMax] = dDiff;
    }
}

/*----------------------------------------------------------------------------------------------
	Keep track of the minimal and maximum points where the glyph curve intersects the given
	cell.
----------------------------------------------------------------------------------------------*/
void GlyphBoundaries::AddEntryMinMax(int icellX, int icellY, int gbcSide, int mVal)
{
	GlyphBoundaryCell * pgbcell = m_rggbcellSub + CellIndex(icellX, icellY);
	pgbcell->AddEntryMinMax(gbcSide, mVal);
}

void GlyphBoundaries::AddExitMinMax(int icellX, int icellY, int gbcSide, int mVal)
{
	GlyphBoundaryCell * pgbcell = m_rggbcellSub + CellIndex(icellX, icellY);
	pgbcell->AddExitMinMax(gbcSide, mVal);
}

void GlyphBoundaryCell::AddEntryMinMax(int nSide, int mVal)
{
	if (mVal < m_mEntry[nSide][gbcMin])
		m_mEntry[nSide][gbcMin] = mVal;
	if (mVal > m_mEntry[nSide][gbcMax])
		m_mEntry[nSide][gbcMax] = mVal;
}

void GlyphBoundaryCell::AddExitMinMax(int nSide, int mVal)
{
	if (mVal < m_mExit[nSide][gbcMin])
		m_mExit[nSide][gbcMin] = mVal;
	if (mVal > m_mExit[nSide][gbcMax])
		m_mExit[nSide][gbcMax] = mVal;
}


/*----------------------------------------------------------------------------------------------
	 Does the cell have any data in it?
----------------------------------------------------------------------------------------------*/
bool GlyphBoundaryCell::HasData()
{
	// They are initialized such that right < left.
	return (m_dValues[GlyphBoundaries::gbcLeft] < m_dValues[GlyphBoundaries::gbcRight]);
}

/*----------------------------------------------------------------------------------------------
	 Does the cell have an entry/exit on the given side?
----------------------------------------------------------------------------------------------*/
bool GlyphBoundaryCell::HasEntry(int nSide)
{
	// They are initialized such that max < min.
	return (m_mEntry[nSide][gbcMin] <= m_mEntry[nSide][gbcMax]);
}

bool GlyphBoundaryCell::HasExit(int nSide)
{
	// They are initialized such that max < min.
	return (m_mExit[nSide][gbcMin] <= m_mExit[nSide][gbcMax]);
}

/*----------------------------------------------------------------------------------------------
	 Return a bitmap indicating which cells of the grid overlap with the curve.
	 Zero means we don't want subboxes for this glyph - it has a simple shape.
----------------------------------------------------------------------------------------------*/
int GlyphBoundaries::CellGridBitmap()
{
	int nBitmap = 0;
	for (int icellY = gbgridCellsV-1; icellY >= 0; icellY--)
	{
		for (int icellX = gbgridCellsH-1; icellX >= 0; icellX--)
		{
			nBitmap = nBitmap << 1;
			GlyphBoundaryCell * pgbcell = m_rggbcellSub + CellIndex(icellX, icellY);
			if (pgbcell->HasData())
				nBitmap = nBitmap + 1;
		}
	}
	return nBitmap;
}

/*----------------------------------------------------------------------------------------------
	 Develop a grid of octaboxes that correspond to the glyph curve, and also an octabox
	 for the entire glyph.
	 If fComplex is false, only the octabox for the entire glyph is needed.
----------------------------------------------------------------------------------------------*/
void GlyphBoundaries::OverlayGrid(GrcFont * pfont, bool fComplex)
{
	std::vector<int> vmx;
	std::vector<int> vmy;
	std::vector<int> viEndPt;
	std::vector<bool> vfOnCurve;	// not used

	const float ccellX = (float)gbgridCellsH;
	const float ccellY = (float)gbgridCellsV;

	m_mxBbMin = m_myBbMin = 99999;	// something very large
	m_mxBbMax = m_myBbMax = -99999;	// something very small
	if (pfont->IsSpace(m_wGlyphID))
	{
		// Leave metrics empty.
		m_gbcellEntire.m_dValues[gbcDNMin] = 0;
		m_gbcellEntire.m_dValues[gbcDNMax] = 0;
		m_gbcellEntire.m_dValues[gbcDPMin] = 0;
		m_gbcellEntire.m_dValues[gbcDPMax] = 0;
	}
	else if (pfont->GetGlyfPts(m_wGlyphID, &viEndPt, &vmx, &vmy, &vfOnCurve))
	{
		// Calculate bounding box, which is used to normalize all the octaboxes.
		for (int i = 0; i < signed(vmx.size()); i++)
		{
			m_mxBbMin = (vmx[i] < m_mxBbMin) ? vmx[i] : m_mxBbMin;
			m_mxBbMax = (vmx[i] > m_mxBbMax) ? vmx[i] : m_mxBbMax;
			m_myBbMin = (vmy[i] < m_myBbMin) ? vmy[i] : m_myBbMin;
			m_myBbMax = (vmy[i] > m_myBbMax) ? vmy[i] : m_myBbMax;
		}
		float dxMin, dyMin, dxMax, dyMax;
		NormalizePoint(m_mxBbMin, m_myBbMin, &dxMin, &dyMin);
		Assert(dxMin == 0);
		Assert(dyMin == 0);
		NormalizePoint(m_mxBbMax, m_myBbMax, &dxMax, &dyMax);
		Assert(dxMax == 1.0);
		Assert(dyMax == 1.0);

		// Now iterate over all the points again (even ones off-curve, since this is
		// just an approximation), and adjust the relevant cell's min/max values
		// accordingly.

		int icellXPrev = -1;
		int icellYPrev = -1;
		float dxPrev, dyPrev;
		int icurve = 0;

		for (int i = 0; i < signed(vmx.size()); i++)
		{
			int mx = vmx[i];
			int my = vmy[i];
			float dx, dy;
			NormalizePoint(mx, my, &dx, &dy); // dx, dy range from 0 .. 1.0.

			// Figure out which grid cell this point belongs in; indices are 0 .. 3.
			int icellX = int((dx * ccellX) - .001);
			int icellY = int((dy * ccellY) - .001);

			AddPoint(icellX, icellY, mx, my, dx, dy, true);
 
			// Figure out which direction we're moving: if this point is in a new cell,
			// adjust the enter/exit max and mins.
			if (icellXPrev != -1)
			{
				while (icellX > icellXPrev)
				{
					// Moving right. Handle the intersections on the right of the previous cell and the
					// left of this cell.
					
					// Interpolate between the two points.
					float dxBoundary = float((icellXPrev + 1) / ccellX); // border between two cells
					float dRatio = (dxBoundary - dxPrev) / (dx - dxPrev);
					float dyBoundary = (dRatio * dy) + ((1 - dRatio) * dyPrev);
					int icellYBoundary = int((dyBoundary * ccellY) - .001);
					int mxBoundary, myBoundary;
					UnnormalizePoint(dxBoundary, dyBoundary, &mxBoundary, &myBoundary);

					// Add the intersection to the cell on the left (which might not be the previous cell, because
					// the Y coordinate might have shifted quite a bit as well).
					AddPoint(icellXPrev, icellYBoundary, mxBoundary, myBoundary, dxBoundary, dyBoundary, false);
					// Also adjust the intersections based on the fact that the curve is leaving the old cell.
					AddExitMinMax(icellXPrev, icellYBoundary, gbcRight, myBoundary);

					// Advance to the next cell on the right which still might not be the final cell of the
					// of the line. But it is the neighboring cell.
					++icellXPrev;

					// Now indicate the line is entering this new cell from the left.
					AddPoint(icellXPrev, icellYBoundary, mxBoundary, myBoundary, dxBoundary, dyBoundary, false);
					AddEntryMinMax(icellXPrev, icellYBoundary, gbcLeft, myBoundary);
				}

				// Repeat above logic for the other three directions...

				while (icellX < icellXPrev)
				{
					// Moving left. Handle the intersections on the left of the previous cell and the
					// right of this cell.
					
					// Interpolate between the two points.
					float dxBoundary = float(icellXPrev / ccellX); // border between two cells
					float dRatio = (dxBoundary - dxPrev) / (dx - dxPrev);
					float dyBoundary = (dRatio * dy) + ((1 - dRatio) * dyPrev);
					int icellYBoundary = int((dyBoundary * ccellY) - .001);
					int mxBoundary, myBoundary;
					UnnormalizePoint(dxBoundary, dyBoundary, &mxBoundary, &myBoundary);

					// Add the intersection to the cell on the right (which might not be the previous cell, because
					// the Y coordinate might have shifted quite a bit as well).
					AddPoint(icellXPrev, icellYBoundary, mxBoundary, myBoundary, dxBoundary, dyBoundary, false);
					// Also adjust the intersections based on the fact that the curve is leaving the old cell.
					AddExitMinMax(icellXPrev, icellYBoundary, gbcLeft, myBoundary);

					// Advance to the next cell on the left which still might not be the final cell of the
					// of the line. But it is the neighboring cell.
					--icellXPrev;

					// Now indicate the line is entering this new cell from the left.
					AddPoint(icellXPrev, icellYBoundary, mxBoundary, myBoundary, dxBoundary, dyBoundary, false);
					AddEntryMinMax(icellXPrev, icellYBoundary, gbcRight, myBoundary);
				}

				while (icellY > icellYPrev)
				{
					// Moving up. Handle the intersections on the top of the previous cell and the
					// bottom of this cell.
					
					// Interpolate between the two points.
					float dyBoundary = float((icellYPrev + 1) / ccellY); // border between two cells
					float dRatio = (dyBoundary - dyPrev) / (dy - dyPrev);
					float dxBoundary = (dRatio * dx) + ((1 - dRatio) * dxPrev);
					int icellXBoundary = int((dxBoundary * ccellX) - .001);
					int mxBoundary, myBoundary;
					UnnormalizePoint(dxBoundary, dyBoundary, &mxBoundary, &myBoundary);

					// Add the intersection to the cell on the bottom (which might not be the previous cell, because
					// the X coordinate might have shifted quite a bit as well).
					AddPoint(icellXBoundary, icellYPrev, mxBoundary, myBoundary, dxBoundary, dyBoundary, false);
					// Also adjust the intersections based on the fact that the curve is leaving the old cell.
					AddExitMinMax(icellXBoundary, icellYPrev, gbcTop, mxBoundary);

					// Advance to the next cell above which still might not be the final cell of the
					// of the line. But it is the neighboring cell.
					++icellYPrev;

					// Now indicate the line is entering this new cell from the bottom.
					AddPoint(icellXBoundary, icellYPrev, mxBoundary, myBoundary, dxBoundary, dyBoundary, false);
					AddEntryMinMax(icellXBoundary, icellYPrev, gbcBottom, mxBoundary);
				}

				while (icellY < icellYPrev)
				{
					// Moving down. Handle the intersections on the bottom of the previous cell and the
					// top of this cell.
					
					// Interpolate between the two points.
					float dyBoundary = float(icellYPrev / ccellY); // border between two cells
					float dRatio = (dyBoundary - dyPrev) / (dy - dyPrev);
					float dxBoundary = (dRatio * dx) + ((1 - dRatio) * dxPrev);
					int icellXBoundary = int((dxBoundary * ccellX) - .001);
					int mxBoundary, myBoundary;
					UnnormalizePoint(dxBoundary, dyBoundary, &mxBoundary, &myBoundary);

					// Add the intersection to the cell on the top (which might not be the previous cell, because
					// the X coordinate might have shifted quite a bit as well).
					AddPoint(icellXBoundary, icellYPrev, mxBoundary, myBoundary, dxBoundary, dyBoundary, false);
					// Also adjust the intersections based on the fact that the curve is leaving the old cell.
					AddExitMinMax(icellXBoundary, icellYPrev, gbcBottom, mxBoundary);

					// Advance to the next cell below which still might not be the final cell of the
					// of the line. But it is the neighboring cell.
					--icellYPrev;

					// Now indicate the line is entering this new cell from the top.
					AddPoint(icellXBoundary, icellYPrev, mxBoundary, myBoundary, dxBoundary, dyBoundary, false);
					AddEntryMinMax(icellXBoundary, icellYPrev, gbcTop, mxBoundary);
				}
			}

			if (viEndPt[icurve] == i)
			{
				// Break the curve.
				icurve++;
				icellXPrev = -1;
				icellYPrev = -1;
			}
			else
			{
				// Continue the curve.
				icellXPrev = icellX;
				icellYPrev = icellY;
				dxPrev = dx;
				dyPrev = dy;
			}
		} // end of looping over points

		// Now figure out which corners need to be added based on the entry and exit points.

		// We want to add point at the lower left corner if the minimum entry on the baseline 
		// is less than (to the left of) the minimum exit (or there is no exit), or if 
		// looking at the left vertical grid line if the minimum exit is less than (below)
		// the minimum entry.

		int gbcMin = GlyphBoundaryCell::gbcMin;
		int gbcMax = GlyphBoundaryCell::gbcMax;

		int mx, my;
		for (int icellX = 0; icellX < gbgridCellsH; icellX++)
		{
			for (int icellY = 0; icellY < gbgridCellsV; icellY++)
			{
				GlyphBoundaryCell * pgbcell = m_rggbcellSub + CellIndex(icellX, icellY);
				// Bottom left
				int leftMinEntry = pgbcell->m_mEntry[gbcLeft][gbcMin];
				int leftMinExit = pgbcell->m_mExit[gbcLeft][gbcMin];
				int bottomMinEntry = pgbcell->m_mEntry[gbcBottom][gbcMin];
				int bottomMinExit = pgbcell->m_mExit[gbcBottom][gbcMin];

				if ((pgbcell->HasEntry(gbcLeft)
							&& (!pgbcell->HasExit(gbcLeft) || leftMinEntry < leftMinExit))
					|| (pgbcell->HasExit(gbcBottom)
							&& (!pgbcell->HasEntry(gbcBottom) || bottomMinExit < bottomMinEntry)))
				{
					UnnormalizePoint(float(icellX / ccellX), float(icellY / ccellY), &mx, &my);
					AddPoint(icellX, icellY, mx, my, float(icellX / ccellX), float(icellY / ccellY), false);
				}

				// Top left
				int leftMaxEntry = pgbcell->m_mEntry[gbcLeft][gbcMax];
				int leftMaxExit = pgbcell->m_mExit[gbcLeft][gbcMax];
				int topMinEntry = pgbcell->m_mEntry[gbcTop][gbcMin];
				int topMinExit = pgbcell->m_mExit[gbcTop][gbcMin];

				if ((pgbcell->HasExit(gbcLeft)
							&& (!pgbcell->HasEntry(gbcLeft) || leftMaxExit > leftMaxEntry))
					|| (pgbcell->HasEntry(gbcTop)
							&& (!pgbcell->HasExit(gbcTop) || topMinEntry < topMinExit)))
				{
					UnnormalizePoint(float(icellX / ccellX), float((icellY+1) / ccellY), &mx, &my);
					AddPoint(icellX, icellY, mx, my, float(icellX / ccellX), float((icellY+1) / ccellY), false);
				}

				// Bottom right
				int rightMinEntry = pgbcell->m_mEntry[gbcRight][gbcMin];
				int rightMinExit = pgbcell->m_mExit[gbcRight][gbcMin];
				int bottomMaxEntry = pgbcell->m_mEntry[gbcBottom][gbcMax];
				int bottomMaxExit = pgbcell->m_mExit[gbcBottom][gbcMax];

				if ((pgbcell->HasExit(gbcRight)
							&& (!pgbcell->HasEntry(gbcRight) || rightMinExit < rightMinEntry))
					|| (pgbcell->HasEntry(gbcBottom)
							&& (!pgbcell->HasExit(gbcBottom) || bottomMaxEntry > bottomMaxExit)))
				{
					UnnormalizePoint(float((icellX+1) / ccellX), float(icellY / ccellY), &mx, &my);
					AddPoint(icellX, icellY, mx, my, float((icellX+1) / ccellX), float(icellY / ccellY), false);
				}

				// Top right
				int rightMaxEntry = pgbcell->m_mEntry[gbcRight][gbcMax];
				int rightMaxExit = pgbcell->m_mExit[gbcRight][gbcMax];
				int topMaxEntry = pgbcell->m_mEntry[gbcTop][gbcMax];
				int topMaxExit = pgbcell->m_mExit[gbcTop][gbcMax];

				if ((pgbcell->HasEntry(gbcRight)
							&& (!pgbcell->HasExit(gbcRight) || rightMaxEntry > rightMaxExit))
					|| (pgbcell->HasExit(gbcTop)
							&& (!pgbcell->HasEntry(gbcTop) || topMaxExit > topMaxEntry)))
				{
					UnnormalizePoint(float((icellX+1) / ccellX), float((icellY+1) / ccellY), &mx, &my);
					AddPoint(icellX, icellY, mx, my, float((icellX+1) / ccellX), float((icellY+1) / ccellY), false);
				}
			}
		}

		//	If complex metrics were not requested, clear all the subboxes but leave the
		//	full-glyph diagonals.
		//	This is obviously not the most efficient way to implement this, but it is the
		//	easiest and least messy.
		if (!fComplex)
			ClearSubBoxCells();
	}
	else
	{
		// Leave metrics empty
		m_gbcellEntire.m_dValues[gbcDNMin] = 0;
		m_gbcellEntire.m_dValues[gbcDNMax] = 0;
		m_gbcellEntire.m_dValues[gbcDPMin] = 0;
		m_gbcellEntire.m_dValues[gbcDPMax] = 0;
	}

	RoundSubBoxCells();
}

/*----------------------------------------------------------------------------------------------
	 Return true if subbox octaboxes are needed for this glyph.
----------------------------------------------------------------------------------------------*/
bool GlyphBoundaries::ComplexFit()
{
	int nBitmap = this->CellGridBitmap();
	return (nBitmap != 0);
}

/*----------------------------------------------------------------------------------------------
	 Output glyph boundaries data for one glyph to the Glat table. Return the number
	 of bytes written.
----------------------------------------------------------------------------------------------*/
int GlyphBoundaries::OutputToGlat(GrcBinaryStream * pbstrm)
{
	int cb = 0;
	int nBitmap = this->CellGridBitmap();
	pbstrm->WriteShort(nBitmap);
	cb += 2;

	cb += OutputGlatFullDiagonals(pbstrm);

	for (int icellY = 0; icellY < gbgridCellsV; icellY++)
	{
		for (int icellX = 0; icellX < gbgridCellsH; icellX++)
		{
			cb += OutputGlatSubBox(pbstrm, icellX, icellY);
		}
	}
	return cb;
}

/*----------------------------------------------------------------------------------------------
	 Scale full-glyph octabox coordinates and output into the Glat table.
	 Note that we don't include the bounding box rectangle since it is stored elsewhere
	 in the font.
----------------------------------------------------------------------------------------------*/
int GlyphBoundaries::OutputGlatFullDiagonals(GrcBinaryStream * pbstrm)
{
	Assert(m_gbcellEntire.m_dValues[gbcDNMin] >= 0);
	Assert(m_gbcellEntire.m_dValues[gbcDNMax] >= 0);
	Assert(m_gbcellEntire.m_dValues[gbcDPMin] >= 0);
	Assert(m_gbcellEntire.m_dValues[gbcDPMax] >= 0);
	Assert(m_gbcellEntire.m_dValues[gbcDNMin] <= 1.0);
	Assert(m_gbcellEntire.m_dValues[gbcDNMax] <= 1.0);
	Assert(m_gbcellEntire.m_dValues[gbcDPMin] <= 1.0);
	Assert(m_gbcellEntire.m_dValues[gbcDPMax] <= 1.0);

	// The possible range for negatively sloped diagonals is [0 .. 1]. Scale them to [0 .. 255].
	int sDNMin = min(int(m_gbcellEntire.m_dValues[gbcDNMin] * 255), 255);
	int sDNMax = min(int(m_gbcellEntire.m_dValues[gbcDNMax] * 255), 255);
	int sDPMin = min(int(m_gbcellEntire.m_dValues[gbcDPMin] * 255), 255);
	int sDPMax = min(int(m_gbcellEntire.m_dValues[gbcDPMax] * 255), 255);

	pbstrm->WriteByte(sDNMin);
	pbstrm->WriteByte(sDNMax);
	pbstrm->WriteByte(sDPMin);
	pbstrm->WriteByte(sDPMax);

	return 4;
}

/*----------------------------------------------------------------------------------------------
	 Scale octabox sub-box coordinates and output into the Glat table.
----------------------------------------------------------------------------------------------*/
int GlyphBoundaries::OutputGlatSubBox(GrcBinaryStream * pbstrm, int icellX, int icellY)
{
	GlyphBoundaryCell * pgbcell = m_rggbcellSub + CellIndex(icellX, icellY);

	if (pgbcell->HasData())
	{
		Assert(pgbcell->m_dValues[gbcLeft]  >= 0);
		Assert(pgbcell->m_dValues[gbcRight] >= 0);
		Assert(pgbcell->m_dValues[gbcBottom]>= 0);
		Assert(pgbcell->m_dValues[gbcTop]   >= 0);
		Assert(pgbcell->m_dValues[gbcDNMin] >= 0);
		Assert(pgbcell->m_dValues[gbcDNMax] >= 0);
		Assert(pgbcell->m_dValues[gbcDPMin] >= 0);
		Assert(pgbcell->m_dValues[gbcDPMax] >= 0);
		Assert(pgbcell->m_dValues[gbcLeft]  <= 1.0);
		Assert(pgbcell->m_dValues[gbcRight] <= 1.0);
		Assert(pgbcell->m_dValues[gbcBottom]<= 1.0);
		Assert(pgbcell->m_dValues[gbcTop]   <= 1.0);
		Assert(pgbcell->m_dValues[gbcDNMin] <= 1.0);
		Assert(pgbcell->m_dValues[gbcDNMax] <= 1.0);
		Assert(pgbcell->m_dValues[gbcDPMin] <= 1.0);
		Assert(pgbcell->m_dValues[gbcDPMax] <= 1.0);

		// The possible range for horizontal and vertical boundaries is [0 .. 1]. Scale them to [0 .. 255].
		int sLeft   = min(int(pgbcell->m_dValues[gbcLeft]   * 255), 255);
		int sRight  = min(int(pgbcell->m_dValues[gbcRight]  * 255), 255);
		int sBottom = min(int(pgbcell->m_dValues[gbcBottom] * 255), 255);
		int sTop    = min(int(pgbcell->m_dValues[gbcTop]    * 255), 255);

		// The possible range for diagonals is [0 .. 1]. Scale them to [0 .. 255].
		int sDNMin = min(int(pgbcell->m_dValues[gbcDNMin] * 255), 255);
		int sDNMax = min(int(pgbcell->m_dValues[gbcDNMax] * 255), 255);
		int sDPMin = min(int(pgbcell->m_dValues[gbcDPMin] * 255), 255);
		int sDPMax = min(int(pgbcell->m_dValues[gbcDPMax] * 255), 255);

		pbstrm->WriteByte(sLeft);
		pbstrm->WriteByte(sRight);
		pbstrm->WriteByte(sBottom);
		pbstrm->WriteByte(sTop);

		pbstrm->WriteByte(sDNMin);
		pbstrm->WriteByte(sDNMax);
		pbstrm->WriteByte(sDPMin);
		pbstrm->WriteByte(sDPMax);

		return 8;
	}
	else
		return 0;
}

/*----------------------------------------------------------------------------------------------
	 Output glyph boundaries for a glyph that does not really exist in the font
	 (ie, a pseudo-glyph).
----------------------------------------------------------------------------------------------*/
int GlyphBoundaries::OutputToGlatNonexistent(GrcBinaryStream * pbstrm)
{
	// Empty bitmap
	pbstrm->WriteShort(0);
	// 4 bogus diagonals
	pbstrm->WriteByte(0);
	pbstrm->WriteByte(0);
	pbstrm->WriteByte(0);
	pbstrm->WriteByte(0);

	return 6;
}

/*----------------------------------------------------------------------------------------------
	 Clear all the data in the sub-box cells.
	 Currently is only called when we have a simple glyph for which we don't need sub-boxes.
----------------------------------------------------------------------------------------------*/
void GlyphBoundaries::ClearSubBoxCells()
{
	for (int icellX = 0; icellX < gbgridCellsH; icellX++)
	{
		for (int icellY = 0; icellY < gbgridCellsV; icellY++)
		{
			GlyphBoundaryCell * pgbcell = m_rggbcellSub + CellIndex(icellX, icellY);
			pgbcell->Initialize();
			Assert(!pgbcell->HasData());
		}
	}
}

/*----------------------------------------------------------------------------------------------
	 Ignore tiny rounding errors.
----------------------------------------------------------------------------------------------*/
void GlyphBoundaries::RoundSubBoxCells()
{
	for (int icellX = 0; icellX < gbgridCellsH; icellX++)
	{
		for (int icellY = 0; icellY < gbgridCellsV; icellY++)
		{
			GlyphBoundaryCell * pgbcell = m_rggbcellSub + CellIndex(icellX, icellY);
			pgbcell->m_dValues[GlyphBoundaries::gbcLeft] = (float)(((int)(pgbcell->m_dValues[GlyphBoundaries::gbcLeft] * 10000)) / 10000.0);
			pgbcell->m_dValues[GlyphBoundaries::gbcBottom] = (float)(((int)(pgbcell->m_dValues[GlyphBoundaries::gbcBottom] * 10000)) / 10000.0);
			pgbcell->m_dValues[GlyphBoundaries::gbcDPMin] = (float)(((int)(pgbcell->m_dValues[GlyphBoundaries::gbcDPMin] * 10000)) / 10000.0);
			pgbcell->m_dValues[GlyphBoundaries::gbcDNMin] = (float)(((int)(pgbcell->m_dValues[GlyphBoundaries::gbcDNMin] * 10000)) / 10000.0);
			pgbcell->m_dValues[GlyphBoundaries::gbcRight] = (float)(((int)(pgbcell->m_dValues[GlyphBoundaries::gbcRight] * 10000)) / 10000.0);
			pgbcell->m_dValues[GlyphBoundaries::gbcTop] = (float)(((int)(pgbcell->m_dValues[GlyphBoundaries::gbcTop] * 10000)) / 10000.0);
			pgbcell->m_dValues[GlyphBoundaries::gbcDPMax] = (float)(((int)(pgbcell->m_dValues[GlyphBoundaries::gbcDPMax] * 10000)) / 10000.0);
			pgbcell->m_dValues[GlyphBoundaries::gbcDNMax] = (float)(((int)(pgbcell->m_dValues[GlyphBoundaries::gbcDNMax] * 10000)) / 10000.0);
		}
	}
}

/*----------------------------------------------------------------------------------------------
	 Output the glyph boundary estimatesto the .GDX file.
----------------------------------------------------------------------------------------------*/
void GlyphBoundaries::DebugXml(std::ofstream & strmOut)
{
	int mxLeft, mxRight, myBottom, myTop, mDnMin, mDnMax, mDpMin, mDpMax;

	// Note that left, right, bottom and top are not actually recorded; they are defined to be 0 and 1.
	UnnormalizePoint(0, 0, &mxLeft, &myBottom);
	UnnormalizePoint(1.0, 1.0, &mxRight, &myTop);
	UnnormalizeSumAndDiff(m_gbcellEntire.m_dValues[gbcDNMin], m_gbcellEntire.m_dValues[gbcDPMin],
		&mDnMin, &mDpMin);
	UnnormalizeSumAndDiff(m_gbcellEntire.m_dValues[gbcDNMax], m_gbcellEntire.m_dValues[gbcDPMax],
		&mDnMax, &mDpMax);

	//strmOut << std::fixed; // for outputing floating point
	//strmOut.precision(2);
	strmOut
		<< "      <glyphAttrValue name=\"octabox.full\" value=\""

		// Note that left, right, bottom and top are not actually recorded; they are defined to be 0 and 1.
		//<< "0  100 ; 0  100 ; "
		//<< int(m_gbcellEntire.m_dValues[gbcDNMin] * 100)  << "  "
		//<< int(m_gbcellEntire.m_dValues[gbcDNMax] * 100)  << " ; "
		//<< int(m_gbcellEntire.m_dValues[gbcDPMin] * 100)  << "  "
		//<< int(m_gbcellEntire.m_dValues[gbcDPMax] * 100)

		<< mxLeft << "  " << mxRight << " ; "
		<< myBottom << "  " << myTop << " ; "
		<< mDnMin << "  " << mDnMax << " ; "
		<< mDpMin << "  " << mDpMax
		<< "\" />\n";

    int count = 0;
	for (int icellY = 0; icellY < gbgridCellsV; icellY++)
	{
		for (int icellX = 0; icellX < gbgridCellsH; icellX++)
		{
			GlyphBoundaryCell * pgbcell = m_rggbcellSub + CellIndex(icellX, icellY);

			if (pgbcell->HasData())
			{
				UnnormalizePoint(pgbcell->m_dValues[gbcLeft], pgbcell->m_dValues[gbcBottom], 
					&mxLeft, &myBottom);
				UnnormalizePoint(pgbcell->m_dValues[gbcRight], pgbcell->m_dValues[gbcTop],
					&mxRight, &myTop);
				UnnormalizeSumAndDiff(pgbcell->m_dValues[gbcDNMin], pgbcell->m_dValues[gbcDPMin],
					&mDnMin, &mDpMin);
				UnnormalizeSumAndDiff(pgbcell->m_dValues[gbcDNMax], pgbcell->m_dValues[gbcDPMax],
					&mDnMax, &mDpMax);

				strmOut
					<< "      <glyphAttrValue name=\"octabox.sub_" << std::setfill('0') << std::setw(2) << count;
                strmOut << "_" << icellX+1 << "-" << icellY+1 << "\" value=\""

					//<< int(pgbcell->m_dValues[gbcLeft] * 100)   << "  "
					//<< int(pgbcell->m_dValues[gbcRight] * 100)  << " ; "
					//<< int(pgbcell->m_dValues[gbcBottom] * 100) << "  "
					//<< int(pgbcell->m_dValues[gbcTop] * 100)    << " ; "
					//<< int(pgbcell->m_dValues[gbcDNMin] * 100)  << "  "
					//<< int(pgbcell->m_dValues[gbcDNMax] * 100)  << " ; "
					//<< int(pgbcell->m_dValues[gbcDPMin] * 100)  << "  "
					//<< int(pgbcell->m_dValues[gbcDPMax] * 100)

					<< mxLeft << "  " << mxRight << " ; "
					<< myBottom << "  " << myTop << " ; "
					<< mDnMin << "  " << mDnMax << " ; "
					<< mDpMin << "  " << mDpMax
					<< "\" />\n";
                ++count;
			}
		}
	}
}