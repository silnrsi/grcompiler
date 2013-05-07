/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GdlRule.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Classes to implement rules and rule items.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef GDL_RULE_INCLUDED
#define GDL_RULE_INCLUDED

/*----------------------------------------------------------------------------------------------
Class: GdlAttrValueSpec
Description: The action setting the value for a single attribute.
Hungarian: avs
----------------------------------------------------------------------------------------------*/
class GdlAttrValueSpec : public GdlObject
{
	friend class GdlRule;
	friend class GdlRuleItem;
	friend class GdlSetAttrItem;
	friend class GdlSubstitutionItem;

public:
	//	Constructors & destructors:
	GdlAttrValueSpec(Symbol psymName, Symbol psymOp, GdlExpression * pexpValue)
		:	m_psymName(psymName),
			m_psymOperator(psymOp),
			m_pexpValue(pexpValue),
			m_fFlattened(false)
	{
		m_pexpValue->PropagateLineAndFile(pexpValue->LineAndFile());
	}

	//	copy constructor
	GdlAttrValueSpec(const GdlAttrValueSpec & avs)
		:	GdlObject(avs),
			m_psymName(avs.m_psymName),
			m_psymOperator(avs.m_psymOperator),
			m_pexpValue(avs.m_pexpValue->Clone()),
			m_fFlattened(avs.m_fFlattened)
	{
	}

	~GdlAttrValueSpec()
	{
		delete m_pexpValue;
	}

	//	General:
	bool Flattened()
	{
		return m_fFlattened;
	}
	void SetFlattened(bool f)
	{
		m_fFlattened = f;
	}

protected:
	//	Parser:
	void PropagateLineAndFile(GrpLineAndFile & lnf)
	{
		if (LineIsZero())
		{
			SetLineAndFile(lnf);
			m_pexpValue->PropagateLineAndFile(lnf);
		}
	}

	//	Post-parser:
	void ReplaceAliases(GdlRule *);
	bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *);

	//	Pre-compiler:
	void FixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit, Symbol psymOutClass);
	void FlattenPointSlotAttrs(GrcManager * pcman, std::vector<GdlAttrValueSpec *> & vpavsNew);
	void CheckAttachAtPoint(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit,
		bool * pfXY, bool *pfGpoint);
	void CheckAttachWithPoint(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit,
		bool * pfXY, bool *pfGpoint);
	void FixFeatureTestsInRules(GrcFont * pfont);
	bool CheckRulesForErrors(GrcGlyphAttrMatrix * pgax,  GrcFont * pfont,
		GdlRenderer * prndr, Symbol psymTable, int rco,
		GdlRuleItem * prit, int irit,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel);
	void AdjustSlotRefsForPreAnys(int critPrependedAnys, GdlRuleItem * prit);
	void AdjustToIOIndices(GdlRuleItem * prit,
		std::vector<int> & viritInput, std::vector<int> & viritOutput);
	bool ReplaceKern(GrcManager * pcman,
		GdlAttrValueSpec ** ppavsShift, GdlAttrValueSpec ** ppavsAdvance);
	void MaxJustificationLevel(int * pnJLevel);
	bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);

	//	Compiler:
	bool GenerateAttrSettingCode(GrcManager *, int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int irit, int nIIndex, int iritAttachTo);
	bool IsKeySlotAttr();

private:
	void operator=(GdlAttrValueSpec);	// don't define the assignment operator

public:
	//	debuggers:
	void PrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml,
		bool * pfAtt, bool * pfAttAt, bool * pfAttWith, int cpavs);
	void PrettyPrintAttach(GrcManager * pcman, std::ostream & strmOut, bool fXml);
	void DebugXml(GrcManager * pcman, std::ostream & strmOut, std::string staPathToCur);

protected:
	//	Instance variables:
	Symbol			m_psymName;
	Symbol			m_psymOperator;
	GdlExpression *	m_pexpValue;

	//	for compiler use:
	int m_nInternalID;	// internal ID for slot attribute
	bool m_fFlattened;	// an expression that was created from a more general expression being
						// "flattened;" ie, "attach.with = ptA" => "attach.with { x = ptA.x;
						// y = ptA.y; xoffset = ptA.xoffset; yoffset = ptA.yoffset }"

