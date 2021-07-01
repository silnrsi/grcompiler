/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GdlExpression.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Arithmetic and logical expressions that can appear in an GDL file.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef GDL_EXP_INCLUDED
#define GDL_EXP_INCLUDED

class GdlRenderer;
class GdlRule;
class GrcGlyphAttrMatrix;
class GrcLigComponentList;
class GdlGlyphClassDefn;
class GdlAttrValueSpec;


/*----------------------------------------------------------------------------------------------
Class: GdlExpression
Description: Abstract superclass representing the various kinds of expressions that can serve
	as the values of glyph attributes, slot attributes, etc.
Hungarian: exp
----------------------------------------------------------------------------------------------*/

class GdlExpression : public GdlObject
{
	friend class GdlUnaryExpression;
	friend class GdlBinaryExpression;
	friend class GdlCondExpression;
	friend class GdlLookupExpression;

public:
	//	Constructors:
	GdlExpression() : m_exptResult(kexptUnknown)
	{
	}

	//	copy constructor
	GdlExpression(const GdlExpression & exp)
		:	GdlObject(exp),
			m_exptResult(exp.m_exptResult)
	{
	}

	virtual GdlExpression * Clone() = 0;

	virtual ~GdlExpression()
	{
	}

protected:
	//	Initialization:
	virtual void SetType(ExpressionType exptResult)
	{
		if (m_exptResult == kexptBoolean && exptResult == kexptNumber)
			return;

		Assert(
			exptResult == m_exptResult ||
			m_exptResult == kexptUnknown ||
			(m_exptResult == kexptNumber &&
				exptResult == kexptBoolean));

		m_exptResult = exptResult;
	}

public:
	//	Parser:
	virtual void PropagateLineAndFile(GrpLineAndFile &) = 0;

public:
	//	Post-parser:
	virtual bool ReplaceAliases(GdlRule *) = 0;
	virtual bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *) = 0;
	virtual bool ResolveToInteger(int * pnRet, bool fSlotRef) = 0;
	virtual bool ResolveToFeatureID(unsigned int * pnRet);

public:
	//	Pre-compiler:
	virtual ExpressionType ExpType() = 0;
	bool TypeCheck(ExpressionType nExpectedType);
	bool TypeCheck(ExpressionType, ExpressionType, ExpressionType);
	bool TypeCheck(std::vector<ExpressionType>& vnExpectedTypes);
	virtual bool CheckTypeAndUnits(ExpressionType * pexpt) = 0;
	virtual void GlyphAttrCheck(Symbol psymAttr) = 0;
	virtual void FixFeatureTestsInRules(GrcFont *) = 0;
	virtual GdlExpression * ConvertFeatureSettingValue(GdlFeatureDefn * pfeat, bool & fErr) = 0;
	virtual void LookupExpCheck(bool fInIf, Symbol psymFeature) = 0;
	virtual GdlExpression * SimplifyAndUnscale(GrcGlyphAttrMatrix * pgax,
		utf16 wGlyphID, SymbolSet & setpsym, GrcFont * pfont,
		bool fGAttrDefChk, bool * pfCanSub) = 0;
	virtual GdlExpression * SimplifyAndUnscale(utf16 wGlyphID, GrcFont * pfont)
	{
		SymbolSet setpsym;
		bool fCanSub;
		return SimplifyAndUnscale(NULL, wGlyphID, setpsym, pfont, true, &fCanSub);
	}
	virtual void SetSpecialZero()
	{
	}
	virtual void CheckAndFixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit) = 0;
	virtual void CheckCompleteAttachmentPoint(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit,
		bool * pfXY, bool * pfGpoint) = 0;
	virtual bool PointFieldEquivalents(GrcManager * pcman,
		GdlExpression ** ppexpX, GdlExpression ** ppexpY,
		GdlExpression ** ppexpGpoint,
		GdlExpression ** ppexpXoffset, GdlExpression ** ppexpYoffset);
	virtual bool CheckRuleExpression(GrcFont * pfont, GdlRenderer * prndr,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
		bool fValue, bool fValueIsInputSlot) = 0;
	virtual void AdjustSlotRefsForPreAnys(int critPrependedAnys) = 0;
	virtual void AdjustToIOIndices(std::vector<int> & virit, GdlRuleItem *) = 0;
	virtual void MaxJustificationLevel(int * pnLevel) = 0;
	virtual bool TestsJustification() = 0;
	virtual bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded) = 0;
	virtual bool CheckAttachToLookup()
	{
		return true;	// only implemented for GdlBinaryExpression
	}


	//	Compiler:
	virtual void GenerateEngineCode(int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int irit, std::vector<int> * pviritInput, int nIIndex,
		bool fAttachAt, int iritAttachTo, int * pnValue) = 0;

	//	debuggers:
	virtual void PrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml,
		bool fParens = false) = 0;

private:
	//void operator=(GdlExpression);	// don't call the assignment operator

protected:
	//	Instance variables:
	ExpressionType m_exptResult;
};


/*----------------------------------------------------------------------------------------------
Class: GdlSimpleExpression
Description: Abstract superclass for expressions that have no expressions embedded, in case
	it becomes handy.
Hungarian: 
----------------------------------------------------------------------------------------------*/

