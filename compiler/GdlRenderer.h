/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GdlRenderer.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    GdlRenderer is the top-level object corresponding to a rendering behavior description in
	a single GDL file.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef RENDERER_INCLUDED
#define RENDERER_INCLUDED

/*----------------------------------------------------------------------------------------------
Class: GdlRenderer
Description: Top-level object; there is only one instance of this class per compile.
Hungarian: rndr
----------------------------------------------------------------------------------------------*/

class GrcTable;

class GdlGlyphClassDefn;
class GdlFeatureDefn;

class GdlRenderer : public GdlObject
{
public:
	//	Constructor & destructor:
	GdlRenderer();
	~GdlRenderer();

	//	General:
	bool AutoPseudo()			{ return m_fAutoPseudo; }
	void SetAutoPseudo(bool f)	{ m_fAutoPseudo = f; }

	bool Bidi()					{ return (m_nBidi > 0); } // -1 = false (default)
	int RawBidi()				{ return m_nBidi; }
	void SetBidi(int n)			{ m_nBidi = n; }

	int ScriptDirections()		{ return m_grfsdc; }
	void AddScriptDirection(int fsdc)
	{
		m_grfsdc |= fsdc;
	}
	bool ClearScriptDirections()
	{
		bool fRet = (m_grfsdc != kfsdcNone);
		m_grfsdc = kfsdcNone;
		return fRet;
	}

	void AddScriptTag(int n)
	{
		for (size_t i = 0; i < m_vnScriptTags.size(); i++)
		{
			if (m_vnScriptTags[i] == n)
				return;
		}
		m_vnScriptTags.push_back(n);
	}
	bool ClearScriptTags()
	{
		bool fRet = (m_vnScriptTags.size() > 0);
		m_vnScriptTags.clear();
		return fRet;
	}

	int NumScriptTags()
	{
		return m_vnScriptTags.size();
	}
	int ScriptTag(int i)
	{
		Assert(i < static_cast<int>(m_vnScriptTags.size()));
		return m_vnScriptTags[i];
	}

	GdlNumericExpression * ExtraAscent()				{ return m_pexpXAscent; }
	void SetExtraAscent(GdlNumericExpression * pexp)	{ m_pexpXAscent = pexp; }

	GdlNumericExpression * ExtraDescent()				{ return m_pexpXDescent; }
	void SetExtraDescent(GdlNumericExpression * pexp)	{ m_pexpXDescent = pexp; }

	void AddGlyphClass(GdlGlyphClassDefn * pglfc)
	{
		m_vpglfc.push_back(pglfc);
	}
	void AddFeature(GdlFeatureDefn * pfeat)
	{
		m_vpfeat.push_back(pfeat);
	}
	bool AddLanguage(GdlLanguageDefn * plang);

	NameDefnMap & NameAssignmentsMap()
	{
		return m_hmNameDefns;
	}

	//	Parser:
	GdlRuleTable * GetRuleTable(GrpLineAndFile & lnf, std::string staTableName);
	GdlRuleTable * FindRuleTable(std::string staTableName);
	GdlRuleTable * FindRuleTable(Symbol psymTableName);

	//	Post-parser:
	bool ReplaceAliases();
	bool HandleOptionalItems();
	bool CheckSelectors();

	//	Pre-compiler:
	bool PreCompileFeatures(GrcManager * pcman, GrcFont * pfont, int * pfxdFeatVersion);
	void CheckLanguageFeatureSize();
	int ExplicitPseudos(PseudoSet & setpglf);
	int ActualForPseudo(utf16 wPseudo);
	bool AssignGlyphIDs(GrcFont *, utf16 wGlyphIDLim,
		std::map<utf16, utf16> & hmActualForPseudos);
	void AssignGlyphAttrsToClassMembers(GrcGlyphAttrMatrix * pgax,
		GrcLigComponentList * plclist);
	void AssignGlyphAttrDefaultValues(GrcFont * pfont,
		GrcGlyphAttrMatrix * pgax, int cwGlyphs,
		std::vector<Symbol> & vpsymSysDefined, std::vector<int> & vnSysDefValues,
		std::vector<GdlExpression *> & vpexpExtra,
		std::vector<Symbol> & vpsymGlyphAttrs);
	DirCode ConvertBidiCode(UCharDirection diricu, utf16 wUnicode);
	void StorePseudoToActualAsGlyphAttr(GrcGlyphAttrMatrix * pgax, int nAttrID,
		std::vector<GdlExpression *> & vpexpExtra);

