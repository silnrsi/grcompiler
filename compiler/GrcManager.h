/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GrcManager.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    The main object that manages the compliation process.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef GRC_MANAGER_INCLUDED
#define GRC_MANAGER_INCLUDED

class GdlFeatureDefn;
class GdlGlyphClassDefn;
class GdlNameDefn;


/*----------------------------------------------------------------------------------------------
Used for outputting name table.
----------------------------------------------------------------------------------------------*/
struct PlatEncChange
{
	size_t cbBytesPerChar;	// 1 = 8-bit, 2 = 16-bit
	uint16 platformID;
	uint16 encodingID;
	uint16 engLangID;
	bool fChangeName;
	utf16 * pchwFullName;
	utf16 * pchwUniqueName;
	utf16 * pchwPostscriptName;
	size_t cchwFullName;
	size_t cchwUniqueName;
	size_t cchwPostscriptName;
};


//	Processing passes in the parser where we process and assign IDs to glyph attributes:
enum GlyphAttrProcessPasses
{
	kgappBuiltIn	= 0,
	kgappCompBase	= 1,
	kgappCompBox	= 2,
	kgappJustify	= 3,
	kgappOther		= 4
};


//  Compresssion schemes
enum TableCompressor
{
    ktcNone = 0,
    ktcLZ4 = 1
};


/*----------------------------------------------------------------------------------------------
Class: GrcManager
Description: The object that manages the complication process. There is one global instance.
Hungarian: cman
----------------------------------------------------------------------------------------------*/
class GrcManager
{
	typedef std::map<std::string, Symbol> SymbolTableMap;

public:
	//	Constructor & destructor:
	GrcManager();
	~GrcManager();
protected:
	void Init();
	void Clear();
public:
	void ClearFsmWorkSpace();

public:
	//	General:
	GdlRenderer * Renderer()				{ return m_prndr; }
	GrcSymbolTable * SymbolTable()			{ return m_psymtbl; }
	std::vector<Symbol> * GlyphAttrVec()	{ return &m_vpsymGlyphAttrs; }
	GrcGlyphAttrMatrix * GlyphAttrMatrix()	{ return m_pgax; }
	GrcLigComponentList * LigCompList()		{ return m_plclist; }
	int NumGlyphs()							{ return m_cwGlyphIDs; }
	utf16 PhantomGlyph()					{ return m_wPhantom; }

	int SilfTableVersion()
	{
		return m_fxdSilfTableVersion;
	}
	void SetSilfTableVersion(int fxd, bool fUserSpec)
	{
		m_fxdSilfTableVersion = fxd;
		m_fUserSpecifiedVersion = fUserSpec;
	}
	void FixSilfTableVersion(int fxd)
	{
		m_fxdSilfTableVersion = fxd;
	}
	int MaxSilfVersion()
	{
		// Highest version of the Silf table this version of the compiler can generate:
		return kfxdMaxSilfVersion;
	}

	int DefaultSilfVersion()
	{
		return 0x00020000;
	}
	bool UserSpecifiedVersion()
	{
		return m_fUserSpecifiedVersion;
	}

	void SetTableVersion(int ti, int fxdVersion)
	{
		switch (ti)
		{
		case ktiSilf:	m_fxdSilfTableVersion = fxdVersion; break;
		case ktiGloc:	m_fxdGlocTableVersion = fxdVersion;	break;
		case ktiGlat:	m_fxdGlatTableVersion = fxdVersion;	break;
		case ktiFeat:	m_fxdFeatTableVersion = fxdVersion;	break;
		case ktiSill:	m_fxdSillTableVersion = fxdVersion;	break;
		default:
			break;
		}
	}
	int TableVersion(int ti)
	{
		switch (ti)
		{
		case ktiSilf:	return m_fxdSilfTableVersion;
		case ktiGloc:	return m_fxdGlocTableVersion;
		case ktiGlat:	return m_fxdGlatTableVersion;
		case ktiFeat:	return m_fxdFeatTableVersion;
		case ktiSill:	return m_fxdSillTableVersion;
		default:		return 0;
		}
	}

