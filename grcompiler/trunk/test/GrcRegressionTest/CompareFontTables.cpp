/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 2007 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: CompareFontTables.cpp
Responsibility: Sharon Correll

Description:
    Methods to run the comparisons for the benchmark and test fonts.
-------------------------------------------------------------------------------*//*:End Ignore*/

#include "main.h"

//:>********************************************************************************************
//:>	Global variables
//:>********************************************************************************************

//:>********************************************************************************************
//:>	Functions
//:>********************************************************************************************

/*----------------------------------------------------------------------------------------------
	Compare the benchmark font against the newly created test font.
----------------------------------------------------------------------------------------------*/
int CompareFontTables(TestCase * ptcase, GrcRtFileFont * pfontBmark, GrcRtFileFont * pfontTest)
{
	bool fOk = false;

	int ec = 0; // error count

	// benchmark font buffers
	const gr::byte * pHeadTblB; const gr::byte * pCmapTblB; const gr::byte * pSileTblB; const gr::byte * pSilfTblB;
	const gr::byte * pFeatTblB; const gr::byte * pGlatTblB; const gr::byte * pGlocTblB; const gr::byte * pNameTblB; const gr::byte * pSillTblB;
	size_t cbHeadSzB, cbCmapSzB, /*cbSileSzB,*/ cbSilfSzB, cbFeatSzB, cbGlatSzB, cbGlocSzB, cbNameSzB, cbSillSzB;
	// test font buffers
	const gr::byte * pHeadTblT; const gr::byte * pCmapTblT; const gr::byte * pSileTblT; const gr::byte * pSilfTblT;
	const gr::byte * pFeatTblT; const gr::byte * pGlatTblT; const gr::byte * pGlocTblT; const gr::byte * pNameTblT; const gr::byte * pSillTblT;
	size_t cbHeadSzT, cbCmapSzT, /*cbSileSzT,*/ cbSilfSzT, cbFeatSzT, cbGlatSzT, cbGlocSzT, cbNameSzT, cbSillSzT;

	// head table
	try {
		pHeadTblB = static_cast<const gr::byte *> (pfontBmark->getTable(TtfUtil::TableIdTag(ktiHead), &cbHeadSzB));
		if (pHeadTblB == NULL)
			OutputError(ec, ptcase, "ERROR: benchmark font has empty head table");
		else
		{
			try {
				pHeadTblT = static_cast<const gr::byte *> (pfontTest->getTable(TtfUtil::TableIdTag(ktiHead), &cbHeadSzT));
				if (pHeadTblT == NULL)
					OutputError(ec, ptcase, "ERROR: test font has empty head table");
				else
				{
					if (cbHeadSzB != cbHeadSzT)
						OutputError(ec, ptcase, "ERROR: size of head tables do not match");
					if (TtfUtil::DesignUnits(pHeadTblB) != TtfUtil::DesignUnits(pHeadTblT))
						OutputError(ec, ptcase, "ERROR: design units do not match");
					if (TtfUtil::IsItalic(pHeadTblB) != TtfUtil::IsItalic(pHeadTblT))
						OutputError(ec, ptcase, "ERROR: italic flags do not match");
				}
			}
			catch (...)
			{
				OutputError(ec, ptcase, "ERROR, could not read test font head table");
				pHeadTblT = NULL;
			}
		}
	}
	catch (...)
	{
		OutputError(ec, ptcase, "ERROR: could not read benchmark font head table");
		pHeadTblB = NULL;
		pHeadTblT = NULL;
	}

	// TODO: handle Sile table.
	pSileTblB = NULL;
	pSileTblT = NULL;

	// cmap

	try {
		pCmapTblB = static_cast<const gr::byte *> (pfontBmark->getTable(TtfUtil::TableIdTag(ktiCmap), &cbCmapSzB));
		if (pCmapTblB == NULL)
			OutputError(ec, ptcase, "ERROR: benchmark font has empty cmap table");
		else
		{
			try {
				pCmapTblT = static_cast<const gr::byte *> (pfontTest->getTable(TtfUtil::TableIdTag(ktiCmap), &cbCmapSzT));
				if (pHeadTblT == NULL)
					OutputError(ec, ptcase, "ERROR: test font has empty cmap table");
				else
				{
					if (cbCmapSzB != cbCmapSzT)
						OutputError(ec, ptcase, "ERROR: size of cmap tables do not match");
					// TBD: do we need to test the contents of the cmap?
					// The Graphite compiler shouldn't be changing it.
				}
			}
			catch (...)
			{
				OutputError(ec, ptcase, "ERROR: could not read test font cmap table");
				pCmapTblT = NULL;
			}
		}
	}
	catch (...)
	{
		OutputError(ec, ptcase, "ERROR: could not read benchmark font cmap table");
		pCmapTblB = NULL;
		pCmapTblT = NULL;
	}

	// name

	// Currently the only stuff we're getting from the name table are our feature names,
	// so use the version from the Graphite font (not the base font if any).
	//////if (m_fUseSepBase)
	//////	pgg->SetupGraphics(&chrpOriginal);

	// name - need feature names later
	try {
        pNameTblB = static_cast<const gr::byte *> (pfontBmark->getTable(TtfUtil::TableIdTag(ktiName), &cbNameSzB));
		if (pNameTblB == NULL)
			OutputError(ec, ptcase, "ERROR: benchmark font has empty name table");
		else
		{
			try {
				pNameTblT = static_cast<const gr::byte *> (pfontTest->getTable(TtfUtil::TableIdTag(ktiName), &cbNameSzT));
				if (pNameTblT == NULL)
					OutputError(ec, ptcase, "ERROR: test font has empty name table");
				else
				{
					if (cbNameSzB != cbNameSzT)
						OutputError(ec, ptcase, "ERROR: size of name tables do not match");
				}
			}
			catch (...)
			{
				OutputError(ec, ptcase, "ERROR: could not read test font name table");
			}
		}
	}
	catch (...)
	{
		OutputError(ec, ptcase, "ERROR: could not read benchmark font name table");
	}

	/****
	Obtain font name from InitNew() now instead of reading from font file. InitNew should
	should have a correct font name passed to it since it should come from a font registered
	by GrFontInst.exe. This commented code could be use to verify name in font file matches.
	NOTE: if we ever use this code again, make sure we're using the base font name table,
	not the Graphite wrapper font name table.
	// find the font family name
	if (!TtfUtil::Get31EngFamilyInfo(vbName.Begin(), lnNameOff, lnNameSz))
	{	// use Name table which is Symbol encode instead
		// this could cause problems if a real Symbol writing system is used in the name table
		// however normally real Unicode values are used instead a Symbol writing system
		if (!TtfUtil::Get30EngFamilyInfo(vbName.Begin(), lnNameOff, lnNameSz))
		{
			ReturnResult(kresFail);
		}
		// test for Symbol writing system. first byte of Unicode id should be 0xF0
		if (vbName[lnNameOff + 1] == (unsigned char)0xF0) // 1 - Unicode id is big endian
			ReturnResult(kresFail);
	}
	if (!TtfUtil::SwapWString(vbName.Begin() + lnNameOff, lnNameSz / sizeof(utf16)))
		ReturnResult(kresFail);

	m_stuFaceName = std::wstring((utf16 *)(vbName.begin() + lnNameOff), lnNameSz / sizeof(utf16));
	****/

	// Silf
	try {
		if ((pSilfTblB = static_cast<const gr::byte *> (pfontBmark->getTable(TtfUtil::TableIdTag(ktiSilf), &cbSilfSzB))) == NULL)
			OutputError(ec, ptcase, "ERROR: benchmark font has empty Silf table");
		else
		{
			try {
				if ((pSilfTblT = static_cast<const gr::byte *> (pfontTest->getTable(TtfUtil::TableIdTag(ktiSilf), &cbSilfSzT))) == NULL)
					OutputError(ec, ptcase, "ERROR: test font has empty Silf table");
				else
				{
					if (cbSilfSzB != cbSilfSzT)
						OutputError(ec, ptcase, "ERROR: size of Silf tables do not match");
				}
			}
			catch (...)
			{
				OutputError(ec, ptcase, "ERROR: could not read test font Silf table");
				pSilfTblT = NULL;
			}
		}
	}
	catch (...)
	{
		OutputError(ec, ptcase, "ERROR: could not read benchmark font Silf table");
		pSilfTblB = NULL;
		pSilfTblT = NULL;
	}

	// Feat
	try {
		if ((pFeatTblB = static_cast<const gr::byte *> (pfontBmark->getTable(TtfUtil::TableIdTag(ktiFeat), &cbFeatSzB))) == NULL)
			OutputError(ec, ptcase, "ERROR: benchmark font has empty Feat table");
		else
		{
			try {
				if ((pFeatTblT = static_cast<const gr::byte *> (pfontTest->getTable(TtfUtil::TableIdTag(ktiFeat), &cbFeatSzT))) == NULL)
					OutputError(ec, ptcase, "ERROR: test font has empty Feat table");
				else
				{
					if (cbFeatSzB != cbFeatSzT)
						OutputError(ec, ptcase, "ERROR: size of Feat tables do not match");
				}
			}
			catch (...)
			{
				OutputError(ec, ptcase, "ERROR: could not read test font Feat table");
				pFeatTblT = NULL;
			}
		}
	}
	catch (...)
	{
		OutputError(ec, ptcase, "ERROR: could not read benchmark font Feat table");
		pFeatTblB = NULL;
		pFeatTblT = NULL;
	}


	// Glat
	try {
		if ((pGlatTblB = static_cast<const gr::byte *> (pfontBmark->getTable(TtfUtil::TableIdTag(ktiGlat), &cbGlatSzB))) == NULL)
			OutputError(ec, ptcase, "ERROR: benchmark font has empty Glat table");
		else
		{
			try {
				if ((pGlatTblT = static_cast<const gr::byte *> (pfontTest->getTable(TtfUtil::TableIdTag(ktiGlat), &cbGlatSzT))) == NULL)
					OutputError(ec, ptcase, "ERROR: test font has empty Glat table");
				else
				{
					if (cbGlatSzB != cbGlatSzT)
						OutputError(ec, ptcase, "ERROR: size of Glat tables do not match");
				}
			}
			catch (...)
			{
				OutputError(ec, ptcase, "ERROR: could not read test font Glat table");
				pGlocTblT = NULL;
			}
		}
	}
	catch (...)
	{
		OutputError(ec, ptcase, "ERROR: could not read benchmark font Glat table");
		pGlocTblB = NULL;
		pGlocTblT = NULL;
	}

	// Gloc
	try {
		if ((pGlocTblB = static_cast<const gr::byte *> (pfontBmark->getTable(TtfUtil::TableIdTag(ktiGloc), &cbGlocSzB))) == NULL)
			OutputError(ec, ptcase, "ERROR: benchmark font has empty Gloc table");
		else
		{
			try {
				if ((pGlocTblT = static_cast<const gr::byte *> (pfontTest->getTable(TtfUtil::TableIdTag(ktiGloc), &cbGlocSzT))) == NULL)
					OutputError(ec, ptcase, "ERROR: test font has empty Gloc table");
				else
				{
					if (cbGlatSzB != cbGlatSzT)
						OutputError(ec, ptcase, "ERROR: size of Gloc tables do not match");
				}
			}
			catch (...)
			{
				OutputError(ec, ptcase, "ERROR: could not read test font Gloc table");
				pGlatTblT = NULL;
			}
		}
	}
	catch (...)
	{
		OutputError(ec, ptcase, "ERROR: could not read benchmark font Gloc table");
		pGlatTblB = NULL;
		pGlatTblT = NULL;
	}

	// Sill
	try {
		pSillTblB = static_cast<const gr::byte *> (pfontBmark->getTable(TtfUtil::TableIdTag(ktiSill), &cbSillSzB));
		try {
			pSillTblT = static_cast<const gr::byte *> (pfontTest->getTable(TtfUtil::TableIdTag(ktiSill), &cbSillSzT));
			if (cbSillSzB != cbSillSzT)
				OutputError(ec, ptcase, "ERROR: size of Sill tables do not match");
		}
		catch (...)
		{
			OutputError(ec, ptcase, "ERROR: in attempting to read test font Sill table");
			pSillTblT = NULL;
		}
	}
	catch (...)
	{
		OutputError(ec, ptcase, "ERROR: in attempting to read benchmark font Sill table");
		pSillTblB = NULL;
		pSillTblT = NULL;
	}

	if (pSilfTblT == NULL)
		return ec;
	Assert(pSilfTblB);

	GrBufferIStream grstrmB, grstrmT;
	grstrmB.OpenBuffer((gr::byte *)pSilfTblB, cbSilfSzB);
	grstrmT.OpenBuffer((gr::byte *)pSilfTblT, cbSilfSzT);
	int chwMaxGlyphID;
	CompareSilfTables(ec, ptcase, grstrmB, grstrmT, &chwMaxGlyphID);
	grstrmB.Close();
	grstrmT.Close();

	if (chwMaxGlyphID == -1)
	{
		int ecBogus;
		OutputError(ecBogus, ptcase, "[Skipping Gloc and Glat tables since max glyph IDs do not match]");
	}
	else if (pGlatTblB && pGlocTblB && pGlatTblT && pGlocTblT)
	{
		GrBufferIStream grstrmGlatB, grstrmGlocB, grstrmGlatT, grstrmGlocT;
		grstrmGlatB.OpenBuffer((gr::byte *)pGlatTblB, cbGlatSzB);
		grstrmGlocB.OpenBuffer((gr::byte *)pGlocTblB, cbGlocSzB);
		grstrmGlatT.OpenBuffer((gr::byte *)pGlatTblT, cbGlatSzT);
		grstrmGlocT.OpenBuffer((gr::byte *)pGlocTblT, cbGlocSzT);
		CompareGlatAndGlocTables(ec, ptcase, chwMaxGlyphID, grstrmGlatB, grstrmGlocB, grstrmGlatT, grstrmGlocT);
		grstrmGlatB.Close();
		grstrmGlocB.Close();
		grstrmGlatT.Close();
		grstrmGlocT.Close();
	}

	if (pFeatTblB && pFeatTblT)
	{
		grstrmB.OpenBuffer((gr::byte *)pFeatTblB, cbFeatSzB);
		grstrmT.OpenBuffer((gr::byte *)pFeatTblT, cbFeatSzT);
		CompareFeatTables(ec, ptcase, grstrmB, grstrmT, (gr::byte*)pNameTblB, (gr::byte*)pNameTblT);
		grstrmB.Close();
		grstrmT.Close();
	}

	if (pSillTblB && pSillTblT)
	{
		grstrmB.OpenBuffer((gr::byte *)pSillTblB, cbSillSzB);
		grstrmT.OpenBuffer((gr::byte *)pSillTblT, cbSillSzT);
		CompareSillTables(ec, ptcase, grstrmB, grstrmT);
		grstrmB.Close();
		grstrmT.Close();
	}

	delete[] pHeadTblB;
	delete[] pHeadTblT;

	delete[] pCmapTblB;
	delete[] pCmapTblT;

	delete[] pNameTblB;
	delete[] pNameTblT;

	delete[] pSilfTblB;
	delete[] pSilfTblT;

	delete[] pFeatTblB;
	delete[] pFeatTblT;

	delete[] pGlatTblB;
	delete[] pGlatTblT;

	delete[] pGlocTblB;
	delete[] pGlocTblT;

	delete[] pSillTblB;
	delete[] pSillTblT;

	return ec;
}