class GdlSimpleExpression : public GdlExpression
{
public:
	//	Constructors & destructors:
	GdlSimpleExpression()
		:	GdlExpression()
	{}

///	virtual GdlExpression * Clone() { return new GdlSimpleExpression(); }

	virtual ~GdlSimpleExpression()
	{
	}

	//	copy constructor
	GdlSimpleExpression(const GdlSimpleExpression & exp)
		:	GdlExpression(exp)
	{
	}


public:
	//	Parser:
	virtual void PropagateLineAndFile(GrpLineAndFile &)
	{
	}

public:
	//	Post-parser:
	virtual bool ReplaceAliases(GdlRule *)
	{
		return true;
	}
	virtual bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *)
	{
		return true;
	}
	virtual bool ResolveToInteger(int * /*pnRet*/, bool /*fSlotRef*/)
	{
		return false;
	}

public:
	//	Pre-compiler:
	virtual ExpressionType ExpType()
	{
		return kexptUnknown;
	}
	
	virtual bool CheckTypeAndUnits(ExpressionType * pexpt)
	{
		*pexpt = kexptUnknown;
		return true;
	}

	virtual void GlyphAttrCheck(Symbol /*psymAttr*/) { }
	virtual void FixFeatureTestsInRules(GrcFont *) { }

	virtual GdlExpression * ConvertFeatureSettingValue(GdlFeatureDefn * /*pfeat*/, bool & /*fErr*/)
		{ return this; }

	virtual void LookupExpCheck(bool /*fInIf*/, Symbol /*psymFeature*/) { }

	virtual GdlExpression * SimplifyAndUnscale(GrcGlyphAttrMatrix * /*pgax*/,
		utf16 /*wGlyphID*/, SymbolSet & /*setpsym*/, GrcFont * /*pfont*/,
		bool /*fGAttrDefChk*/, bool * /*pfCanSub*/)
	{
		return this;
	}

	virtual void CheckAndFixGlyphAttrsInRules(GrcManager * /*pcman*/,
		std::vector<GdlGlyphClassDefn *> & /*vpglfcInClasses*/, int /*irit*/)
	{ }

	virtual void CheckCompleteAttachmentPoint(GrcManager * /*pcman*/,
		std::vector<GdlGlyphClassDefn *> & /*vpglfcInClasses*/, int /*irit*/,
		bool * /*pfXY*/, bool * /*pfGpoint*/)
	{ }

	virtual bool PointFieldEquivalents(GrcManager * /*pcman*/,
		GdlExpression ** /*ppexpX*/, GdlExpression ** /*ppexpY*/,
		GdlExpression ** /*ppexpGpoint*/,
		GdlExpression ** /*ppexpXoffset*/, GdlExpression ** /*ppexpYoffset*/)
	{
		return false;
	}

	virtual bool CheckRuleExpression(GrcFont * /*pfont*/, GdlRenderer * /*prndr*/,
		std::vector<bool> & /*vfLb*/, std::vector<bool> & /*vfIns*/, std::vector<bool> & /*vfDel*/,
		bool /*fValue*/, bool /*fValueIsInputSlot*/)
	{
		return true;
	}

	virtual void AdjustSlotRefsForPreAnys(int /*critPrependedAnys*/)
	{
	}

	virtual void AdjustToIOIndices(std::vector<int> & /*virit*/, GdlRuleItem *)
	{
	}

	virtual void MaxJustificationLevel(int * /*pnLevel*/)
	{
	}

	virtual bool TestsJustification()
	{
		return false;
	}

	virtual bool CompatibleWithVersion(int /*fxdVersion*/, int * /*pfxdNeeded*/, int * /*pfxdCpilrNeeded*/)
	{
		return true;
	}

	//	Compiler:
	virtual void GenerateEngineCode(int /*fxdRuleVersion*/, std::vector<gr::byte> & /*vbOutput*/,
		int /*irit*/, std::vector<int> * /*pviritInput*/, int /*nIIndex*/,
		bool /*fAttachAt*/, int /*iritAttachTo*/, int * /*pnValue*/)
	{
	}

	//	debuggers:
	virtual void PrettyPrint(GrcManager * /*pcman*/, std::ostream & strmOut, bool /*fXml*/,
		bool /*fParens*/ = false)
	{
		strmOut << "???";
	};
};


/*----------------------------------------------------------------------------------------------
Class: GdlNumericExpression
Description: Scaled or unscaled number
Hungarian: 
----------------------------------------------------------------------------------------------*/

class GdlNumericExpression : public GdlSimpleExpression
{
	friend class GdlUnaryExpression;
	friend class GdlBinaryExpression;
	friend class GdlCondExpression;
	friend class GdlLookupExpression;

public:
	//	Constructors & destructors:
	GdlNumericExpression(int nValue)
		:	GdlSimpleExpression(),
			m_nValue(nValue),
			m_munits(kmunitNone),
			m_fBoolean(false)
	{
		SetType(kexptNumber);
	}
	GdlNumericExpression(int nValue, int munits)
		:	GdlSimpleExpression(),
			m_nValue(nValue),
			m_munits(munits),
			m_fBoolean(false)
	{
		if (m_munits == kmunitNone)
			SetType(kexptNumber);
		else
			SetType(kexptMeas);
	}
	GdlNumericExpression(int nValue, bool fBool)
		:	GdlSimpleExpression(),
			m_nValue(nValue),
			m_munits(kmunitNone),
			m_fBoolean(fBool)
	{
		SetType(kexptBoolean);
	}