	int CompilerVersion()
	{
		return m_fxdCompilerVersion;
	}
	// See documentation at the beginning of the main.cpp file.
	void SetCompilerVersionFor(int fxdSilfVersion)
	{
		switch (fxdSilfVersion)
		{
		case 0x00010000:	m_fxdCompilerVersion = 0x00010000;		break;
		case 0x00020000:	m_fxdCompilerVersion = 0x00020000;		break;
		case 0x00020001:	m_fxdCompilerVersion = 0x00040001;		break;
		case 0x00030000:	m_fxdCompilerVersion = 0x00030000;		break;
		case 0x00030001:	m_fxdCompilerVersion = 0x00040000;		break;
		case 0x00030002:	m_fxdCompilerVersion = 0x00040001;		break;
		case 0x00040000:	m_fxdCompilerVersion = 0x00040002;		break;
		case 0x00040001:	m_fxdCompilerVersion = 0x00050000;		break;
		case 0x00050000:	m_fxdCompilerVersion = 0x00050000;		break;
		default:			m_fxdCompilerVersion = 0x00FF0000;		break;	// unknown
		}
	}

	TableCompressor Compressor() const
	{
	    return m_tcCompressor;
	}
	void SetCompressor(TableCompressor tc)
	{
	    m_tcCompressor = tc;
	}

	int VersionForTable(int ti);
	int VersionForTable(int ti, int fxdRequestedVersion);
	int VersionForRules();
	int CalculateSilfVersion(int fxdSpecVersion);

	void SetNameTableStart(int n)
	{
		m_nNameTblStart = n;
	}
	int NameTableStart()
	{
		 return m_nNameTblStart;
	}
	int NameTableStartMin()
	{
		return 256;
	}

	int NumJustLevels();

	//	environment getters & setters
	Symbol Table()				{ return m_venv.back().Table(); }
	int Pass()					{ return m_venv.back().Pass(); }
	int MUnits()				{ return m_venv.back().MUnits(); }
	int PointRadius()			{ return m_venv.back().PointRadius(); }
	int PointRadiusUnits()		{ return m_venv.back().PointRadiusUnits(); }
	int MaxRuleLoop()			{ return m_venv.back().MaxRuleLoop(); }
	int MaxBackup()				{ return m_venv.back().MaxBackup(); }
	bool AttrOverride()			{ return m_venv.back().AttrOverride(); }
	utf16 CodePage()			{ return m_venv.back().CodePage(); }

	void SetTable(Symbol psym)			{ m_venv.back().SetTable(psym); }
	void SetPass(int n)					{ m_venv.back().SetPass(n); }
	void SetMUnits(int m)				{ m_venv.back().SetMUnits(m); }
	void SetPointRadius(int n, int m)	{ m_venv.back().SetPointRadius(n, m); }
	void SetMaxRuleLoop(int n)			{ m_venv.back().SetMaxRuleLoop(n); }
	void SetMaxBackup(int n)			{ m_venv.back().SetMaxBackup(n); }
	void SetAttrOverride(bool f)		{ m_venv.back().SetAttrOverride(f); }
	void SetCodePage(utf16 w)			{ m_venv.back().SetCodePage(w); }

	GdlRuleTable * RuleTable(GrpLineAndFile & lnf)
	{
		return m_prndr->GetRuleTable(lnf, Table()->FieldAt(0));
	}