public:
	void test_SetLineAndFile(GrpLineAndFile & lnf)
	{
		SetLineAndFile(lnf);
		m_pexpValue->PropagateLineAndFile(lnf);
	}

};	//	end of GdlAttrValueSpec


/*----------------------------------------------------------------------------------------------
Class: GdlRuleItem
Description:
Hungarian: rit
----------------------------------------------------------------------------------------------*/

class GdlRuleItem : public GdlObject
{
	friend class GdlRule;
	friend class GdlLineBreakItem;
	friend class GdlSetAttrItem;
	friend class GdlSubstitutionItem;

public:
	//	Constructors & destructors:
	GdlRuleItem()
		:	m_psymInput(NULL),
			m_pexpConstraint(NULL)
	{
	}
	GdlRuleItem(Symbol psym)
		:	m_psymInput(psym),
			m_pexpConstraint(NULL)
	{
	}

	//	copy constructor
	GdlRuleItem(const GdlRuleItem & rit)
		:	GdlObject(rit),
			m_iritContextPos(rit.m_iritContextPos),
			m_psymInput(rit.m_psymInput),
			m_staAlias(rit.m_staAlias),
			m_iritContextPosOrig(rit.m_iritContextPosOrig),
			m_nInputFsmID(rit.m_nInputFsmID)
	{
		if (rit.m_pexpConstraint)
			m_pexpConstraint = rit.m_pexpConstraint->Clone();
		else
			m_pexpConstraint = NULL;
	}

	virtual GdlRuleItem * Clone()
	{
		return new GdlRuleItem(*this);
	}

	virtual ~GdlRuleItem()
	{
		if (m_pexpConstraint)
			delete m_pexpConstraint;
	}

	//	Alpha version of original item number (1-based), for error messages
	std::string PosString()
	{
		char rgchItem[20];
		itoa(m_iritContextPosOrig + 1, rgchItem, 10);
		return rgchItem;
	}

	//	Increment the context position by the given number
	void IncContextPosition(int dirit)
	{
		m_iritContextPos += dirit;
	}

public:
	//	For classes that don't do substitutions, the output is the input
	virtual Symbol OutputSymbol()
	{
		return m_psymInput;
	}

	//	Parser:
	virtual void AddAssociation(GrpLineAndFile &, int n);
	virtual void AddAssociation(GrpLineAndFile &, std::string sta);
	virtual void AddAttrValueSpec(GdlAttrValueSpec * /*pavs*/)
	{
		Assert(false);	// should have been converted to an GdlSetAttrItem
	}

	void SetConstraint(GdlExpression * pexp)
	{
		Assert(!m_pexpConstraint);
		m_pexpConstraint = pexp;
		pexp->PropagateLineAndFile(LineAndFile());
	}

	void SetSlotName(std::string sta)
	{
		m_staAlias = sta;
	}

	//	Post-parser:
	virtual void ReplaceAliases(GdlRule *);
	virtual bool AdjustSlotRefs(std::vector<bool>& vfOmit, std::vector<int>& vnNewIndices,
		GdlRule * prule);
	virtual void CheckSelectors(GdlRule * prule, int irit, int crit);

	//	Pre-compiler:
	virtual void FixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, GdlRule * prule, int irit);
	virtual void FlattenPointSlotAttrs(GrcManager * pcman);
	void AssignFsmInternalID(GrcManager * pcman, int nPassID);
	virtual void FindSubstitutionSlots(int irit,
		std::vector<bool> & vfInput, std::vector<bool> & vfOutput);
	void MarkClassAsReplacementClass(GrcManager * pcman,
		ReplacementClassSet & setpglfcReplace, bool fInput);
	virtual void FixFeatureTestsInRules(GrcFont *);
	virtual bool CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
		GdlRenderer * prndr, GdlRule * prule, Symbol psymTable,
		int grfrco, int irit, bool fAnyAssocs,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
		std::vector<int> & vcwClassSizes);
	virtual bool AnyAssociations();
	bool CheckForJustificationConstraint();
	virtual void AdjustSlotRefsForPreAnys(int critPrependedAnys);
	virtual void AssignIOIndices(int * pcritInput, int * pcritOutput,
		std::vector<int> & viritInput, std::vector<int> & viritOutput);
	virtual void AdjustToIOIndices(std::vector<int> & viritInput, std::vector<int> & viritOutput);
	virtual void SetAttachTo(int /*n*/)
	{
		Assert(false);	// only useful for GdlSetAttrItem
	}
	virtual int AttachTo()
	{
		return -1;
	}
	virtual int AttachedToSlot();
	bool OverlapsWith(GdlRuleItem * prit, GrcFont * pfont, int grfsdc);