	//	copy constructor
	GdlNumericExpression(const GdlNumericExpression & exp)
		:	GdlSimpleExpression(exp),
			m_nValue(exp.m_nValue),
			m_munits(exp.m_munits),
			m_fBoolean(exp.m_fBoolean)
	{}

	virtual GdlExpression * Clone()
	{
		return new GdlNumericExpression(*this);
	}

	//	Getters:
	int Value()	{ return m_nValue; }
	int	Units()	{ return m_munits; }
	bool IsBoolean() { return m_fBoolean; }

public:
	//	Parser:
	virtual void PropagateLineAndFile(GrpLineAndFile &);

public:
	//	Post-parser:
	virtual bool ReplaceAliases(GdlRule *);
	virtual bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *);
	virtual bool ResolveToInteger(int * pnRet, bool fSlotRef);

public:
	//	Pre-compiler:
	virtual ExpressionType ExpType();
	virtual bool CheckTypeAndUnits(ExpressionType * pexpt);
	virtual void GlyphAttrCheck(Symbol psymAttr);
	virtual void FixFeatureTestsInRules(GrcFont *);
	virtual GdlExpression * ConvertFeatureSettingValue(GdlFeatureDefn * pfeat, bool & fErr);
	virtual void LookupExpCheck(bool fInIf, Symbol psymFeature);
	virtual GdlExpression * SimplifyAndUnscale(GrcGlyphAttrMatrix * pgax,
		utf16 wGlyphID, SymbolSet & setpsym, GrcFont * pfont,
		bool fGAttrDefChk, bool * pfCanSub);
	virtual void SetSpecialZero();
	virtual void CheckAndFixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit);
	virtual void CheckCompleteAttachmentPoint(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit,
		bool * pfXY, bool * pfGpoint);
	virtual bool CheckRuleExpression(GrcFont * pfont, GdlRenderer * prndr,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
		bool fValue, bool fValueIsInputSlot);
	virtual void AdjustSlotRefsForPreAnys(int critPrependedAnys);
	virtual void AdjustToIOIndices(std::vector<int> & virit, GdlRuleItem *);
	virtual void MaxJustificationLevel(int * pnLevel);
	virtual bool TestsJustification();
	virtual bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);

	//	Compiler:
	virtual void GenerateEngineCode(int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int irit, std::vector<int> * pviritInput, int nIIndex,
		bool fAttachAt, int iritAttachTo, int * pnValue);
	void SetValue(int nValue) { m_nValue = nValue; }

	//	debuggers:
	virtual void PrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml,
		bool fParens = false);

protected:
	//	Instance variables:
	int m_nValue;
	int m_munits;		// Scaling in effect when expression was encountered, or kmunitNone
	bool m_fBoolean;	// was originally indicated as 'true' or 'false
};


/*----------------------------------------------------------------------------------------------
Class: GdlSlotRefExpression
Description: Number or alias that should be interpreted as a slot in the rule;
	eg, @2, @vowel
Hungarian: 
----------------------------------------------------------------------------------------------*/

class GdlSlotRefExpression : public GdlSimpleExpression
{
	friend class GdlUnaryExpression;
	friend class GdlBinaryExpression;
	friend class GdlCondExpression;
	friend class GdlLookupExpression;

public:
	//	Constructors & destructors:
	GdlSlotRefExpression(int sr)
		:	GdlSimpleExpression(),
			m_srNumber(sr)
	{
		SetType(kexptSlotRef);
	}
	GdlSlotRefExpression(std::string sta)
		:	GdlSimpleExpression(),
			m_srNumber(-1),
			m_staName(sta)
	{
		SetType(kexptSlotRef);
	}

	//	copy constructor
	GdlSlotRefExpression(const GdlSlotRefExpression & exp)
		:	GdlSimpleExpression(exp),
			m_srNumber(exp.m_srNumber),
			m_staName(exp.m_staName),
			m_nIOIndex(exp.m_nIOIndex)
	{
	}

	virtual GdlExpression * Clone()
	{
		return new GdlSlotRefExpression(*this);
	}

	virtual ~GdlSlotRefExpression()
	{
	}

public:
	//	General:
	int SlotNumber()
	{
		return m_srNumber;
	}
	std::string Alias()
	{
		return m_staName;
	}

public:
	//	Parser:
	virtual void PropagateLineAndFile(GrpLineAndFile &);

public:
	//	Post-parser:
	virtual bool ReplaceAliases(GdlRule *);
	virtual bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *);
	virtual bool ResolveToInteger(int * pnRet, bool fSlotRef);

