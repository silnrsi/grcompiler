/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GrcSymTable.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Defines the symbol table and related classes.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef SYMTABLE_INCLUDED
#define SYMTABLE_INCLUDED

class GrcSymbolTable;
class GrcSymbolTableEntry;
class GrcStructName;

typedef GrcSymbolTableEntry* Symbol;	// hungarian: psym

/*----------------------------------------------------------------------------------------------
Class: GrcSymbolTableEntry
Description: A single entry in the symbol table.
Hungarian: sym
----------------------------------------------------------------------------------------------*/

class GrcSymbolTableEntry
{
	friend class GrcSymbolTable;
	friend class SymbolLess;

public:
	//	Operator precedences
	enum OpPrec {
		kprecNone = -1,
		kprecFunctional=0,	// min(), max() -- low precedence
		kprecAssignment,	// =, +=, -=
		kprecConditional,	// ?
		kprecComparative,	// ==, <, >=, etc.
		kprecLogical,		// &&, ||
		kprecAdditive,		// +, -
		kprecMultiplicative	// *, /
	};		// hungarian: prec

public:
	//	Constructor & destructor:
	GrcSymbolTableEntry(std::string sta, SymbolType symt, GrcSymbolTable* psymtbl)
	:	m_staFieldName(sta),
		m_symt(symt),
		m_symt2(ksymtNone),
		m_psymtbl(psymtbl),
		m_pData(NULL),
		m_fHasData(false),
		m_psymtblSubTable(NULL),
		m_expt(kexptUnknown),
		m_fUserDefined(true),
		m_psymGeneric(NULL),
		m_fGeneric(false)
	{
	}

	~GrcSymbolTableEntry();

	//	Getters:
	GdlDefn * Data()			{ return m_pData; }
	bool HasData()				{ return m_fHasData; }
	ExpressionType ExpType()	{ return m_expt; }
	GrpLineAndFile & LineAndFile() { return m_lnf; }

	//	Setters:
	void SetData(GdlDefn * pData)				{ m_pData = pData; m_fHasData = true; }
	void SetSymbolType(SymbolType symt)			{ m_symt = symt; }
	void SetSubTable(GrcSymbolTable * psymtbl)	{ m_psymtblSubTable = psymtbl; }
	void SetExpType(ExpressionType expt)		{ m_expt = expt; }

public:
	//	General:
	bool FitsSymbolType(SymbolType symt);
	std::string FieldAt(int);
	int	FieldCount();
	int FieldIndex(std::string);
	bool FieldIs(int, std::string);
	std::string FullName();
	std::string FullAbbrev();
	static std::string Abbreviation(std::string staFieldName);
	void GetStructuredName(GrcStructName * pxns);
	bool MatchesOp(std::string);
	bool IsComparativeOp();
	bool IsBogusSlotAttr();
	bool IsReadOnlySlotAttr();
	bool IsWriteOnlySlotAttr();
	bool IsIndexedSlotAttr();
	bool IsIndexedGlyphAttr();
	int Level();
	std::string LastField()
	{
		return m_staFieldName;
	}
	bool LastFieldIs(std::string sta)
	{
		return m_staFieldName == sta;
	}

	Symbol ParentSymbol();
	Symbol BaseDefnForNonGeneric();
	Symbol BaseClassDefn();

	std::string TypeDescriptorString()	// for error messages
	{
		switch (m_symt)
		{
		case ksymtFeature:			return "feature";
		case ksymtLanguage:			return "language";
		case ksymtGlyphAttr:		return "glyph attribute";
		case ksymtGlyphMetric:		return "glyph metric";
		case ksymtSlotAttr:			return "slot attribute";
		default:					return "";
		}
	}

	//	Pre-compiler:
	int InternalID()					{ return m_nInternalID; }
	bool IsUserDefined()				{ return m_fUserDefined; }
	Symbol Generic()					{ return m_psymGeneric; }
	bool IsGeneric()					{ return m_fGeneric; }