///	void CheckLBsInRules(Symbol psymTable, int * pcritPreLB, int * pcritPostLB);
	virtual void ReplaceKern(GrcManager * pcman);
	virtual void MaxJustificationLevel(int * pnJLevel);
	virtual bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);
	bool IsSpaceItem(std::vector<utf16> & vwSpaceGlyphs);

	//	Compiler:
	virtual bool IsMarkedKeySlot() { return false; }	// only slots that perform modifications
	virtual bool CanBeKeySlot() { return false; }
	void MarkKeyGlyphsForPass(GrcGlyphAttrMatrix * pgax, unsigned int nAttrIdSkipP, int nPassID);
	void GenerateConstraintEngineCode(GrcManager *, int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int irit, std::vector<int> & viritInput, int iritFirstModItem);
	virtual void GenerateActionEngineCode(GrcManager *, int fxdRuleVersion,
		std::vector<gr::byte> & vbOutput,
		GdlRule * prule, int irit, bool * pfSetInsertToFalse);
	static void GenerateInsertEqualsFalse(std::vector<gr::byte> & vbOutput);
	void GetMachineClasses(FsmMachineClass ** ppfsmcAssignments,
		FsmMachineClassSet & setpfsmc);

private:
	void operator=(GdlRuleItem);	// don't call the assignment operator--compile error

public:
	//	debuggers:
	virtual void LhsPrettyPrint(GrcManager * pcman, GdlRule * prule, int irit,
		std::ostream & strmOut, bool fXml);
	virtual void RhsPrettyPrint(GrcManager * pcman, GdlRule * prule, int irit,
		std::ostream & strmOut, bool fXml);
	virtual void ContextPrettyPrint(GrcManager * pcman, GdlRule * prule, int irit,
		std::ostream & strmOut, bool fXml);
	virtual void ConstraintPrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml, bool fSpace = false);

	virtual void DebugXmlLhs(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur);
	virtual void DebugXmlRhs(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur);
	virtual void DebugXmlContext(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur,
		int & iritRhs);
	virtual void DebugXmlConstraint(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur);

protected:
	//	Instance variables:
	int m_iritContextPos;		// position within context (0-based)
	Symbol m_psymInput;
	GdlExpression * m_pexpConstraint;

	//	for parser use:
	std::string m_staAlias;
	int m_iritContextPosOrig;	// original--not adjusted for optional items (0-based) or
								// inserted ANY's--for error messages

	//	for compiler use:
	int m_nInputFsmID;

	int m_nInputIndex;	// index of item relative to input stream (ignoring inserted items)
	int m_nOutputIndex;	// index of item relative to output stream (ignoring deleted items)

};	//	end of GdlRuleItem


/*----------------------------------------------------------------------------------------------
Class: GdlLineBreakItem
Description: A line-break item in the context of a rule
Hungarian: ritlb
----------------------------------------------------------------------------------------------*/

class GdlLineBreakItem : public GdlRuleItem
{
	friend class GdlRule;

public:
	//	Constructors & destructors:
	GdlLineBreakItem(Symbol psym)
		:	GdlRuleItem(psym)
	{
	}

	GdlLineBreakItem(const GdlRuleItem & rit)
		:	GdlRuleItem(rit)
	{
	}
	GdlLineBreakItem(const GdlLineBreakItem & rit)
		:	GdlRuleItem(rit)
	{
	}

