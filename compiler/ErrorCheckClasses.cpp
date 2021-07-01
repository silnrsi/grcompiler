/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: PreCompiler.cpp
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Methods to implement the pre-compiler, which does error checking and adjustments.
-------------------------------------------------------------------------------*//*:End Ignore*/

/***********************************************************************************************
	Include files
***********************************************************************************************/
#include "main.h"

#ifdef _MSC_VER
#pragma hdrstop
#endif

#undef THIS_FILE
DEFINE_THIS_FILE


/***********************************************************************************************
	Classes and Glyphs
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Do the pre-compilation tasks for the classes and glyphs. Return false if
	compilation cannot continue due to an unrecoverable error.
----------------------------------------------------------------------------------------------*/
bool GrcManager::PreCompileClassesAndGlyphs(GrcFont * pfont)
{
//	MarkUnusedGlyphMetrics();

	if (!m_prndr->CheckRecursiveGlyphClasses())
		return false;

	if (!GeneratePseudoGlyphs(pfont))
		return false;

	if (!m_prndr->AssignGlyphIDs(pfont, m_wGlyphIDLim, m_hmActualForPseudo))
		return false;

	//	Do this after assigning glyph IDs, since above routine is not smart enough
	//	to handle the fact that we are putting psuedos in the ANY class by means of glyphid().
	if (!AddAllGlyphsToTheAnyClass(pfont, m_hmActualForPseudo))
		return false;

	//	Do this before assigning internal glyph attr IDs, because we only want to assign
	//	IDs for justify levels that are being used.
	if (!MaxJustificationLevel(&m_nMaxJLevel))
		return false;

	if (!AssignInternalGlyphAttrIDs())
		return false;

//	SetGlyphMetricsFromFont(pfont);

	if (!AssignGlyphAttrsToClassMembers(pfont))
		return false;

	if (!ProcessGlyphAttributes(pfont))
		return false;

	if (m_prndr->HasCollisionPass())	// do this after glyph attributes have been processed
		CalculateCollisionOctaboxes(pfont);

	if (!m_prndr->FixGlyphAttrsInRules(this, pfont))
		return false;

	//	Delay assigning internal IDs to classes until we have checked the validity of the
	//	table and pass structure.

	if (!FinalGlyphAttrResolution(pfont))
		return false;

	if (!StorePseudoToActualAsGlyphAttr())
		return false;

	CheckForEmptyClasses();

	return true;
}