	void SetInternalID(int nID)			{ m_nInternalID = nID; }
	void SetUserDefined(bool f)			{ m_fUserDefined = f; }
	void SetGeneric(Symbol psym)		{ m_psymGeneric = psym; }

	Symbol BaseLigComponent();
	Symbol BasePoint();
	Symbol BaseFeatSetting();

	Symbol PointSisterField(std::string staFieldName);

	bool IsComponentRef();
	bool IsComponentBoxField();
	bool IsComponentBase();
	bool IsAttachTo();
	bool IsAttachAtField();
	bool IsAttachWithField();
	bool IsAttachment();
	bool IsMovement();	// shift, kern, advance
	bool DoesJustification();
	bool IsMeasureAttr();
	bool IsUserDefinableSlotAttr();
	int UserDefinableSlotAttrIndex();

	Symbol SubField(std::string);
	bool HasSubFields()
	{
		return m_psymtblSubTable;
	}

	GdlGlyphClassDefn * GlyphClassDefnData();
	GdlFeatureDefn * FeatureDefnData();
	GdlLanguageDefn * LanguageDefnData();

	int JustificationLevel();

	bool AdjustExpTypeIfPossible(ExpressionType expt);

	//	Compiler:
	int SlotAttrEngineCodeOp();
	int GlyphMetricEngineCodeOp();

protected:	
	//	Instance variables:
	std::string			m_staFieldName;

	GrcSymbolTable *	m_psymtblSubTable;
	GrcSymbolTable *	m_psymtbl;
	bool				m_fHasData;		// if false, it doesn't make sense to ask for
										// this item's data, precedence, whatever
										// (eg, "attach", "component.X")
	SymbolType		m_symt;
	SymbolType		m_symt2;		// secondary type (eg, ? is both operator and special)

	ExpressionType	m_expt;			// expression type (numeric, boolean, measurement, slot ref)

	GrpLineAndFile	m_lnf;			// where the symbol was defined, or first encountered

	GdlDefn *		m_pData;
	OpPrec			m_prec;			// precedence--only relevant for operators

	//	for compiler use:
	Symbol m_psymGeneric;	// generic version of a glyph attribute (ie, without
							// the class name)
	bool m_fGeneric;
	bool m_fUserDefined;	// for generic glyph attributes
	bool m_fUsed;			// for glyph metrics (currently not used)
	int	m_nInternalID;		// internal IDs for glyph attrs (currently only used for them)

	// Comparison function for set manipulation:
	bool operator<(const GrcSymbolTableEntry & sym) const
	{
		return (strcmp(m_staFieldName.data(), sym.m_staFieldName.data()) < 0);
	}

};

// Functor class for set manipulation
class SymbolLess
{
	friend class GrcSymbolTableEntry;
public:
	operator()(const Symbol psym1, const Symbol psym2) const
	{
		return (*psym1 < *psym2);
	}
};

typedef std::set<Symbol, SymbolLess> SymbolSet;


/*----------------------------------------------------------------------------------------------
Class: GrcSymbolTable
Description: A multi-layer hash map of symbols and identifiers that have been encountered
	in a GDL file. Each layer corresponds to one field in a structured (dotted) name.
Hungarian: symt
----------------------------------------------------------------------------------------------*/

class GrcSymbolTable
{
	friend class GrcSymbolTableEntry;

	typedef GrcSymbolTableEntry::OpPrec OpPrec;

	typedef std::pair<std::string, Symbol> SymbolTablePair;
	typedef std::map<std::string, Symbol> SymbolTableMap;
	//typedef HashMap<StrAnsi, Symbol, HashStrAnsi, EqlStrAnsi> SymbolTableMap;

public:
	//	Constructor and destructor:
	GrcSymbolTable(bool fMain)
		:	m_csymAnonClass(0),
			m_psymParent(NULL),
			m_cLevel(0)
	{
		if (fMain)
			InitWithPreDefined();

		//m_staLabelDbg.Assign(""); // DEBUG
	}