/*----------------------------------------------------------------------------------------------
	Compare the Silf tables of the benchmark and the test fonts.
----------------------------------------------------------------------------------------------*/
void CompareSilfTables(int & ec, TestCase * ptcase, GrIStream & grstrmB, GrIStream & grstrmT,
	int * pchwMaxGlyphID)
{
	int fxdSilfVersionB = ReadVersion(grstrmB);
	int fxdSilfVersionT = ReadVersion(grstrmT);

	Assert(fxdSilfVersionB <= kSilfVersion);
	if (fxdSilfVersionT > kSilfVersion)
	{
		OutputError(ec, ptcase, "ERROR: Unknown Silf table version in test font");
		return;
	}

	if (fxdSilfVersionB >= 0x00030000)
		 // compiler version
		grstrmB.ReadIntFromFont();
	if (fxdSilfVersionT >= 0x00030000)
		grstrmT.ReadIntFromFont();

	//	number of tables
	unsigned short cSubTablesB = grstrmB.ReadUShortFromFont();
	Assert(cSubTablesB == 1);	// for now
	Assert(cSubTablesB <= kMaxSubTablesInFont);
	unsigned short cSubTablesT = grstrmT.ReadUShortFromFont();
	if (cSubTablesT < 1)
	{
		OutputError(ec, ptcase, "ERROR: Silf table has no subtable");
		return;
	}
	if (cSubTablesT > 1)
		OutputError(ec, ptcase, "ERROR: Silf table has greater than 1 subtable");

    if (fxdSilfVersionB >= 0x00020000)
		// reserved
		grstrmB.ReadShortFromFont();
	if (fxdSilfVersionT >= 0x00020000)
		grstrmT.ReadShortFromFont();

	//	subtable offsets
	int nSubTableOffsetsB[kMaxSubTablesInFont];
	int nSubTableOffsetsT[kMaxSubTablesInFont];
	int i;
	for (i = 0; i < cSubTablesB; i++)
		nSubTableOffsetsB[i] = grstrmB.ReadIntFromFont();
	for (i = 0; i < cSubTablesT; i++)
		nSubTableOffsetsT[i] = grstrmT.ReadIntFromFont();
	grstrmB.SetPositionInFont(nSubTableOffsetsB[0]);
	grstrmT.SetPositionInFont(nSubTableOffsetsT[0]);

	//	Now we are at the beginning of the desired sub-table.

	//	Get the position of the start of the table.
	long lSubTableStartB, lSubTableStartT;
	grstrmB.GetPositionInFont(&lSubTableStartB);
	grstrmT.GetPositionInFont(&lSubTableStartT);

	//	rule version
	int fxdRuleVersionB = (fxdSilfVersionB >= 0x00030000) ? ReadVersion(grstrmB) : fxdSilfVersionB;
	int fxdRuleVersionT = (fxdSilfVersionT >= 0x00030000) ? ReadVersion(grstrmT) : fxdSilfVersionT;

	long lPassBlockPosB = -1;
	long lPseudosPosB = -1;
	if (fxdSilfVersionB >= 0x00030000)
	{
		lPassBlockPosB = grstrmB.ReadUShortFromFont() + lSubTableStartB;
		lPseudosPosB = grstrmB.ReadUShortFromFont() + lSubTableStartB;
	}
	long lPassBlockPosT = -1;
	long lPseudosPosT = -1;
	if (fxdSilfVersionT >= 0x00030000)
	{
		lPassBlockPosT = grstrmT.ReadUShortFromFont() + lSubTableStartT;
		lPseudosPosT = grstrmT.ReadUShortFromFont() + lSubTableStartT;
	}

	//	maximum glyph ID
	*pchwMaxGlyphID = grstrmB.ReadUShortFromFont();
	if (grstrmT.ReadUShortFromFont() != *pchwMaxGlyphID)
	{
		OutputError(ec, ptcase, "ERROR: Silf table - maxiumum glyph ID");
		*pchwMaxGlyphID = -1;
	}

	//	extra ascent
	if (grstrmB.ReadShortFromFont() != grstrmT.ReadShortFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - extra ascent");
	// extra descent
	if (grstrmB.ReadShortFromFont() != grstrmT.ReadShortFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - extra descent");

	//	number of passes
	gr::byte cPasses = grstrmB.ReadByteFromFont();
	if (cPasses != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - number of passes");
	//	index of first substitution pass
	gr::byte ipassSub1 = grstrmB.ReadByteFromFont();
	if (ipassSub1 != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - first substitution pass");
	//	index of first positioning pass
	gr::byte ipassPos1 = grstrmB.ReadByteFromFont();
	if (ipassPos1 != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - first positioning pass");
	//	index of first justification pass
	gr::byte ipassJust1 = grstrmB.ReadByteFromFont();
	if (ipassJust1 != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - first justification pass");
	//	index of first reordered pass, or 0xFF if no reordering
	gr::byte ipassReordered1 = grstrmB.ReadByteFromFont();
	if (ipassReordered1 != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - first reordered pass");

	//	line-break flag
	if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - line-break flag");

	//	range of possible cross-line-boundary contextualization
	if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - pre LB context");
	if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - post LB context");

	//	actual glyph ID for pseudo-glyph (ID of bogus attribute)
	//long posB; grstrmB.GetPositionInFont(&posB); - debuggers
	//WriteToLog("benchmark position = ", posB, "; ");
	//long posT; grstrmT.GetPositionInFont(&posT);
	//WriteToLog("test font position = ", posT, "; ");
	//gr::byte tempB = grstrmB.ReadByteFromFont();
	//WriteToLog("next byte B = ", tempB, "; ");
	//gr::byte tempT = grstrmT.ReadByteFromFont();
	//WriteToLog("next byte T = ", tempT, "\n");
	//if (tempB != tempT)
	if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - actual-for-pseudo attr");
	//	breakweight
	if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - breakweight attr");
	//	directionality
	if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - directionality attr");

	if (fxdSilfVersionB >= 0x00020000 && fxdSilfVersionT >= 0x00020000)
	{
		// reserved
		grstrmB.ReadByteFromFont();
		grstrmB.ReadByteFromFont();

		grstrmT.ReadByteFromFont();
		grstrmT.ReadByteFromFont();

		//	justification levels
		int cJLevels = grstrmB.ReadByteFromFont();
		if (cJLevels != grstrmT.ReadByteFromFont())
		{
			OutputError(ec, ptcase, "ERROR: Silf table - justification levels");
			return;
		}
		for (int i = 0; i < cJLevels; i++)
		{
			//	justification glyph attribute IDs
			if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
				OutputError(ec, ptcase, "ERROR: Silf table - stretch", i);
			if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
				OutputError(ec, ptcase, "ERROR: Silf table - shrink", i);
			if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
				OutputError(ec, ptcase, "ERROR: Silf table - step", i);
			if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
				OutputError(ec, ptcase, "ERROR: Silf table - weight", i);
			grstrmB.ReadByteFromFont(); // runto
			grstrmT.ReadByteFromFont();
			// reserved
			grstrmB.ReadByteFromFont();
			grstrmB.ReadByteFromFont();
			grstrmB.ReadByteFromFont();

			grstrmT.ReadByteFromFont();
			grstrmT.ReadByteFromFont();
			grstrmT.ReadByteFromFont();
		}
	}
	else if (fxdSilfVersionB >= 0x00020000)
	{
		// reserved
		grstrmB.ReadByteFromFont();
		grstrmB.ReadByteFromFont();
		//	justification levels
		int cJLevels = grstrmB.ReadByteFromFont();
		if (cJLevels > 0)
			OutputError(ec, ptcase, "ERROR: Silf table - missing justification data");
		gr::byte rgb[8];
		for (int i = 0; i < cJLevels; i++)
			grstrmB.ReadBlockFromFont(rgb, 8);
	}
	else if (fxdSilfVersionT >= 0x00020000)
	{
		// reserved
		grstrmB.ReadByteFromFont();
		grstrmB.ReadByteFromFont();
		//	justification levels
		int cJLevels = grstrmB.ReadByteFromFont();
		if (cJLevels > 0)
			OutputError(ec, ptcase, "ERROR: Silf table - justification data found but not expected");
		gr::byte rgb[8];
		for (int i = 0; i < cJLevels; i++)
			grstrmB.ReadBlockFromFont(rgb, 8);
	}

	//	number of component attributes
	if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - number of components");

	//	number of user-defined slot attributes
	if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - number of user-defined slot attributes");

	//	max number of ligature components per glyph
	if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - max number of ligature components");

	//	directions supported
	if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - supported directions");

	//	reserved
	grstrmB.ReadByteFromFont();
	grstrmB.ReadByteFromFont();
	grstrmB.ReadByteFromFont();
	grstrmT.ReadByteFromFont();
	grstrmT.ReadByteFromFont();
	grstrmT.ReadByteFromFont();

	//	critical features
	if (fxdSilfVersionB >= 0x00020000 && fxdSilfVersionT >= 0x00020000)
	{
		// reserved
		grstrmB.ReadByteFromFont();
		grstrmT.ReadByteFromFont();
		// critical features
		if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
			OutputError(ec, ptcase, "ERROR: Silf table - critical features");
		// reserved
		grstrmB.ReadByteFromFont();
		grstrmT.ReadByteFromFont();
	}
	else if (fxdSilfVersionB >= 0x00020000)
	{
		grstrmB.ReadByteFromFont();
		if (grstrmB.ReadByteFromFont() != 0)
			OutputError(ec, ptcase, "ERROR: Silf table - critical features not equal to zero");
		grstrmB.ReadByteFromFont();
	}
	else if (fxdSilfVersionT >= 0x00020000)
	{
		grstrmT.ReadByteFromFont();
		if (grstrmT.ReadByteFromFont() != 0)
			OutputError(ec, ptcase, "ERROR: Silf table - critical features found but not expected");
		grstrmT.ReadByteFromFont();
	}

	//	rendering behaviors
	int cBehaviorsB = grstrmB.ReadByteFromFont();
	int cBehaviorsT = grstrmT.ReadByteFromFont();
	if (cBehaviorsB != cBehaviorsT)
		OutputError(ec, ptcase, "ERROR: Silf table - rendering behaviors");
	for (i = 0; i < cBehaviorsB; i++)
		grstrmB.ReadUShortFromFont();
	for (i = 0; i < cBehaviorsT; i++)
		grstrmT.ReadUShortFromFont();

	//	linebreak glyph ID
	if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - linebreak glyph ID");

	//	Jump to the beginning of the pass offset block, if we have this information.
	if (fxdSilfVersionB >= 0x00030000)
		grstrmB.SetPositionInFont(lPassBlockPosB);
	else
		//	Otherwise assume that's where we are!
		Assert(lPassBlockPosB == -1);

	if (fxdSilfVersionT >= 0x00030000)
		grstrmT.SetPositionInFont(lPassBlockPosT);
	else
		Assert(lPassBlockPosT == -1);

	//	offsets to passes, relative to the start of this subtable;
	//	note that we read (cPasses + 1) of these
	int rgnPassOffsets[kMaxPasses];
	bool fPassesOk = true;
	for (i = 0; i <= cPasses; i++)
	{
		rgnPassOffsets[i] = grstrmB.ReadIntFromFont();
		if (rgnPassOffsets[i] != grstrmT.ReadIntFromFont())
		{
			OutputError(ec, ptcase, "ERROR: Silf table - pass offsets", i);
			fPassesOk = false;
		}
	}

	//	Jump to the beginning of the pseudo-glyph info block, if we have this information.
	if (fxdSilfVersionB >= 0x00030000)
		grstrmB.SetPositionInFont(lPseudosPosB);
	else
		//	Otherwise assume that's where we are!
		Assert(lPseudosPosB == -1);

	if (fxdSilfVersionT >= 0x00030000)
		grstrmT.SetPositionInFont(lPseudosPosT);
	else
		Assert(lPseudosPosT == -1);

	//	number of pseudo-glyphs and search constants
	int cpsd = grstrmB.ReadUShortFromFont();
	if (cpsd != grstrmT.ReadUShortFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - pseudo search count");
	if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - pseudo search increment");
	if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - pseudo search loop");
	if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
		OutputError(ec, ptcase, "ERROR: Silf table - pseudo search start");

	//	unicode-to-pseudo map
	for (i = 0; i < cpsd; i++)
	{
		int unicodeB, unicodeT;
		if (fxdSilfVersionB <= 0x00010000)
			unicodeB = grstrmB.ReadUShortFromFont();
		else
			unicodeB = grstrmB.ReadIntFromFont();
		if (fxdSilfVersionT <= 0x00010000)
			unicodeT = grstrmT.ReadUShortFromFont();
		else
			unicodeT = grstrmT.ReadIntFromFont();
		if (unicodeB != unicodeT)
			OutputError(ec, ptcase, "ERROR: Silf table - unicode for pseudo", i);

		if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
			OutputError(ec, ptcase, "ERROR: Silf table = pseudo value", i);
	}

	CompareClassMaps(ec, ptcase, grstrmB, grstrmT);

	if (fPassesOk)
		ComparePasses(ec, ptcase, grstrmB, grstrmT,
			fxdSilfVersionB, fxdSilfVersionT, cPasses,
			lSubTableStartB, lSubTableStartT, rgnPassOffsets);
}

/*----------------------------------------------------------------------------------------------
	Compare the class tables.
----------------------------------------------------------------------------------------------*/
void CompareClassMaps(int & ec, TestCase * ptcase, GrIStream & grstrmB, GrIStream & grstrmT)
{
	// number of classes
	int cClasses = grstrmB.ReadUShortFromFont();
	if (cClasses != grstrmT.ReadUShortFromFont())
	{
		OutputError(ec, ptcase, "ERROR: class map - number of classes");
		return;
	}

	// number of linearly stored classes
	int cClassesLinear = grstrmB.ReadUShortFromFont();
	if (cClassesLinear != grstrmT.ReadUShortFromFont())
	{
		OutputError(ec, ptcase, "ERROR: class map - number of linear classes");
		return;
	}

	// offsets
	data16 * wOffsets = new data16[cClasses + 1];
	bool fOffsetsOk = true;
	for (int i = 0; i <= cClasses; i++)
	{
		wOffsets[i] = grstrmB.ReadUShortFromFont();
		if (wOffsets[i] != grstrmT.ReadUShortFromFont())
		{
			OutputError(ec, ptcase, "ERROR: class map - offset", i);
			fOffsetsOk = false;
		}
	}
	if (!fOffsetsOk)
	{
		delete[] wOffsets;
		return;
	}

	int iClass = 0;
	int ibOffset = 4 + (2 * (cClasses + 1));
	while (iClass < cClasses)
	{
		bool fLinear = (iClass < cClassesLinear);
		if (fLinear)
		{
			int cGlyphs = (wOffsets[iClass + 1] - wOffsets[iClass]) >> 1; // divided by 2
			for (int ig = 0; ig < cGlyphs; ig++)
			{
				if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
					OutputError(ec, ptcase, "ERROR: class map - glyph of class", iClass);
				ibOffset += 2; // 2 bytes
			}
		}
		else
		{
			int cGlyphs = grstrmB.ReadUShortFromFont();
			if (cGlyphs != grstrmT.ReadUShortFromFont())
			{
				OutputError(ec, ptcase, "ERROR: class map - number of glyphs in class", iClass);
				return;
			}
			if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
				OutputError(ec, ptcase, "ERROR: Silf table - class search increment", iClass);
			if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
				OutputError(ec, ptcase, "ERROR: Silf table - class search loop", iClass);
			if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
				OutputError(ec, ptcase, "ERROR: Silf table - class search start", iClass);
			ibOffset += 8;
			for (int ig = 0; ig < cGlyphs; ig++)
			{
				if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
					OutputError(ec, ptcase, "ERROR: Silf table - glyph ID in class", iClass);
				if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
					OutputError(ec, ptcase, "ERROR: Silf table - glyph index in class", iClass);
				ibOffset += 4;
			}
		}
		iClass++;
		Assert(ibOffset == wOffsets[iClass]);
	}

	delete[] wOffsets;
}

/*----------------------------------------------------------------------------------------------
	Compare the class tables.
----------------------------------------------------------------------------------------------*/
void ComparePasses(int & ec, TestCase * ptcase, GrIStream & grstrmB, GrIStream & grstrmT,
   int fxdSilfVersionB, int fxdSilfVersionT, int cPasses,
   int lSubTableStartB, int lSubTableStartT, int * prgnPassOffsets)
{
	for (int iPass = 0; iPass < cPasses; iPass++)
	{
		int nOffsetB = lSubTableStartB + prgnPassOffsets[iPass];
		int nOffsetT = lSubTableStartT + prgnPassOffsets[iPass];

		long lPassInfoStart;
		grstrmB.GetPositionInFont(&lPassInfoStart);
		if (lPassInfoStart != nOffsetB)
			grstrmB.SetPositionInFont(nOffsetB);
		grstrmT.GetPositionInFont(&lPassInfoStart);
		if (lPassInfoStart != nOffsetT)
			grstrmT.SetPositionInFont(nOffsetT);

		//	flags
		if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
			OutputError(ec, ptcase, "ERROR: pass", iPass, " - flags");

		//	MaxRuleLoop
		if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
			OutputError(ec, ptcase, "ERROR: pass", iPass, " - MaxRuleLoop");

		//	max rule context
		if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
			OutputError(ec, ptcase, "ERROR: pass", iPass, " - max rule context");

		//	MaxBackup
		if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
			OutputError(ec, ptcase, "ERROR: pass", iPass, " - MaxBackup");

		//	number of rules
		int crul = grstrmB.ReadShortFromFont();
		if (crul != grstrmT.ReadShortFromFont())
		{
			OutputError(ec, ptcase, "ERROR: pass", iPass, " - number of rules");
			return;
		}

		//	offset to pass constraint code, relative to start of subtable
		int nPConstraintOffsetB = 0;
		long lFsmPosB = -1;
		if (fxdSilfVersionB >= 0x00020000)
		{
			if (fxdSilfVersionB >= 0x00030000)
				lFsmPosB = grstrmB.ReadUShortFromFont() + nOffsetB; // offset to row info
			else
				grstrmB.ReadShortFromFont();	// pad bytes
			nPConstraintOffsetB = grstrmB.ReadIntFromFont();
		}
		int nPConstraintOffsetT = 0;
		long lFsmPosT = -1;
		if (fxdSilfVersionT >= 0x00020000)
		{
			if (fxdSilfVersionT >= 0x00030000)
				lFsmPosT = grstrmT.ReadUShortFromFont() + nOffsetT; // offset to row info
			else
				grstrmT.ReadShortFromFont();	// pad bytes
			nPConstraintOffsetT = grstrmT.ReadIntFromFont();
		}

		//	offset to rule constraint code, relative to start of subtable
		//int nConstraintOffset = grstrmB.ReadIntFromFont();
		grstrmB.ReadIntFromFont();
		grstrmT.ReadIntFromFont();
		//	offset to action code, relative to start of subtable
		//int nActionOffset = grstrmB.ReadIntFromFont();
		grstrmB.ReadIntFromFont();
		grstrmT.ReadIntFromFont();
		//	offset to debug strings; 0 if stripped
		//int nDebugOffset = grstrmB.ReadIntFromFont();
		grstrmB.ReadIntFromFont();
		grstrmT.ReadIntFromFont();

		//	Jump to beginning of FSM, if we have this information.
		if (fxdSilfVersionB >= 0x00030000)
			grstrmB.SetPositionInFont(lFsmPosB);
		else
			// Otherwise assume that's where we are!
			Assert(lFsmPosB == -1);
		if (fxdSilfVersionT >= 0x00030000)
			grstrmT.SetPositionInFont(lFsmPosT);
		else
			// Otherwise assume that's where we are!
			Assert(lFsmPosT == -1);

		int cFsmCells = CompareFsmTables(ec, ptcase, grstrmB, grstrmT, fxdSilfVersionB, fxdSilfVersionT, iPass);

		//	rule sort keys
		int irul;
		for (irul = 0; irul < crul; irul++)
		{
			if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
				OutputError(ec, ptcase, "ERROR: pass", iPass, ", rule sort key", irul);
		}

		//	rule pre-mod-context item counts
		for (irul = 0; irul < crul; irul++)
		{
			if (grstrmB.ReadByteFromFont() != grstrmT.ReadByteFromFont())
				OutputError(ec, ptcase, "ERROR: pass", iPass, ", rule pre-mod-context", irul);
		}

		//	constraint offset for pass-level constraints
		int cbPassConstraintsB, cbPassConstraintsT = 0;
		if (fxdSilfVersionB >= 0x00020000)
		{
			grstrmB.ReadByteFromFont();
			cbPassConstraintsB = grstrmB.ReadUShortFromFont();				
		}
		if (fxdSilfVersionB >= 0x00020000)
		{
			grstrmT.ReadByteFromFont();
			cbPassConstraintsT = grstrmT.ReadUShortFromFont();
		}
		if (cbPassConstraintsB != cbPassConstraintsT)
			OutputError(ec, ptcase, "ERROR: pass", iPass, ", pass-level constraint offset");

		//	constraint and action offsets for rules
		data16 cbConstraints;
		for (irul = 0; irul <= crul; irul++)
		{
			cbConstraints = grstrmB.ReadUShortFromFont(); // save the last item, it gives the total
			if (cbConstraints != grstrmT.ReadUShortFromFont())
				OutputError(ec, ptcase, "ERROR: pass", iPass, ", rule constraint offset", irul);
		}
		data16 cbActions;
		for (irul = 0; irul <= crul; irul++)
		{
			cbActions = grstrmB.ReadUShortFromFont(); // save the last item, it gives the total
			if (cbActions != grstrmT.ReadUShortFromFont())
				OutputError(ec, ptcase, "ERROR: pass", iPass, ", rule action offset", irul);
		}

		// FSM cells
		for (int iCell = 0; iCell < cFsmCells; iCell++)
		{
			if (grstrmB.ReadShortFromFont() != grstrmT.ReadShortFromFont())
				OutputError(ec, ptcase, "ERROR: pass", iPass, ", FSM cell", iCell);
		}

		if (fxdSilfVersionB >= 0x00020000)
			// reserved - pad byte
			grstrmB.ReadByteFromFont();
		if (fxdSilfVersionT >= 0x00020000)
			// reserved - pad byte
			grstrmT.ReadByteFromFont();

		gr::byte * pbB;
		gr::byte * pbT;
		int ib;

		//	Constraint and action blocks
		int cb = cbPassConstraintsB;
		gr::byte * prgbPConstraintBlockB = new gr::byte[cb];
		gr::byte * prgbPConstraintBlockT = new gr::byte[cb];
		grstrmB.ReadBlockFromFont(prgbPConstraintBlockB, cb);
		grstrmT.ReadBlockFromFont(prgbPConstraintBlockT, cb);
		for (ib = 0, pbB = prgbPConstraintBlockB, pbT = prgbPConstraintBlockT;
			ib < cb;
			ib++, pbB++, pbT++)
		{
			if (*pbB != *pbT)
				OutputError(ec, ptcase, "ERROR: pass", iPass, ", pass constraint byte", ib);
		}
		delete[] prgbPConstraintBlockB;
		delete[] prgbPConstraintBlockT;

		cb = cbConstraints;
		gr::byte * prgbConstraintBlockB = new gr::byte[cb];
		gr::byte * prgbConstraintBlockT = new gr::byte[cb];
		grstrmB.ReadBlockFromFont(prgbConstraintBlockB, cb);
		grstrmT.ReadBlockFromFont(prgbConstraintBlockT, cb);
		for (ib = 0, pbB = prgbConstraintBlockB, pbT = prgbConstraintBlockT;
			ib < cb;
			ib++, pbB++, pbT++)
		{
			if (*pbB != *pbT)
				OutputError(ec, ptcase, "ERROR: pass", iPass, ", rule constraint byte", ib);
		}
		delete[] prgbConstraintBlockB;
		delete[] prgbConstraintBlockT;

		cb = cbActions;
		gr::byte * prgbActionBlockB = new gr::byte[cb];
		gr::byte * prgbActionBlockT = new gr::byte[cb];
		grstrmB.ReadBlockFromFont(prgbActionBlockB, cb);
		grstrmT.ReadBlockFromFont(prgbActionBlockT, cb);
		for (ib = 0, pbB = prgbActionBlockB, pbT = prgbActionBlockT;
			ib < cb;
			ib++, pbB++, pbT++)
		{
			if (*pbB != *pbT)
				OutputError(ec, ptcase, "ERROR: pass", iPass, ", action byte", ib);
		}
		delete[] prgbActionBlockB;
		delete[] prgbActionBlockT;
	}
}

/*----------------------------------------------------------------------------------------------
	Compare the FSM tables.
----------------------------------------------------------------------------------------------*/
int CompareFsmTables(int & ec, TestCase * ptcase,
	GrIStream & grstrmB, GrIStream & grstrmT,
	int fxdSilfVersionB, int fxdSilfVersionT, int iPass)
{
	//	number of FSM states
	int crow = grstrmB.ReadShortFromFont();
	if (crow != grstrmT.ReadShortFromFont())
		OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - number of FSM states");
	//	number of transitional states
	int crowTransitional = grstrmB.ReadShortFromFont();
	if (crowTransitional != grstrmT.ReadShortFromFont())
		OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - number of FSM transitional states");
	//	number of success states
	int crowSuccess = grstrmB.ReadShortFromFont();
	if (crowSuccess != grstrmT.ReadShortFromFont())
		OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - number of FSM success states");
	int crowFinal = crow - crowTransitional;
	int crowNonAcpt = crow - crowSuccess;

	//	number of columns
	int ccol = grstrmB.ReadShortFromFont();
	if (ccol != grstrmT.ReadShortFromFont())
		OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - number of FSM columns");

	// Sanity check
	Assert(crowTransitional <= crow && crowSuccess <= crow);

	//	number of FSM glyph ranges and search constants
	int cmcr = grstrmB.ReadShortFromFont();
	if (cmcr != grstrmT.ReadShortFromFont())
		OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - # of glyph ranges");
	if (grstrmB.ReadShortFromFont() != grstrmT.ReadShortFromFont())
		OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - glyph range search increment");
	if (grstrmB.ReadShortFromFont() != grstrmT.ReadShortFromFont())
		OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - glyph range search loop");
	if (grstrmB.ReadShortFromFont() != grstrmT.ReadShortFromFont())
		OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - glyph range search start");

	for (int imcr = 0; imcr < cmcr; imcr++)
	{
		if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
			OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - glyph range first", imcr);
		if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
			OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - glyph range last", imcr);
		if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
			OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - glyph range column", imcr);
	}

	// rule map and offsets (extra item at end gives final offset, ie, total)
	int crulInMap;
	int i;
	for (i = 0; i < (crowSuccess + 1); i++)
	{
		crulInMap = grstrmB.ReadUShortFromFont();	// last offset functions as the total length of the rule list
		if (crulInMap != grstrmT.ReadUShortFromFont())
			OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - rule map offset", i);
	}

	for (i = 0; i < crulInMap; i++)
	{
		if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
			OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - rule map item", i);
	}

	//	min rule pre-context number of items
	int critMinRulePreContext = grstrmB.ReadByteFromFont();
	if (critMinRulePreContext != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - min rule pre-context", i);
	int critMaxRulePreContext = grstrmB.ReadByteFromFont();
	if (critMaxRulePreContext != grstrmT.ReadByteFromFont())
		OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - max rule pre-context", i);

	Assert(critMinRulePreContext <= kMaxSlotsPerRule);
	Assert(critMaxRulePreContext <= kMaxSlotsPerRule);

	int cStartStates = critMaxRulePreContext - critMinRulePreContext + 1;

	//	start states
	for (int ic = 0; ic < cStartStates;	ic++)
	{
		if (grstrmB.ReadShortFromFont() != grstrmT.ReadShortFromFont())
			OutputError(ec, ptcase, "ERROR: pass FSM", iPass, " - start state", ic);
	}

	return (crow - crowFinal) * ccol;
}

/*----------------------------------------------------------------------------------------------
	Compare the Glat and Gloc tables.
----------------------------------------------------------------------------------------------*/
void CompareGlatAndGlocTables(int & ec, TestCase * ptcase, int wMaxGlyphID,
	GrIStream & grstrmGlatB, GrIStream & grstrmGlocB,
	GrIStream & grstrmGlatT, GrIStream & grstrmGlocT)
{
	Assert(ReadVersion(grstrmGlatB) == 0x00010000);
	Assert(ReadVersion(grstrmGlocB) == 0x00010000);
	if (ReadVersion(grstrmGlatT) != 0x00010000)
	{
		OutputError(ec, ptcase, "ERROR: unknown Glat table version");
		return;
	}
	if (ReadVersion(grstrmGlocT) != 0x00010000)
	{
		OutputError(ec, ptcase, "ERROR: unknown Gloc table version");
		return;
	}

	// flag bits
	data16 wFlagsB = grstrmGlocB.ReadUShortFromFont();
	data16 wFlagsT = grstrmGlocT.ReadUShortFromFont();
	if (wFlagsB != wFlagsT)
		OutputError(ec, ptcase, "ERROR: Gloc table flags do not match");
	bool fLongFormatB = wFlagsB & 0x0001;
	bool fLongFormatT = wFlagsT & 0x0001;

	// number of attributes
	data16 cAttribs = grstrmGlocB.ReadUShortFromFont();
	if (cAttribs != grstrmGlocT.ReadUShortFromFont())
	{
		OutputError(ec, ptcase, "ERROR: Gloc table - number of attributes");
		return;
	}

	// offsets and attribute values
	int nOffsetB = fLongFormatB ? grstrmGlocB.ReadIntFromFont() : grstrmGlocB.ReadUShortFromFont();
	int nOffsetT = fLongFormatT ? grstrmGlocT.ReadIntFromFont() : grstrmGlocT.ReadUShortFromFont();
	if (fLongFormatB == fLongFormatT && nOffsetB != nOffsetT)
		OutputErrorWithValues(ec, ptcase, "ERROR: Gloc table - offset", 0, nOffsetB, nOffsetT);
	int iAttrEntry = 0;
	int cbGlatOffset = 4; // read version
	for (int wGlyphID = 1; wGlyphID < wMaxGlyphID; wGlyphID++)
	{
		int nOffsetBnext = fLongFormatB ? grstrmGlocB.ReadIntFromFont() : grstrmGlocB.ReadUShortFromFont();
		while (cbGlatOffset < nOffsetBnext)
		{
			int nAttrNumB = grstrmGlatB.ReadByteFromFont();
			int nAttrNumT = grstrmGlatT.ReadByteFromFont();
			if (nAttrNumB != nAttrNumT)
			{
				OutputError(ec, ptcase, "ERROR: Glat table - glyph", wGlyphID, ", attr entry first attr", iAttrEntry);
				return;
			}
			int cAttrsB = grstrmGlatB.ReadByteFromFont();
			int cAttrsT = grstrmGlatT.ReadByteFromFont();
			if (cAttrsB != cAttrsT)
			{
				OutputError(ec, ptcase, "ERROR: Glat table - glyph", wGlyphID, ", attr entry # of attrs", iAttrEntry);
				return;
			}
			cbGlatOffset += 2; // 2 bytes
			for (int iAttr = 0; iAttr < cAttrsB; iAttr++)
			{
				int nValueB = grstrmGlatB.ReadShortFromFont();
				int nValueT = grstrmGlatT.ReadShortFromFont();
				if (nValueB != nValueT)
                {
					OutputError(ec, ptcase, "ERROR: Glat table - glyph", wGlyphID, ", attr value", nAttrNumB+iAttr);
                    OutputError(ec, ptcase, "   Values: ", nValueB, " -> ", nValueT);
                }
				cbGlatOffset += 2; // 2 bytes
			}
			iAttrEntry++;
		}
		// Go on to next glyph ID.
		nOffsetB = nOffsetBnext;
		nOffsetT = fLongFormatT ? grstrmGlocT.ReadIntFromFont() : grstrmGlocT.ReadUShortFromFont();
		if (fLongFormatB == fLongFormatT && nOffsetB != nOffsetT)
			OutputErrorWithValues(ec, ptcase, "ERROR: Gloc table - offset", wGlyphID, nOffsetB, nOffsetT);
	}
}

/*----------------------------------------------------------------------------------------------
	Compare the Feat tables.
----------------------------------------------------------------------------------------------*/
void CompareFeatTables(int & ec, TestCase * ptcase, GrIStream & grstrmB, GrIStream & grstrmT,
	const gr::byte * pNameTblB, const gr::byte * pNameTblT)
{
	int fxdVersionB = ReadVersion(grstrmB);
	int fxdVersionT = ReadVersion(grstrmT);
    Assert(fxdVersionB <= kFeatVersion);
	if (fxdVersionT > kFeatVersion)
	{
		OutputError(ec, ptcase, "ERROR: Feat table - unknown version number");
		return;
	}
	
	//	number of features
	int cfeatB = grstrmB.ReadUShortFromFont();
	int cfeatT = grstrmT.ReadUShortFromFont();
	Assert(cfeatB <= kMaxFeatures);
	if (cfeatB != cfeatT)
		OutputError(ec, ptcase, "ERROR: Feat table - number of features");

	//	reserved
	grstrmB.ReadUShortFromFont();
	grstrmT.ReadUShortFromFont();
	grstrmB.ReadIntFromFont();
	grstrmT.ReadIntFromFont();
	
	int cfset = 0;
	int ifeat;
	for (ifeat = 0; ifeat < min(cfeatB, cfeatT); ifeat++)
	{
		//	ID
		featid nIdB, nIdT;
		if (fxdVersionB >= 0x00020000)
			nIdB = (unsigned int)grstrmB.ReadIntFromFont();
		else
			nIdB = grstrmB.ReadUShortFromFont();
		if (fxdVersionT >= 0x00020000)
			nIdT = (unsigned int)grstrmT.ReadIntFromFont();
		else
			nIdT = grstrmT.ReadUShortFromFont();
		if (nIdB != nIdT)
			OutputError(ec, ptcase, "ERROR: Feat table - feature ID", ifeat);

		//	number of settings
		data16 cfsetThis = grstrmB.ReadUShortFromFont();
		if (cfsetThis != grstrmT.ReadUShortFromFont())
			OutputError(ec, ptcase, "ERROR: Feat table - feature", ifeat, ", number of settings");
		cfset += cfsetThis;

		if (fxdVersionB >= 0x00020000)
			grstrmB.ReadShortFromFont(); // pad bytes
		if (fxdVersionT >= 0x00020000)
			grstrmT.ReadShortFromFont();

		//	offset to settings list
		grstrmB.ReadIntFromFont();
		grstrmT.ReadIntFromFont();

		//	flags
		if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
			OutputError(ec, ptcase, "ERROR: Feat table - feature", ifeat, ", flags");

		//	index into name table of UI label
		unsigned int nStrOffsetB = grstrmB.ReadShortFromFont();
		unsigned int nStrOffsetT = grstrmT.ReadShortFromFont();
		std::wstring strB = StringFromNameTable(pNameTblB, 1033, nStrOffsetB);
		std::wstring strT = StringFromNameTable(pNameTblT, 1033, nStrOffsetT);
		if (wcscmp(strB.c_str(), strT.c_str()) != 0)
			OutputError(ec, ptcase, "ERROR: Feat table - label for feature", ifeat);
	}

	if (cfeatB != cfeatT)
		return;

	// setttings
	for (int ifset = 0; ifset < cfset; ifset++)
	{
		if (grstrmB.ReadShortFromFont() != grstrmT.ReadShortFromFont())
			OutputError(ec, ptcase, "ERROR: Feat table - value for setting", ifset);

		// name table offsets
		unsigned int nStrOffsetB = grstrmB.ReadUShortFromFont();
		unsigned int nStrOffsetT = grstrmT.ReadUShortFromFont();
		std::wstring strB = StringFromNameTable(pNameTblB, 1033, nStrOffsetB);
		std::wstring strT = StringFromNameTable(pNameTblT, 1033, nStrOffsetT);
		if (wcscmp(strB.c_str(), strT.c_str()) != 0)
			OutputError(ec, ptcase, "ERROR: Feat table - label for setting", ifset);
	}
}

/*----------------------------------------------------------------------------------------------
	Compare the Sill tables.
----------------------------------------------------------------------------------------------*/
void CompareSillTables(int & ec, TestCase * ptcase, GrIStream & grstrmB, GrIStream & grstrmT)
{
	ReadVersion(grstrmB);
	ReadVersion(grstrmT);

	// number of languages
	int clangB = grstrmB.ReadUShortFromFont();
	int clangT = grstrmT.ReadUShortFromFont();
	if (clangB != clangT)
		OutputError(ec, ptcase, "ERROR: Sill table - number of languages");
	
	// search constants
	if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
		OutputError(ec, ptcase, "ERROR: Sill table - search increment");
	if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
		OutputError(ec, ptcase, "ERROR: Sill table - search loop");
	if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
		OutputError(ec, ptcase, "ERROR: Sill table - search start");

	int cSettings = 0;
	for (int ilang = 0; ilang <= std::min(clangB, clangT); ilang++)
	{
		if (grstrmB.ReadIntFromFont() != grstrmT.ReadIntFromFont())
			OutputError(ec, ptcase, "ERROR: Sill table - language code", ilang);
		int c = grstrmB.ReadUShortFromFont();
		if (c!= grstrmT.ReadUShortFromFont())
			OutputError(ec, ptcase, "ERROR: Sill table - number of settings", ilang);
		cSettings += c;
		if (grstrmB.ReadUShortFromFont() != grstrmT.ReadUShortFromFont())
			OutputError(ec, ptcase, "ERROR: Sill table - offset", ilang);
	}

	if (clangB != clangT)
		return;

	for (int iSetting = 0; iSetting < cSettings; iSetting++)
	{
		if (grstrmB.ReadIntFromFont() != grstrmT.ReadIntFromFont()) // should be unsigned, but oh well
			OutputError(ec, ptcase, "ERROR: Sill table - feature ID", iSetting);
		if (grstrmB.ReadShortFromFont() != grstrmT.ReadShortFromFont())
			OutputError(ec, ptcase, "ERROR: Sill table - default value", iSetting);
		// pad bytes
		grstrmB.ReadShortFromFont();
		grstrmT.ReadShortFromFont();
	}
}

/*----------------------------------------------------------------------------------------------
	Reinterpret the version number from a font table.
----------------------------------------------------------------------------------------------*/
int ReadVersion(GrIStream & grstrm)
{
	int fxdVersion = grstrm.ReadIntFromFont();

	if (fxdVersion < 0x00010000)
		fxdVersion = 0x00010000; // kludge for bug with which some fonts were generated

	return fxdVersion;
}