	virtual GdlRuleItem * Clone()
	{
		return new GdlLineBreakItem(*this);
	}

protected:
	//	Parser:

	//	Post-parser:
	virtual void ReplaceAliases(GdlRule *);
	virtual bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *);

	//	Pre-compiler:
	virtual void FixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, GdlRule * prule, int irit);
	virtual bool CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
		GdlRenderer * prndr, GdlRule * prule, Symbol psymTable,
		int grfrco, int irit,  bool fAnyAssocs,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
		std::vector<int> & vcwClassSizes);
	virtual void AdjustSlotRefsForPreAnys(int critPrependedAnys);
	virtual void AdjustToIOIndices(std::vector<int> & viritInput, std::vector<int> & viritOutput);

public:
	//	debuggers:
	virtual void ContextPrettyPrint(GrcManager * pcman, GdlRule * prule, int irit,
		std::ostream & strmOut, bool fXml);
	virtual void DebugXmlConstraint(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur);

};	//	end of GdlLineBreakItem


/*----------------------------------------------------------------------------------------------
Class: GdlSetAttrItem
Description: An item in a positioning rule, having the effect of setting slot attributes.
Hungarian: ritset
----------------------------------------------------------------------------------------------*/

class GdlSetAttrItem : public GdlRuleItem
{
	friend class GdlRule;

public:
	//	Constructors & destructors:
	GdlSetAttrItem()
		:	GdlRuleItem()
	{
		m_nAttachTo = -1;
	}

	GdlSetAttrItem(Symbol psym)
		:	GdlRuleItem(psym)
	{
		m_nAttachTo = -1;
	}

	//	copy constructors
	GdlSetAttrItem(const GdlRuleItem & rit)
		:	GdlRuleItem(rit)
	{
		m_nAttachTo = -1;
	}

	GdlSetAttrItem(const GdlSetAttrItem & rit)
		:	GdlRuleItem(rit)
	{
		m_nAttachTo = -1;
		Assert(m_vpavs.size() == 0);
		for (size_t i = 0; i < rit.m_vpavs.size(); ++i)
			m_vpavs.push_back(new GdlAttrValueSpec(*rit.m_vpavs[i]));
	}

	virtual ~GdlSetAttrItem()
	{
		for (size_t i = 0; i < m_vpavs.size(); ++i)
			delete m_vpavs[i];
	}
	
	virtual GdlRuleItem * Clone()
	{
		return new GdlSetAttrItem(*this);
	}

protected:
	//	Parser:
	virtual void AddAttrValueSpec(GdlAttrValueSpec * pavs)
	{
		m_vpavs.push_back(pavs);
		pavs->PropagateLineAndFile(LineAndFile());
	}

	//	Post-parser:
	virtual void ReplaceAliases(GdlRule *);
	virtual bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *);

	//	Pre-compiler:
	virtual void FixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, GdlRule * prule, int irit);
	virtual void FlattenPointSlotAttrs(GrcManager * pcman);
	virtual Symbol OutputClassSymbol();
	void CheckCompBox(GrcManager * pcman, Symbol psymCompRef);
	virtual void FixFeatureTestsInRules(GrcFont *);
	virtual bool CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
		GdlRenderer * prndr, GdlRule * prule, Symbol psymTable,
		int grfrco, int irit,  bool fAnyAssocs,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
		std::vector<int> & vcwClassSizes);
	virtual void AdjustSlotRefsForPreAnys(int critPrependedAnys);
	virtual void AdjustToIOIndices(std::vector<int> & viritInput, std::vector<int> & viritOutput);
	virtual void ReplaceKern(GrcManager * pcman);
	virtual void MaxJustificationLevel(int * pnJLevel);
	virtual bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);
	virtual void SetAttachTo(int n)
	{
		m_nAttachTo = n;
	}
	virtual int AttachTo()
	{
		return m_nAttachTo;
	}
	virtual int AttachedToSlot();

protected:
	int AttachToSettingValue();