	~GrcSymbolTable();

public:
	//	Initialization:
	void InitWithPreDefined();

private:
	void InitGlobals();
	void InitDirectives();
	void InitFeatureSettings();
	void InitGlyphAttrs();
	void InitGlyphMetrics();
	void InitOperators();
	void InitSlotAttrs();
	void InitSpecial();
	void InitTableTypes();
	void InitUnits();
	void InitProcStates();

protected:
	void MakeSubTableOf(Symbol psym)
	{
		m_psymParent = psym;
		m_cLevel = psym->Level() + 1;
		psym->SetSubTable(this);

		// DEBUG
/*
		if (m_psymParent)
		{
			m_staLabelDbg.Assign(m_psymParent->m_psymtbl->m_staLabelDbg);
			m_staLabelDbg.Append("%");
			m_staLabelDbg.Append(m_psymParent->m_staFieldName);
		}
		else
			m_staLabelDbg.Assign("");
*/
	}

protected:
	//	Add the structured field name to the symbol table if it was not
	//	already there. Return an error code if the type differs from what was
	//	already there.
	//	Special behaviors:
	//		(1) if we are adding a class, define the glyph attributes
	//			for the class (component, linebreak, directionality)
	//		(2) if we are adding a field to the component......
	Symbol AddSymbolAux(const GrcStructName & xns, SymbolType symtLeaf, SymbolType symtOther,
		GrpLineAndFile const&);

	Symbol PreDefineSymbol(const GrcStructName & xns, SymbolType symt,
		ExpressionType expt = kexptUnknown,
		OpPrec prec = GrcSymbolTableEntry::kprecNone);	// m_nLineNumber = -1

	Symbol AddType2(const GrcStructName & xns, SymbolType symt)
	{
		Symbol psymRet = FindSymbol(xns);
		Assert(psymRet);
		psymRet->m_symt2 = symt;
		return psymRet;
	}

	//	Getters:
	int Level()	{ return m_cLevel; }

public:
	//	Getters for symbols:
	SymbolType Type(GrcStructName * pxns);
	GdlDefn * Data(GrcStructName * pxns);
	int LineNumber(GrcStructName * pxns);
	int GetOpPrec(GrcStructName * pxns);

	//	Setters for symbols:
	void Data(GrcStructName * pxns, GdlDefn * pData);
	void SetLineAndFile(GrcStructName * pxns, GrpLineAndFile const&);

public:
	//	General:
	Symbol AddSymbol(const GrcStructName & xns, SymbolType symt, GrpLineAndFile const&);

	Symbol AddClassSymbol(const GrcStructName & xns, GrpLineAndFile const&);
	Symbol AddFeatureSymbol(const GrcStructName & xns, GrpLineAndFile const&);
	Symbol AddLanguageSymbol(const GrcStructName & xns, GrpLineAndFile const& lnf);
	Symbol AddGlyphAttrSymbol(const GrcStructName & xns, GrpLineAndFile const&,
		ExpressionType expt, bool fMetric = false);
	//Symbol AddComponentField(const GrcStructName & xns, GrpLineAndFile &);
	Symbol AddAnonymousClassSymbol(GrpLineAndFile const&);

	Symbol FindField(std::string staField);
	Symbol FindSymbol(const GrcStructName & xns);
	Symbol FindSymbol(const std::string staName);

	Symbol FindSlotAttr(const GrcStructName & xns, GrpLineAndFile const&);

protected:
	int AddGlyphAttrSymbolInMap(Vector<Symbol> & vpsymGlyphAttrIDs,
		Symbol psymGeneric);
	int AddGlyphAttrSymbolInMap(Vector<Symbol> & vpsymGlyphAttrIDs,
		Symbol psymGeneric, int ipsymToAssign);

public:
	//	Special functions:

	//	If one of the fields of the symbol name is a style, return a pointer to that
	//	symbol; otherwise return a NULL pointer.
	Symbol FindStyle(GrcStructName * pName);

	//	Iterators:
	SymbolTableMap::iterator EntriesBegin()
	{
		return m_hmstasymEntries.begin();
	}