	bool OutputDebugFiles()				{ return m_fOutputDebugFiles; }
	bool OutputDebugXml()				{ return m_fOutputDebugXml; }
	void SetOutputDebugFiles(bool fXml, bool fOther)
	{
		m_fOutputDebugXml = fXml;
		m_fOutputDebugFiles = fOther;
	}
	bool IgnoreBadGlyphs()				{ return m_fIgnoreBadGlyphs; }
	void SetIgnoreBadGlyphs(bool f)		{ m_fIgnoreBadGlyphs = f; }
	bool IsVerbose()					{ return m_fVerbose; }
	void SetVerbose(bool f) 			{ m_fVerbose = f; }
	int SeparateControlFile()			{ return m_fSepCtrlFile; }
	void SetSeparateControlFile(bool f)	{ m_fSepCtrlFile = f; }
	bool IncludePassOptimizations()		{ return m_fPassOptimizations; }
	void SetPassOptimizations(bool f)	{ m_fPassOptimizations = f; }
	bool OffsetAttrs()					{ return m_fOffsetAttrs; }
	void SetOffsetAttrs(bool f)			{ m_fOffsetAttrs = f; }

public:
	//	Parser:
	bool Parse(std::string staFileName, std::string staGdlppFile, std::string staOutputPath);
protected:
	bool RunPreProcessor(std::string staFileName, std::string * staFilePreProc,
		std::string & staGdlppFile, std::string & staOutputPath);
	void RecordPreProcessorErrors(FILE * pFilePreProcErr);
	std::string PreProcName(std::string sta);
	bool ParseFile(std::ifstream & strmIn, std::string staFileName);
	void InitPreDefined();
	void WalkParseTree(RefAST ast);
	void WalkTopTree(RefAST ast);
	void WalkEnvTree(RefAST ast, TableType tblt, GdlRuleTable *, GdlPass *);
	void WalkDirectivesTree(RefAST ast, int * pnCollisionFix = NULL, int * pnAutoKern = NULL,
		int * pnCollisionThreshold = NULL, bool * pfPostBidi = NULL);
	void WalkTableTree(RefAST ast);
	void WalkTableElement(RefAST ast, TableType tblt, GdlRuleTable * prultbl, GdlPass * ppass);
	void WalkGlyphTableTree(RefAST ast);
	void WalkGlyphTableElement(RefAST ast);
	void WalkGlyphClassTree(RefAST ast, GdlGlyphClassDefn * pglfc, GlyphClassType glfct);
	void WalkGlyphAttrTree(RefAST ast, std::vector<std::string> & vsta);
	void WalkFeatureTableTree(RefAST ast);
	void WalkFeatureTableElement(RefAST ast);
	void WalkFeatureSettingsTree(RefAST ast, std::vector<std::string> & vsta);
	void WalkLanguageTableTree(RefAST ast);
	void WalkLanguageTableElement(RefAST ast);
	void WalkLanguageItem(RefAST ast, GdlLangClass * plcls);
	void WalkLanguageCodeList(RefAST astList, GdlLangClass * plcls);
	void WalkNameTableTree(RefAST ast);
	void WalkNameTableElement(RefAST ast);
	void WalkNameIDTree(RefAST ast, std::vector<std::string> & vsta);
	void WalkRuleTableTree(RefAST ast, int nodetyp);
	void WalkPassTree(RefAST ast, GdlRuleTable * prultbl, GdlPass * ppassPrev);
	void WalkIfTree(RefAST astContents, GdlRuleTable *, GdlPass *);
	bool AllContentsArePasses(RefAST ast);
	void WalkRuleTree(RefAST ast, GdlRuleTable * prultbl, GdlPass * ppass);
	void WalkSlotAttrTree(RefAST ast, GdlRuleItem * prit, std::vector<std::string> & vsta);
	GdlExpression * WalkExpressionTree(RefAST ast);