public:
	//	Pre-compiler:
	virtual ExpressionType ExpType();
	virtual bool CheckTypeAndUnits(ExpressionType * pexpt);
	virtual void GlyphAttrCheck(Symbol psymAttr);
	virtual void FixFeatureTestsInRules(GrcFont *);
	virtual GdlExpression * ConvertFeatureSettingValue(GdlFeatureDefn * pfeat, bool & fErr);
	virtual void LookupExpCheck(bool fInIf, Symbol psymFeature);
	virtual GdlExpression * SimplifyAndUnscale(GrcGlyphAttrMatrix * pgax,
		utf16 wGlyphID, SymbolSet & setpsym, GrcFont * pfont,
		bool fGAttrDefChk, bool * pfCanSub);
	virtual void CheckAndFixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit);
	virtual void CheckCompleteAttachmentPoint(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit,
		bool * pfXY, bool * pfGpoint);
	virtual bool CheckRuleExpression(GrcFont * pfont, GdlRenderer * prndr,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
		bool fValue, bool fValueIsInputSlot);
	virtual void AdjustSlotRefsForPreAnys(int critPrependedAnys);
	virtual void AdjustToIOIndices(std::vector<int> & virit, GdlRuleItem *);
	virtual void MaxJustificationLevel(int * pnLevel);
	virtual bool TestsJustification();
	virtual bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);

	//	Compiler:
	virtual void GenerateEngineCode(int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int irit, std::vector<int> * pviritInput, int nIIndex,
		bool fAttachAt, int iritAttachTo, int * pnValue);

	int AdjustedIndex()
	{
		return m_nIOIndex;
	}

	//	debuggers:
	virtual void PrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml,
		bool fParens = false);

protected:
	//	Instance variables:
	//	either the number or the name is used, not both
	int m_srNumber;	// 1-based
	std::string	m_staName;

	//	for compiler use:
	int m_nIOIndex;		// adjusted input index or output index (which ever is relevant
						// for the context) - 0-based
};


/*----------------------------------------------------------------------------------------------
Class: GdlStringExpression
Description: An GDL string function.
Hungarian: 
----------------------------------------------------------------------------------------------*/

class GdlStringExpression : public GdlSimpleExpression
{
	friend class GdlUnaryExpression;
	friend class GdlBinaryExpression;
	friend class GdlCondExpression;
	friend class GdlLookupExpression;

public:
	//	Constructors & destructors:
	GdlStringExpression(std::string sta, int nCodepage)
		:	GdlSimpleExpression(),
			m_staValue(sta),
			m_nCodepage(nCodepage)
	{}

	//	copy constructor
	GdlStringExpression(const GdlStringExpression & exp)
		:	GdlSimpleExpression(exp),
			m_staValue(exp.m_staValue),
			m_nCodepage(exp.m_nCodepage)
	{}

	virtual GdlExpression * Clone()
	{
		return new GdlStringExpression(*this);
	}

public:
	//	Parser:
	virtual void PropagateLineAndFile(GrpLineAndFile &);

public:
	//	Post-parser:
	virtual bool ReplaceAliases(GdlRule *);
	virtual bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *);
	virtual bool ResolveToInteger(int * pnRet, bool fSlotRef);
	virtual bool ResolveToFeatureID(unsigned int *pnRet);

	std::wstring ConvertToUnicode();

public:
	//	Pre-compiler:
	virtual ExpressionType ExpType();
	virtual bool CheckTypeAndUnits(ExpressionType * pexpt);
	virtual void GlyphAttrCheck(Symbol psymAttr);
	virtual void FixFeatureTestsInRules(GrcFont *);
	virtual GdlExpression * ConvertFeatureSettingValue(GdlFeatureDefn * pfeat, bool & fErr);
	virtual void LookupExpCheck(bool fInIf, Symbol psymFeature);
	virtual GdlExpression * SimplifyAndUnscale(GrcGlyphAttrMatrix * pgax,
		utf16 wGlyphID, SymbolSet & setpsym, GrcFont * pfont,
		bool fGAttrDefChk, bool * pfCanSub);
	virtual void CheckAndFixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit);
	virtual void CheckCompleteAttachmentPoint(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit,
		bool * pfXY, bool * pfGpoint);
	virtual bool CheckRuleExpression(GrcFont * pfont, GdlRenderer * prndr,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
		bool fValue, bool fValueIsInputSlot);
	virtual void AdjustSlotRefsForPreAnys(int critPrependedAnys);
	virtual void AdjustToIOIndices(std::vector<int> & virit, GdlRuleItem *);
	virtual void MaxJustificationLevel(int * pnLevel);
	virtual bool TestsJustification();
	virtual bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);

	std::string StringValue() { return m_staValue; }

	//	Compiler:
	virtual void GenerateEngineCode(int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int irit, std::vector<int> * pviritInput, int nIIndex,
		bool fAttachAt, int iritAttachTo, int * pnValue);

	//	debuggers:
	virtual void PrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml,
		bool fParens = false);

protected:
	//	Instance variables:
	std::string m_staValue;
	int m_nCodepage;
};

/*----------------------------------------------------------------------------------------------
Class: GdlUnaryExpression
Description: Unary expression, for example, -(a + b), !boolean.
Hungarian: 
----------------------------------------------------------------------------------------------*/

class GdlUnaryExpression : public GdlExpression
{
	friend class GdlBinaryExpression;
	friend class GdlCondExpression;
	friend class GdlLookupExpression;

public:
	//	Constructors & destructors:
	GdlUnaryExpression(Symbol psymOperator, GdlExpression * pexpOperand)
		:	GdlExpression(),
			m_psymOperator(psymOperator),
			m_pexpOperand(pexpOperand)
	{
		if (m_pexpOperand && m_pexpOperand->LineIsZero())
			m_pexpOperand->PropagateLineAndFile(m_lnf);
	}