public:
	//	Compiler:
	virtual bool IsMarkedKeySlot();
	virtual bool CanBeKeySlot()
	{
		return (m_vpavs.size() > 0);
	}
	virtual void GenerateActionEngineCode(GrcManager *, int fxdRuleVersion,
		std::vector<gr::byte> & vbOutput,
		GdlRule * prule, int irit, bool * pfSetInsertToFalse);
	bool GenerateAttrSettingCode(GrcManager *, int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int irit, int nIIndex);

	//	debuggers:
	virtual void LhsPrettyPrint(GrcManager * pcman, GdlRule * prule, int irit,
		std::ostream & strmOut, bool fXml);
	virtual void RhsPrettyPrint(GrcManager * pcman, GdlRule * prule, int irit,
		std::ostream & strmOut, bool fXml);
	virtual void ContextPrettyPrint(GrcManager * pcman, GdlRule * prule, int irit,
		std::ostream & strmOut, bool fXml);
	virtual void AttrSetterPrettyPrint(GrcManager * pcman, GdlRule * prule, int irit,
		std::ostream & strmOut, bool fXml);

	virtual void DebugXmlLhs(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur);
	virtual void DebugXmlRhs(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur);
	virtual void DebugXmlContext(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur,
		int & iritRhs);
	//virtual void DebugXmlAttrSetter(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur);

protected:
	//	Instance variables:
	std::vector<GdlAttrValueSpec *> m_vpavs;

	int m_nAttachTo;	// index of attach.to slot attr (0-based); -1 if none

};	//	end of GdlSetAttrItem


/*----------------------------------------------------------------------------------------------
Class: GdlSubstitutionItem
Description: An item in a substitution rule.
Hungarian: ritsub
----------------------------------------------------------------------------------------------*/

class GdlSubstitutionItem : public GdlSetAttrItem
{
	friend class GdlRule;

public:
	//	Constructors & destructors:
	GdlSubstitutionItem(Symbol psymInput, Symbol psymOutput)
		:	GdlSetAttrItem(psymInput),
			m_psymOutput(psymOutput),
			m_pexpSelector(NULL),
			m_nSelector(-1)
	{
	}

	GdlSubstitutionItem(const GdlSetAttrItem & rit)
		:	GdlSetAttrItem(rit),
			m_psymOutput(rit.m_psymInput),
			m_pexpSelector(NULL),
			m_nSelector(-1)
	{
	}

	//	copy constructor
	GdlSubstitutionItem(const GdlSubstitutionItem & rit)
		:	GdlSetAttrItem(rit),
			m_psymOutput(rit.m_psymOutput),
//			m_vpexpAssocs(rit.m_vpexpAssocs),
			m_nSelector(rit.m_nSelector),
			m_nOutputFsmID(rit.m_nOutputFsmID),
			m_nInputSubsID(rit.m_nInputSubsID),
			m_nOutputSubsID(rit.m_nOutputSubsID)
	{
		for (size_t i = 0; i < rit.m_vpexpAssocs.size(); ++i)
		{
			m_vpexpAssocs.push_back(
				dynamic_cast<GdlSlotRefExpression*>(rit.m_vpexpAssocs[i]->Clone()));
		}

		m_pexpSelector =
			(rit.m_pexpSelector)?
				dynamic_cast<GdlSlotRefExpression*>(rit.m_pexpSelector->Clone()):
				NULL;
	}

	virtual GdlRuleItem * Clone()
	{
		return new GdlSubstitutionItem(*this);
	}

	virtual ~GdlSubstitutionItem()
	{
		for (size_t i = 0; i < m_vpexpAssocs.size(); ++i)
			delete m_vpexpAssocs[i];

		if (m_pexpSelector)
			delete m_pexpSelector;
	}

protected:
	virtual Symbol OutputSymbol()
	{
		return m_psymOutput;
	}

	//	Parser:
public:
	virtual void AddAssociation(GrpLineAndFile &, int n);
	virtual void AddAssociation(GrpLineAndFile &, std::string sta);