	void ProcessGlobalSetting(RefAST);
	void ProcessGlyphClassMember(RefAST ast, GdlGlyphClassDefn * pglfc, GlyphClassType glfct,
		GdlGlyphDefn ** ppglfRet);
	GdlGlyphDefn * ProcessGlyph(RefAST astGlyph, GlyphType glft, int nCodePage = -1);
	void ProcessFunction(RefAST ast, std::vector<std::string> & vsta,
		bool fSlotAttr, GdlRuleItem * prit = NULL, Symbol psymOp = NULL);
	void ProcessFunctionArg(bool fSlotAttr, GrcStructName const& xns,
		int nPR, int mPRUnits, bool fOverride, GrpLineAndFile const& lnf,
		ExpressionType expt, GdlRuleItem * prit, Symbol psymOp, GdlExpression * pexpValue);
	void BadFunctionError(GrpLineAndFile & lnf, std::string staFunction,
		std::string staArgsExpected);
	void ProcessItemRange(RefAST astItem, GdlRuleTable * prultbl, GdlPass * ppass,
		GdlRule * prule, int * pirit, int lrc, bool fHasLhs);
	void ProcessRuleItem(RefAST astItem, GdlRuleTable * prultbl, GdlPass * ppass,
		GdlRule * prule, int * pirit, int lrc, bool fHasLhs);
	std::string ProcessClassList(RefAST ast, RefAST * pastNext);
	std::string ProcessAnonymousClass(RefAST ast, RefAST * pastNext);
	void ProcessSlotIndicator(RefAST ast, GdlAlias * palias);
	void ProcessAssociations(RefAST ast, GdlRuleTable * prultbl, GdlRuleItem * prit, int lrc);
	GdlGlyphClassDefn * ConvertClassToIntersection(Symbol psymClass, GdlGlyphClassDefn * pglfc,
		GrpLineAndFile & lnf);
	GdlGlyphClassDefn * ConvertClassToDifference(Symbol psymClass, GdlGlyphClassDefn * pglfc,
		GrpLineAndFile & lnf);

	GrpLineAndFile LineAndFile(RefAST);
	int NumericValue(RefAST);
	int NumericValue(RefAST, bool * pfM);
	Symbol IdentifierSymbol(RefAST ast, std::vector<std::string> & vsta, bool * pfGlyphAttr);
	bool ClassPredefGlyphAttr(std::vector<std::string> & vsta, ExpressionType * pexpt, SymbolType * psymt);
public:	// so they can be called by the test procedures
	GrcEnv * PushTableEnv(GrpLineAndFile &, std::string staTableName);
	GrcEnv * PushPassEnv(GrpLineAndFile &, int nPass);
	GrcEnv * PushGeneralEnv(GrpLineAndFile &);
	GrcEnv * PopEnv(GrpLineAndFile &, std::string staStmt);
protected:
	GrcEnv * PushEnvAux();
public:	// so they can be called by the test procedures
	GdlGlyphClassDefn * AddGlyphClass(GrpLineAndFile const&, std::string staClassName);
	GdlGlyphClassDefn * AddAnonymousClass(GrpLineAndFile const&);
protected:
	void AddGlyphToClass(GdlGlyphClassDefn * pglfc, GdlGlyphClassMember * pglfd);

	//	debuggers:
	void DebugParseTree(RefAST);

public:
	//	Post-parser:
	bool PostParse();
protected:
	void ProcessMasterTables();

public:
	//	Pre-compiler:
	bool PreCompile(GrcFont *);
	bool Compile(GrcFont *);

protected:
	bool PreCompileFeatures(GrcFont *);
	bool PreCompileClassesAndGlyphs(GrcFont *);
	bool PreCompileRules(GrcFont *);
	bool PreCompileLanguages(GrcFont * pfont);

	bool GeneratePseudoGlyphs(GrcFont *);
	utf16 FirstFreeGlyph(GrcFont *);
	void CreateAutoPseudoGlyphDefn(utf16 wAssigned, int nUnicode, utf16 wGlyphID);
	void SortPseudoMappings();

	bool AddAllGlyphsToTheAnyClass(GrcFont * pfont, std::map<utf16, utf16> & hmActualForPseudo);

	bool MaxJustificationLevel(int * pnJLevel);
	bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded,
		bool * pfFixPassConstraints);

	bool AssignInternalGlyphAttrIDs();
	void CalculateCollisionOctaboxes(GrcFont * pfont);

	bool AssignGlyphAttrsToClassMembers(GrcFont * pfont);
	bool ProcessGlyphAttributes(GrcFont * pfont);
	void ConvertBetweenXYAndGpoint(GrcFont * pfont, utf16 wGlyphID);
	bool FinalGlyphAttrResolution(GrcFont * pfont);
	void MinAndMaxGlyphAttrValues(int nAttrID,
		int cJLevels, int nAttrIdJStr, int nAttrIdJShr, int nAttrIdJStep, int nAttrIdJWeight,
		int nAttrIdSkipPasses,
		int * pnMin, int * pnMax);
	bool StorePseudoToActualAsGlyphAttr();
	bool CheckForEmptyClasses();