	bool FixRulePreContexts(Symbol psymAnyClass);
	bool FixGlyphAttrsInRules(GrcManager * pcman, GrcFont * pfont);
	bool CheckTablesAndPasses(GrcManager * pcman, int * pcpassValid);
	void MarkReplacementClasses(GrcManager * pcman,
		ReplacementClassSet & setpglfc);
	void DeleteAllBadGlyphs();
	bool CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont);
	bool CheckLBsInRules();
	void ReplaceKern(GrcManager * pcman);
	void MaxJustificationLevel(int * pnLevel);
	bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded,
		bool * pfFixPassConstraints);
	void MovePassConstraintsToRules(int fxdSilfVersion);

	void SetNumUserDefn(int c);
	//{
	//	m_cnUserDefn = max(m_cnUserDefn, c+1);
	//}
	int NumUserDefn()
	{
		return m_cnUserDefn;
	}

	void SetNumLigComponents(int c);
	//{
	//	m_cnComponents = max(m_cnComponents, c);
	//}
	int NumLigComponents()
	{
		return m_cnComponents;
	}

	//	Compiler:
	void GenerateFsms(GrcManager * pcman);
	void CalculateContextOffsets();

	int LineBreakFlags()
	{
		// Bit 0: ON means there is at least one rule that uses line-end contextuals.
		int nBitmap = (int)(m_fLineBreak);

		// Bit 1: ON means NO line-breaks occur before the justification pass; ie, it is safe
		// to optimize by not rerunning the substitution pass.
		// OFF means the entire substitution pass must be rerun to handle line-end contextuals.
		if (m_fLineBreak && !m_fLineBreakB4Just)
			nBitmap += 2;	// ie, safe to optimize

		return nBitmap;
	}
	int PreXlbContext()
	{
		return m_critPreXlbContext;
	}
	int PostXlbContext()
	{
		return m_critPostXlbContext;
	}
	//	debuggers:
	void DebugEngineCode(GrcManager * pcman, std::ostream & strmOut);
	void DebugRulePrecedence(GrcManager * pcman, std::ostream & strmOut);
	void DebugFsm(GrcManager * pcman, std::ostream & strmOut);
	void DebugCmap(GrcFont * pfont, utf16 * rgchwUniToGlyphID, unsigned int * rgnGlyphIDToUni);
	void DebugClasses(std::ostream & strmOut,
		std::vector<GdlGlyphClassDefn *> & vpglfcReplcmt, int cpglfcLinear);
	void DebugXmlClasses(std::ofstream & strmOut);
	void DebugXmlFeatures(std::ofstream & strmOut);
	void DebugXmlRules(GrcManager * pcman, std::ofstream & strmOut);
	void RecordSingleMemberClasses(std::vector<std::string> & vstaSingleMemberClasses);

	//	Output:
	void OutputReplacementClasses(int fxdSilfVersion,
		std::vector<GdlGlyphClassDefn *> & vpglfc, int cpglfcLinear,
		GrcBinaryStream * pbstrm);
	void CountPasses(int * pcpass, int * pcpassLB, int * pcpassSub,
		int * pcpassJust, int * pcpassPos, int * pipassBidi);
	void OutputPasses(GrcManager * pcman, GrcBinaryStream * pbstrm, long lTableStart,
		std::vector<int> & vnOffsets);
	bool AssignFeatTableNameIds(utf16 wFirstNameId, utf16 wNameIdMinNew,
		std::vector<std::wstring> & vstuExtNames, std::vector<utf16> & vwLangIds, 
		std::vector<utf16> & vwNameTblIds, size_t & cchwStringData,
		uint8 * pNameTbl, std::vector<GdlFeatureDefn *> & vpfeatInput);
	void OutputFeatTable(GrcBinaryStream * pbstrm, long lTableStart, int fxdVersion);
	void OutputSillTable(GrcBinaryStream * pbstrm, long lTableStart);

protected:
	//	Instance variables:

	std::vector<GdlRuleTable *>			m_vprultbl;

	std::vector<GdlGlyphClassDefn *>	m_vpglfc;
	std::vector<GdlFeatureDefn *>		m_vpfeat;
	std::vector<GdlLanguageDefn *>		m_vplang;
	NameDefnMap						m_hmNameDefns;
//	GdlStdStyle						m_rgsty[FeatureDefn::kstvLim];

	bool m_fAutoPseudo;
	int m_nBidi;		// boolean; -1 if not set
	int m_iPassBidi;
	int m_grfsdc;		// supported script directions
	std::vector<int>	m_vnScriptTags;
	GdlNumericExpression * m_pexpXAscent;
	GdlNumericExpression * m_pexpXDescent;
	//	true if any line-breaks are relevant to rendering:
	bool m_fLineBreak;
	//	true if any line-breaks are relevant to rendering before the justification table:
	bool m_fLineBreakB4Just;
	//	limits on cross-line-boundary contextualization:
	int m_critPreXlbContext;
	int m_critPostXlbContext;

	int m_cnUserDefn;	// number of user-defined slot attributes
	int m_cnComponents;	// max number of components per ligature

	enum { kInfiniteXlbContext = 255 };
};


/*----------------------------------------------------------------------------------------------
Class: GdlStdStyle
Description: Standard style information; each styl in the list corresponds to a separate font
	file; eg, bold, italic, bold-italic.
Hungarian: sty
----------------------------------------------------------------------------------------------*/
class GdlStdStyle : public GdlObject
{
protected:
	//	instance variables:
	int m_stvSetting;	// feature setting value (which also is this item's index
						// in the m_rgsty array)
	int	m_nInternalID;	// the index into the array of glyph attr values
	std::string	m_staFontName;	// name of the font
};


#endif // RENDERER_INCLUDED
