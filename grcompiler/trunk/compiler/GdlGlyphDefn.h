/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GdlGlyph.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Description of simple glyph or range of glyphs.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef GLYPH_INCLUDED
#define GLYPH_INCLUDED

/*----------------------------------------------------------------------------------------------
Class: GdlGlyphDefn
Description: A glyph or set of glyphs specified by a single codepoint, Unicode value,
	Postscript name, or a contiguous range of these.
Hungarian: glf
----------------------------------------------------------------------------------------------*/

class PsuedoLess;

class GdlGlyphDefn : public GdlGlyphClassMember
{
	friend class PseudoLess;

public:
	//	Constructors:
	//	unicode(0x1234), glyphid(0x3333)
	GdlGlyphDefn(GlyphType glft, int nFirst) 
		:	m_glft(glft),
			m_nFirst(nFirst),
			m_nLast(nFirst),
			m_wCodePage(0),
			m_nUnicodeInput(0),
			m_pglfOutput(NULL),
			m_fGAResolved(false),
			m_fNoRangeCheck(false)
	{
		Assert(m_glft != kglftPseudo);
		Assert(m_glft != kglftCodepoint); // must include a codepage
	}

	//	unicode(0x1234..1237), glyphid(0x3333..03335), codepage(0x1234, 1252);
	GdlGlyphDefn(GlyphType glft, int nFirst, int nLast)
		:	m_glft(glft),
			m_nFirst(nFirst),
			m_nLast(nLast),
			m_wCodePage(0),
			m_nUnicodeInput(0),
			m_pglfOutput(NULL),
			m_fGAResolved(false),
			m_fNoRangeCheck(false)
	{
		Assert(m_glft != kglftPseudo);

		if (m_glft == kglftCodepoint)
		{
			m_wCodePage = (utf16)m_nLast;
			m_nLast = m_nFirst;
		}
	}

	//	codepoint(1..2, 0x2222)
	GdlGlyphDefn(GlyphType glft, int nFirst, int nLast, utf16 wCodePage)
		:	m_glft(glft),
			m_nFirst(nFirst),
			m_nLast(nLast),
			m_wCodePage(wCodePage),
			m_nUnicodeInput(0),
			m_pglfOutput(NULL),
			m_fGAResolved(false)
	{
		Assert(m_glft == kglftCodepoint);
	}

	//	postscript("Ccedilla")
	GdlGlyphDefn(GlyphType glft, std::string sta)
		:	m_glft(glft),
			m_nFirst(0),
			m_nLast(0),
			m_wCodePage(0),
			m_nUnicodeInput(0),
			m_pglfOutput(NULL),
			m_sta(sta),
			m_fGAResolved(false)
	{
		Assert(m_glft == kglftPostscript);
	}

	//	codepoint("abc", 0x04e4)
	GdlGlyphDefn(GlyphType glft, std::string sta, utf16 wCodePage)
		:	m_glft(glft),
			m_nFirst(0),
			m_nLast(0),
			m_wCodePage(wCodePage),
			m_nUnicodeInput(0),
			m_pglfOutput(NULL),
			m_sta(sta),
			m_fGAResolved(false)
	{
		Assert(m_glft == kglftCodepoint);
	}

	//	pseudo(unicode(0x3344))
	GdlGlyphDefn(GlyphType glft, GdlGlyphDefn * pglf)
		:	m_glft(glft),
			m_pglfOutput(pglf),
			m_nFirst(0),
			m_nLast(0),
			m_wCodePage(0),
			m_nUnicodeInput(0),
			m_fGAResolved(false)
	{
		Assert(m_glft == kglftPseudo);
		Assert(m_pglfOutput->m_glft != kglftPseudo);
	}

	//	pseudo(unicode(0x3344), 0xf123)
	GdlGlyphDefn(GlyphType glft, GdlGlyphDefn * pglf, int nInput)
		:	m_glft(glft),
			m_pglfOutput(pglf),
			m_nFirst(0),
			m_nLast(0),
			m_wCodePage(0),
			m_nUnicodeInput(nInput),
			m_fGAResolved(false)
	{
		Assert(m_glft == kglftPseudo);
		Assert(m_pglfOutput->m_glft != kglftPseudo);
	}