public:
	int PseudoForUnicode(int nUnicode);
	int ActualForPseudo(utf16 wPseudo);
	utf16 LbGlyphId()
	{
		return m_wLineBreak;
	}
	void StoreModifiedExpression(GdlExpression * pexp)	// store an extra expression so it can be deleted later
	{
		m_vpexpModified.push_back(pexp);
	}

protected:
	bool AssignClassInternalIDs();
	void DetermineTableVersion();
public:
	void AddToFsmClasses(GdlGlyphClassDefn * pglfc, int nPassID);
protected:

public:
	//	Compiler:
	int SlotAttributeIndex(Symbol psym);
	void GenerateFsms();
	////void InitializeFsmArrays();
	std::vector<GdlGlyphClassDefn *> * FsmClassesForPass(int nPassID);
	void CalculateContextOffsets();
	void CalculateGlatVersion();
	void PassOptimizations();

	//	Output:
	bool AssignFeatTableNameIds(utf16 wFirstNameId, utf16 wNameIdMinNew,
		std::vector<std::wstring> & vstuExtNames, std::vector<utf16> & vwLangIds,
		std::vector<utf16> & vwNameTblIds,
		size_t & cchwStringData, uint8 * pNameTbl);
	int OutputToFont(char * pchSrcFileName, char * pchDstFileName,
		utf16 * pchDstFontFamily, bool fModFontName, utf16 * pchSrcFontFamily);
	int FinalAttrValue(utf16 wGlyphID, int nAttrID);
	void ConvertBwForVersion(int wGlyphId, int nAttrIdBw);
	void SplitLargeStretchValue(int wGlyphId, int nAttrIdJStr);
protected:
	bool AddFeatsModFamily(uint16 * pchFamilyName, uint8 ** ppNameTbl, uint32 * pcbNameTbl);
	void BuildDateString(utf16 * stuDate);
	bool FindNameTblEntries(void * pNameTblRecord, int cNameTblRecords, 
		uint16 suPlatformId, uint16 suEncodingId, uint16 suLangId, 
		int * piFamily, int * piSubFamily, int * piFullName,
		int * piVendor, int * piPSName, int * piUniqueName, int * piPrefFamily, int * piCompatibleFull);
	bool BuildFontNames(bool f8bit, uint16 * pchFamilyName, size_t cchwFamilyName, utf16 * stuDate,
		uint8 * pSubFamily, uint16 cbSubFamily,
		uint8 * pVendor, uint16 cbVendor,
		PlatEncChange *);
	bool AddFeatsModFamilyAux(uint8 * pTblOld, uint32 cbTblOld, uint8 * pTblNew, uint32 cbTblNew, 
		std::vector<std::wstring> & vstuExtNames, std::vector<uint16> & vnLangIds,
		std::vector<uint16> & vnNameTblIds, 
		uint16 * pchwFamilyName, uint16 cchwFamilyName, std::vector<PlatEncChange> & vpec,
		int nNameTblMinNew);
	bool OutputOS2Table(uint8 * pOs2TblSrc, uint32 cbOs2TblSrc,
		uint8 * pOs2TblMin, uint32 chbOs2TblMin, GrcBinaryStream * pbstrm, uint32 * pchSizeRet);
	bool OutputCmapTable(uint8 * pCmapTblSrc, uint32 cbCmapTblSrc,
		GrcBinaryStream * pbstrm, uint32 * pchSizeRet);
	int OutputCmap31Table(void * pCmapSubTblSrc, GrcBinaryStream * pbstrm, bool fFrom310,
		bool * pfNeed310);
	int OutputCmap310Table(void * pCmapSubTblSrc, GrcBinaryStream * pbstrm, bool fFrom31);
	void OutputSileTable(GrcBinaryStream * pbstrm,
		utf16 * pchStrFontFamily, char * pchSrcFileName, unsigned int luMasterChecksum,
		unsigned int * pnCreateTime, unsigned int * pnModifyTime,
		int * pibOffset, int * pcbSize);
	void OutputGlatAndGloc(GrcBinaryStream * pbstrm, int * pnGlocOffset, int * pnGlocSize,
		int * pnGlatOffset, int * pnGlatSize);
	int OutputGlatOctaboxes(GrcBinaryStream * pbstrm);
	void OutputSilfTable(GrcBinaryStream * pbstrm, int * pnSilfOffset, int * pnSilfSize);
	void OutputFeatTable(GrcBinaryStream * pbstrm, int * pnFeatOffset, int * pnFeatSize);
	void OutputSileTable(GrcBinaryStream * pbstrm, utf16 * pchwFontName, long nChecksum);
	void OutputSillTable(GrcBinaryStream * pbstrm, int * pnSillOffset, int * pnSillSize);

	void ReadSourceFontFeatures(std::ifstream & strmSrc, size_t iTableFeatSrc, size_t iTableFeatLen,
		size_t iTableNameSrc, size_t iTableNameLen);

	bool Compress(std::stringbuf & sb);