	//	copy constructor
	GdlUnaryExpression(const GdlUnaryExpression & exp)
		:	GdlExpression(exp),
			m_psymOperator(exp.m_psymOperator),
			m_pexpOperand(exp.m_pexpOperand->Clone())
	{
	}

	virtual GdlExpression * Clone()
	{
		return new GdlUnaryExpression(*this);
	}

	virtual ~GdlUnaryExpression()
	{
		delete m_pexpOperand;
	}

	//	Getters:
	Symbol Operator()			{ return m_psymOperator; }
	GdlExpression* Operand()	{ return m_pexpOperand; }

public:
	//	Parser:
	virtual void PropagateLineAndFile(GrpLineAndFile &);

public:
	//	Post-parser:
	virtual bool ReplaceAliases(GdlRule *);
	virtual bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *);
	virtual bool ResolveToInteger(int * pnRet, bool fSlotRef);

public:
	//	Pre-compiler:
	virtual ExpressionType ExpType();
	virtual bool CheckTypeAndUnits(ExpressionType * pexpt);
	virtual void GlyphAttrCheck(Symbol psymAttr);
	virtual void FixFeatureTestsInRules(GrcFont *);
	virtual GdlExpression * ConvertFeatureSettingValue(GdlFeatureDefn * pfeat, bool & fErr);
	virtual void LookupExpCheck(bool fInIf, Symbol psymFeature);
	virtual GdlExpression * SimplifyAndUnscale(GrcGlyphAttrMatrix * pgax,
		utf16 wGlyphID, SymbolSet & setpsym, GrcFont * pfont,
		bool fGAttrDefChk, bool * pfCanSub);
	virtual void CheckAndFixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit);
	virtual void CheckCompleteAttachmentPoint(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit,
		bool * pfXY, bool * pfGpoint);
	virtual bool CheckRuleExpression(GrcFont * pfont, GdlRenderer * prndr,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
		bool fValue, bool fValueIsInputSlot);
	virtual void AdjustSlotRefsForPreAnys(int critPrependedAnys);
	virtual void AdjustToIOIndices(std::vector<int> & virit, GdlRuleItem *);
	virtual void MaxJustificationLevel(int * pnLevel);
	virtual bool TestsJustification();
	virtual bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);

	//	Compiler:
	virtual void GenerateEngineCode(int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int irit, std::vector<int> * pviritInput, int nIIndex,
		bool fAttachAt, int iritAttachTo, int * pnValue);

	//	debuggers:
	virtual void PrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml,
		bool fParens = false);

protected:
	//	Instance variables:
	Symbol			m_psymOperator;
	GdlExpression *	m_pexpOperand;
};


/*----------------------------------------------------------------------------------------------
Class: GdlBinaryExpression
Description: Binary expression or function, for example, a + b, min(x,y).
Hungarian: 
----------------------------------------------------------------------------------------------*/

class GdlBinaryExpression : public GdlExpression
{
	friend class GdlUnaryExpression;
	friend class GdlCondExpression;
	friend class GdlLookupExpression;

public:
	//	Constructors & destructors:
	GdlBinaryExpression(Symbol psymOperator, GdlExpression * pexpOp1, GdlExpression * pexpOp2)
		:	GdlExpression(),
			m_psymOperator(psymOperator),
			m_pexpOperand1(pexpOp1),
			m_pexpOperand2(pexpOp2)
	{
		if (m_pexpOperand1 && m_pexpOperand1->LineIsZero())
			m_pexpOperand1->PropagateLineAndFile(m_lnf);
		if (m_pexpOperand2 && m_pexpOperand2->LineIsZero())
			m_pexpOperand2->PropagateLineAndFile(m_lnf);
	}

	//	copy constructor
	GdlBinaryExpression(const GdlBinaryExpression & exp)
		:	GdlExpression(exp),
			m_psymOperator(exp.m_psymOperator),
			m_pexpOperand1(exp.m_pexpOperand1->Clone()),
			m_pexpOperand2(exp.m_pexpOperand2->Clone())
	{
	}

	virtual GdlExpression * Clone()
	{
		return new GdlBinaryExpression(*this);
	}

	virtual ~GdlBinaryExpression()
	{
		delete m_pexpOperand1;
		delete m_pexpOperand2;
	}

	//	Getters:
	Symbol	Operator()			{ return m_psymOperator; }
	GdlExpression*	Operand1()	{ return m_pexpOperand1; }
	GdlExpression*	Operand2()	{ return m_pexpOperand2; }

public:
	//	Parser:
	virtual void PropagateLineAndFile(GrpLineAndFile &);

public:
	//	Post-parser:
	virtual bool ReplaceAliases(GdlRule *);
	virtual bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *);
	virtual bool ResolveToInteger(int * pnRet, bool fSlotRef);