/*----------------------------------------------------------------------------------------------
	Check For recursive class definitions. Return false if there is an error.
----------------------------------------------------------------------------------------------*/
bool GdlRenderer::CheckRecursiveGlyphClasses()
{
	bool f;
	std::vector<GdlGlyphClassDefn*> vpglfcStack;
	for (size_t iglfc = 0; iglfc < m_vpglfc.size(); iglfc++)
	{
		f = m_vpglfc[iglfc]->CheckRecursiveGlyphClasses(vpglfcStack);
		if (!f)
			return false;
		Assert(vpglfcStack.size() == 0);
	}
	return true;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlGlyphClassDefn::CheckRecursiveGlyphClasses(std::vector<GdlGlyphClassDefn*> & vpglfcStack)
{
	for (size_t iglfd = 0; iglfd < vpglfcStack.size(); iglfd++)
	{
		if (vpglfcStack[iglfd] == this)
			return false;
	}

	vpglfcStack.push_back(this);
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		bool f = m_vpglfdMembers[iglfd]->CheckRecursiveGlyphClasses(vpglfcStack);
		if (!f)
		{
			g_errorList.AddError(4148, this, "Recursive class definition: ", this->Name());
			return false;
		}
	}
	vpglfcStack.pop_back();

	return true;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlGlyphDefn::CheckRecursiveGlyphClasses(std::vector<GdlGlyphClassDefn*> & /*vpglfcStack*/)
{
	return true; // okay, no embedded classes
}

/*----------------------------------------------------------------------------------------------
	Handle the generation of pseudo glyphs. Return false if compilation cannot continue
	due to an unrecoverable error.
----------------------------------------------------------------------------------------------*/
bool GrcManager::GeneratePseudoGlyphs(GrcFont * pfont)
{
	PseudoSet setpglfExplicitPseudos;
	int cExplicitPseudos = m_prndr->ExplicitPseudos(setpglfExplicitPseudos);

	std::vector<unsigned int> vnAutoUnicode;
	std::vector<utf16> vwAutoGlyphID;
	int cAutoPseudos = (m_prndr->AutoPseudo()) ?
		pfont->AutoPseudos(vnAutoUnicode, vwAutoGlyphID) :
		0;

	utf16 wFirstFree = pfont->FirstFreeGlyph();
	m_wGlyphIDLim = wFirstFree;
	int cwFree = kMaxGlyphsPerFont - wFirstFree;

	if (cwFree < 2)
	{
		char rgch[20];
		itoa(kMaxGlyphsPerFont - 3, rgch, 10);
		g_errorList.AddError(4101, NULL,
			"Font exceeds maximum of ", rgch, " used glyphs",
			GrpLineAndFile(0, 0, ""));
		return false;	// terminate compilation
	}

	if (cExplicitPseudos + cAutoPseudos + 2 > cwFree)	// + 2 for line-break pseudo & non-existent pseudo
	{
		g_errorList.AddError(4102, NULL,
			"Insufficient free glyphs in font to assign pseudo glyphs.",
			GrpLineAndFile(0, 0, ""));
		return true;	// continue compilation
	}

	//	Define the line-break character, and make a glyph class to hold it.
	m_wLineBreak = wFirstFree++;
	////m_wLineBreak = 127;		// for testing
	GdlGlyphClassDefn * pglfcLb = AddAnonymousClass(GrpLineAndFile(0, 0, ""));
	GdlGlyphClassMember * pglfd =
		pglfcLb->AddGlyphToClass(GrpLineAndFile(), kglftGlyphID, (int)m_wLineBreak);
	GdlGlyphDefn * pglf = dynamic_cast<GdlGlyphDefn *>(pglfd);
	Assert(pglf);
	pglf->SetNoRangeCheck();
	Symbol psymLb = m_psymtbl->FindSymbol("#");
	psymLb->SetData(pglfcLb);

	//	Handle explicit pseudos.
	utf16 wFirstPseudo = wFirstFree;
	m_nMaxPseudoUnicode = 0;
	std::set<unsigned int> setnUnicode; // to recognize duplicates
	for (PseudoSet::iterator itset = setpglfExplicitPseudos.begin();
		itset != setpglfExplicitPseudos.end();
		++itset)
	{
		GdlGlyphDefn * pglfPseudo = *itset;
		if (pglfPseudo)  // vector has many empty items in it
		{
			pglfPseudo->SetAssignedPseudo(wFirstFree++);

			unsigned int nUnicode = pglfPseudo->UnicodeInput();
			if (nUnicode == 0)
				;	// no Unicode input specified
			else if (setnUnicode.find(nUnicode) != setnUnicode.end()) // is a member
			{
				//	Duplicate pseudo mapping.
				g_errorList.AddError(4103, pglfPseudo, pglfPseudo->CodepointString(),
					"Duplicate Unicode input -> pseudo assignment.");
			}
			else
			{
				m_vnUnicodeForPseudo.push_back(nUnicode);
				m_vwPseudoForUnicode.push_back(pglfPseudo->AssignedPseudo());
				setnUnicode.insert(nUnicode);
				m_nMaxPseudoUnicode = max(m_nMaxPseudoUnicode, nUnicode);
			}
		}
	}
	//	Handle auto-pseudos.
	Assert(vnAutoUnicode.size() == vwAutoGlyphID.size());
	m_wFirstAutoPseudo = wFirstFree;
	for (size_t iw = 0; iw < vnAutoUnicode.size(); iw++)
	{
		utf16 wAssigned = wFirstFree++;
		CreateAutoPseudoGlyphDefn(wAssigned, vnAutoUnicode[iw], vwAutoGlyphID[iw]);
	}

	if (wFirstFree - wFirstPseudo >= kMaxPseudos)
	{
		char rgch1[20];
		char rgch2[20];
		itoa(wFirstFree - wFirstPseudo, rgch1, 10);
		itoa(kMaxPseudos - 1, rgch2, 10);
		g_errorList.AddError(4104, NULL,
			"Number of pseudo-glyphs (",
			rgch1,
			") exceeds maximum of ",
			rgch2);
	}
	else
	{
		SortPseudoMappings();
	}

	m_wPhantom = wFirstFree++;	// phantom glyph before the beginning of the input

	m_cwGlyphIDs = wFirstFree;

	return true;
}

/*----------------------------------------------------------------------------------------------
	Fill in the set with the explicit pseudo-glyphs in the class database. Return the
	number found. Record an error if the pseudo has an invalid output function (ie, more
	than one glyph specified).
----------------------------------------------------------------------------------------------*/
int GdlRenderer::ExplicitPseudos(PseudoSet & setpglf)
{
	for (size_t iglfc = 0; iglfc < m_vpglfc.size(); iglfc++)
		m_vpglfc[iglfc]->ExplicitPseudos(setpglf, true);
	return setpglf.size();
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::ExplicitPseudos(PseudoSet & setpglf, bool fProcessClasses)
{
	if (fProcessClasses)
	{
		for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
			m_vpglfdMembers[iglfd]->ExplicitPseudos(setpglf, false);
	}
	// else the method is already called for this class directly
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphDefn::ExplicitPseudos(PseudoSet & setpglf, bool /*fProcessClasses*/)
{
	if (m_glft == kglftPseudo)
	{
		Assert(m_pglfOutput);

		// Old std::set implementation:
		//GdlGlyphDefn * p = this;	// kludge until Set can handle const args.
		//setpglf.insert(p);  

		for (size_t i = 0; i < setpglf.size(); i++) {
			if (setpglf[i] == this)
				return;  // already there
		}
		setpglf.push_back(this);
	}
}

/*----------------------------------------------------------------------------------------------
	Create a glyph definition to hold an auto-pseudo glyph. We create a bogus glyph class to
	put it in, just for our convenience. It's just as if they had typed:

		bogus = pseudo(glyphid(<wGlyphID>), <nUnicode>)
----------------------------------------------------------------------------------------------*/
void GrcManager::CreateAutoPseudoGlyphDefn(utf16 wAssigned, int nUnicode, utf16 wGlyphID)
{
	GdlGlyphDefn * pglfOutput = new GdlGlyphDefn(kglftGlyphID, wGlyphID);
	GdlGlyphDefn * pglf = new GdlGlyphDefn(kglftPseudo, pglfOutput, nUnicode);
	pglf->SetAssignedPseudo(wAssigned);

	GdlGlyphClassDefn * pglfc = new GdlGlyphClassDefn();
	GrpLineAndFile lnf;	// bogus
	pglfc->AddMember(pglf, lnf);
	m_prndr->AddGlyphClass(pglfc);
	
	m_vnUnicodeForPseudo.push_back(nUnicode);
	m_vwPseudoForUnicode.push_back(wAssigned);
}

/*----------------------------------------------------------------------------------------------
	Return the pseudo-glyph assigned to the given Unicode value, or 0 if none.
----------------------------------------------------------------------------------------------*/
int GrcManager::PseudoForUnicode(int nUnicode)
{
	for (size_t iw = 0; iw < m_vnUnicodeForPseudo.size(); iw++)
	{
		if (m_vnUnicodeForPseudo[iw] == unsigned(nUnicode))
			return m_vwPseudoForUnicode[iw];
	}
	return 0;
}

/*----------------------------------------------------------------------------------------------
	Return the actual glyph ID for the given pseudo-glyph, or 0 if none.
----------------------------------------------------------------------------------------------*/
int GrcManager::ActualForPseudo(utf16 wPseudo)
{
	//utf16 wActual = 0;
	std::map<utf16, utf16>::iterator hmit = m_hmActualForPseudo.find(wPseudo);
	if (hmit == m_hmActualForPseudo.end()) // no value
		return 0;
	else
		return hmit->second;

	//if (m_hmActualForPseudo.Retrieve(wPseudo, &wActual))
	//	return wActual;
	//else
	//	return 0;
}

/*--------------------------------------------------------------------------------------------*/
int GdlRenderer::ActualForPseudo(utf16 wPseudo)
{
	for (size_t ipglfc = 0; ipglfc < m_vpglfc.size(); ipglfc++)
	{
		utf16 wActual = m_vpglfc[ipglfc]->ActualForPseudo(wPseudo);
		if (wActual != 0)
			return wActual;
	}
	return 0;
}

/*--------------------------------------------------------------------------------------------*/
int GdlGlyphClassDefn::ActualForPseudo(utf16 wPseudo)
{
	for (size_t ipglfd = 0; ipglfd < m_vpglfdMembers.size(); ipglfd++)
	{
		utf16 wActual = m_vpglfdMembers[ipglfd]->ActualForPseudo(wPseudo);
		if (wActual != 0)
			return wActual;
	}
	return 0;
}

/*--------------------------------------------------------------------------------------------*/
int GdlGlyphDefn::ActualForPseudo(utf16 wPseudo)
{
	if (m_glft == kglftPseudo && m_wPseudo == wPseudo && m_pglfOutput)
	{
		utf16 wOutput = m_pglfOutput->m_vwGlyphIDs[0];
		return wOutput;
	}
	return 0;
}

/*----------------------------------------------------------------------------------------------
	Sort the unicode-to-pseudo mappings in order of the unicode values.
----------------------------------------------------------------------------------------------*/
void GrcManager::SortPseudoMappings()
{
	Assert(m_vnUnicodeForPseudo.size() == m_vwPseudoForUnicode.size());

	for (int i1 = 0; i1 < signed(m_vnUnicodeForPseudo.size()) - 1; i1++)
	{
		unsigned int nTmp = m_vnUnicodeForPseudo[i1];

		for (size_t i2 = i1 + 1; i2 < m_vnUnicodeForPseudo.size(); i2++)
		{
			if (m_vnUnicodeForPseudo[i2] < nTmp)
			{
				//	Swap
				m_vnUnicodeForPseudo[i1] = m_vnUnicodeForPseudo[i2];
				m_vnUnicodeForPseudo[i2] = nTmp;
				nTmp = m_vnUnicodeForPseudo[i1];

				utf16 wTmp = m_vwPseudoForUnicode[i1];
				m_vwPseudoForUnicode[i1] = m_vwPseudoForUnicode[i2];
				m_vwPseudoForUnicode[i2] = wTmp;
			}
		}
	}
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Define the ANY class to include all glyphs. This must be done after we've set up
	the pseudo-glyphs and defined the phantom glyph, because they must be included too.
----------------------------------------------------------------------------------------------*/
bool GrcManager::AddAllGlyphsToTheAnyClass(GrcFont * pfont,
	std::map<utf16, utf16> & hmActualForPseudo)
{
	Symbol psym = m_psymtbl->FindSymbol("ANY");
	GdlGlyphClassDefn * pglfcAny = psym->GlyphClassDefnData();
	Assert(pglfcAny);

	GdlGlyphDefn * pglf = new GdlGlyphDefn(kglftGlyphID, (utf16)0, m_cwGlyphIDs - 1);
	GrpLineAndFile lnf;	// bogus
	pglfcAny->AddMember(pglf, lnf);

	pglfcAny->AssignGlyphIDs(pfont, m_cwGlyphIDs, hmActualForPseudo);

	return true;
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Determine the glyph ID equivalents for each glyph definition; ie, convert Unicode,
	codepoints, postscript to glyph ID.
----------------------------------------------------------------------------------------------*/
bool GdlRenderer::AssignGlyphIDs(GrcFont * pfont, utf16 wGlyphIDLim,
	std::map<utf16, utf16> & hmActualForPseudo)
{
	for (size_t iglfc = 0; iglfc < m_vpglfc.size(); iglfc++)
		m_vpglfc[iglfc]->AssignGlyphIDs(pfont, wGlyphIDLim, hmActualForPseudo);
		
	return true;
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::AssignGlyphIDs(GrcFont * pfont, utf16 wGlyphIDLim,
	std::map<utf16, utf16> & hmActualForPseudo)
{
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		m_vpglfdMembers[iglfd]->AssignGlyphIDsToClassMember(pfont, wGlyphIDLim,
			hmActualForPseudo);
	}
}

void GdlGlyphIntersectionClassDefn::AssignGlyphIDs(GrcFont * pfont, utf16 wGlyphIDLim,
	std::map<utf16, utf16> & hmActualForPseudo)
{
	for (size_t iglfd = 0; iglfd < m_vpglfdSets.size(); iglfd++)
	{
		m_vpglfdSets[iglfd]->AssignGlyphIDsToClassMember(pfont, wGlyphIDLim,
			hmActualForPseudo);
	}
	ComputeMembers();
}

void GdlGlyphDifferenceClassDefn::AssignGlyphIDs(GrcFont * pfont, utf16 wGlyphIDLim,
	std::map<utf16, utf16> & hmActualForPseudo)
{
	m_pglfdMinuend->AssignGlyphIDsToClassMember(pfont, wGlyphIDLim,
		hmActualForPseudo);
	// The subtrahend is not processed at the top level.
	m_pglfdSubtrahend->AssignGlyphIDs(pfont, wGlyphIDLim, hmActualForPseudo);

	ComputeMembers();
}

/*----------------------------------------------------------------------------------------------
	Determine the glyph ID equivalents for the recipient by virtue of its being a member
	of a class. Only do this for simple glyphs; classes are handled separately.
----------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::AssignGlyphIDsToClassMember(GrcFont * /*pfont8*/, utf16 /*wGlyphIDLim*/,
	std::map<utf16, utf16> & /*hmActualForPseudo*/, bool /*fLookUpPseudos*/)
{
	//	Do nothing; this class will be handled separately at the top level.
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphDefn::AssignGlyphIDsToClassMember(GrcFont * pfont, utf16 wGlyphIDLim,
	std::map<utf16, utf16> & hmActualForPseudo, bool fLookUpPseudos)
{
	Assert(m_vwGlyphIDs.size() == 0);

	utf16 w;
	unsigned int n;
	utf16 wGlyphID;
	unsigned int nUnicode;
	utf16 wFirst, wLast;

	bool fIgnoreBad = g_cman.IgnoreBadGlyphs();

	switch (m_glft)
	{
	case kglftGlyphID:
		if (m_nFirst > m_nLast)
			g_errorList.AddError(4105,this,
				"Invalid glyph ID range");

		wFirst = (utf16)m_nFirst;
		wLast = (utf16)m_nLast;
		for (w = wFirst; w <= wLast; ++w)
		{
			if (!m_fNoRangeCheck && w >= wGlyphIDLim)
				g_errorList.AddError(4106, this,
					"Glyph ID out of range: ",
					GlyphIDString(w));
			else
				m_vwGlyphIDs.push_back(w);

			// Since incrementing 0xFFFF will produce zero:
			if (w == 0xFFFF)
				break;
		}
		break;

	case kglftUnicode:
		if (m_nFirst > m_nLast)
			g_errorList.AddError(4107, this,
				"Invalid Unicode range");

		for (n = m_nFirst; n <= m_nLast; ++n)
		{
			if (n == 0x0000FFFE || n == 0x0000FFFF)
			{
				g_errorList.AddError(4108, this, "U+",
					CodepointIDString(n),
					" is not a valid Unicode codepoint");
				wGlyphID = 0;
			}
			else
			{
				if (!fLookUpPseudos || (wGlyphID = g_cman.PseudoForUnicode(n)) == 0)
					wGlyphID = pfont->GlyphFromCmap(n, this);
				if (wGlyphID == 0)
				{
					if (fIgnoreBad)
					{
						g_errorList.AddWarning(4501, this,
							"Unicode character not present in cmap: U+",
							CodepointIDString(n), "; definition will be ignored");
						m_vwGlyphIDs.push_back(kBadGlyph);
					}
					else
						g_errorList.AddError(4109, this,
							"Unicode character not present in cmap: U+",
							CodepointIDString(n));
				}
				else
					m_vwGlyphIDs.push_back(wGlyphID);
			}

			// Since incrementing 0xFFFFFFFF will produce zero:
			if (n == 0xFFFFFFFF)
				break;
		}
		break;

	case kglftPostscript:
		wGlyphID = pfont->GlyphFromPostscript(m_sta, this, !fIgnoreBad);
		if (wGlyphID == 0)
		{
			if (fIgnoreBad)
			{
				g_errorList.AddWarning(4502, this,
					"Invalid postscript name: ",
					m_sta, "; definition will be ignored");
				m_vwGlyphIDs.push_back(kBadGlyph);
			}
			else
				g_errorList.AddError(4110, this,
					"Invalid postscript name: ",
					m_sta);
		}
		else
			m_vwGlyphIDs.push_back(wGlyphID);
		break;

	case kglftCodepoint:
		char rgchCdPg[20];
		itoa(m_wCodePage, rgchCdPg, 10);
		if (m_nFirst == 0 && m_nLast == 0)
		{
			for (size_t ich = 0; ich < m_sta.length(); ich++)
			{
				char rgchCdPt[2] = {0,0};
				rgchCdPt[0] = m_sta[ich];
				nUnicode = pfont->UnicodeFromCodePage(m_wCodePage, m_sta[ich], this);
				if (nUnicode == 0)
					g_errorList.AddError(4111, this,
						"Codepoint '",
						rgchCdPt,
						"' not valid for codepage ",
						rgchCdPg);
				else
				{
					if (!fLookUpPseudos || (wGlyphID = g_cman.PseudoForUnicode(nUnicode)) == 0)
						wGlyphID = pfont->GlyphFromCmap(nUnicode, this);
					if (wGlyphID == 0)
					{
						if (fIgnoreBad)
						{
							g_errorList.AddWarning(4503, this,
								"Unicode character U+",
								UsvString(nUnicode),
								" (ie, codepoint '",
								rgchCdPt,
								"' in codepage ",
								std::string(rgchCdPg),
								") not present in cmap; definition will be ignored");
							m_vwGlyphIDs.push_back(kBadGlyph);
						}
						else
							g_errorList.AddError(4112, this,
								"Unicode character U+",
								UsvString(nUnicode),
								" (ie, codepoint '",
								rgchCdPt,
								"' in codepage ",
								std::string(rgchCdPg),
								") not present in cmap");
					}
					else
						m_vwGlyphIDs.push_back(wGlyphID);
				}
			}
		}
		else
		{
			if (m_nFirst > m_nLast)
				g_errorList.AddError(4113, this,
					"Invalid codepoint range");

			utf16 wFirst = (utf16)m_nFirst;
			utf16 wLast = (utf16)m_nLast;

			for (w = wFirst; w <= wLast; w++)
			{
				nUnicode = pfont->UnicodeFromCodePage(m_wCodePage, w, this);
				if (nUnicode == 0)
					g_errorList.AddError(4114, this,
						"Codepoint 0x",
						UsvString(w),
						" not valid for codepage ",
						rgchCdPg);
				else
				{
					wGlyphID = pfont->GlyphFromCmap(nUnicode, this);
					if (wGlyphID == 0)
					{
						if (fIgnoreBad)
						{
							g_errorList.AddWarning(4504, this,
								"Unicode character U+",
								UsvString(nUnicode),
								" (ie, codepoint 0x",
								UsvString(w),
								" in codepage ",
								rgchCdPg,
								") not present in cmap; definition will be ignored");
							m_vwGlyphIDs.push_back(kBadGlyph);
						}
						else
							g_errorList.AddError(4115, this,
								"Unicode character U+",
								UsvString(nUnicode),
								" (ie, codepoint 0x",
								UsvString(w),
								" in codepage ",
								rgchCdPg,
								") not present in cmap");
					}
					else
						m_vwGlyphIDs.push_back(wGlyphID);
				}
			}

			// Since incrementing 0xFFFF will produce zero:
			if (w == 0xFFFF)
				break;
		}
		break;

	case kglftPseudo:
		Assert(m_nFirst == 0);
		Assert(m_nLast == 0);
		Assert(m_pglfOutput);
		//	While we're at it, determine the output glyph ID. Record an error if there
		//	is more than one, or none, or the glyph ID == 0.
		m_pglfOutput->AssignGlyphIDsToClassMember(pfont, wGlyphIDLim, hmActualForPseudo, false);
		if (m_pglfOutput->m_vwGlyphIDs.size() > 1)
		{
			if (fIgnoreBad)
			{
				g_errorList.AddWarning(4505, this,
					"Pseudo-glyph -> glyph ID mapping results in more than one glyph; definition will be ignored");
				m_vwGlyphIDs.push_back(kBadGlyph);
			}
			else
				g_errorList.AddError(4116, this,
					"Pseudo-glyph -> glyph ID mapping results in more than one glyph");
		}
		else if (m_pglfOutput->m_vwGlyphIDs.size() == 0)
		{
			if (fIgnoreBad)
			{
				g_errorList.AddWarning(4506, this,
					"Pseudo-glyph -> glyph ID mapping results in no valid glyph; definition will be ignored");
				m_vwGlyphIDs.push_back(kBadGlyph);
			}
			else
				g_errorList.AddError(4117, this,
					"Pseudo-glyph -> glyph ID mapping results in no valid glyph");
		}
		else if (m_pglfOutput->m_vwGlyphIDs[0] == 0)
		{
			if (fIgnoreBad)
			{
				g_errorList.AddWarning(4507, this,
					"Pseudo-glyph cannot be mapped to glyph ID 0; definition will be ignored");
				m_vwGlyphIDs.push_back(kBadGlyph);
			}
			else
				g_errorList.AddError(4118, this,
					"Pseudo-glyph cannot be mapped to glyph ID 0");
		}
		else
		{
			//	It is the assigned pseudo glyph ID which is the 'contents' of this glyph defn.
			m_vwGlyphIDs.push_back(m_wPseudo);

			//	Store the pseudo-to-actual assignment in the map.
			std::pair<utf16, utf16> hmPair;
			hmPair.first = m_wPseudo;
			hmPair.second = m_pglfOutput->m_vwGlyphIDs[0];
			hmActualForPseudo.insert(hmPair);
			//hmActualForPseudo.Insert(m_wPseudo, m_pglfOutput->m_vwGlyphIDs[0], true);
		}
		break;
		
	default:
		Assert(false);
	}
}

/*----------------------------------------------------------------------------------------------
	Return the number of glyph IDs per class.
----------------------------------------------------------------------------------------------*/
int GdlGlyphClassDefn::GlyphIDCount()
{
	int c = 0;
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
		 c += m_vpglfdMembers[iglfd]->GlyphIDCount();
	return c;
}

/*--------------------------------------------------------------------------------------------*/
int GdlGlyphDefn::GlyphIDCount()
{
	int cGlyph = 0;
	for (size_t iw = 0; iw < m_vwGlyphIDs.size(); iw++)
	{
		if (m_vwGlyphIDs[iw] != kBadGlyph)
			cGlyph++;
	}
	return cGlyph;
}

/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Compute the members of the classes that are more complicated than a simple union.
----------------------------------------------------------------------------------------------*/
void GdlGlyphIntersectionClassDefn::ComputeMembers()
{
	std::vector<utf16> vgidResult;

	std::vector<utf16> vgid1;
	m_vpglfdSets[0]->FlattenGlyphList(vgid1);

	for (size_t iglfc = 1; iglfc < this->m_vpglfdSets.size(); iglfc++)
	{
		vgidResult.clear();
		std::vector<utf16> vgid2;
		m_vpglfdSets[iglfc]->FlattenGlyphList(vgid2);
		for (size_t igid1 = 0; igid1 < vgid1.size(); igid1++)
		{
			bool fFound = false;
			for (size_t igid2 = 0; igid2 < vgid2.size(); igid2++)
			{
				if (vgid2[igid2] == vgid1[igid1])
				{
					fFound = true;
					break;
				}
			}
			if (fFound)
				vgidResult.push_back(vgid1[igid1]);

		}
		vgid1.assign(vgidResult.begin(), vgidResult.end());
	}

	// Fake a simple class definition that contains these glyphs.
	GrpLineAndFile lnf = this->LineAndFile();
	for (size_t igid = 0; igid < vgidResult.size(); igid++)
	{
		GdlGlyphDefn * pglfd = new GdlGlyphDefn(kglftGlyphID, vgidResult[igid]);
		pglfd->AddGlyphID(vgidResult[igid]);
		this->AddMember(pglfd, lnf);
	}
}


void GdlGlyphDifferenceClassDefn::ComputeMembers()
{
	std::vector<utf16> vgidResult;
	m_pglfdMinuend->FlattenGlyphList(vgidResult);

	std::vector<utf16> vgid2;
	m_pglfdSubtrahend->FlattenGlyphList(vgid2);

	for (size_t igid2 = 0; igid2 < vgid2.size(); igid2++)
	{
		for (size_t igid1 = 0; igid1 < vgidResult.size(); igid1++)
		{
			if (vgidResult[igid1] == vgid2[igid2])
			{
				vgidResult.erase(vgidResult.begin() + igid1);
				break;
			}
		}
	}

	// Fake a simple class definition that contains these glyphs.
	GrpLineAndFile lnf = this->LineAndFile();
	for (size_t igid = 0; igid < vgidResult.size(); igid++)
	{
		GdlGlyphDefn * pglfd = new GdlGlyphDefn(kglftGlyphID, vgidResult[igid]);
		pglfd->AddGlyphID(vgidResult[igid]);
		this->AddMember(pglfd, lnf);
	}
}

/**********************************************************************************************/
/*----------------------------------------------------------------------------------------------
	Calculate the highest justification level used. If justification is not referenced at all,
	the result = -2; -1 means only non-leveled attributes are used (justify.stretch, etc).
	Return false if they have used too high a level.
----------------------------------------------------------------------------------------------*/
bool GrcManager::MaxJustificationLevel(int * pnJLevel)
{
	*pnJLevel = -2; // no reference to justification

	m_prndr->MaxJustificationLevel(&m_nMaxJLevel);
	m_fBasicJust = (m_nMaxJLevel == -2);
	return (m_nMaxJLevel <= kMaxJustLevel);
}

/*--------------------------------------------------------------------------------------------*/
void GdlRenderer::MaxJustificationLevel(int * pnJLevel)
{
	//	Glyph atrributes:
	for (size_t ipglfc = 0; ipglfc < m_vpglfc.size(); ipglfc++)
	{
		m_vpglfc[ipglfc]->MaxJustificationLevel(pnJLevel);
		if (*pnJLevel >= kMaxJustLevel)
			return;
	}
	//	Rules:
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		m_vprultbl[iprultbl]->MaxJustificationLevel(pnJLevel);
		if (*pnJLevel >= kMaxJustLevel)
			return;
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::MaxJustificationLevel(int * pnJLevel)
{
	//	For each attribute assignment in the value list:
	for (size_t ipglfa = 0; ipglfa < m_vpglfaAttrs.size(); ipglfa++)
	{
		Symbol psym = m_vpglfaAttrs[ipglfa]->GlyphSymbol();
		int n = psym->JustificationLevel();
		if (n > kMaxJustLevel)
		{
			char rgch[10];
			itoa(kMaxJustLevel, rgch, 10);
			g_errorList.AddError(4122, this,
				"Highest justification level permitted = ", rgch);
		}
		*pnJLevel = max(*pnJLevel, n);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::MaxJustificationLevel(int * pnJLevel)
{
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		m_vppass[ippass]->MaxJustificationLevel(pnJLevel);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlPass::MaxJustificationLevel(int * pnJLevel)
{
	for (size_t iprule = 0; iprule < m_vprule.size(); iprule++)
	{
		m_vprule[iprule]->MaxJustificationLevel(pnJLevel);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRule::MaxJustificationLevel(int * pnJLevel)
{
	// Note: justify attributes are illegal in rule-level constraints.

	for (size_t iprit = 0; iprit < m_vprit.size(); iprit++)
	{
		m_vprit[iprit]->MaxJustificationLevel(pnJLevel);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleItem::MaxJustificationLevel(int * pnJLevel)
{
	if (m_pexpConstraint)
	{
		int n = -2;
		m_pexpConstraint->MaxJustificationLevel(&n);
		if (n > kMaxJustLevel)
		{
			char rgch[10];
			itoa(kMaxJustLevel, rgch, 10);
			g_errorList.AddError(4122, this,
				"Highest justification level permitted = ", rgch);
		}
		*pnJLevel = max(*pnJLevel, n);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlSetAttrItem::MaxJustificationLevel(int * pnJLevel)
{
	GdlRuleItem::MaxJustificationLevel(pnJLevel);

	for (size_t ipavs = 0; ipavs < m_vpavs.size(); ipavs++)
	{
		int n = -2;
		m_vpavs[ipavs]->MaxJustificationLevel(&n);
		if (n > kMaxJustLevel)
		{
			char rgch[10];
			itoa(kMaxJustLevel, rgch, 10);
			g_errorList.AddError(4122, this,
				"Highest justification level permitted = ", rgch);
		}
		*pnJLevel = max(*pnJLevel, n);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlAttrValueSpec::MaxJustificationLevel(int * pnJLevel)
{
	int n = m_psymName->JustificationLevel();
	if (n > kMaxJustLevel)
	{
		char rgch[10];
		itoa(kMaxJustLevel, rgch, 10);
		g_errorList.AddError(4122, this,
			"Highest justification level permitted = ", rgch);
	}
	*pnJLevel = max(*pnJLevel, n);
}

/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Return true if there is at least one collision-fixing pass.
----------------------------------------------------------------------------------------------*/
bool GdlRenderer::HasCollisionPass()
{
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		if (m_vprultbl[iprultbl]->HasCollisionPass())
			return true;
	}
	return false;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlRuleTable::HasCollisionPass()
{
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		if (m_vppass[ippass]->CollisionFix() > 0)
			return true;
	}
	return false;
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Calculate octaboxes to use in collision fixing.
----------------------------------------------------------------------------------------------*/
void GrcManager::CalculateCollisionOctaboxes(GrcFont * pfont)
{
	Symbol psymComplex = m_psymtbl->FindSymbol(GrcStructName("collision", "complexFit"));
	int nAttrIdComplex = psymComplex->InternalID();

	m_vgbdy.resize(m_wGlyphIDLim);
	for (utf16 wGid = 0; wGid < m_wGlyphIDLim; wGid++)
	{
		// The collision.complexFit attr tells whether the shape of this glyph is complex
		// enough to require a grid of octaboxes to represent its shape rather than a single
		// octabox.
		bool fComplex = false;
		GdlExpression * pexp;
		int nPR;
		int munitPR;
		bool fOverride, fShadow;
		GrpLineAndFile lnf;
		m_pgax->Get(wGid, nAttrIdComplex,
				&pexp, &nPR, &munitPR, &fOverride, &fShadow, &lnf);
		if (!pexp)
			fComplex = false;
		else
		{
			int n;
			if (!pexp->ResolveToInteger(&n, false))
				fComplex = false;
			else
				fComplex = (n > 0);
		}
		m_vgbdy[wGid].Initialize(wGid);
		m_vgbdy[wGid].OverlayGrid(pfont, fComplex);
	}
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Assign an internal ID for each glyph attribute. Specifically, the ID is assigned to the
	generic form of the glyph attribute, and the class-specific versions make use of it.

	The IDs are assigned with the first batch belonging to the component bases; the next batch
	being the corresponding component fields in a specified order, the justification
	attributes, and finally and all other glyph	attributes following. So the list might look
	like this:

	0:	component.X
	1:	component.Y
	2:	component.Z
	3:	component.X.top
	4:	component.X.bottom
	5:	component.X.left
	6:	component.X.right
	7:	component.Y.top
	8:	component.Y.bottom
	9:	component.Y.left
	10:	component.Y.right
	11:	component.Z.top
	12:	component.Z.bottom
	13:	component.Z.left
	14:	component.Z.right
	15:	pointA.x
	16:	pointA.y
	    etc.

	So given the total number of components, we can find the list of components,
	the corresponding component box fields, and the other glyph attributes.

	Review: should we include glyph metrics in this list too? (At least the used ones).
----------------------------------------------------------------------------------------------*/
bool GrcManager::AssignInternalGlyphAttrIDs()
{
	int cpass = m_prndr->NumberOfPasses();

	bool fCollFix = m_prndr->HasCollisionPass();

	//	Assign the first batch of IDs to the built-in attributes;
	//	this is an optimization for the Graphite2 engine.
	m_psymtbl->AssignInternalGlyphAttrIDs(this, m_psymtbl, m_vpsymGlyphAttrs, kgappBuiltIn, -1, -1, -1, cpass);
	m_cpsymBuiltIn = m_vpsymGlyphAttrs.size();

	//	Assign the next batch of IDs to component bases (ie, component.X).
	m_psymtbl->AssignInternalGlyphAttrIDs(this, m_psymtbl, m_vpsymGlyphAttrs, kgappCompBase, -1, -1, -1, 0);
	m_cpsymComponents = m_vpsymGlyphAttrs.size() - m_cpsymBuiltIn;

	//	Assign the next batch to component box fields. (ie, component.X.top/bottom/left/right).
	m_psymtbl->AssignInternalGlyphAttrIDs(this, m_psymtbl, m_vpsymGlyphAttrs, kgappCompBox,
		m_cpsymBuiltIn, m_cpsymComponents, -1, 0);

	//	Assign the next batch to the justification attributes.
	m_psymtbl->AssignInternalGlyphAttrIDs(this, m_psymtbl, m_vpsymGlyphAttrs, kgappJustify, -1, -1,
		NumJustLevels(), 0);

	//	Finally, assign IDs to everything else.
	m_psymtbl->AssignInternalGlyphAttrIDs(this, m_psymtbl, m_vpsymGlyphAttrs, kgappOther, -1, -1, -1, 0);

	if (m_vpsymGlyphAttrs.size() >= kMaxGlyphAttrs)
	{
		char rgch1[20];
		char rgch2[20];
		itoa(m_vpsymGlyphAttrs.size(), rgch1, 10);
		itoa(kMaxGlyphAttrs - 1, rgch2, 10);
		g_errorList.AddError(4123, NULL,
			"Number of glyph attributes (",
			rgch1,
			") exceeds maximum of ",
			rgch2);
	}

	return true;
}

/*----------------------------------------------------------------------------------------------
	Loop through the symbol table, assigning internal IDs to each glyph attribute.
	Arguments:
		pcman				- to access flags
		psymtblMain			- main, top-level symbol table
		vpsymGlyphAttrIDs	- list of assigned symbols
		gapp				- 1: process built-in attributes
							  2: process component bases;
							  3: process component box fields;
							  4: process justification attributes
							  5: process everything else
		cpsymBuiltIn		- only used on pass 3
		cpsymComponents		- only used on pass 3
		cJLevels			- only used on pass 4
		cpass				- only used in pass 1
----------------------------------------------------------------------------------------------*/
bool GrcSymbolTable::AssignInternalGlyphAttrIDs(GrcManager * pcman, GrcSymbolTable * psymtblMain,
	std::vector<Symbol> & vpsymGlyphAttrIDs, int gapp, int cpsymBuiltIn, int cpsymComponents,
	int cJLevels, int cpass)
{
	bool fCollFix = pcman->Renderer()->HasCollisionPass();
	bool fBidi = pcman->Renderer()->Bidi();
	bool fPassOpt = pcman->IncludePassOptimizations();

	if (gapp == kgappJustify)
	{
		//	Justification attributes must be put in a specific order, with the corresponding
		//	attributes for the various levels contiguous. Eg, if cJLevels = 2:
		//		justify.0.stretch
		//		justify.1.stretch
		//		justify.2.stretch
		//		justify.0.shrink
		//		justify.1.shrink
		//		justify.2.shrink
		//		justify.0.step
		//		etc.
		//	(Actually, there is a series of "high-word" stretch values that come immediately
		//	after stretch, which are used to hold bits 16-32 of any very large stretch values.)
		std::vector<std::string> vstaJAttr;
		vstaJAttr.push_back("stretch");
		vstaJAttr.push_back("stretchHW");	// high word, for large stretch values
		vstaJAttr.push_back("shrink");
		vstaJAttr.push_back("step");
		vstaJAttr.push_back("weight");
		std::vector<int> vnLevel0Ids;
		for (size_t istaJAttr = 0; istaJAttr < vstaJAttr.size(); istaJAttr++)
		{
			int nLevel;
			for (nLevel = 0; nLevel < cJLevels; nLevel++)
			{
				char rgchLev[20];
				itoa(nLevel, rgchLev, 10);
				GrcStructName xnsJAttr("justify", rgchLev, vstaJAttr[istaJAttr]);

				Symbol psymJAttr = FindSymbol(xnsJAttr);
				Assert(psymJAttr);
				int id = AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymJAttr);
				if (nLevel == 0)
					vnLevel0Ids.push_back(id);
			}
		}
		// Set the ID of the non-leveled attributes to the same as level 0.
		if (cJLevels > 0)
			for (size_t istaJAttr = 0; istaJAttr < vstaJAttr.size(); istaJAttr++)
			{
				GrcStructName xnsJAttrNoLevel("justify", vstaJAttr[istaJAttr]);
				Symbol psymJAttrNoLevel = FindSymbol(xnsJAttrNoLevel);
				Assert(psymJAttrNoLevel);
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymJAttrNoLevel);
				psymJAttrNoLevel->SetInternalID(vnLevel0Ids[istaJAttr]); // change it
			}
		return true;
	}

	// Make a separate list of symbols to process, because the iterators get confused when you are
	// changing the hash-map underneath it at the same time.
	std::vector<Symbol> vpsymToProcess;
	for (SymbolTableMap::iterator it = EntriesBegin();
		it != EntriesEnd();
		++it)
	{
		Symbol psym = it->second; // GetValue();
		//Symbol psym = it->GetValue();
		vpsymToProcess.push_back(psym);
	}

	for (size_t ipsym = 0; ipsym < vpsymToProcess.size(); ipsym++)
	{
		Symbol psym = vpsymToProcess[ipsym];

		if (psym->m_psymtblSubTable)
		{
			if (!psym->IsGeneric() && gapp == kgappCompBase && psym->IsComponentBase())
			{
				Symbol psymGeneric = psym->Generic();
				if (!psymGeneric)
				{
					//	Undefined glyph attribute--ignore.
				}
				else
				{
					AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymGeneric);
					psym->SetInternalID(psymGeneric->InternalID());
				}

				//	Don't process the component fields until we have processed all the 
				//	component bases.
			}
			else
				psym->m_psymtblSubTable->AssignInternalGlyphAttrIDs(pcman, psymtblMain,
					vpsymGlyphAttrIDs, gapp, cpsymBuiltIn, cpsymComponents, cJLevels, 0);
		}
		else if (!psym->IsGeneric() &&
			psym->FitsSymbolType(ksymtGlyphAttr))
			// || it->FitsSymbolType(ksymtGlyphMetric) && it->Used()
		{
			Symbol psymGeneric = psym->Generic();
			//bool f = psym->FitsSymbolType(ksymtGlyphAttr);
			if (!psymGeneric)
				// Probably because this was a glyph metric--already gave an error.
				continue;
			Assert(psymGeneric);

			if (gapp == kgappCompBox && psym->IsComponentBoxField())
			{
				int ipsymOffset = 0;
				std::string sta = psym->LastField();
				if (sta == "top")
					ipsymOffset = 0;
				else if (sta == "bottom")
					ipsymOffset = 1;
				else if (sta == "left")
					ipsymOffset = 2;
				else if (sta == "right")
					ipsymOffset = 3;
				else
				{
					Assert(false);
				}

				Assert(ipsymOffset < kFieldsPerComponent);

				int nBaseID = psym->BaseLigComponent()->InternalID();
				int ipsym = cpsymBuiltIn + cpsymComponents + ((nBaseID - cpsymBuiltIn) * kFieldsPerComponent) 
					+ ipsymOffset;
#ifndef NDEBUG
				int i = 
#endif
				    AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymGeneric, ipsym);
				Assert(i == ipsym);
				psymGeneric->SetInternalID(ipsym);
				psym->SetInternalID(psymGeneric->InternalID());
			}
			else if (gapp == kgappOther && psym->IsIgnorableOffsetAttr() && !g_cman.OffsetAttrs())
			{
				// Ignore - but set the internal ID so we can recognize it.
				psym->Generic()->SetInternalID(kInvalid);
			}
			else if (gapp == kgappOther && !psym->IsComponentBoxField())
			{
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymGeneric);
				psym->SetInternalID(psymGeneric->InternalID());

				//	Is this an attribute that might need to be converted to gpoint?
				int iv = psym->FieldIndex("gpath");
				iv = (iv == -1) ? psym->FieldIndex("x") : iv;
				iv = (iv == -1) ? psym->FieldIndex("y") : iv;
				if (iv > -1 && g_cman.OffsetAttrs())
				{
					//	We are going to convert all 'gpath' attributes to 'gpoint',
					//	so create that attribute too. And we might convert x/y coordinates
					//	to gpoint.
					//	(We have to do this before we create the matrix to hold all
					//	the glyph attribute values--in AssignGlyphAttrsToClassMembers--
					//	so that that routine will make room for it.)
					GrcStructName xns;
					psym->GetStructuredName(&xns);
					xns.DeleteField(iv);
					xns.InsertField(iv, "gpoint");
					Symbol psymGPoint =
						psymtblMain->AddGlyphAttrSymbol(xns, psym->LineAndFile(), kexptNumber);
					
					Symbol psymGPointGeneric = psymGPoint->Generic();
					Assert(psymGPointGeneric);
					AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymGPointGeneric);
					psymGPoint->SetInternalID(psymGPointGeneric->InternalID());
				}
				iv = psym->FieldIndex("gpoint");
				if (iv > -1)
				{
					//	We might need to convert gpoint to x/y.
					GrcStructName xns;
					psym->GetStructuredName(&xns);
					xns.DeleteField(iv);
					xns.InsertField(iv, "x");
					Symbol psymX =
						psymtblMain->AddGlyphAttrSymbol(xns, psym->LineAndFile(), kexptMeas);
					
					Symbol psymXGeneric = psymX->Generic();
					Assert(psymXGeneric);
					AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymXGeneric);
					psymX->SetInternalID(psymXGeneric->InternalID());

					xns.DeleteField(iv);
					xns.InsertField(iv, "y");
					Symbol psymY =
						psymtblMain->AddGlyphAttrSymbol(xns, psym->LineAndFile(), kexptMeas);
					
					Symbol psymYGeneric = psymY->Generic();
					Assert(psymYGeneric);
					AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymYGeneric);
					psymY->SetInternalID(psymYGeneric->InternalID());
				}
			}				
		}
		else if (gapp == kgappBuiltIn && psym->FitsSymbolType(ksymtGlyphAttr)
			&& psym->FieldCount() == 1
			&& (psym->FieldIs(0, "directionality")
				|| psym->FieldIs(0, "breakweight")
				|| psym->FieldIs(0, "*actualForPseudo*")
				|| psym->FieldIs(0, "*skipPasses*")))
		{
			if (psym->FieldIs(0, "*skipPasses*") && !fPassOpt) {
				// Leave undefined, so it doesn't get output to Silf table and confuse OTS.
				psym->SetInternalID(0);
			}
			else {
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psym);
				if (psym->FieldIs(0, "*skipPasses*") && cpass > kPassPerSPbitmap)
				{
					Symbol psym2 = PreDefineSymbol(GrcStructName("*skipPasses2*"), ksymtGlyphAttr, kexptNumber);
					psym2->m_fGeneric = true;
					AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psym2);
					Assert(psym2->InternalID() == psym->InternalID() + 1);
				}
			}
		}
		else if (gapp == kgappBuiltIn && psym->FitsSymbolType(ksymtGlyphAttr)
			&& psym->FieldCount() == 2
			&& psym->FieldIs(0, "mirror"))
		{
			//	Put mirror.glyph first, immediately followed by mirror.isEncoded.
			Symbol psymGlyph = psymtblMain->FindSymbol(GrcStructName("mirror", "glyph"));
			Symbol psymIsEnc = psymtblMain->FindSymbol(GrcStructName("mirror", "isEncoded"));
			if (fBidi)
			{
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymGlyph);
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymIsEnc);
				Assert(psymGlyph->InternalID() != 0);
				Assert(psymGlyph->InternalID() + 1 == psymIsEnc->InternalID());
			}
			else
			{
				psymGlyph->SetInternalID(0);
				psymIsEnc->SetInternalID(0);
			}
		}
		else if (gapp == kgappBuiltIn && psym->FitsSymbolType(ksymtGlyphAttr)
			&& psym->FieldCount() > 1
			&& psym->FieldIs(0, "collision"))
		{
			if (fCollFix)
			{
				//	Put collision.flags first, immediately followed by the others in a specific order.
				//	This must match the assumptions in the engine.
				Symbol psymColFlags = psymtblMain->FindSymbol(GrcStructName("collision", "flags"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymColFlags);
				Symbol psymColMinX = psymtblMain->FindSymbol(GrcStructName("collision", "min", "x"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymColMinX);
				Symbol psymColMinY = psymtblMain->FindSymbol(GrcStructName("collision", "min", "y"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymColMinY);
				Symbol psymColMaxX = psymtblMain->FindSymbol(GrcStructName("collision", "max", "x"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymColMaxX);
				Symbol psymColMaxY = psymtblMain->FindSymbol(GrcStructName("collision", "max", "y"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymColMaxY);
				Symbol psymColMargin = psymtblMain->FindSymbol(GrcStructName("collision", "margin"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymColMargin);
				Symbol psymColMarginWt = psymtblMain->FindSymbol(GrcStructName("collision", "marginweight"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymColMarginWt);
				// Not defined as glyph attributes:
				//Symbol psymExclGlyph = psymtblMain->FindSymbol(GrcStructName("collision", "exclude", "glyph"));
				//AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymExclGlyph);
				//Symbol psymExclOffX = psymtblMain->FindSymbol(GrcStructName("collision", "exclude", "offset", "x"));
				//AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymExclOffX);
				//Symbol psymExclOffY = psymtblMain->FindSymbol(GrcStructName("collision", "exclude", "offset", "y"));
				//AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymExclOffY);
				Symbol psymSeqClass = psymtblMain->FindSymbol(GrcStructName("sequence", "class"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymSeqClass);
				Symbol psymSeqProxClass = psymtblMain->FindSymbol(GrcStructName("sequence", "proxClass"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymSeqProxClass);
				Symbol psymSeqOrder = psymtblMain->FindSymbol(GrcStructName("sequence", "order"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymSeqOrder);
				Symbol psymSeqAboveXoff = psymtblMain->FindSymbol(GrcStructName("sequence", "above", "xoffset"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymSeqAboveXoff);
				Symbol psymSeqAboveWt = psymtblMain->FindSymbol(GrcStructName("sequence", "above", "weight"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymSeqAboveWt);
				Symbol psymSeqBelowXlim = psymtblMain->FindSymbol(GrcStructName("sequence", "below", "xlimit"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymSeqBelowXlim);
				Symbol psymSeqBelowWt = psymtblMain->FindSymbol(GrcStructName("sequence", "below", "weight"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymSeqBelowWt);
				Symbol psymSeqValignHt = psymtblMain->FindSymbol(GrcStructName("sequence", "valign", "height"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymSeqValignHt);
				Symbol psymSeqValignWt = psymtblMain->FindSymbol(GrcStructName("sequence", "valign", "weight"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymSeqValignWt);

				// This isn't put into the font tables, but an ID is needed for processing.
				Symbol psymComplexFit = psymtblMain->FindSymbol(GrcStructName("collision", "complexFit"));
				AddGlyphAttrSymbolInMap(vpsymGlyphAttrIDs, psymComplexFit);

				Assert(psymColFlags->InternalID() != 0);
				Assert(psymColFlags->InternalID() + 1 == psymColMinX->InternalID());
				Assert(psymColFlags->InternalID() + 2 == psymColMinY->InternalID());
				Assert(psymColFlags->InternalID() + 3 == psymColMaxX->InternalID());
				Assert(psymColFlags->InternalID() + 4 == psymColMaxY->InternalID());
				Assert(psymColFlags->InternalID() + 5 == psymColMargin->InternalID());
				Assert(psymColFlags->InternalID() + 6 == psymColMarginWt->InternalID());
				//Assert(psymColFlags->InternalID() + 7 == psymExclGlyph->InternalID());
				//Assert(psymColFlags->InternalID() + 8 == psymExclOffX->InternalID());
				//Assert(psymColFlags->InternalID() + 9 == psymExclOffY->InternalID());
				Assert(psymColFlags->InternalID() + 7 == psymSeqClass->InternalID());
				Assert(psymColFlags->InternalID() + 8 == psymSeqProxClass->InternalID());
				Assert(psymColFlags->InternalID() + 9 == psymSeqOrder->InternalID());
				Assert(psymColFlags->InternalID() + 10 == psymSeqAboveXoff->InternalID());
				Assert(psymColFlags->InternalID() + 11 == psymSeqAboveWt->InternalID());
				Assert(psymColFlags->InternalID() + 12 == psymSeqBelowXlim->InternalID());
				Assert(psymColFlags->InternalID() + 13 == psymSeqBelowWt->InternalID());
				Assert(psymColFlags->InternalID() + 14 == psymSeqValignHt->InternalID());
				Assert(psymColFlags->InternalID() + 15 == psymSeqValignWt->InternalID());
				// Keep this last:
				Assert(psymColFlags->InternalID() + 16 == psymComplexFit->InternalID());
			}
			// Otherwise we don't want to assign glyph attr IDs to the collision attributes, because
			// the older table format doesn't know how to handle them.
		}
	}

	return true;
}


/*----------------------------------------------------------------------------------------------
	Add the generic symbol into the map that indicates internal glyph attribute IDs, if
	it is not already there. Return the internal ID.
----------------------------------------------------------------------------------------------*/
int GrcSymbolTable::AddGlyphAttrSymbolInMap(std::vector<Symbol> & vpsymGlyphAttrIDs,
	Symbol psymGeneric)
{
	for (size_t ipsym = 0; ipsym < vpsymGlyphAttrIDs.size(); ipsym++)
	{
		if (vpsymGlyphAttrIDs[ipsym] == psymGeneric)
			return ipsym;
	}
	
	psymGeneric->SetInternalID(vpsymGlyphAttrIDs.size());
	vpsymGlyphAttrIDs.push_back(psymGeneric);
	return psymGeneric->InternalID();
}


int GrcSymbolTable::AddGlyphAttrSymbolInMap(std::vector<Symbol> & vpsymGlyphAttrIDs,
	Symbol psymGeneric, int ipsymToAssign)
{
	if (signed(vpsymGlyphAttrIDs.size()) > ipsymToAssign)
	{
		Assert(vpsymGlyphAttrIDs[ipsymToAssign] == NULL ||
			vpsymGlyphAttrIDs[ipsymToAssign] == psymGeneric);
		psymGeneric->SetInternalID(ipsymToAssign);
		vpsymGlyphAttrIDs[ipsymToAssign] = psymGeneric;
		return ipsymToAssign;
	}
	else
	{
		//	Add blank slots.
		while (signed(vpsymGlyphAttrIDs.size()) < ipsymToAssign)
			vpsymGlyphAttrIDs.push_back(NULL);

		Assert(static_cast<size_t>(ipsymToAssign) == vpsymGlyphAttrIDs.size());
		psymGeneric->SetInternalID(ipsymToAssign);
		vpsymGlyphAttrIDs.push_back(psymGeneric);
		return ipsymToAssign;
	}
}



/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Assign the glyph attributes to all the glyphs in all the classes.
----------------------------------------------------------------------------------------------*/
bool GrcManager::AssignGlyphAttrsToClassMembers(GrcFont * pfont)
{
	Assert(m_pgax == NULL);

	size_t cGlyphAttrs = m_vpsymGlyphAttrs.size();
	size_t cStdStyles = max(signed(m_vpsymStyles.size()), 1);
	Assert(cStdStyles == 1);	// for now
	m_pgax = new GrcGlyphAttrMatrix(m_cwGlyphIDs, cGlyphAttrs, cStdStyles);

	//	Array of pointers to ligature component maps, if any.
	m_plclist = new GrcLigComponentList(m_cwGlyphIDs);

	//	List of system-defined glyph attributes:
	//	directionality; default = 0 (neutral)
	std::vector<Symbol> vpsymSysDefined;
	std::vector<int> vnSysDefValues;
	vpsymSysDefined.push_back(SymbolTable()->FindSymbol("directionality"));
	vnSysDefValues.push_back(kdircNeutral);
	//	breakweight; default = letter
	vpsymSysDefined.push_back(SymbolTable()->FindSymbol("breakweight"));
	vnSysDefValues.push_back(klbLetterBreak);
	if (m_prndr->Bidi())
	{
		//	mirror.glyph
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("mirror", "glyph")));
		vnSysDefValues.push_back(0);
		//	mirror.isEncoded
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("mirror", "isEncoded")));
		vnSysDefValues.push_back(0);
	}
	if (m_prndr->HasCollisionPass())
	{
		//	collision.flags and friends
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("collision", "flags")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("collision", "min", "x")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("collision", "max", "x")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("collision", "min", "y")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("collision", "max", "y")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("collision", "margin")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("collision", "marginweight")));
		vnSysDefValues.push_back(0);
		// Not defined as glyph attributes:
		//vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("collision", "exclude", "glyph")));
		//vnSysDefValues.push_back(0);
		//vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("collision", "exclude", "offset", "x")));
		//vnSysDefValues.push_back(0);
		//vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("collision", "exclude", "offset", "y")));
		//vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("collision", "complexFit")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("sequence", "class")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("sequence", "proxClass")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("sequence", "order")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("sequence", "above", "xoffset")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("sequence", "above", "weight")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("sequence", "below", "xlimit")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("sequence", "below", "weight")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("sequence", "valign", "height")));
		vnSysDefValues.push_back(0);
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("sequence", "valign", "weight")));
		vnSysDefValues.push_back(0);

	}
	if (IncludePassOptimizations())
	{
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("*skipPasses*")));
		// Default value for the *skipPasses* attributes is a bitmap with 1 set for every pass.
		int cpass = m_prndr->NumberOfPasses();
		int cpass1 = (cpass > kPassPerSPbitmap) ? kPassPerSPbitmap : cpass;
		unsigned int nDefaultSkipP = 0;
		for (int i = 0; i < cpass1; i++)
			nDefaultSkipP = (nDefaultSkipP << 1) + 1;
		vnSysDefValues.push_back(nDefaultSkipP);
		if (cpass > kPassPerSPbitmap)
		{
			vpsymSysDefined.push_back(SymbolTable()->FindSymbol(GrcStructName("*skipPasses2*")));
			int cpass2 = (cpass > kPassPerSPbitmap * 2) ? kPassPerSPbitmap * 2 : cpass;
			unsigned int nDefaultSkipP2 = 0;
			for (int i2 = kPassPerSPbitmap; i2 < cpass2; i2++)
				nDefaultSkipP2 = (nDefaultSkipP2 << 1) + 1;
			vnSysDefValues.push_back(nDefaultSkipP2);
		}
	}
	// justify.weight = 1
	if (NumJustLevels() > 0)
	{
		GrcStructName xnsJ0Weight("justify", "0", "weight");
		vpsymSysDefined.push_back(SymbolTable()->FindSymbol(xnsJ0Weight));
		vnSysDefValues.push_back(1);
		// Don't need to handle both since they have the same glyph-attr ID.
		//GrcStructName xnsJWeight("justify", "weight");
		//vpsymSysDefined.push_back(SymbolTable()->FindSymbol(xnsJWeight));
		//vnSysDefValues.push_back(1);
		// Other justify attrs have default of zero, and so do not need to be initialized.
	}

	m_prndr->AssignGlyphAttrDefaultValues(pfont, m_pgax, m_cwGlyphIDs,
		vpsymSysDefined, vnSysDefValues, m_vpexpModified,
		m_vpsymGlyphAttrs);

	m_prndr->AssignGlyphAttrsToClassMembers(m_pgax, m_plclist);

	if (m_cpsymComponents >= kMaxComponents)
	{
		char rgchMax[20];
		itoa(kMaxComponents - 1, rgchMax, 10);
		char rgchCount[20];
		itoa(m_cpsymComponents, rgchCount, 10);
		g_errorList.AddError(4124, NULL,
			"Total number of ligature components (",
			rgchCount,
			") exceeds maximum of ",
			rgchMax);
	}

	return true;
}

/*--------------------------------------------------------------------------------------------*/
void GdlRenderer::AssignGlyphAttrsToClassMembers(GrcGlyphAttrMatrix * pgax,
	GrcLigComponentList * plclist)
{
	for (size_t ipglfc = 0; ipglfc < m_vpglfc.size(); ipglfc++)
	{
		m_vpglfc[ipglfc]->AssignGlyphAttrsToClassMembers(pgax, this, plclist);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::AssignGlyphAttrsToClassMembers(GrcGlyphAttrMatrix * pgax,
	GdlRenderer * prndr, GrcLigComponentList * plclist)
{
	int cgid = GlyphIDCount();
	int igid = 0;
	AssignGlyphAttrsToClassMembers(pgax, prndr, plclist, m_vpglfaAttrs, cgid, igid);
}

/*----------------------------------------------------------------------------------------------
	Assign the given glyph attributes to all the glyphs in the class.
----------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::AssignGlyphAttrsToClassMembers(GrcGlyphAttrMatrix * pgax,
	GdlRenderer * prndr, GrcLigComponentList * plclist,
	std::vector<GdlGlyphAttrSetting *> & vpglfaAttrs, int cgid, int & igid)
{
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		m_vpglfdMembers[iglfd]->AssignGlyphAttrsToClassMembers(pgax, prndr, plclist,
			vpglfaAttrs, cgid, igid);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphDefn::AssignGlyphAttrsToClassMembers(GrcGlyphAttrMatrix * pgax,
	GdlRenderer * prndr, GrcLigComponentList * plclist,
	std::vector<GdlGlyphAttrSetting *> & vpglfaAttrs, int cgid, int & igid)
{
	int igidInitial = igid;

	//	For each attribute assignment in the value list:
	for (size_t ipglfa = 0; ipglfa < vpglfaAttrs.size(); ipglfa++)
	{
		Symbol psym = vpglfaAttrs[ipglfa]->GlyphSymbol();
		Assert(!psym->IsGeneric());
		int nGlyphAttrID = psym->InternalID();

		if (!g_cman.OffsetAttrs() && psym->IsIgnorableOffsetAttr())
			continue;

		//	The new attribute assignment:
		GdlAssignment * pasgnValue = vpglfaAttrs[ipglfa]->Assignment();
		GdlExpression * pexpNew = pasgnValue->Expression();
		int nPRNew = pasgnValue->PointRadius();
		int mPrUnitsNew = pasgnValue->PointRadiusUnits();
		bool fOverrideNew = pasgnValue->Override();
		GrpLineAndFile lnfNew = pasgnValue->LineAndFile();
		int nStmtNoNew = lnfNew.PreProcessedLine();

		GdlClassMemberExpression * pexpilNew = dynamic_cast<GdlClassMemberExpression *>(pexpNew);
		if (pexpilNew)
		{
			Assert(pexpilNew->Name()->FitsSymbolType(ksymtClass));
			pexpilNew->SetClassSize(cgid);
			if (pexpilNew->GlyphIndex() != -1 && pexpilNew->GlyphIndex() != igid)
			{
				//	Make a copy for this specific glyph definition.
				GdlClassMemberExpression * pexpilThisGlyphId 
					= dynamic_cast<GdlClassMemberExpression *>(pexpilNew->Clone());
				pexpilNew = pexpilThisGlyphId;
				pexpNew = dynamic_cast<GdlExpression *>(pexpilNew);
				g_cman.StoreModifiedExpression(pexpNew);
			}
			pexpilNew->SetGlyphIndex(igid);
		}

		igid = igidInitial;	// start over for each attribute

		//	For each glyph ID covered by this definition's range:
		for (size_t iwGlyphID = 0; iwGlyphID < m_vwGlyphIDs.size(); iwGlyphID++)
		{
			if (m_vwGlyphIDs[iwGlyphID] == kBadGlyph) // invalid glyph
				continue;

			//	Compare the new assignment with the previous.
			GdlExpression * pexpOld;
			int nPROld;
			int mPrUnitsOld;
			bool fOverrideOld, fShadow;
			GrpLineAndFile lnfOld;
			pgax->Get(m_vwGlyphIDs[iwGlyphID], nGlyphAttrID,
				&pexpOld, &nPROld, &mPrUnitsOld, &fOverrideOld, &fShadow, &lnfOld);
			int nStmtNoOld = lnfOld.PreProcessedLine();
			Assert(!fShadow);

			if (pexpOld == NULL ||
				(nStmtNoNew > nStmtNoOld && fOverrideNew) ||
				(nStmtNoOld > nStmtNoNew && !fOverrideOld))
			{
				// For indexed-lookup expressions, we need to make a separate copy 
				// for each individual glyph ID.
				GdlExpression * pexpThisGlyphId = pexpNew;
				if (pexpilNew && pexpilNew->GlyphIndex() != igid)
				{
					//	Make a copy for this specific glyph ID.
					pexpThisGlyphId = pexpNew->Clone();
					GdlClassMemberExpression * pexpilThisGlyphId 
						= dynamic_cast<GdlClassMemberExpression *>(pexpThisGlyphId);
					pexpilThisGlyphId->SetGlyphIndex(igid);
					g_cman.StoreModifiedExpression(pexpThisGlyphId);
				}

				//	The current attribute assignment overrides the previous--set it.
				pgax->Set(m_vwGlyphIDs[iwGlyphID], nGlyphAttrID,
					pexpThisGlyphId, nPRNew, mPrUnitsNew, fOverrideNew, false, lnfNew);
			}

			//	If this glyph is a ligature, add the given component to its list.
			int ivComponent = psym->FieldIndex("component");
			if (ivComponent > -1)
			{
				plclist->AddComponentFor(m_vwGlyphIDs[iwGlyphID],
					psym->Generic()->BaseLigComponent(), prndr);
			}

			igid++;
		}
	}
}


/*----------------------------------------------------------------------------------------------
	Set the (non-zero) system-defined glyph attributes to default values for all the glyphs.
----------------------------------------------------------------------------------------------*/
void GdlRenderer::AssignGlyphAttrDefaultValues(GrcFont * pfont,
	GrcGlyphAttrMatrix * pgax, int cwGlyphs,
	std::vector<Symbol> & vpsymSysDefined, std::vector<int> & vnSysDefValues,
	std::vector<GdlExpression *> & vpexpExtra,
	std::vector<Symbol> & /*vpsymGlyphAttrs*/)
{
	bool fIcuAvailable = false;
	try {
		//int charType = u_charType(0x0020);
		//int charCat = UCharCategory(u_charType(0x0020));
		//int charDir = u_charDirection(0x0020);
		if (UCharCategory(u_charType(0x0020)) == U_SPACE_SEPARATOR
			&& u_charDirection(0x0020) == U_WHITE_SPACE_NEUTRAL)
		{
			fIcuAvailable = true;
		}
	}
	catch (...)
	{
	}

	Assert(vpsymSysDefined.size() == vnSysDefValues.size());

	for (size_t i = 0; i < vpsymSysDefined.size(); i++) // loop over attributes
	{
		bool fErrorForAttr = false;
		Symbol psym = vpsymSysDefined[i];
		int nDefaultValue = vnSysDefValues[i]; // default for the entire attribute, non-char-specific

		int nGlyphAttrID = psym->InternalID();

		//	Set all values to the defaults for the corresponding Unicode character.

		GrcFont::iterator fit;
		int iUni;
		for (iUni = 0, fit = pfont->Begin(); fit != pfont->End(); ++fit, ++iUni) // loop over chars
		{
			int nUnicode = *fit;

			int wGlyphID = pfont->GlyphFromCmap(nUnicode, NULL);
			if (wGlyphID > 0)
			{
				//  Read the character property from ICU.
				//	How do we handle values from the Private Use Area? Just hard-code the
				//	range to skip?
				int nStdValue;
				bool fInitFailed = false;
				bool fClassMember = false;

                if (psym->LastFieldIs("breakweight"))
                {
					bool fIsSep;
                    if (fIcuAvailable)
                    {
                        UCharCategory catICU = UCharCategory(u_charType(nUnicode));
						fIsSep = (catICU == U_SPACE_SEPARATOR
							|| catICU == U_LINE_SEPARATOR || catICU == U_PARAGRAPH_SEPARATOR);
                    }
                    else
                    {
                        if (nUnicode == 0x0020 || nUnicode == 0x1680 || nUnicode == 0x180E
                            || (nUnicode >= 0x2000 && nUnicode <= 0x200B)
                            || nUnicode == 0x205F || nUnicode == 0x3000)
							// Don't include non-breaking spaces: U+00A0, U+202F, U+FEFF
						{
                            fIsSep = 1;
						}
                        else
						{
                            fIsSep = 0;
							fInitFailed = true; // not sure we got the right answer
						}
                    }
                    nStdValue = (fIsSep) ? klbWordBreak : nDefaultValue;
                }
                else if (psym->LastFieldIs("directionality"))
                {
                    nStdValue = (int)this->DefaultDirCode(nUnicode, &fInitFailed);
					if (fInitFailed && fIcuAvailable)
                    {
						UCharDirection diricu = u_charDirection(nUnicode);
						nStdValue = ConvertBidiCode(diricu, nUnicode);
                        fInitFailed = 0;
						//if (!Bidi() && nStdValue == kdircL)
						//	nStdValue = 0;	// don't bother storing this for non-bidi fonts
                    }
                }
				else if (psym->LastFieldIs("*skipPasses*") || psym->LastFieldIs("*skipPasses2*"))
				{
					nStdValue = nDefaultValue;
				}
				else if (psym->FieldAt(0) == "mirror" && psym->LastFieldIs("glyph") && Bidi())
				{
					int nUnicodeMirror = (int)u_charMirror(nUnicode);
					if (nUnicodeMirror == nUnicode)
						nStdValue = 0;
					else
						nStdValue = pfont->GlyphFromCmap(nUnicodeMirror, NULL);
					fClassMember = true;
				}
				else if (psym->LastFieldIs("isEncoded") && Bidi())
				{
					bool fIsEnc = u_isMirrored(nUnicode);
					nStdValue = (int)fIsEnc;
				}
                else
                    break;	// ...out of the character loop; this is not an attribute
							// it makes sense to read from the db

				if (fInitFailed)
				{
					if (!fErrorForAttr)
					{
						// First time an error has been encountered for this attribute.
						g_errorList.AddWarning(4509, NULL,
							"Unable to initialize ",
							psym->FullName(),
							" glyph attribute from Unicode char props database");
					}
					fErrorForAttr = true; // don't give the warning again
				}
				else if (!pgax->Defined(wGlyphID, nGlyphAttrID))
				{
					GdlExpression * pexpDefault;
					if (fClassMember)
						pexpDefault = new GdlClassMemberExpression(nStdValue);
					else
						pexpDefault = new GdlNumericExpression(nStdValue);
					vpexpExtra.push_back(pexpDefault);
					pgax->Set(wGlyphID, nGlyphAttrID,
						pexpDefault, 0, 0, false, false, GrpLineAndFile());
				}
			}
		}			

		if (nDefaultValue == 0)
			continue;	// don't need to set zero values explicitly

		//	Now set any remaining attributes that weren't handled above to the standard defaults.
		for (int wGlyphID = 0; wGlyphID < cwGlyphs; wGlyphID++)
		{
			if (!pgax->Defined(wGlyphID, nGlyphAttrID))
			{
				GdlExpression * pexpDefault = new GdlNumericExpression(nDefaultValue);
				vpexpExtra.push_back(pexpDefault);
				pgax->Set(wGlyphID, nGlyphAttrID,
					pexpDefault, 0, 0, false, false, GrpLineAndFile());
			}
		}
	}

	//	Assign 'kGpointNotSet' as the default value for all gpoint attributes. We use
	//	a special value for this to distinguish the situation of gpoint = 0, which
	//	may be a legitimate value.
//	for (int ipsym = 0; ipsym < vpsymGlyphAttrs.Size(); ipsym++)
//	{
//		Symbol psym = vpsymGlyphAttrs[ipsym];
//		int nGlyphAttrID = psym->InternalID();
//		if (psym->LastFieldIs("gpoint"))
//		{
//			for (int wGlyphID = 0; wGlyphID < cwGlyphs; wGlyphID++)
//			{
//				if (!pgax->Defined(wGlyphID, nGlyphAttrID))
//				{
//					GdlExpression * pexpDefault = new GdlNumericExpression(kGpointNotSet);
//					vpexpExtra.Push(pexpDefault);
//					pgax->Set(wGlyphID, nGlyphAttrID,
//						pexpDefault, 0, 0, false, false, GrpLineAndFile());
//				}
//			}
//		}
//	}

}


/*----------------------------------------------------------------------------------------------
	Convert the bidi categories defined by the character properties engine to those used
	by Graphite.
----------------------------------------------------------------------------------------------*/
DirCode GdlRenderer::ConvertBidiCode(UCharDirection diricu, utf16 wUnicode)
{
	std::string staCode;

	switch (diricu)
	{
	case U_LEFT_TO_RIGHT:				return kdircL;
	case U_RIGHT_TO_LEFT:				return kdircR;
	case U_EUROPEAN_NUMBER:				return kdircEuroNum;
	case U_EUROPEAN_NUMBER_SEPARATOR:	return kdircEuroSep;
	case U_EUROPEAN_NUMBER_TERMINATOR:	return kdircEuroTerm;
	case U_ARABIC_NUMBER:				return kdircArabNum;
	case U_COMMON_NUMBER_SEPARATOR:		return kdircComSep;
	case U_WHITE_SPACE_NEUTRAL:			return kdircWhiteSpace;
	case U_OTHER_NEUTRAL:				return kdircNeutral;
	case U_LEFT_TO_RIGHT_EMBEDDING:		return kdircLRE;
	case U_LEFT_TO_RIGHT_OVERRIDE:		return kdircLRO;
	case U_RIGHT_TO_LEFT_ARABIC:		return kdircRArab;
	case U_RIGHT_TO_LEFT_EMBEDDING:		return kdircRLE;
	case U_RIGHT_TO_LEFT_OVERRIDE:		return kdircRLO;
	case U_POP_DIRECTIONAL_FORMAT:		return kdircPDF;
	case U_DIR_NON_SPACING_MARK:		return kdircNSM;
	case U_BOUNDARY_NEUTRAL:			return kdircBndNeutral;

	case U_BLOCK_SEPARATOR:				staCode = "B"; break; // not handled
	case U_SEGMENT_SEPARATOR:			staCode = "S"; break; // not handled
	default:
		char rgch[20];
		itoa(diricu, rgch, 10);
		staCode = rgch;
		break;
	}

	if (Bidi())
	{
		g_errorList.AddWarning(4510, NULL,
			"Default Unicode bidi char type for 0x",
			GdlGlyphDefn::UsvString(wUnicode), " = ", staCode,
			", which is not handled; char will be treated as neutral (ON)");
	}
	// otherwise the issue is irrelevant; don't bother with the warning

	return kdircNeutral;
}

/*----------------------------------------------------------------------------------------------
	Return the default directionality code for the given USV.
----------------------------------------------------------------------------------------------*/
DirCode GdlRenderer::DefaultDirCode(int nUnicode, bool * pfInitFailed)
{
	DirCode dircDefault;

	switch (nUnicode)
	{
	case kchwSpace:		dircDefault = kdircWhiteSpace; break;
	case kchwLRM:		dircDefault = kdircL; break;
	case kchwRLM:		dircDefault = kdircR; break;
	case kchwALM:		dircDefault = kdircRArab; break;
	case kchwLRO:		dircDefault = kdircLRO; break;
	case kchwRLO:		dircDefault = kdircRLO; break;
	case kchwLRE:		dircDefault = kdircLRE; break;
	case kchwRLE:		dircDefault = kdircRLE; break;
	case kchwPDF:		dircDefault = kdircPDF; break;
	case kchwLRI:		dircDefault = kdircLRI; break;
	case kchwRLI:		dircDefault = kdircRLI; break;
	case kchwFSI:		dircDefault = kdircFSI; break;
	case kchwPDI:		dircDefault = kdircPDI; break;

	// The following matching parentheses come from the Unicode BidiBrackets.txt file.

	case 0x0028:	case 0x005B:	case 0x007B:	case 0x0F3A:	case 0x0F3C:
	case 0x169B:	case 0x2045:	case 0x207D:	case 0x208D:	case 0x2329:
	case 0x2768:	case 0x276A:	case 0x276C:	case 0x276E:	case 0x2770:
	case 0x2772:	case 0x2774:	case 0x27C5:	case 0x27E6:	case 0x27E8:
	case 0x27EA:	case 0x27EC:	case 0x27EE:	case 0x2983:	case 0x2985:
	case 0x2987:	case 0x2989:	case 0x298B:	case 0x298D:	case 0x298F:
	case 0x2991:	case 0x2993:	case 0x2995:	case 0x2997:	case 0x29D8:
	case 0x29DA:	case 0x29FC:	case 0x2E22:	case 0x2E24:	case 0x2E26:
	case 0x2E28:	case 0x3008:	case 0x300A:	case 0x300C:	case 0x300E:
	case 0x3010:	case 0x3014:	case 0x3016:	case 0x3018:	case 0x301A:
	case 0xFE59:	case 0xFE5B:	case 0xFE5D:	case 0xFF08:	case 0xFF3B:
	case 0xFF5B:	case 0xFF5F:	case 0xFF62:
		dircDefault = kdircOPP;
		break;

	case 0x0029:	case 0x005D:	case 0x007D:	case 0x0F3B:	case 0x0F3D:
	case 0x169C:	case 0x2046:	case 0x207E:	case 0x208E:	case 0x232A:
	case 0x2769:	case 0x276B:	case 0x276D:	case 0x276F:	case 0x2771:
	case 0x2773:	case 0x2775:	case 0x27C6:	case 0x27E7:	case 0x27E9:
	case 0x27EB:	case 0x27ED:	case 0x27EF:	case 0x2984:	case 0x2986:
	case 0x2988:	case 0x298A:	case 0x298C:	case 0x298E:	case 0x2990:
	case 0x2992:	case 0x2994:	case 0x2996:	case 0x2998:	case 0x29D9:
	case 0x29DB:	case 0x29FD:	case 0x2E23:	case 0x2E25:	case 0x2E27:
	case 0x2E29:	case 0x3009:	case 0x300B:	case 0x300D:	case 0x300F:
	case 0x3011:	case 0x3015:	case 0x3017:	case 0x3019:	case 0x301B:
	case 0xFE5A:	case 0xFE5C:	case 0xFE5E:	case 0xFF09:	case 0xFF3D:
	case 0xFF5D:	case 0xFF60:	case 0xFF63:
		dircDefault = kdircCPP;
		break;

	default:
		// various kinds of spaces
		dircDefault = nUnicode >= 0x2000 && nUnicode <= 0x200b ? kdircWhiteSpace : kdircNeutral;
		*pfInitFailed = (bool)Bidi();		// we only care about the failure if this is a bidi font
		break;
	}

	return dircDefault;
}

/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Process each glyph attribute assignment for each glyph:
	* make sure that the statements are appropriate for the context of the glyph table
		(rather than a rule table);
	* do type checking;
	* convert g-paths to g-points;
	* convert x/y coordinates to g-points and vice versa.
----------------------------------------------------------------------------------------------*/
bool GrcManager::ProcessGlyphAttributes(GrcFont * pfont)
{
	int cStdStyles = max(signed(m_vpsymStyles.size()), 1);

	for (utf16 wGlyphID = 0; wGlyphID < m_cwGlyphIDs; wGlyphID++)
	{
		for (size_t iAttrID = 0; iAttrID < m_vpsymGlyphAttrs.size(); iAttrID++)
		{
			for (int iStyle = 0; iStyle < cStdStyles; iStyle++)
			{
				GdlExpression * pexp;
				int nPR;
				int munitPR;
				bool fOverride, fShadow;
				GrpLineAndFile lnf;
				m_pgax->Get(wGlyphID, iAttrID,
					&pexp, &nPR, &munitPR, &fOverride, &fShadow, &lnf);
				Assert(!fShadow);

				if (!pexp)
					continue;

				Symbol psymAttr = m_vpsymGlyphAttrs[iAttrID];

				bool fOkay = pexp->TypeCheck(psymAttr->ExpType());
				if (!fOkay)
					g_errorList.AddWarning(4511, pexp,
						"Inconsistent or inappropriate type in glyph attribute: ",
						psymAttr->FullName(),
						lnf);

				pexp->GlyphAttrCheck(psymAttr);

				GdlExpression * pexpNew = pexp->SimplifyAndUnscale(wGlyphID, pfont);
				Assert(pexpNew);
				if (pexpNew && pexpNew != pexp)
				{
					m_vpexpModified.push_back(pexpNew);	// so we can delete it later
					m_pgax->Set(wGlyphID, iAttrID,
						pexpNew, nPR, munitPR, fOverride, false, lnf);
				}

				// We decided not to do this:
				//if (psymAttr->IsCollisionAttr() && psymAttr->LastFieldIs("maxoverlap"))
				//{
				//	// Distinguish between values of false and 0. False means ignore, and value is stored
				//	// as 0. An actual 0 is changed to 1, since the difference between 0 and 1 is negligible.
				//	GdlNumericExpression * pexpNum = dynamic_cast<GdlNumericExpression *>(pexpNew);
				//	if (pexpNum)
				//	{
				//		if (pexpNum->IsBoolean())
				//		{
				//			if (pexpNum->Value() == 1) // "true"
				//				g_errorList.AddError(9999, pexp,
				//				"Invalid value for collision.maxoverlap: true",
				//				lnf);
				//			// else "false" = 0
				//		}
				//		else if (pexpNum->Value() == 0)
				//		{
				//			pexpNum->SetValue(1); // because 0 means false for this attribute
				//		}
				//	}
				//}

				//	Convert g-paths to g-points
				int nGPathValue;
				int ivGPath = psymAttr->FieldIndex("gpath");
				if (ivGPath > -1)
				{
					if (ivGPath != psymAttr->FieldCount() - 1)
					{
						//	not of the form <class-name>.<point-name>.gpath = X
						g_errorList.AddError(4125, pexp,
							"Invalid use of gpath attribute: ",
							psymAttr->FullName(),
							lnf);
					}
					else if (!pexpNew->ResolveToInteger(&nGPathValue, false))
					{
						g_errorList.AddError(4126, pexp,
							"Invalid value for gpath attribute--must be an integer: ",
							psymAttr->FullName(),
							lnf);
					}
					else
					{
						//	Find the corresponding gpoint attribute.
						Symbol psymGPoint = psymAttr->PointSisterField("gpoint");
						Assert(psymGPoint);

						//	Set its value.
						utf16 wActual = ActualForPseudo(wGlyphID);
						if (wActual == 0)
							wActual = wGlyphID;
						int nGPointValue = pfont->ConvertGPathToGPoint(wActual, nGPathValue, pexp);
						if (nGPointValue == -1)
						{
							char rgch[20];
							itoa(nGPointValue, rgch, 10);
							g_errorList.AddWarning(4512, NULL,
								"Invalid path for glyph ",
								GdlGlyphDefn::GlyphIDString(wGlyphID),
								": ",
								rgch,
								lnf);
							nGPointValue = 0;
						}
								
						pexpNew = new GdlNumericExpression(nGPointValue);
						pexpNew->CopyLineAndFile(*pexp);

						m_vpexpModified.push_back(pexpNew);	// so we can delete it later
						m_pgax->Set(wGlyphID, psymGPoint->InternalID(),
							pexpNew, nPR, munitPR, fOverride, false, lnf);
					}
				}
			}
		}

		if (this->OffsetAttrs())
			ConvertBetweenXYAndGpoint(pfont, wGlyphID);

		// Just in case, since incrementing 0xFFFF will produce zero.
		if (wGlyphID == 0xFFFF)
			break;
	}

	return true;
}

/*----------------------------------------------------------------------------------------------
	Convert x/y point coordinates to an actual on-curve point if there is one that matches
	closely (within PointRadius).

	On the other hand, if the specified g-point is the single point on its curve, convert it
	to x/y coordinates (due to the fact that single-point curves disappear from the final
	API point list). Actually do this in all cases, because g-points aren't available in the
	Linux version of the engine.

	We do this in a separate loop after both the x and y fields have been processed,
	simplified to unscaled integers, etc.
----------------------------------------------------------------------------------------------*/
void GrcManager::ConvertBetweenXYAndGpoint(GrcFont * pfont, utf16 wGlyphID)
{
	int cStdStyles = max(signed(m_vpsymStyles.size()), 1);
	utf16 wActual = ActualForPseudo(wGlyphID);
	if (wActual == 0)
		wActual = wGlyphID;

	for (size_t iAttrID = 0; iAttrID < m_vpsymGlyphAttrs.size(); iAttrID++)
	{
		for (int iStyle = 0; iStyle < cStdStyles; iStyle++)
		{
			Symbol psymAttr = m_vpsymGlyphAttrs[iAttrID];

			if (psymAttr->LastFieldIs("x"))
			{
				Symbol psymAttrX = psymAttr;
				Symbol psymAttrY = psymAttrX->PointSisterField("y");
				if (!psymAttrY)
					continue; // need both x and y to convert
				int iAttrIDx = iAttrID;
				int iAttrIDy = psymAttrY->InternalID();

				GdlExpression * pexpX;
				int nPRx;
				int munitPRx;
				bool fOverride, fShadowX, fShadowY;
				GrpLineAndFile lnf;
				m_pgax->Get(wGlyphID, iAttrIDx,
					&pexpX, &nPRx, &munitPRx, &fOverride, &fShadowX, &lnf);

				if (!pexpX)
					continue;
				int nX;
				if (!pexpX->ResolveToInteger(&nX, false))
					continue;

				GdlExpression * pexpY;
				int nPRy;
				int munitPRy;
				m_pgax->Get(wGlyphID, iAttrIDy,
					&pexpY, &nPRy, &munitPRy, &fOverride, &fShadowY, &lnf);

				if (!pexpY)
					continue;
				int nY;
				if (!pexpY->ResolveToInteger(&nY, false))
					continue;

				Assert(fShadowX == fShadowY);
				if (fShadowX || fShadowY)
					continue;

				//	Find the corresponding gpoint attribute.
				Symbol psymGPoint = psymAttrX->PointSisterField("gpoint");
				Assert(psymGPoint);

				nPRx = pfont->ScaledToAbsolute(nPRx, munitPRx);
				nPRy = pfont->ScaledToAbsolute(nPRy, munitPRy);

				//	See if we can find a corresponding on-curve point.
				int nGPointValue = pfont->GetPointAtXY(wActual, nX, nY, max(nPRx, nPRy), pexpX);
				if (nGPointValue > -1 && !pfont->IsPointAlone(wActual, nGPointValue, pexpX))
				{
					//	We found one. Set the value of the gpoint field.
					GdlExpression * pexpGpoint = new GdlNumericExpression(nGPointValue);
					pexpGpoint->CopyLineAndFile(*pexpX);

					m_vpexpModified.push_back(pexpGpoint);	// so we can delete it later
					m_pgax->Set(wGlyphID, psymGPoint->InternalID(),
						pexpGpoint, 0, munitPRx, fOverride, false, lnf);

					//	Clear the x and y fields.
					m_pgax->Set(wGlyphID, iAttrIDx, NULL, 0, munitPRx, true, false, lnf);
					m_pgax->Set(wGlyphID, iAttrIDy, NULL, 0, munitPRy, true, false, lnf);

					//	Don't delete the actual expressions, because they are owned by the
					//	original assignment statements, which will delete them.
				}
			}
			else if (psymAttr->LastFieldIs("gpoint"))
			{
				Symbol psymAttrGpoint = psymAttr;

				int iAttrIDgpoint = iAttrID;
				GdlExpression * pexpGpoint;
				int nPR;
				int munitPR;
				bool fOverride, fShadow;
				GrpLineAndFile lnf;
				m_pgax->Get(wGlyphID, iAttrIDgpoint,
					&pexpGpoint, &nPR, &munitPR, &fOverride, &fShadow, &lnf);
				Assert(!fShadow);

				if (!pexpGpoint)
					continue;
				int nGPoint;
				if (!pexpGpoint->ResolveToInteger(&nGPoint, false))
					continue;

				//	Convert gpoint to x/y. On Linux gpoint will never work.
				//	It won't work on Windows either if the point comprises a single-point path,
				//	so in that case, delete the gpoint setting altogether. In other cases,
				//	leave both around, so if gpoint doesn't work, the engine can fall back
				//	to x/y.
				int mX, mY;
				if (pfont->GetXYAtPoint(wActual, nGPoint, &mX, &mY, pexpGpoint))
				{
					//	Create equivalent x/y statements.

					Symbol psymX = psymAttrGpoint->PointSisterField("x");
					Symbol psymY = psymAttrGpoint->PointSisterField("y");
					Assert(psymX);
					Assert(psymY);
					int nIDX = psymX->InternalID();
					int nIDY = psymY->InternalID();

					GdlExpression * pexpX = new GdlNumericExpression(mX, kmunitUnscaled);
					pexpX->CopyLineAndFile(*pexpGpoint);

					GdlExpression * pexpY = new GdlNumericExpression(mY, kmunitUnscaled);
					pexpY->CopyLineAndFile(*pexpGpoint);

					m_vpexpModified.push_back(pexpX);	// so we can delete them later
					m_vpexpModified.push_back(pexpY);

					if (m_pgax->Defined(wGlyphID, nIDX) && m_pgax->Defined(wGlyphID, nIDY))
					{
						Symbol psymBasePt = psymAttr->BasePoint();
						g_errorList.AddWarning(4513, pexpGpoint,
							"Both x/y coordinates and gpoint are defined for ",
							psymBasePt->FullName(),
							" for glyph ",
							GdlGlyphDefn::GlyphIDString(wGlyphID),
							"; only gpoint will be used");
					}

					bool fDeleteGpoint = pfont->IsPointAlone(wActual, nGPoint, pexpGpoint);

					// Store the new expressions as glyph attribute assignments.
					m_pgax->Set(wGlyphID, nIDX, pexpX, nPR, munitPR,
						fOverride, !fDeleteGpoint, lnf);
					m_pgax->Set(wGlyphID, nIDY, pexpY, nPR, munitPR,
						fOverride, !fDeleteGpoint, lnf);

					if (fDeleteGpoint)
					{
						//	Since this is a single-point path, it won't show up in the
						//	hinted glyph from the graphics object, so we really have to
						//	use the x/y coordinates. So clear the gpoint field.
						m_pgax->Set(wGlyphID, iAttrIDgpoint, NULL, 0, munitPR, true, false, lnf);
					}

					//	Don't delete the actual expression, because it is owned by the
					//	original assignment statements, which will delete it.
				}
			}
		}
	}
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	First, flatten any slot attributes that are reading points to use integers instead
	(ie, change { attach.at = udap } to { attach.at { x = udap.x; y = udap.y }} ).
	Then substitute the internal glyph attribute IDs into the rules. Do error checking, making
	sure glyph attributes are defined appropriately where expected.
----------------------------------------------------------------------------------------------*/
bool GdlRenderer::FixGlyphAttrsInRules(GrcManager * pcman, GrcFont * pfont)
{
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		m_vprultbl[iprultbl]->FixGlyphAttrsInRules(pcman, pfont);
	}

	return true;
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::FixGlyphAttrsInRules(GrcManager * pcman, GrcFont * pfont)
{
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		m_vppass[ippass]->FixGlyphAttrsInRules(pcman, pfont);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlPass::FixGlyphAttrsInRules(GrcManager * pcman, GrcFont * pfont)
{
	for (size_t iprule = 0; iprule < m_vprule.size(); iprule++)
	{
		m_vprule[iprule]->FixGlyphAttrsInRules(pcman, pfont);
	}

	//	While we're at it...
	FixFeatureTestsInPass(pfont);
}

/*--------------------------------------------------------------------------------------------*/
void GdlRule::FixGlyphAttrsInRules(GrcManager * pcman, GrcFont * pfont)
{
	//	Make a list of all the input-classes in the rule, for checking for the definition
	//	of glyph attributes in constraints and attribute-setters.
	std::vector<GdlGlyphClassDefn *> vpglfcInClasses;
	size_t iprit;
	for (iprit = 0; iprit < m_vprit.size();	iprit++)
	{
		Symbol psymInput = m_vprit[iprit]->m_psymInput;
		if (psymInput &&
			(psymInput->FitsSymbolType(ksymtClass) ||
				psymInput->FitsSymbolType(ksymtSpecialLb)) &&
				!psymInput->LastFieldIs(GdlGlyphClassDefn::Undefined()))
		{
			GdlGlyphClassDefn * pglfc = psymInput->GlyphClassDefnData();
			Assert(pglfc);
			vpglfcInClasses.push_back(pglfc);
		}
		else
			//	invalid input class
			vpglfcInClasses.push_back(NULL);
	}
	Assert(m_vprit.size() == vpglfcInClasses.size());

	//	Flatten slot attributes that use points to use integers instead. Do this
	//	entire process before fixing glyph attrs, because there can be some interaction between
	//	slots in a rule (eg, attach.to/at). So we can be sure at that point what state
	//	things are in.
	for (iprit = 0; iprit < m_vprit.size(); iprit++)
	{
		m_vprit[iprit]->FlattenPointSlotAttrs(pcman);
	}

	//	While we're at it, fix the feature tests. Do this before fixing the glyph attributes,
	//	because it is possible to have feature tests embedded in conditional statements.
	FixFeatureTestsInRules(pfont);

	//	Now do the fixes, error checks, etc.
	for (iprit = 0; iprit < m_vprit.size(); iprit++)
	{
		m_vprit[iprit]->FixGlyphAttrsInRules(pcman, vpglfcInClasses, this, iprit);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleItem::FixGlyphAttrsInRules(GrcManager * pcman,
	std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, GdlRule * /*prule*/, int irit)
{
	if (!m_psymInput)
		return;

	GdlGlyphClassDefn * pglfcIn = m_psymInput->GlyphClassDefnData();
	if (!pglfcIn)
		return;	// invalid class

	//	Process constraint
	if (m_pexpConstraint)
		m_pexpConstraint->CheckAndFixGlyphAttrsInRules(pcman, vpglfcInClasses, irit);
}

/*--------------------------------------------------------------------------------------------*/
void GdlLineBreakItem::FixGlyphAttrsInRules(GrcManager * pcman,
	std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, GdlRule * prule, int irit)
{
	//	method on superclass: process constraints.
	GdlRuleItem::FixGlyphAttrsInRules(pcman, vpglfcInClasses, prule, irit);
}

/*--------------------------------------------------------------------------------------------*/
void GdlSetAttrItem::FixGlyphAttrsInRules(GrcManager * pcman,
	std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, GdlRule * prule, int irit)
{
	//	method on superclass: process constraints.
	GdlRuleItem::FixGlyphAttrsInRules(pcman, vpglfcInClasses, prule, irit);

	bool fXYAt = false;			// true if the attach.at statements need to include x/y
	bool fGpointAt = false;		// true if the attach.at statements need to include gpoint
	bool fXYWith = false;		// true if the attach.with statements need to include x/y
	bool fGpointWith = false;	// true if the attach.with statements need to include gpoint
								// ...due to the way the glyph attributes are defined

	bool fAttachTo = false;		// true if an attach.to statement is present
	bool fAttachAtX = false;	// true if an attach.at.x/gpoint statement is present
	bool fAttachAtY = false;	// true if an attach.at.y/gpoint statement is present
	bool fAttachWithX = false;	// true if an attach.with.x/gpoint statement is present
	bool fAttachWithY = false;	// true if an attach.with.y/gpoint statement is present

	bool fDidAttachAt = false;	// did checks for flattened attach.at
	bool fDidAttachWith = false; // did checks for flattened attach.with

	Symbol psymOutput = OutputClassSymbol();

	//	Process attribute-setting statements.
	int ipavs;
	for (ipavs = 0; ipavs < signed(m_vpavs.size()); ipavs++)
	{
		GdlAttrValueSpec * pavs = m_vpavs[ipavs];

		//	Check that appropriate glyph attributes exist for the slot attributes that
		//	are making use of them.
		Symbol psym = pavs->m_psymName;
		Assert(psym->FitsSymbolType(ksymtSlotAttr) || psym->FitsSymbolType(ksymtFeature));

		if (psym->IsAttachAtField())
		{
			std::string staT = psym->LastField();
			fAttachAtX = fAttachAtX || staT == "x" || staT == "gpoint" || staT == "gpath";
			fAttachAtY = fAttachAtY || staT == "y" || staT == "gpoint" || staT == "gpath";

			if (staT == "gpath")
			{
				g_errorList.AddError(4127, this,
					"Cannot use gpath function within a rule");
				continue;
			}

			//	The engine currently can't handle single-point paths, and within the rule, we
			//	don't have a meaningful and consistent way to change such gpoint statements to
			//	x/y coordinates. So disallow gpoint statements in rules. If we find a way
			//	to make the engine handle single-point paths, take this code out.
			if (staT == "gpoint")
			{
				int nTmp;
				if (pavs->m_pexpValue->ResolveToInteger(&nTmp, false)) // constant, not glyph attr
				{
					g_errorList.AddError(4128, this,
						"Cannot use gpoint function within a rule");
					continue;
				}
			}

			if (fDidAttachAt && pavs->Flattened())
				//	The precompiler flattened the attach.at command into separate fields;
				//	in that case we don't need to check them twice.
				continue;

			//	The value of attach.at must be defined for the class of the slot
			//	receiving the attachment, not this slot.
			int srAttachToValue = AttachToSettingValue();	// 1-based
			if (srAttachToValue == -1)
				g_errorList.AddWarning(4514, this,
					"Attachment checks could not be done for value of attach.at");
			else if (srAttachToValue == -2)
			{
				fAttachTo = true;
				Assert(false);	// a VERY strange thing to happen.
				g_errorList.AddError(4129, this,
					"Inappropriate value of attach.to");
			}
			else
			{
				fAttachTo = true;
				if (pavs->Flattened())
				{
					pavs->CheckAttachAtPoint(pcman, vpglfcInClasses, srAttachToValue-1,
						&fXYAt, &fGpointAt);
					fDidAttachAt = true;
				}
				else
					pavs->FixGlyphAttrsInRules(pcman, vpglfcInClasses, srAttachToValue-1,
						psymOutput);
			}
		}

		else if (psym->IsAttachWithField())
		{
			std::string staT = psym->LastField();
			fAttachWithX = fAttachWithX || staT == "x" || staT == "gpoint" || staT == "gpath";
			fAttachWithY = fAttachWithY || staT == "y" || staT == "gpoint" || staT == "gpath";

			if (staT == "gpath")
			{
				g_errorList.AddError(4130, this,
					"Cannot use gpath function within a rule");
				continue;
			}

			if (fDidAttachWith && pavs->Flattened())
				//	The precompiler flattened the attach.with command into separate fields;
				//	in that case we don't need to check them twice.
				continue;

			if (!fAttachTo)
				fAttachTo = (AttachToSettingValue() != -1);

			if (pavs->Flattened())
			{
				pavs->CheckAttachWithPoint(pcman, vpglfcInClasses, irit,
					&fXYWith, &fGpointWith);
				fDidAttachWith = true;
			}
			else
				pavs->FixGlyphAttrsInRules(pcman, vpglfcInClasses, irit, psymOutput);
		}

		else if (psym->IsComponentRef())
		{
			CheckCompBox(pcman, psym);
		}

		else if (psymOutput == NULL)
		{	// error condition
		}
		else
		{
			if (psym->IsAttachTo())
			{
				GdlSlotRefExpression * pexpSR =
					dynamic_cast<GdlSlotRefExpression *>(pavs->m_pexpValue);
				if (pexpSR)
				{
					int srAttachTo = pexpSR->SlotNumber();
					if (srAttachTo == 0)
					{
						// no attachment
					}
					else if (prule->NumberOfSlots() <= srAttachTo - 1)
					{
						//	slot out of range--error will be produced later
					}
					// Go ahead and allow this:
//					else if (!dynamic_cast<GdlSetAttrItem *>(prule->Item(srAttachTo - 1)))
//						g_errorList.AddError(4131, this,
//							"Cannot attach to an item in the context");
				}
			}
			pavs->FixGlyphAttrsInRules(pcman, vpglfcInClasses, irit, psymOutput);
		}
	}

	if ((fAttachTo || fAttachAtX || fAttachAtY || fAttachWithX || fAttachWithY) &&
		(!fAttachTo || !fAttachAtX || !fAttachAtY || !fAttachWithX || !fAttachWithY))
	{
		if ((fAttachAtX || fAttachAtY) && !fAttachTo)
			g_errorList.AddError(4132, this,
				"Cannot specify attach.at without attach.to");
		else
			g_errorList.AddWarning(4515, this,
				"Incomplete attachment specification");
	}

	//	Delete any superfluous attach commands (that were added in FlattenPointSlotAttrs
	//	but not needed); ie, either the x/y point fields or the gpath field. It's possible
	//	that we need to keep both versions, if one set of glyphs uses one and another set
	//	uses the other.
	for (ipavs = signed(m_vpavs.size()); --ipavs >= 0; )
	{
		bool fDeleteThis = false;
		Symbol psym = m_vpavs[ipavs]->m_psymName;
		if (psym->IsAttachAtField() && m_vpavs[ipavs]->Flattened())
		{
			if (!fXYAt && (psym->LastFieldIs("x") || psym->LastFieldIs("y")))
				//	Keep attach.at.gpoint; throw away x/y.
				fDeleteThis = true;
			else if (!fGpointAt && psym->LastFieldIs("gpoint"))
				//	Keep attach.at.x/y; throw away gpoint.
				fDeleteThis = true;
		}
		else if (psym->IsAttachWithField() && m_vpavs[ipavs]->Flattened())
		{
			if (!fXYWith && (psym->LastFieldIs("x") || psym->LastFieldIs("y")))
				//	Keep attach.with.gpoint; throw away x/y.
				fDeleteThis = true;
			else if (!fGpointWith && psym->LastFieldIs("gpoint"))
				//	Keep attach.with.x/y; throw away gpoint;
				fDeleteThis = true;
		}

		if (fDeleteThis)
		{
			delete m_vpavs[ipavs];
			m_vpavs.erase(m_vpavs.begin() + ipavs);
		}
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlSubstitutionItem::FixGlyphAttrsInRules(GrcManager * pcman,
	std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, GdlRule * prule, int irit)
{
	GdlSetAttrItem::FixGlyphAttrsInRules(pcman, vpglfcInClasses, prule, irit);
}

/*--------------------------------------------------------------------------------------------*/
#ifdef NDEBUG
void GdlAttrValueSpec::FixGlyphAttrsInRules(GrcManager * pcman,
	std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit, Symbol /*psymOutClass*/)
#else
void GdlAttrValueSpec::FixGlyphAttrsInRules(GrcManager * pcman,
	std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit, Symbol psymOutClass)
#endif
{
	Assert(psymOutClass->FitsSymbolType(ksymtClass) ||
		psymOutClass->FitsSymbolType(ksymtSpecialUnderscore) ||
		psymOutClass->FitsSymbolType(ksymtSpecialAt));

	SymbolType symtName = this->m_psymName->SymType();
	if (m_pexpValue)
	{
		m_pexpValue->CheckAndFixGlyphAttrsInRules(pcman, vpglfcInClasses, irit);
		m_pexpValue->LookupExpCheck(false, ((symtName == ksymtFeature) ? m_psymName : NULL));
	}
}

/*----------------------------------------------------------------------------------------------
	Return the symbol for the output class to use in checking the rule item. Specifically,
	if a substitution item has a selector and no class (ie, is something like '@2') return
	the class from the appropriate selected item. Return NULL if there is no input class,
	or the selector was invalid (in which cases an error was already recorded).
----------------------------------------------------------------------------------------------*/
Symbol GdlSetAttrItem::OutputClassSymbol()
{
	return OutputSymbol();
}

/*--------------------------------------------------------------------------------------------*/
Symbol GdlSubstitutionItem::OutputClassSymbol()
{
	if (m_psymOutput->FitsSymbolType(ksymtSpecialAt))
	{
		if (!m_pritSelInput)
			return NULL;
		return m_pritSelInput->m_psymInput;
	}

	return m_psymOutput;
}


/*----------------------------------------------------------------------------------------------
	Flatten any slot attributes that use points to use integers instead. That is, replace
	them with versions in terms of the fields of a point that are appropriate. So instead of

	  { attach.with = point1 }

	we generate

	  { attach.with {
			x = point1.x; y = point1.y;
			gpoint = point1.gpoint;
			xoffset = point1.xoffset; yoffset = point1.yoffset }
	  }	
	  
	We create as many of the above five as are defined in the symbol table, and later delete
	the superfluous one(s) (x/y or gpoint).
----------------------------------------------------------------------------------------------*/
void GdlRuleItem::FlattenPointSlotAttrs(GrcManager * /*pcman*/)
{
	//	No attribute setters to worry about.
}

/*--------------------------------------------------------------------------------------------*/
void GdlSetAttrItem::FlattenPointSlotAttrs(GrcManager * pcman)
{
	std::vector<GdlAttrValueSpec *> vpavsNew;
	for (size_t ipavs = 0; ipavs < m_vpavs.size(); ipavs++)
	{
		m_vpavs[ipavs]->FlattenPointSlotAttrs(pcman, vpavsNew);
	}
	m_vpavs.clear();
	m_vpavs.assign(vpavsNew.begin(), vpavsNew.end());
}

/*--------------------------------------------------------------------------------------------*/
void GdlAttrValueSpec::FlattenPointSlotAttrs(GrcManager * pcman,
	std::vector<GdlAttrValueSpec *> & vpavsNew)
{
	if (m_psymName->IsBogusSlotAttr())	// eg, shift.gpath
	{
		int nTmp;
		if ((m_psymName->LastFieldIs("xoffset") || m_psymName->LastFieldIs("yoffset")) &&
			m_pexpValue->ResolveToInteger(&nTmp, false) && nTmp == 0)
		{	// ignore
		}
		else
			g_errorList.AddError(4133, this,
				"Invalid slot attribute: ",
				m_psymName->FullName());
		delete this;
	}
	else if (m_psymName->FitsSymbolType(ksymtSlotAttrPtOff)		// attach.at, shift, etc.
		|| m_psymName->FitsSymbolType(ksymtSlotAttrPt))			// collision.min/max
	{
		if (m_psymName->IsReadOnlySlotAttr())
		{
			//	Eventually will produce an error--for now, just pass through as is.
			vpavsNew.push_back(this);
			return;
		}

		if (m_psymOperator->FullName() != "=")
		{
			//	Can't use +=, -= with entire points.
			g_errorList.AddError(4134, this,
				"Invalid point arithmetic; fields must be calculated independently in order to use ",
				m_psymOperator->FullName());
			return;
		}

		Symbol psymX = m_psymName->SubField("x");
		Symbol psymY = m_psymName->SubField("y");
		Symbol psymGpoint = m_psymName->SubField("gpoint");
		Symbol psymXoffset = m_psymName->SubField("xoffset");
		Symbol psymYoffset = m_psymName->SubField("yoffset");
		Assert(psymX);
		Assert(psymY);
		if (m_psymName->FitsSymbolType(ksymtSlotAttrPtOff))
		{
			Assert(psymGpoint);
			Assert(psymXoffset);
			Assert(psymYoffset);
		}
		else
		{
			Assert(psymGpoint == NULL);
			Assert(psymXoffset == NULL);
			Assert(psymYoffset == NULL);
		}

		GdlExpression * pexpX = NULL;
		GdlExpression * pexpY = NULL;
		GdlExpression * pexpGpoint = NULL;
		GdlExpression * pexpXoffset = NULL;
		GdlExpression * pexpYoffset = NULL;

		bool fExpOkay = m_pexpValue->PointFieldEquivalents(pcman,
			&pexpX, &pexpY, &pexpGpoint, &pexpXoffset, &pexpYoffset);
		if (!fExpOkay)
		{
			g_errorList.AddError(4135, this,
				"Invalid point arithmetic");
			delete this;
			return;
		}
		else if (!pexpX && !pexpY && !pexpGpoint && !pexpYoffset && !pexpYoffset)
		{
			GdlLookupExpression * pexpLookup =
				dynamic_cast<GdlLookupExpression *>(m_pexpValue);
			Assert(pexpLookup);
			if (pexpLookup->Name()->FitsSymbolType(ksymtGlyphAttr))
				g_errorList.AddError(4136, this,
					"Glyph attribute is not a point: ",
					pexpLookup->Name()->FullName());
			else
				g_errorList.AddError(4137, this,
					"Undefined glyph attribute: ",
					pexpLookup->Name()->FullName());
			delete this;
			return;
		}
		
		GdlAttrValueSpec * pavs;
		if (pexpX)
		{
			pavs = new GdlAttrValueSpec(psymX, m_psymOperator, pexpX);
			pavs->CopyLineAndFile(*this);
			pavs->SetFlattened(true);
			vpavsNew.push_back(pavs);
		}
		if (pexpY)
		{
			pavs = new GdlAttrValueSpec(psymY, m_psymOperator, pexpY);
			pavs->CopyLineAndFile(*this);
			pavs->SetFlattened(true);
			vpavsNew.push_back(pavs);
		}
		if (g_cman.OffsetAttrs())
		{
			if (pexpGpoint)
			{
				if (psymGpoint->IsBogusSlotAttr())
					delete pexpGpoint;
				else
				{
					pavs = new GdlAttrValueSpec(psymGpoint, m_psymOperator, pexpGpoint);
					pavs->CopyLineAndFile(*this);
					pavs->SetFlattened(true);
					vpavsNew.push_back(pavs);
				}
			}
			if (pexpXoffset)
			{
				if (psymXoffset->IsBogusSlotAttr())
					delete pexpXoffset;
				else
				{
					pavs = new GdlAttrValueSpec(psymXoffset, m_psymOperator, pexpXoffset);
					pavs->CopyLineAndFile(*this);
					pavs->SetFlattened(true);
					vpavsNew.push_back(pavs);
				}
			}
			if (pexpYoffset)
			{
				if (psymYoffset->IsBogusSlotAttr())
					delete pexpYoffset;
				else
				{
					pavs = new GdlAttrValueSpec(psymYoffset, m_psymOperator, pexpYoffset);
					pavs->CopyLineAndFile(*this);
					pavs->SetFlattened(true);
					vpavsNew.push_back(pavs);
				}
			}
		} else {
			if (pexpGpoint)
				delete pexpGpoint;
			if (pexpXoffset)
				delete pexpXoffset;
			if (pexpYoffset)
				delete pexpYoffset;
		}

		delete this;	// replaced
	}
	else
		vpavsNew.push_back(this);
}


/*----------------------------------------------------------------------------------------------
	Check that the given glyph attribute is defined for every glyph that this
	class subsumes. Also check that all the necessary point fields or
	ligature component box fields are defined.
	Arguments
		pgdlAvsOrExp			- for error message--attr value spec or expression
		psymtbl					- global symbol table
		pgax					- glyph attr matrix
		psymGlyphAttr			- attribute to check for
----------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::CheckExistenceOfGlyphAttr(GdlObject * pgdlAvsOrExp,
	GrcSymbolTable * psymtbl, GrcGlyphAttrMatrix * pgax, Symbol psymGlyphAttr)
{
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		m_vpglfdMembers[iglfd]->CheckExistenceOfGlyphAttr(pgdlAvsOrExp, psymtbl, pgax,
			psymGlyphAttr);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphDefn::CheckExistenceOfGlyphAttr(GdlObject * pgdlAvsOrExp,
	GrcSymbolTable * /*psymtbl*/, GrcGlyphAttrMatrix * pgax, Symbol psymGlyphAttr)
{
	int nGlyphAttrID = psymGlyphAttr->InternalID();
	bool fGpoint = psymGlyphAttr->LastFieldIs("gpoint");

	for (size_t iw = 0; iw < m_vwGlyphIDs.size(); iw++)
	{
		if (m_vwGlyphIDs[iw] == kBadGlyph)
			continue;

		utf16 wGlyphID = m_vwGlyphIDs[iw];
		if ((fGpoint && !pgax->GpointDefined(wGlyphID, nGlyphAttrID)) ||
			(!fGpoint && !pgax->Defined(wGlyphID, nGlyphAttrID)))
		{
			g_errorList.AddError(4138, pgdlAvsOrExp,
				"Glyph attribute '",
				psymGlyphAttr->FullName(),
				"' is not defined for glyph ",
				GlyphIDString(wGlyphID));
		}
	}
}

/*----------------------------------------------------------------------------------------------
	Return the value of the attach.to setting, -1 if it was not found, or -2 if the value
	was not slot reference.
----------------------------------------------------------------------------------------------*/
int GdlSetAttrItem::AttachToSettingValue()
{
	for (size_t iavs = 0; iavs < m_vpavs.size(); iavs++)
	{
		Symbol psym = m_vpavs[iavs]->m_psymName;
		if (psym->IsAttachTo())
		{
			GdlSlotRefExpression * pexpSR =
				dynamic_cast<GdlSlotRefExpression *>(m_vpavs[iavs]->m_pexpValue);
			if (pexpSR)
				return pexpSR->SlotNumber();
			else
				return -2;
		}
	}
	return -1;
}

/*----------------------------------------------------------------------------------------------
	The recipient is the setting of the attach.at slot attribute.
	Check that the attachment point in the value is defined for all glyphs subsumed by
	pglfc, which is the slot being attached to.
----------------------------------------------------------------------------------------------*/
void GdlAttrValueSpec::CheckAttachAtPoint(GrcManager * pcman,
	std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit,
	bool * pfXY, bool *pfGpoint)
{
	Assert(!m_psymName->LastFieldIs("gpath"));	// caller checked for this

	m_pexpValue->CheckCompleteAttachmentPoint(pcman, vpglfcInClasses, irit,
		pfXY, pfGpoint);
}

/*----------------------------------------------------------------------------------------------
	The recipient is the setting of the attach.with slot attribute.
	Check that the attachment point in the value is defined for all glyphs subsumed by
	pglfc, which is the slot being attached.
----------------------------------------------------------------------------------------------*/
void GdlAttrValueSpec::CheckAttachWithPoint(GrcManager * pcman,
	std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit,
	bool * pfXY, bool * pfGpoint)
{
	Assert(!m_psymName->LastFieldIs("gpath"));	// caller checked for this

	m_pexpValue->CheckCompleteAttachmentPoint(pcman, vpglfcInClasses, irit,
		pfXY, pfGpoint);
}

/*----------------------------------------------------------------------------------------------
	Check that the necessary fields of the given glyph attribute--an attachment point--
	are defined for all the glyphs subsumed by this class.
	Arguments:
		pgdlAvsOrExp			- for error message--attr value spec or expression
		psymtbl					- global symbol table
		pgax					- glyph attr matrix
		psymGlyphAttr			- attribute to check for
		pfXY					- return true if x and y are defined
		pfGpoint				- return true if gpoint is defined
----------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::CheckCompleteAttachmentPoint(GdlObject * pgdlAvsOrExp,
	GrcSymbolTable * psymtbl, GrcGlyphAttrMatrix * pgax, Symbol psymGlyphAttr,
	bool * pfXY, bool * pfGpoint)
{
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		m_vpglfdMembers[iglfd]->CheckCompleteAttachmentPoint(pgdlAvsOrExp, psymtbl, pgax,
			psymGlyphAttr, pfXY, pfGpoint);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphDefn::CheckCompleteAttachmentPoint(GdlObject * pgdlAvsOrExp,
	GrcSymbolTable * /*psymtbl*/, GrcGlyphAttrMatrix * pgax, Symbol psymGlyphAttr,
	bool * pfXY, bool * pfGpoint)
{
	Symbol psymX = psymGlyphAttr->SubField("x");
	Symbol psymY = psymGlyphAttr->SubField("y");
	Symbol psymGpoint = psymGlyphAttr->SubField("gpoint");
	//Symbol psymXoffset = psymGlyphAttr->SubField("xoffset");
	//Symbol psymYoffset = psymGlyphAttr->SubField("yoffset");

	for (size_t iw = 0; iw < m_vwGlyphIDs.size(); iw++)
	{
		utf16 wGlyphID = m_vwGlyphIDs[iw];

		if (wGlyphID == kBadGlyph)
			continue;

		if (psymGpoint && pgax->GpointDefined(wGlyphID, psymGpoint->InternalID()))
		{
			bool fShadowX = false; bool fShadowY = false;
			bool fAlsoX = (psymX && pgax->DefinedButMaybeShadow(wGlyphID, psymX->InternalID(), &fShadowX));
			bool fAlsoY = (psymY && pgax->DefinedButMaybeShadow(wGlyphID, psymY->InternalID(), &fShadowY));
			// Error already handled in ConvertBetweenXYAndGpoint
//			if (fAlsoX && !fShadowY && fAlsoY && !fShadowY)
//			{
//				g_errorList.AddWarning(4516, pgdlAvsOrExp,
//					"Both x/y coordinates and gpoint are defined for ",
//					psymGlyphAttr->FullName(),
//					" for glyph ",
//					GlyphIDString(wGlyphID),
//					"; only gpoint will be used");
//			}

			*pfGpoint = true;

			if (fAlsoX && fShadowX && fAlsoY && fShadowY)
			{
				*pfXY = true;
			}
			else
			{
				if (fAlsoX)
					pgax->Clear(wGlyphID, psymX->InternalID());
				if (fAlsoY)
					pgax->Clear(wGlyphID, psymY->InternalID());
			}
		}
		else if (psymX && pgax->Defined(wGlyphID, psymX->InternalID()) &&
			psymY && pgax->Defined(wGlyphID, psymY->InternalID()))
		{
			*pfXY = true;
		}
		else
		{
			g_errorList.AddWarning(4517, pgdlAvsOrExp,
				"Point '",
				psymGlyphAttr->FullName(),
				"' not completely defined for glyph ",
				GlyphIDString(wGlyphID));
		}
	}
}

/*----------------------------------------------------------------------------------------------
	The recipient is a slot that is having a component.???.ref attribute set. Check to make
	sure that the appropriate component box is fully defined for all glyphs.
----------------------------------------------------------------------------------------------*/
void GdlSetAttrItem::CheckCompBox(GrcManager * pcman, Symbol psymCompRef)
{
	Assert(psymCompRef->IsComponentRef());

	GdlGlyphClassDefn * pglfc = OutputSymbol()->GlyphClassDefnData();
	if (!pglfc)
		return;

	Symbol psymBaseComp = psymCompRef->BaseLigComponent();

	pglfc->CheckCompBox(this, pcman->SymbolTable(), pcman->GlyphAttrMatrix(), psymBaseComp);
}

/*----------------------------------------------------------------------------------------------
	Check to make sure that the given component's box has been fully defined
	for all the subsumed glyphs.
----------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::CheckCompBox(GdlObject * pgdlSetAttrItem,
	GrcSymbolTable * psymtbl, GrcGlyphAttrMatrix * pgax, Symbol psymCompRef)
{
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		m_vpglfdMembers[iglfd]->CheckCompBox(pgdlSetAttrItem, psymtbl, pgax, psymCompRef);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphDefn::CheckCompBox(GdlObject * pgdlSetAttrItem,
	GrcSymbolTable * /*psymtbl*/, GrcGlyphAttrMatrix * pgax, Symbol psymCompRef)
{
	Symbol psymTop = psymCompRef->SubField("top");
	Symbol psymBottom = psymCompRef->SubField("bottom");
	Symbol psymLeft = psymCompRef->SubField("left");
	Symbol psymRight = psymCompRef->SubField("right");

	for (size_t iw = 0; iw < m_vwGlyphIDs.size(); iw++)
	{
		utf16 wGlyphID = m_vwGlyphIDs[iw];

		if (wGlyphID == kBadGlyph)
			continue;

		if (!psymTop || !pgax->Defined(wGlyphID, psymTop->InternalID()))
		{
			g_errorList.AddError(4139, pgdlSetAttrItem,
				"Top of box for ",
				psymCompRef->FullName(),
				" not defined for glyph ",
				GlyphIDString(wGlyphID));
		}
		if (!psymBottom || !pgax->Defined(wGlyphID, psymBottom->InternalID()))
		{
			g_errorList.AddError(4140, pgdlSetAttrItem,
				"Bottom of box for ",
				psymCompRef->FullName(),
				" not defined for glyph ",
				GlyphIDString(wGlyphID));
		}
		if (!psymLeft || !pgax->Defined(wGlyphID, psymLeft->InternalID()))
		{
			g_errorList.AddError(4141, pgdlSetAttrItem,
				"Left of box for ",
				psymCompRef->FullName(),
				" not defined for glyph ",
				GlyphIDString(wGlyphID));
		}
		if (!psymRight || !pgax->Defined(wGlyphID, psymRight->InternalID()))
		{
			g_errorList.AddError(4142, pgdlSetAttrItem,
				"Right of box for ",
				psymCompRef->FullName(),
				" not defined for glyph ",
				GlyphIDString(wGlyphID));
		}
	}
}

/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Process all the feature testing-constraints: supply the feature contexts to the values
	(ie, in the test ligatures == some, the "some" must be converted to the value for
	'some' setting of the ligatures feature. Record an error if the setting is undefined.
----------------------------------------------------------------------------------------------*/
void GdlRule::FixFeatureTestsInRules(GrcFont * pfont)
{
	for (size_t ipexp = 0; ipexp < m_vpexpConstraints.size(); ipexp++)
	{
		m_vpexpConstraints[ipexp]->FixFeatureTestsInRules(pfont);
		m_vpexpConstraints[ipexp]->LookupExpCheck(true, NULL);
	}

	for (size_t irit = 0; irit < m_vprit.size(); irit++)
	{
		m_vprit[irit]->FixFeatureTestsInRules(pfont);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleItem::FixFeatureTestsInRules(GrcFont * pfont)
{
	if (m_pexpConstraint)
	{
		m_pexpConstraint->FixFeatureTestsInRules(pfont);
		m_pexpConstraint->LookupExpCheck(false, NULL);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlSetAttrItem::FixFeatureTestsInRules(GrcFont * pfont)
{
	GdlRuleItem::FixFeatureTestsInRules(pfont);

	for (size_t iavs = 0; iavs < this->m_vpavs.size(); iavs++)
		m_vpavs[iavs]->FixFeatureTestsInRules(pfont);
}

/*--------------------------------------------------------------------------------------------*/
void GdlPass::FixFeatureTestsInPass(GrcFont * pfont)
{
	for (size_t ipexp = 0; ipexp < m_vpexpConstraints.size(); ipexp++)
	{
		m_vpexpConstraints[ipexp]->FixFeatureTestsInRules(pfont);
		m_vpexpConstraints[ipexp]->LookupExpCheck(true, NULL);
	}
}
/*--------------------------------------------------------------------------------------------*/
void GdlAttrValueSpec::FixFeatureTestsInRules(GrcFont * pfont)
{
	// Particularly handle feature tests in conditional statements.
	m_pexpValue->FixFeatureTestsInRules(pfont);
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Resolve all glyph attribute definitions to integers.
----------------------------------------------------------------------------------------------*/
bool GrcManager::FinalGlyphAttrResolution(GrcFont * pfont)
{
	int cStdStyles = max(signed(m_vpsymStyles.size()), 1);

	//Symbol psymJStr = m_psymtbl->FindSymbol(GrcStructName("justify", "0", "stretch"));
	Symbol psymJStr = m_psymtbl->FindSymbol(GrcStructName("justify", "stretch"));
	int nAttrIdJStr = psymJStr->InternalID();
	//Symbol psymJShr = m_psymtbl->FindSymbol(GrcStructName("justify", "0", "shrink"));
	Symbol psymJShr = m_psymtbl->FindSymbol(GrcStructName("justify", "shrink"));
	int nAttrIdJShr = psymJShr->InternalID();
	//Symbol psymJStep = m_psymtbl->FindSymbol(GrcStructName("justify", "0", "step"));
	Symbol psymJStep = m_psymtbl->FindSymbol(GrcStructName("justify", "step"));
	int nAttrIdJStep = psymJStep->InternalID();
	//Symbol psymJWeight = m_psymtbl->FindSymbol(GrcStructName("justify", "0", "weight"));
	Symbol psymJWeight = m_psymtbl->FindSymbol(GrcStructName("justify", "weight"));
	int nAttrIdJWeight = psymJWeight->InternalID();
	Symbol psymSkipPasses = m_psymtbl->FindSymbol(GrcStructName("*skipPasses*"));
	int nAttrIdSkipPasses = psymSkipPasses->InternalID();

	for (utf16 wGlyphID = 0; wGlyphID < m_cwGlyphIDs; wGlyphID++)
	{
		for (size_t iAttrID = 0; iAttrID < m_vpsymGlyphAttrs.size(); iAttrID++)
		{
			for (int iStyle = 0; iStyle < cStdStyles; iStyle++)
			{
				SymbolSet setpsym;
				GdlExpression * pexp;
				GdlExpression * pexpNew;
				int nPR;
				int munitPR;
				bool fOverride, fShadow;
				GrpLineAndFile lnf;
				m_pgax->Get(wGlyphID, iAttrID, &pexp, &nPR, &munitPR, &fOverride, &fShadow, &lnf);
				if (pexp)
				{
					bool fCanSub;
					pexpNew =
						pexp->SimplifyAndUnscale(m_pgax, wGlyphID, setpsym, pfont, true, &fCanSub);

					int nMinValue, nMaxValue;
					MinAndMaxGlyphAttrValues(iAttrID,
						NumJustLevels(), nAttrIdJStr, nAttrIdJShr, nAttrIdJStep, nAttrIdJWeight,
						nAttrIdSkipPasses,
						&nMinValue, &nMaxValue);

					if (pexpNew && pexpNew != pexp)
					{
						m_vpexpModified.push_back(pexpNew);	// so we can delete it later
						m_pgax->Set(wGlyphID, iAttrID,
							pexpNew, nPR, munitPR, fOverride, false, lnf);
						pexp = pexpNew;
					}
					int n;
					if (!pexp->ResolveToInteger(&n, false))
					{
						g_errorList.AddError(4143, pexp,
							"Could not resolve definition of glyph attribute ",
							m_vpsymGlyphAttrs[iAttrID]->FullName(),
							" for glyph ",
							GdlGlyphDefn::GlyphIDString(wGlyphID));
					}
					else if (n <= nMinValue)
					{
						char rgch1[20];
						char rgch2[20];
						itoa(n, rgch1, 10);
						itoa(nMinValue + 1, rgch2, 10);
						g_errorList.AddError(4144, pexp,
							"Value of glyph attribute ",
							m_vpsymGlyphAttrs[iAttrID]->FullName(),
							" for glyph ",
							GdlGlyphDefn::GlyphIDString(wGlyphID),
							" = ", rgch1,
							"; minimum is ",
							rgch2);
					}
					else if (n >= nMaxValue)
					{
						char rgch1[20];
						char rgch2[20];
						itoa(n, rgch1, 10);
						itoa(nMaxValue - 1, rgch2, 10);
						g_errorList.AddError(4145, pexp,
							"Value of glyph attribute ",
							m_vpsymGlyphAttrs[iAttrID]->FullName(),
							" for glyph ",
							GdlGlyphDefn::GlyphIDString(wGlyphID),
							" = ", rgch1,
							"; maximum is ",
							rgch2);
					}
					else if (n == 0 && m_vpsymGlyphAttrs[iAttrID]->LastFieldIs("gpoint"))
					{
						//	Replace gpoint = 0 with a special value, since we use zero to
						//	indicate "no legitimate value."
						pexp->SetSpecialZero();
					}
				}
			}
		}

		// Just in case, since incrementing 0xFFFF will produce zero.
		if (wGlyphID == 0xFFFF)
			break;
	}

	return true;
}

/*----------------------------------------------------------------------------------------------
	Return the minimum and maximum values for the given attribute (actually this is the
	minimum - 1 and the maximum + 1).
----------------------------------------------------------------------------------------------*/
void GrcManager::MinAndMaxGlyphAttrValues(int nAttrID,
	int cJLevels, int nAttrIdJStr, int nAttrIdJShr, int nAttrIdJStep, int nAttrIdJWeight,
	int nAttrIdSkipPasses,
	int * pnMin, int * pnMax)
{
	*pnMin = kMinGlyphAttrValue;
	*pnMax = kMaxGlyphAttrValue;
	if (nAttrIdJStr <= nAttrID && nAttrID < nAttrIdJStr + cJLevels)
	{
		//	justify.stretch
		*pnMin = -1;
		*pnMax = 0x40000000;
	}
	else if (nAttrIdJShr <= nAttrID && nAttrID < nAttrIdJShr + cJLevels)
	{
		//	justify.shrink
		*pnMin = -1;
	}
	else if (nAttrIdJStep <= nAttrID && nAttrID < nAttrIdJStep + cJLevels)
	{
		//	justify.step
		*pnMin = -1;
	}
	else if (nAttrIdJWeight <= nAttrID && nAttrID < nAttrIdJWeight + cJLevels)
	{
		//	justify.weight
		*pnMin = 0;
		*pnMax = 255;
	}
	else if (nAttrIdSkipPasses == nAttrID)
	{
		*pnMin = -1;
		*pnMax = 0x10000; // 1 + actual max=0xFFFF (since test is >= )
	}
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	For each pseudo glyph in the system, store the associated actual glyph ID as a glyph
	attribute, specifically as the value of the glyph attribute "*actualForPseudo*.
----------------------------------------------------------------------------------------------*/
bool GrcManager::StorePseudoToActualAsGlyphAttr()
{
	Symbol psym = m_psymtbl->FindSymbol("*actualForPseudo*");
	Assert(psym);
	int nAttrID = psym->InternalID();

	m_prndr->StorePseudoToActualAsGlyphAttr(m_pgax, nAttrID, m_vpexpModified);

	return true;
}

/*--------------------------------------------------------------------------------------------*/
void GdlRenderer::StorePseudoToActualAsGlyphAttr(GrcGlyphAttrMatrix * pgax, int nAttrID,
	std::vector<GdlExpression *> & vpexpExtra)
{
	for (size_t iglfc = 0; iglfc < m_vpglfc.size(); iglfc++)
		m_vpglfc[iglfc]->StorePseudoToActualAsGlyphAttr(pgax, nAttrID, vpexpExtra);
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::StorePseudoToActualAsGlyphAttr(GrcGlyphAttrMatrix * pgax, int nAttrID,
	std::vector<GdlExpression *> & vpexpExtra)
{
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
		m_vpglfdMembers[iglfd]->StorePseudoToActualAsGlyphAttr(pgax, nAttrID,vpexpExtra);
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphDefn::StorePseudoToActualAsGlyphAttr(GrcGlyphAttrMatrix * pgax, int nAttrID,
	std::vector<GdlExpression *> & vpexpExtra)
{
	if (m_glft == kglftPseudo && m_pglfOutput && m_pglfOutput->m_vwGlyphIDs.size() > 0)
	{
		utf16 wOutput = m_pglfOutput->m_vwGlyphIDs[0];
		GdlExpression * pexp = new GdlNumericExpression(wOutput);
		vpexpExtra.push_back(pexp);
		pgax->Set(m_wPseudo, nAttrID, pexp, 0, 0, true, false, GrpLineAndFile(0, 0, ""));
	}
}

/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Return the first glyph in the class. Also return a flag indicating if there was more
	than one.
----------------------------------------------------------------------------------------------*/
unsigned int GdlGlyphClassDefn::FirstGlyphInClass(bool * pfMoreThanOne)
{
	if (m_vpglfdMembers.size() == 0)
		return 0;
	int n = 0;
	for (size_t iglfd = 0; n == 0 && iglfd < m_vpglfdMembers.size(); iglfd++)
		n = m_vpglfdMembers[iglfd]->FirstGlyphInClass(pfMoreThanOne);
	if (m_vpglfdMembers.size() > 1)
		*pfMoreThanOne = true;
	return n;
}

/*--------------------------------------------------------------------------------------------*/
unsigned int GdlGlyphDefn::FirstGlyphInClass(bool * pfMoreThanOne)
{
	// This could be more accurate (for instance, it won't exactly handle a class with all
	// bad glyphs except for one good one), but the more-than-one flag is just there for the
	// sake of giving a warning, so this is good enough.
	if (m_vwGlyphIDs.size() > 1)
		*pfMoreThanOne = true;
	for (size_t iw = 0; iw < m_vwGlyphIDs.size(); iw++)
	{
		if (m_vwGlyphIDs[iw] == kBadGlyph)
			continue;
		return m_vwGlyphIDs[iw];
	}
	return 0; // pathological?
}

/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Give a warning about any empty classes.
----------------------------------------------------------------------------------------------*/
bool GrcManager::CheckForEmptyClasses()
{
	for (SymbolTableMap::iterator it = m_psymtbl->EntriesBegin();
		it != m_psymtbl->EntriesEnd();
		++it)
	{
		Symbol psym = it->second; // GetValue();
		//Symbol psym = it.GetValue();

		//if (psym->m_psymtblSubTable)
		//	psym->m_psymtblSubTable->CheckForEmptyClasses();

		if (psym->FitsSymbolType(ksymtClass) && psym->HasData())
		{
			GdlGlyphClassDefn * pglfc = psym->GlyphClassDefnData();
			int cglf = pglfc->GlyphIDCount();
			if (cglf == 0)
				g_errorList.AddWarning(4518, pglfc,
					"Empty class definition: ",
					psym->FullName());
		}
	}

	return true;
}