public:

	//	debuggers:
	void DebugEngineCode();
	void DebugRulePrecedence();
	void DebugGlyphAttributes();
	void DebugClasses();
	void DebugFsm();
	bool DebugXml(GrcFont * pfont, char * pchOutputFilename, bool fAbsGdlFilePaths);
	////void WalkFsmMachineClasses();
	void DebugOutput();
	void DebugCmap(GrcFont * pfont);
	void WriteCmapItem(std::ofstream & strmOut,
		unsigned int nUnicode, bool fSuppPlaneChars, utf16 wGlyphID, bool fUnicodeToGlyph,
		bool fPseudo, bool fInCmap);
	static void DebugHex(std::ostream & strmOut, utf16 wGlyphID);
	static void DebugUnicode(std::ostream & strmOut, int nUnicode, bool f32bit);
	static std::string ExpressionDebugString(ExpressionType expt);
protected:
	void DebugXmlGlyphs(GrcFont * pfont, std::ofstream & strmOut, std::string staPathToCur);
	void CmapAndInverse(GrcFont * pfont, 
		int cnUni, utf16 * rgnUniToGlyphID, unsigned int * rgnGlyphIDToUni,
		std::vector<unsigned int> & vnXUniForPsd, std::vector<utf16> & vwXPsdForUni);

	std::string pathFromOutputToCurrent(char * rgchCurDir, char * rgchOutputPath);
	char splitPath(char * rgchPath, std::vector<std::string> & vstaResult);