public:
	//	Pre-compiler:
	virtual ExpressionType ExpType();
	virtual bool CheckTypeAndUnits(ExpressionType * pexpt);
	virtual void GlyphAttrCheck(Symbol psymAttr);
	virtual void FixFeatureTestsInRules(GrcFont *);
	virtual GdlExpression * ConvertFeatureSettingValue(GdlFeatureDefn * pfeat, bool & fErr);
	virtual void LookupExpCheck(bool fInIf, Symbol psymFeature);
	virtual GdlExpression * SimplifyAndUnscale(GrcGlyphAttrMatrix * pgax,
		utf16 wGlyphID, SymbolSet & setpsym, GrcFont * pfont,
		bool fGAttrDefChk, bool * pfCanSub);
	virtual void CheckAndFixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit);
	virtual void CheckCompleteAttachmentPoint(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit,
		bool * pfXY, bool * pfGpoint);
	virtual bool CheckRuleExpression(GrcFont * pfont, GdlRenderer * prndr,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
		bool fValue, bool fValueIsInputSlot);
	virtual void AdjustSlotRefsForPreAnys(int critPrependedAnys);
	virtual void AdjustToIOIndices(std::vector<int> & virit, GdlRuleItem *);
	virtual void MaxJustificationLevel(int * pnLevel);
	virtual bool TestsJustification();
	virtual bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);
	virtual bool CheckAttachToLookup();

	//	Compiler:
	virtual void GenerateEngineCode(int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int irit, std::vector<int> * pviritInput, int nIIndex,
		bool fAttachAt, int iritAttachTo, int * pnValue);
	bool GenerateSetBitsOp(int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int iritCurrent, std::vector<int> * pviritInput, int nIIndex,
		bool fAttachAt, int iritAttachTo);

	//	debuggers:
	virtual void PrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml,
		bool fParens = false);

protected:
	//	Instance variables:
	Symbol			m_psymOperator;
	GdlExpression*	m_pexpOperand1;
	GdlExpression*	m_pexpOperand2;
};


/*----------------------------------------------------------------------------------------------
Class: GdlCondExpression
Description: Conditional expression, eg, (test)? true_value: false_value
Hungarian: 
----------------------------------------------------------------------------------------------*/

class GdlCondExpression : public GdlExpression
{
	friend class GdlUnaryExpression;
	friend class GdlBinaryExpression;
	friend class GdlLookupExpression;

public:
	//	Constructors & destructors:
	GdlCondExpression(
		GdlExpression* pexpTest,
		GdlExpression * pexpTrue,
		GdlExpression * pexpFalse)
		:	GdlExpression(),
			m_pexpTest(pexpTest),
			m_pexpTrue(pexpTrue),
			m_pexpFalse(pexpFalse)
	{
//		m_pexpTest->SetType(kexptBoolean);
		if (m_pexpTest && m_pexpTest->LineIsZero())
			m_pexpTest->PropagateLineAndFile(m_lnf);
		if (m_pexpTrue && m_pexpTrue->LineIsZero())
			m_pexpTrue->PropagateLineAndFile(m_lnf);
		if (m_pexpFalse && m_pexpFalse->LineIsZero())
			m_pexpFalse->PropagateLineAndFile(m_lnf);
	}

	//	copy constructor
	GdlCondExpression(const GdlCondExpression & exp)
		:	GdlExpression(exp),
			m_pexpTest(exp.m_pexpTest->Clone()),
			m_pexpTrue(exp.m_pexpTrue->Clone()),
			m_pexpFalse(exp.m_pexpFalse->Clone())
	{
	}

	virtual GdlExpression * Clone()
	{
		return new GdlCondExpression(*this);
	}

	virtual ~GdlCondExpression()
	{
		delete m_pexpTest;
		delete m_pexpTrue;
		delete m_pexpFalse;
	}

	//	Getters:
	GdlExpression*	Test()		{ return m_pexpTest; }
	GdlExpression*	TrueExp()	{ return m_pexpTrue; }
	GdlExpression*	FalseExp()	{ return m_pexpFalse; }

public:
	//	Parser:
	virtual void PropagateLineAndFile(GrpLineAndFile &);

public:
	//	Post-parser:
	virtual bool ReplaceAliases(GdlRule *);
	virtual bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *);
	virtual bool ResolveToInteger(int * pnRet, bool fSlotRef);

public:
	//	Pre-compiler:
	virtual ExpressionType ExpType();
	virtual bool CheckTypeAndUnits(ExpressionType * pexpt);
	virtual void GlyphAttrCheck(Symbol psymAttr);
	virtual void FixFeatureTestsInRules(GrcFont *);
	virtual GdlExpression * ConvertFeatureSettingValue(GdlFeatureDefn * pfeat, bool & fErr);
	virtual void LookupExpCheck(bool fInIf, Symbol psymFeature);
	virtual GdlExpression * SimplifyAndUnscale(GrcGlyphAttrMatrix * pgax,
		utf16 wGlyphID, SymbolSet & setpsym, GrcFont * pfont,
		bool fGAttrDefChk, bool * pfCanSub);
	virtual void CheckAndFixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit);
	virtual void CheckCompleteAttachmentPoint(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit,
		bool * pfXY, bool * pfGpoint);
	virtual bool CheckRuleExpression(GrcFont * pfont, GdlRenderer * prndr,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
		bool fValue, bool fValueIsInputSlot);
	virtual void AdjustSlotRefsForPreAnys(int critPrependedAnys);
	virtual void AdjustToIOIndices(std::vector<int> & virit, GdlRuleItem *);
	virtual void MaxJustificationLevel(int * pnLevel);
	virtual bool TestsJustification();
	virtual bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);

	//	Compiler:
	virtual void GenerateEngineCode(int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int irit, std::vector<int> * pviritInput, int nIIndex,
		bool fAttachAt, int iritAttachTo, int * pnValue);

	//	debuggers:
	virtual void PrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml,
		bool fParens = false);