	~GdlGlyphDefn()
	{
		if (m_pglfOutput)
			delete m_pglfOutput;
	}

	//	Error checking:
//	int ErrorCheck()
//	{
//		if (m_sta.Length() == 0 && m_glft == kglftPostscript)
//			return 1;	// ERROR: bad POSTSCRIPT fcn format--omitted PS name
//		if (m_sta.Length() != 0 && m_glft != kglftPostscript)
//			return 1;	// ERROR: bad fcn format--included string
//	}

	//	Getters:
	GlyphType GetGlyphType()
	{
		return m_glft;
	}
	unsigned int First()
	{
		return m_nFirst;
	}
	unsigned int Last()
	{
		return m_nLast;
	}
	utf16 CodePage()
	{
		Assert(m_glft == kglftCodepoint);
		return m_wCodePage;
	}
	unsigned int UnicodeInput()
	{
		Assert(m_glft == kglftPseudo);
		return m_nUnicodeInput;
	}
	GdlGlyphDefn * OutputGlyph()
	{
		Assert(m_glft == kglftPseudo);
		return m_pglfOutput;
	}
	std::string PostscriptName()
	{
		Assert(m_glft == kglftPostscript);
		return m_sta;
	}
	std::string CodepointString()
	{
		Assert(m_glft == kglftCodepoint);
		return m_sta;
	}
	utf16 AssignedPseudo()
	{
		Assert(m_glft == kglftPseudo);
		return m_wPseudo;
	}

	//	Setters:
	void SetAssignedPseudo(utf16 w)
	{
		m_wPseudo = w;
	}

	void SetNoRangeCheck()
	{
		m_fNoRangeCheck = true;
	}

public:
	//	Pre-compiler:
	virtual void ExplicitPseudos(PseudoSet & setpglf);
	virtual int ActualForPseudo(utf16 wPseudo);
	//	Answer true if there is exactly one glyph represented by the object.
//	bool SingleGlyph()
//	{
//		if (m_glft == kglftPostscript)
//			return true;
//		if (m_glft == kglftUnicode || m_glft == kglftGlyphID)
//			return m_wFirst > 0 && m_wFirst == m_wLast;
//		Assert(m_glft == kglftCodepoint);
//		if (m_wFirst == 0 && m_wLast == 0)
//			return m_sta.Length() == 1;
//		else
//			return m_wFirst == m_wLast;
//	}
	virtual int GlyphIDCount();
	virtual unsigned int FirstGlyphInClass(bool * pfMoreThanOne);
	virtual void AssignGlyphIDsToClassMember(GrcFont *, utf16 wGlyphIDLim,
		std::map<utf16, utf16> & hmActualForPseudo,
		bool fLookUpPseudo = true);
	virtual void AssignGlyphAttrsToClassMembers(GrcGlyphAttrMatrix * pgax,
		GdlRenderer * prndr, GrcLigComponentList * plclist,
		Vector<GdlGlyphAttrSetting *> & vpglfaAttrs);
	virtual void CheckExistenceOfGlyphAttr(GdlObject * pgdlAvsOrExp,
		GrcSymbolTable * psymtbl, GrcGlyphAttrMatrix * pgax, Symbol psymGlyphAttr);
	virtual void CheckCompleteAttachmentPoint(GdlObject * pgdlAvsOrExp,
		GrcSymbolTable * psymtbl, GrcGlyphAttrMatrix * pgax, Symbol psymGlyphAttr,
		bool * pfXY, bool * pfGpoint);
	virtual void CheckCompBox(GdlObject * pritset,
		GrcSymbolTable * psymtbl, GrcGlyphAttrMatrix * pgax, Symbol psymCompRef);
	virtual void StorePseudoToActualAsGlyphAttr(GrcGlyphAttrMatrix * pgax, int nAttrID,
		std::vector<GdlExpression *> & vpexpExtra);