protected:
	//	Instance variables:

	//	The version of the Silf table to output.
	int m_fxdSilfTableVersion;
	//	The compiler version with which to mark the font. This might or might not be the
	//	actual current version of the compiler. If we are outputting a lower version of
	//	the tables than what this compiler can generate, we can put a lower compiler version
	//	in the font (and that way the font can be used with earlier engines).
	int m_fxdCompilerVersion;
	//	Did the user include a /v option?
	bool m_fUserSpecifiedVersion;

	//	Other table versions.
	int m_fxdGlocTableVersion;
	int m_fxdGlatTableVersion;
	int m_fxdFeatTableVersion;
	int m_fxdSillTableVersion;

	//	Are we creating a separate control file?
	bool m_fSepCtrlFile;

	//	Basic justification: true if no justify attributes are present
	bool m_fBasicJust;
	//	Highest justification level used
	int m_nMaxJLevel;
	//	Space contextual flags
	//////////////SpaceContextuals m_spcon;

	//	Where to start the feature names in the name table
	int m_nNameTblStart;

	//	Ignore nonexistent glyphs?
	bool m_fIgnoreBadGlyphs;

	//	The top-level object representing the GDL program
	GdlRenderer * m_prndr;

	GrcSymbolTable * m_psymtbl;

	//	Temporary structures used during parsing--master tables of glyph attribute
	//	and feature settings.
	GrcMasterTable *		m_mtbGlyphAttrs;
	GrcMasterTable *		m_mtbFeatures;
	GrcMasterValueList *	m_mvlNameStrings;
	std::vector<Symbol>		m_vpsymStyles;
	// Also language classes:
	std::vector<GdlLangClass *>	m_vplcls;

	int m_fxdFeatVersion;	// version of feature table to generate

	std::vector<GrcEnv> m_venv;
	std::map<Symbol, int> m_hmpsymnCurrPass;	// for each table, the current pass
	std::vector<GdlExpression *> m_vpexpConditionals;
	std::vector<GdlExpression *> m_vpexpPassConstraints;

	bool m_fOutputDebugFiles;
	bool m_fOutputDebugXml;

	//	For compiler use:

	int m_wGlyphIDLim;	// lim of range of actual glyph IDs in the font
	int m_cwGlyphIDs;

	int m_cpsymBuiltIn;		// total number of built-in attributes
	int m_cpsymComponents;	// total number of ligature components encountered

	//	Pseudo-code mappings: the two vectors form pairs of underlying unicode values and 
	//	coresponding pseudo-glyph IDs.
	std::vector<unsigned int> m_vnUnicodeForPseudo;
	std::vector<utf16> m_vwPseudoForUnicode;
	unsigned int m_nMaxPseudoUnicode;
	utf16 m_wFirstAutoPseudo;

	std::map<utf16, utf16> m_hmActualForPseudo;

	utf16 m_wLineBreak;	// line break pseudo glyph

	//	Used to represent a "phantom glyph"--one that matches a rule's pre-context when
	//	the stream position is near the beginning and therefore there aren't enough slots in
	//	the stream; this glyph is a member of the ANY class and no other.
	utf16 m_wPhantom;

	//	The following vector maps the internal glyph attr ID to the symbol in the symbol table
	//	(which in turn has a record of the internal ID).
	std::vector<Symbol> m_vpsymGlyphAttrs;

	//	The following matrix contains the glyph attribute assignments for
	//	all of the glyphs in the system. Used by the parser and post-parser.
	GrcGlyphAttrMatrix * m_pgax;

	std::vector<GlyphBoundaries> m_vgbdy;

	//	The following defines an array containing the ligature component mappings for
	//	each glyph. For glyphs that are not ligatures, the array contains NULL.
	//	For ligatures, it contains a pointer to a structure holding a vector something like:
	//		clsABC.component.A
	//		clsABC.component.B
	//		clsABC.component.C
	GrcLigComponentList * m_plclist;

	//	Extra instances of expressions that were simplified or changed in any way
	//	from the originals; pointers are stored here so that they can be properly deleted.
	//	In a sense it is an extension to the master tables; it contains expressions that
	//	would normally be owned there but aren't.
	std::vector<GdlExpression *> m_vpexpModified;

	//	The following vector maps the internal replacement-class IDs to the replacement-classes
	//	themselves (which in turn have a record of the ID). (Replacement-classes are classes
	//	that are used to do replacements in substitution rules;
	//	eg, in "clsA > clsB / _ clsC" clsA and clsB are replacement classes.)
	std::vector<GdlGlyphClassDefn *> m_vpglfcReplcmtClasses;
	int m_cpglfcLinear;	// number of linear classes

	//	Each vector in the array maps the internal FSM-class IDs to the FSM-classes themselves
	//	(which in turn have a record of the ID). (FSM-classes are classes that are used for
	//	matching input.) There is one vector per pass.
	std::vector<GdlGlyphClassDefn *> * m_prgvpglfcFsmClasses;

	int cReplcmntClasses;

	bool m_fVerbose;
	bool m_fPassOptimizations;
	bool m_fOffsetAttrs;
	
	// compiler
	std::vector<GdlFeatureDefn *> m_vpfeatInput;	// features defined in the input font, if any

	TableCompressor m_tcCompressor;
public:
	//	For test procedures:
	void test_Recycle();
};

#endif // GRC_MANAGER_INCLUDED