	SymbolTableMap::iterator EntriesEnd()
	{
		return m_hmstasymEntries.end();
	}

public:
	//	Pre-compiler methods:
	bool AssignInternalGlyphAttrIDs(GrcSymbolTable * psymtblMain,
		Vector<Symbol> & vpsymGlyphAttrIDs, int nPass, int cComponents, int nMaxJLevel);
	Symbol BaseLigComponent();

	//	Debuggers:
	void GlyphAttrList(Vector<Symbol> & vpsym);

protected:
	//	Instance variables:
	Symbol			m_psymParent;

	SymbolTableMap	m_hmstasymEntries;

	int m_cLevel;

	//	the following counter is used to generate unique names for anonymous
	//	classes:
	int	m_csymAnonClass;

	//std::string m_staLabelDbg; // debug
};


/*----------------------------------------------------------------------------------------------
Class: GrcStructName
Description: A list of strings that corresponds to a single dotted identifier; eg,
	"clsAElig.component.A.box.xmin".
Hungarian: xns (okay, I was desperate)
----------------------------------------------------------------------------------------------*/
class GrcStructName
{
public:
	//	Constructors:
	GrcStructName()
	{
	}

	GrcStructName(std::string staName)
	{
		m_vstaFields.Push(staName);
	}
	GrcStructName(std::string sta1, std::string sta2)
	{
		m_vstaFields.Push(sta1);
		m_vstaFields.Push(sta2);
	}
	GrcStructName(std::string sta1, std::string sta2, std::string sta3)
	{
		m_vstaFields.Push(sta1);
		m_vstaFields.Push(sta2);
		m_vstaFields.Push(sta3);
	}
	GrcStructName(std::string sta1, std::string sta2, std::string sta3, std::string sta4)
	{
		m_vstaFields.Push(sta1);
		m_vstaFields.Push(sta2);
		m_vstaFields.Push(sta3);
		m_vstaFields.Push(sta4);
	}
	GrcStructName(std::string sta1, std::string sta2, std::string sta3, std::string sta4, std::string sta5)
	{
		m_vstaFields.Push(sta1);
		m_vstaFields.Push(sta2);
		m_vstaFields.Push(sta3);
		m_vstaFields.Push(sta4);
		m_vstaFields.Push(sta5);
	}
	GrcStructName(Vector<std::string> & vsta)
	{
		vsta.CopyTo(m_vstaFields);
	}

	//	Copy constructor:
	GrcStructName(const GrcStructName & xns)
	{
		xns.m_vstaFields.CopyTo(m_vstaFields);
	}

	//	General:
	int NumFields() const
	{
		return m_vstaFields.Size();
	}
	std::string FieldAt(int i) const
	{
		return m_vstaFields[i];
	}
	bool FieldEquals(int i, std::string sta) const
	{
		return m_vstaFields[i] == sta;
	}

	void InsertField(int i, std::string sta)
	{
		Assert(i <= m_vstaFields.size());
		m_vstaFields.Insert(i, sta);
	}
	void DeleteField(int i)
	{
		Assert(i < m_vstaFields.size());
		m_vstaFields.Delete(i);
	}

	std::string FullString() const
	{
		std::string staRet = m_vstaFields[0];
		for (int i = 1; i < m_vstaFields.Size(); i++)
		{
			staRet += ".";
			staRet += m_vstaFields[i];
		}
		return staRet;
	}

	//	Copy all but the first field into the given name. For instance, if this name is
	//	classX.component.X.top, the argument becomes component.X.top.
	void CopyMinusFirstField(GrcStructName & nwf) const
	{
		for (int i = 1; i < m_vstaFields.Size(); i++)
			nwf.m_vstaFields.Push(m_vstaFields[i]);
	}

protected:
	//	Instance variables:
	mutable Vector<std::string>	m_vstaFields;	// mutable because the Vector methods are not const
};



inline int GrcSymbolTableEntry::Level()
{
	return m_psymtbl->Level();
}

inline Symbol GrcSymbolTableEntry::ParentSymbol()
{
	return m_psymtbl->m_psymParent;
}


#endif // !SYMTABLE_INCLUDED