	//	Post-parser:
protected:
	virtual void ReplaceAliases(GdlRule *);
	virtual bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *);
	virtual void CheckSelectors(GdlRule * prule, int irit, int crit);

	//	Pre-compiler:
	virtual void FixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, GdlRule * prule, int irit);
	virtual Symbol OutputClassSymbol();
	virtual void FindSubstitutionSlots(int irit,
		std::vector<bool> & vfInput, std::vector<bool> & vfOutput);
	virtual bool CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
		GdlRenderer * prndr, GdlRule * prule, Symbol psymTable,
		int grfrco, int irit,  bool fAnyAssocs,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
		std::vector<int> & vcwClassSizes);
	virtual void AdjustSlotRefsForPreAnys(int critPrependedAnys);
	virtual void AssignIOIndices(int * pcritInput, int * pcritOutput,
		std::vector<int> & viritInput, std::vector<int> & viritOutput);
	virtual void AdjustToIOIndices(std::vector<int> & viritInput, std::vector<int> & viritOutput);
	virtual bool AnyAssociations();

	//	Compiler:
	virtual bool CanBeKeySlot()
	{
		//	True unless this is an insertion slot.
		if (m_psymInput->FitsSymbolType(ksymtSpecialUnderscore))
			return false;
		else
			return true;
	}
	virtual void GenerateActionEngineCode(GrcManager *, int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		GdlRule * prule, int irit, bool * pfSetInsertToFalse);
public:
	//	debuggers:
	virtual void LhsPrettyPrint(GrcManager * pcman, GdlRule * prule, int irit,
		std::ostream & strmOut, bool fXml);
	virtual void RhsPrettyPrint(GrcManager * pcman, GdlRule * prule, int irit,
		std::ostream & strmOut, bool fXml);
	virtual void DebugXmlLhs(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur);
	virtual void DebugXmlRhs(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur);

protected:
	//	Instance variables:
	Symbol								m_psymOutput;	// possibly '_' or '@'
	GdlSlotRefExpression*				m_pexpSelector;	// original, 1-based
	std::vector<GdlSlotRefExpression*>	m_vpexpAssocs;	// original, 1-based

	GdlRuleItem * m_pritSelInput;	// slot indicated by selector

	//	for pre-compiler use:
	std::vector<int> m_vnAssocs;	// m_vpexpAssocs converted to input indices
	int m_nSelector;		// m_pexpSelector converted to input index,
							//     or -1 if default (same item)

	//	for compiler use: -- not yet implemented
	int m_nOutputFsmID;
	int m_nInputSubsID;		
	int m_nOutputSubsID;

};	//	end of GdlSubstitutionItem


/*----------------------------------------------------------------------------------------------
Class: GdlAlias
Description: A mapping of a slot alias to a slot index. Can also be used as an item that can
	hold either a slot alias or an index.
Hungarian: alias
----------------------------------------------------------------------------------------------*/

class GdlAlias
{
	friend class GdlRule;

public:
	//	Constructors:
	GdlAlias()
		:	m_srIndex(-1)
	{
	}

	GdlAlias(std::string sta, int sr)
		:	m_staName(sta),
			m_srIndex(sr)
	{
	}

	GdlAlias(const GdlAlias & alias)
		:	m_staName(alias.m_staName),
			m_srIndex(alias.m_srIndex)
	{
	}

public:
	//	Parser:
	int Index()			{ return m_srIndex; }
	std::string Name()	{ return m_staName; }

	void SetIndex(int sr)
	{
		m_srIndex = sr;
	}
	void SetName(std::string sta)
	{
		m_staName = sta;
	}

	//	Post-parser:

	//	Adjust all the slot references based on the slots that were omitted because
	//	they were optional. Return false if there is a reference to an omitted slot.
	bool AdjustSlotRefs(std::vector<bool> & vfOmit, std::vector<int> & vnNewIndices)
	{
		//	Should no longer be necessary because we replace all the aliases
		//	before we do this.
		Assert(false);

		if (vfOmit[m_srIndex])
			return false;
		m_srIndex = vnNewIndices[m_srIndex];
		return true;
	}

protected:
	std::string	m_staName;
	int			m_srIndex;	// 1-based

};	//	end of GdlAlias


