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
	int RawBidi()				{ return m_nBidi; }		// false, true, -1=not set, 2=full bidi pass
	void SetBidi(int n)			{ m_nBidi = n; }

	bool HasFlippedPass()		{ return m_fHasFlippedPass; }
	void SetHasFlippedPass(bool f)	{ m_fHasFlippedPass = f; }

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

	size_t NumScriptTags()
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
	// currently not used:
	void RemoveGlyphClass(GdlGlyphClassDefn * pglfc) // assumes it is only present once
	{
		for (size_t iglfc = 0; iglfc < m_vpglfc.size(); iglfc++)
		{
			if (m_vpglfc[iglfc] == pglfc)
			{
				m_vpglfc.erase(m_vpglfc.begin() + iglfc);
				break;
			}
		}
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
	bool PreCompileFeatures(GrcManager * pcman, GrcFont * pfont, uint32_t * pfxdFeatVersion);
	void CheckLanguageFeatureSize();
	bool CheckRecursiveGlyphClasses();
	size_t ExplicitPseudos(PseudoSet & setpglf);
	int ActualForPseudo(utf16 wPseudo);
	bool AssignGlyphIDs(GrcFont *, gid16 wGlyphIDLim,
		std::map<utf16, utf16> & hmActualForPseudos);
	void AssignGlyphAttrsToClassMembers(GrcGlyphAttrMatrix * pgax,
		GrcLigComponentList * plclist);
	void AssignGlyphAttrDefaultValues(GrcFont * pfont,
		GrcGlyphAttrMatrix * pgax, size_t cwGlyphs,
		std::vector<Symbol> & vpsymSysDefined, std::vector<int> & vnSysDefValues,
		std::vector<GdlExpression *> & vpexpExtra,
		std::vector<Symbol> & vpsymGlyphAttrs);
	DirCode DefaultDirCode(int nUnicode, bool * pfInitFailed);
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
	void RewriteSlotAttrAssignments(GrcManager * pcman, GrcFont * pfont);
	void MaxJustificationLevel(int * pnLevel);
	bool HasCollisionPass();
	bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded,
		bool * pfFixPassConstraints);
	void MovePassConstraintsToRules(int fxdSilfVersion);
	void CalculateSpaceContextuals(GrcFont * pfont);
	size_t NumberOfPasses()
	{
		size_t cpass, cpassLB, cpassSub, cpassPos, cpassJust;
		int ipassBidi;
		CountPasses(cpass, cpassLB, cpassSub, cpassJust, cpassPos, ipassBidi);
		return cpass;
	}

	void SetNumUserDefn(size_t c);
	size_t NumUserDefn()
	{
		return m_cnUserDefn;
	}

	void SetNumLigComponents(size_t c);
	size_t NumLigComponents()
	{
		return m_cnComponents;
	}

	//	Compiler:
	void PassOptimizations(GrcGlyphAttrMatrix * pgax, GrcSymbolTable * psymtbl, unsigned int nAttrIdSkipP);
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
	int SpaceContextualFlags()	// shifted for putting in flag byte
	{
		return ((int)(m_spcon)) << 2;
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
	void DebugXmlClasses(std::ofstream & strmOut, std::string staPathToCur);
	void DebugXmlFeatures(std::ofstream & strmOut, std::string staPathToCur);
	void DebugXmlRules(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur);
	void RecordSingleMemberClasses(std::vector<std::string> & vstaSingleMemberClasses,
		std::vector<std::string> & vstaFiles, std::vector<int> & vnLines, std::string staPathToCur);

	//	Output:
	void OutputReplacementClasses(int fxdSilfVersion,
		std::vector<GdlGlyphClassDefn *> & vpglfc, size_t cpglfcLinear,
		GrcBinaryStream * pbstrm);
	void CountPasses(size_t & pcpass, size_t & pcpassLB, size_t & pcpassSub,
		size_t & pcpassJust, size_t & pcpassPos, int & pipassBidi);
	void OutputPasses(GrcManager * pcman, GrcBinaryStream * pbstrm, offset_t lTableStart,
		std::vector<offset_t> & vnOffsets);
	bool AssignFeatTableNameIds(utf16 wFirstNameId, utf16 wNameIdMinNew,
		std::vector<std::wstring> & vstuExtNames, std::vector<utf16> & vwLangIds, 
		std::vector<utf16> & vwNameTblIds, size_t & cchwStringData,
		uint8 * pNameTbl, std::vector<GdlFeatureDefn *> & vpfeatInput);
	void OutputFeatTable(GrcBinaryStream * pbstrm, offset_t lTableStart, int fxdVersion);
	void OutputSillTable(GrcBinaryStream * pbstrm, offset_t lTableStart);

protected:
	//	Instance variables:

	std::vector<GdlRuleTable *>			m_vprultbl;

	std::vector<GdlGlyphClassDefn *>	m_vpglfc;
	std::vector<GdlFeatureDefn *>		m_vpfeat;
	std::vector<GdlLanguageDefn *>		m_vplang;
	NameDefnMap						m_hmNameDefns;
//	GdlStdStyle						m_rgsty[FeatureDefn::kstvLim];

	bool m_fAutoPseudo;
	int m_nBidi;		// boolean or 2 = explict pass; -1 if not set
	int m_ipassBidi;
	int m_fHasFlippedPass;
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
	//	space contextuals:
	SpaceContextuals m_spcon;

	size_t m_cnUserDefn;	// number of user-defined slot attributes
	size_t m_cnComponents;	// max number of components per ligature

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