protected:
	//	Instance variables:
	GdlExpression*	m_pexpTest;
	GdlExpression*	m_pexpTrue;
	GdlExpression*	m_pexpFalse;
};


/*----------------------------------------------------------------------------------------------
Class: GdlLookupExpression
Description: Expression to look up the value of a slot or glyph attribute, eg, linebreak,
	BoundingBox.Left.2, @3.Advance.Width
Hungarian: 
----------------------------------------------------------------------------------------------*/

class GdlLookupExpression : public GdlExpression
{
	friend class GdlUnaryExpression;
	friend class GdlBinaryExpression;
	friend class GdlCondExpression;

public:
	//	name space in which to do look-up:
//	enum LookupType {
//		klookUnknown,
//		klookGlyph,
//		klookSlot,
//		klookFeature
//	};

	//	Constructors & destructors:
	GdlLookupExpression(Symbol psymName, int nSel, int nClus)
		:	GdlExpression(),
			m_psymName(psymName),
			m_nClusterLevel(nClus),
			m_pexpSimplified(NULL)
	{
		if (nSel > -1)
		{
			m_pexpSelector = new GdlSlotRefExpression(nSel);
			m_pexpSelector->PropagateLineAndFile(m_lnf);
		}
		else
			m_pexpSelector = NULL;
	}

	GdlLookupExpression(Symbol psymName, std::string staSel, int nClus)
		:	GdlExpression(),
			m_psymName(psymName),
			m_nClusterLevel(nClus),
			m_pexpSimplified(NULL)
	{
		m_pexpSelector = new GdlSlotRefExpression(staSel);
		m_pexpSelector->PropagateLineAndFile(m_lnf);
	}

	GdlLookupExpression(Symbol psymName, GdlSlotRefExpression * pexpSel, int nClus)
		:	GdlExpression(),
			m_psymName(psymName),
			m_pexpSelector(pexpSel),
			m_nClusterLevel(nClus),
			m_pexpSimplified(NULL)
	{
	}

	GdlLookupExpression(Symbol psymName, int nSel)
		:	GdlExpression(),
			m_psymName(psymName),
			m_nClusterLevel(0),
			m_pexpSimplified(NULL),
			m_fGlyphAttr(false)
	{
		m_pexpSelector = new GdlSlotRefExpression(nSel);
		m_pexpSelector->PropagateLineAndFile(m_lnf);
	}

	GdlLookupExpression(Symbol psymName, std::string staSel)
		:	GdlExpression(),
			m_psymName(psymName),
			m_nClusterLevel(0),
			m_pexpSimplified(NULL),
			m_fGlyphAttr(false)
	{
		m_pexpSelector = new GdlSlotRefExpression(staSel);
		m_pexpSelector->PropagateLineAndFile(m_lnf);
	}

	GdlLookupExpression(Symbol psymName)
		:	GdlExpression(),
			m_psymName(psymName),
			m_pexpSelector(NULL),
			m_nClusterLevel(0),
			m_pexpSimplified(NULL),
			m_fGlyphAttr(false)
	{
	}

	//	copy constructor
	GdlLookupExpression(const GdlLookupExpression& exp)
		:	GdlExpression(exp),
			m_psymName(exp.m_psymName),
			m_nClusterLevel(exp.m_nClusterLevel),
			m_fGlyphAttr(exp.m_fGlyphAttr),
			m_nInternalID(exp.m_nInternalID),
			m_nSubIndex(exp.m_nSubIndex)
	{
		m_pexpSelector =
			(exp.m_pexpSelector) ?
				new GdlSlotRefExpression(*exp.m_pexpSelector) :
				NULL;
		m_pexpSimplified =
			(exp.m_pexpSimplified) ?
				new GdlNumericExpression(*exp.m_pexpSimplified) :
				NULL;
	}

	virtual GdlExpression * Clone()
	{
		return new GdlLookupExpression(*this);
	}

	virtual ~GdlLookupExpression()
	{
		if (m_pexpSelector)
			delete m_pexpSelector;
		if (m_pexpSimplified)
			delete m_pexpSimplified;
	}

	//	Getters:
	Symbol Name()				{ return m_psymName; }

	GdlSlotRefExpression* Selector()
	{
		return m_pexpSelector;
	}

	bool NameFitsSymbolType(SymbolType symt)
	{
		return m_psymName->FitsSymbolType(symt);
	}