/*----------------------------------------------------------------------------------------------
Class: GdlRule
Description:
Hungarian: rule
----------------------------------------------------------------------------------------------*/
class GdlRule : public GdlObject
{
	friend class GdlRuleItem;

public:
	//	Constructors & destructors:
	GdlRule()
	{
		m_nScanAdvance = -1;
		m_nDefaultAdvance = -1;
		m_fBadRule = false;
	}

	~GdlRule()
	{
		size_t i;
		for (i = 0; i < m_vprit.size(); ++i)
			delete m_vprit[i];
		for (i = 0; i < m_vpexpConstraints.size(); ++i)
			delete m_vpexpConstraints[i];
		for (i = 0; i < m_vpalias.size(); ++i)
			delete m_vpalias[i];
	}

public:
	//	General:
	GdlRuleItem * Item(int irit)
	{
		Assert((unsigned int)irit < (unsigned int)m_vprit.size());
		return m_vprit[irit];
	}
	int LookupAliasIndex(std::string sta);

	int NumberOfSlots()
	{
		return m_vprit.size();
	}

	bool IsBadRule()
	{
		return m_fBadRule;
	}

	int SortKey();

public:
	//	Parser:
	GdlRuleItem * ContextItemAt(GrpLineAndFile &, int irit,
		std::string staInput, std::string staAlias = "");
	GdlRuleItem * RhsItemAt(GrpLineAndFile &, int irit,
		std::string staOutput, std::string staAlias = "", bool fSubItem = false);
	GdlRuleItem * LhsItemAt(GrpLineAndFile &, int irit,
		std::string staInput, std::string staAlias = "");

	GdlRuleItem * ContextSelectorItemAt(GrpLineAndFile &, int irit,
		std::string staClassOrAt, int nSel, std::string staAlias = "");
	GdlRuleItem * ContextSelectorItemAt(GrpLineAndFile &, int irit,
		std::string staClassOrAt, std::string staSel, std::string staAlias = "");

	GdlRuleItem * RhsSelectorItemAt(GrpLineAndFile &, int irit,
		std::string staClassOrAt,  int nSel,std::string staAlias = "");
	GdlRuleItem * RhsSelectorItemAt(GrpLineAndFile &, int irit,
		std::string staClassOrAt, std::string staSel, std::string staAlias = "");

	GdlRuleItem * LhsSelectorItemAt(GrpLineAndFile &, int irit,
		std::string staClassOrAt,  int nSel, std::string staAlias = "");
	GdlRuleItem * LhsSelectorItemAt(GrpLineAndFile &, int irit,
		std::string staClassOrAt, std::string staSel, std::string staAlias = "");

	void AddOptionalRange(int iritStart, int crit, bool fContext);
	int ScanAdvance()
	{
		return m_nScanAdvance;
	}
	void SetScanAdvance(int n)
	{
		Assert((n >= -1) && (n <= static_cast<int>(m_vprit.size())));
		m_nScanAdvance = n;
	}
	void AddConstraint(GdlExpression * pexp)
	{
		m_vpexpConstraints.push_back(pexp);
	}

	void InitialChecks();
protected:
	void CheckInputClass();
	void ConvertLhsOptRangesToContext();

public:
	//	Post-parser:
	void ReplaceAliases();
	bool HandleOptionalItems(std::vector<GdlRule*> & vpruleNewList);
	void CheckSelectors();
	bool HasNoItems()
	{
		return m_vprit.size() == 0;
	}
	int ItemCount()
	{
		return m_vprit.size();
	}
	GdlRuleItem * Rule(int irit)
	{
		return m_vprit[irit];
	}
protected:
	bool AdjustOptRanges();
	void GenerateOptRanges(
		std::vector<GdlRule*>&	vpruleNewList,
		std::vector<bool>	&	vfOmitRange,
		size_t					irangeCurr);
	void GenerateOneRuleVersion(
		std::vector<GdlRule*>&	vpruleNewList,
		std::vector<bool>	&	vfOmitRange);
	int PrevRangeSubsumes(int irangeCurr);

public:
	//	Pre-compiler:
	int CountRulePreContexts();
	void FixRulePreContexts(Symbol psymAnyClass, int critNeeded);