	static std::string GlyphIDString(utf16 wGlyphID)
	{
		char rgch[20];
		itoa(int(wGlyphID), rgch, 16);
		std::string staNum(rgch);
		std::string staRet;
		for (int ich = staNum.size(); ich < 4; ich++)
			staRet += "0";
		staRet += staNum;
		return staRet;
	}

	static std::string CodepointIDString(int n)
	{
		char rgch[20];
		itoa(int(n), rgch, 16);
		std::string staNum(rgch);
		std::string staRet;
		for (int ich = staNum.size(); ich < 4; ich++)
			staRet += "0";
		staRet += staNum;
		return staRet;
	}

	virtual bool IncludesGlyph(utf16 w)
	{
		for (int iw = 0; iw < m_vwGlyphIDs.Size(); iw++)
		{
			if (m_vwGlyphIDs[iw] == w)
				return true;
		}
		return false;
	}

	virtual bool HasOverlapWith(GdlGlyphClassMember * glfd, GrcFont * pfont);

	virtual bool HasBadGlyph()
	{
		for (int iw = 0; iw < m_vwGlyphIDs.Size(); iw++)
		{
			if (m_vwGlyphIDs[iw] == kBadGlyph)
				return true;
		}
		return false;
	}

	virtual bool WarnAboutBadGlyphs(bool fTop);
	virtual bool DeleteBadGlyphs();

public:
	//	Compiler:
	virtual void RecordInclusionInClass(GdlPass * ppass, GdlGlyphClassDefn * pglfc);
	virtual void GetMachineClasses(FsmMachineClass ** ppfsmcAssignments,
		FsmMachineClassSet & setpfsmc);

	//	Output:
	void AddGlyphsToUnsortedList(std::vector<utf16> & vwGlyphs);
	void AddGlyphsToSortedList(std::vector<utf16> & vwGlyphs, std::vector<int> & vnIndices);

	//	debugger
	virtual void DebugCmapForMember(GrcFont * pfont,
		utf16 * rgchwUniToGlyphID, unsigned int * rgnGlyphIDToUni);


protected:
	//	Instance variables:
	GlyphType m_glft;			// unicode, glyphID, codepoint, postscript, pseudo

	unsigned int m_nFirst;
	unsigned int m_nLast;
	utf16 m_wCodePage;
	unsigned int m_nUnicodeInput;	// input for pseudo-glyph
	GdlGlyphDefn * m_pglfOutput;	// pseudo-glyph output
	std::string	m_sta;				// postscript name or codepoint string

	//	for compiler use:
	Vector<utf16> m_vwGlyphIDs;	// equivalent glyph IDs
	utf16 m_wPseudo;			// glyph id assigned to pseudo-glyph

	bool m_fGAResolved;			// temporary use: glyph attributes resolved

	bool m_fNoRangeCheck;		// true for pseudo definitions generated by the compiler

	// This isn't an adequate comparison, makes too many assumptions.
	//bool PseudoLessThan(const GdlGlyphDefn * pglf) const
	//{
	//	if (this->m_glft == kglftPseudo && pglf->m_glft == kglftPseudo)
	//	{
	//		if (this->m_nUnicodeInput != pglf->m_nUnicodeInput)
	//		{
	//			return (this->m_nUnicodeInput < pglf->m_nUnicodeInput);
	//		}
	//		if (this->m_pglfOutput && this->m_pglfOutput)
	//		{
	//			return (strcmp(this->m_pglfOutput->m_sta.data(), pglf->m_pglfOutput->m_sta.data()) < 0);
	//		}
	//	}
	//	// Comparing memory addresses is not ideal for the regression test, but it will
	//	// work:
	//	return (this < pglf);
	//}
};


// Functor class for set manipulation
//class PseudoLess
//{
//	friend class GdlGlyphDefn;
//public:
//	operator()(const GdlGlyphDefn * pglf1, const GdlGlyphDefn * pglf2) const
//	{
//		return (pglf1->PseudoLessThan(pglf2));
//	}
//};

// defined in GdlGlyphClassDefn.h:
//typedef std::set<GdlGlyphDefn *> PseudoSet;

#endif // GLYPH_INCLUDED