	//	Setters:
	void SetGlyphAttr(bool f = true) { m_fGlyphAttr = f; }

public:
	//	Parser:
	virtual void PropagateLineAndFile(GrpLineAndFile &);

public:
	//	Post-parser:
	virtual bool ReplaceAliases(GdlRule *);
	virtual bool AdjustSlotRefs(std::vector<bool> &, std::vector<int> &, GdlRule *);
	virtual bool ResolveToInteger(int * pnRet, bool fSlotRef);

public:
	//	Pre-compiler:
	virtual ExpressionType ExpType();
	virtual bool CheckTypeAndUnits(ExpressionType * pexpt);
	virtual void GlyphAttrCheck(Symbol psymAttr);
	virtual void FixFeatureTestsInRules(GrcFont *);
	virtual GdlExpression * ConvertFeatureSettingValue(GdlFeatureDefn * pfeat, bool & fErr);
	virtual void LookupExpCheck(bool fInIf, Symbol psymFeature);
	virtual GdlExpression * SimplifyAndUnscale(GrcGlyphAttrMatrix * pgax,
		utf16 wGlyphID, SymbolSet & setpsym, GrcFont * pfont,
		bool fGAttrDefChk, bool * pfCanSub);
	virtual void CheckAndFixGlyphAttrsInRules(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit);
	virtual void CheckCompleteAttachmentPoint(GrcManager * pcman,
		std::vector<GdlGlyphClassDefn *> & vpglfcInClasses, int irit,
		bool * pfXY, bool * pfGpoint);
	virtual bool PointFieldEquivalents(GrcManager * pcman,
		GdlExpression ** ppexpX, GdlExpression ** ppexpY,
		GdlExpression ** ppexpGpoint,
		GdlExpression ** ppexpXoffset, GdlExpression ** ppexpYoffset);
	virtual bool CheckRuleExpression(GrcFont * pfont, GdlRenderer * prndr,
		std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
		bool fValue, bool fValueIsInputSlot);
	virtual void AdjustSlotRefsForPreAnys(int critPrependedAnys);
	virtual void AdjustToIOIndices(std::vector<int> & virit, GdlRuleItem *);
	virtual void MaxJustificationLevel(int * pnLevel);
	virtual bool TestsJustification();
	virtual bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);

	//	Compiler:
	virtual void GenerateEngineCode(int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int irit, std::vector<int> * pviritInput, int nIIndex,
		bool fAttachAt, int iritAttachTo, int * pnValue);

	//	debuggers:
	virtual void PrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml,
		bool fParens = false);

protected:
	//	Instance variables:
	Symbol					m_psymName;
	GdlSlotRefExpression *	m_pexpSelector;	// 1-based
	int						m_nClusterLevel;

//	LookupType				m_lookType;	// glyph attr, slot attr, feature

	GdlNumericExpression *	m_pexpSimplified;

	bool					m_fGlyphAttr;

	//	for compiler use:
	int m_nInternalID;
	int m_nSubIndex;	//	for indexed attributes (component.X.ref)
};


/*----------------------------------------------------------------------------------------------
Class: GdlClassMemberExpression
Description: Expression to look up the a glyph, with an index into a class.
	Only used within glyph attribute definitions.
Hungarian: expil
----------------------------------------------------------------------------------------------*/

class GdlClassMemberExpression : public GdlLookupExpression
{
public:
	//	Constructors & destructors:
	GdlClassMemberExpression(Symbol psymName)
		:	GdlLookupExpression(psymName)
	{
		m_igid = -1;
		m_cgidClassSize = -1;
		m_gid = -1;
	}
	GdlClassMemberExpression(int gid)	// used for setting defaults
		:	GdlLookupExpression(NULL)
	{
		m_igid = -1;
		m_cgidClassSize = -1;
		m_gid = gid;
	}
	//	copy constructor
	GdlClassMemberExpression(const GdlClassMemberExpression& exp)
		:	GdlLookupExpression(exp),
			m_igid(exp.m_igid),
			m_cgidClassSize(exp.m_cgidClassSize),
			m_gid(exp.m_gid)
	{
	}

	virtual GdlExpression * Clone()
	{
		return new GdlClassMemberExpression(*this);
	}

public:
	// Pre-compiler:
	virtual void GlyphAttrCheck(Symbol psymAttr);
	virtual GdlExpression * SimplifyAndUnscale(GrcGlyphAttrMatrix * pgax,
		utf16 wGlyphID, SymbolSet & setpsym, GrcFont * pfont,
		bool fGAttrDefChk, bool * pfCanSub);
	virtual bool CheckTypeAndUnits(ExpressionType * pexptRet);
	bool ResolveToInteger(int * pnRet, bool fSlotRef);

	virtual bool PointFieldEquivalents(GrcManager * pcman,
		GdlExpression ** ppexpX, GdlExpression ** ppexpY,
		GdlExpression ** ppexpGpoint,
		GdlExpression ** ppexpXoffset, GdlExpression ** ppexpYoffset);

	int ValueCount();

	// Compiler:
	virtual bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);
	virtual void GenerateEngineCode(int fxdRuleVersion, std::vector<gr::byte> & vbOutput,
		int iritCurrent, std::vector<int> * pviritInput, int nIIndex,
		bool fAttachAt, int iritAttachTo, int * pnValue);

	void SetClassSize(int cgid)
	{
		m_cgidClassSize = cgid;
	}
	void SetGlyphIndex(int igid)
	{
		m_igid = igid;
	}
	int GlyphIndex() { return m_igid; }


protected:
	//	Instance variables:
	int m_igid;				// index of glyph within defining class
	int m_cgidClassSize;	// of defining class
	int m_gid;				// resolved glyphid value

};

#endif // !GDL_EXP_INCLUDED