	void FixGlyphAttrsInRules(GrcManager * pcman, GrcFont * pfont);
	void AssignClassInternalIDs(GrcManager * pcman, int nPassID,
		ReplacementClassSet & setpglfc);
	void FixFeatureTestsInRules(GrcFont *);
	void MarkReplacementClasses(GrcManager * pcman, int nPassID,
		ReplacementClassSet & setpglfcReplace);
	void MarkClassAsReplacementClass(GrcManager * pcman,
		ReplacementClassSet & setpglfcReplace, bool fInput);
	void CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
		GdlRenderer * prndr, Symbol psymTable, int grfrco);
	bool CheckForJustificationConstraint();
	void CalculateIOIndices();
	void GiveOverlapWarnings(GrcFont * pfont, int grfsdc);
	bool CheckLBsInRules(Symbol psymTable, int * pcritPreLB, int * pcritPostLB);
	bool HasReprocessing();
	void ReplaceKern(GrcManager * pcman);
	void MaxJustificationLevel(int * pnJLevel);
	bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);
	void MovePassConstraintsToRule(std::vector<GdlExpression *> & m_vpexpPassConstr);
	int ItemCountOriginal();
	int FindSubstitutionItem(int iritDel);
	void CalculateSpaceContextuals(SpaceContextuals * pspconSoFar,
		std::vector<utf16> & vwSpaceGlyphs);

	//	Compiler:
	void PassOptimizations(GrcGlyphAttrMatrix * pgax, unsigned int nAttrIdSkipP, int nPassID);	
	void GenerateEngineCode(GrcManager *, int fxdRuleVersion,
		std::vector<gr::byte> & vbActions, std::vector<gr::byte> & vbConstraints);
	void GenerateConstraintEngineCode(GrcManager *, int fxdRuleVersion, std::vector<gr::byte> & vbOutput);
	GdlRuleItem * InputItem(int n);
	int NumberOfInputItems();
	int NumberOfPreModContextItems()
	{
		return m_critPreModContext;
	}

	//	debuggers:
	void DebugEngineCode(GrcManager * pcman, int fxdRuleVersion, std::ostream & strmOut);
	static void DebugEngineCode(std::vector<gr::byte> & vb, int fxdRuleVersion, std::ostream & strmOut);
	void RulePrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml);
	static std::string SlotAttributeDebugString(int slat);
	static std::string GlyphMetricDebugString(int gmet);
	static std::string EngineCodeDebugString(int op);
	static std::string ProcessStateDebugString(int pstat);
	void DebugXml(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur,
		int nPassNumber, int nRuleNum);

protected:
	//	Instance variables:
	int m_nScanAdvance;	// 0-based; item before which the ^ is placed, or -1 if no ^
	std::vector<GdlRuleItem *>		m_vprit;
	std::vector<GdlExpression *>	m_vpexpConstraints;	// multiple constraints come from multiple -if-s
	std::vector<GdlAlias *>			m_vpalias;

	//	for post-parser use:
	std::vector<bool>		m_vfOptRangeContext;  // are opt ranges below relative to context?
	std::vector<int>		m_viritOptRangeStart;
	std::vector<int>		m_viritOptRangeEnd;

	//	for pre-compiler use:
	//	input- and output indices for each item:
	std::vector<int> m_viritInput;
	std::vector<int> m_viritOutput;

	//	number of items in the context before the first modified item (original, before adding
	//	ANY items)
	int m_critPreModContext;

	//	number of ANY items that were prepended to the front of the rule
	int m_critPrependedAnys;

	//	original context length
	int m_critOriginal;

	//	scan-advance, adjusted
	int m_nOutputAdvance;

	//	where the scan advance would be if there were no caret (only calculated if the caret
	//	is present)
	int m_nDefaultAdvance;

	//	true if errors have been detected in this rule so don't keep processing
	bool m_fBadRule; 

};	// end of GdlRule



#endif // !GDL_RULE_INCLUDED
