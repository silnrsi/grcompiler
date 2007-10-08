/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GdlFeatures.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Classes to implement the features mechanism.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef FEATURES_INCLUDED
#define FEATURES_INCLUDED

#define LG_USENG (1033)
/*----------------------------------------------------------------------------------------------
Class: GdlExtName
Description: External name, that is, in natural language, suitable for display in
	a user-interface. Used by GdlFeatureDefn and GdlNameDefn
Hungarian: extname
----------------------------------------------------------------------------------------------*/

class GdlExtName : public GdlObject
{
public:
	//	Constructors:
	GdlExtName()
		:	m_wLanguageID(0)
	{
	}

	GdlExtName(StrUni stu, utf16 wLangID)
		:	m_stuName(stu),
			m_wLanguageID(wLangID)
	{
	}

	//	Getters:
	StrUni Name()		{ return m_stuName; }
	utf16 LanguageID()	{ return m_wLanguageID; }

	static StrUni s_stuNoName;

protected:
	//	Instance variables:
	StrUni		m_stuName;
	utf16		m_wLanguageID;
};

/*----------------------------------------------------------------------------------------------
Class: GdlFeatureSetting
Description: The information for a single feature setting.
Hungarian: fset
----------------------------------------------------------------------------------------------*/

class GdlFeatureSetting : public GdlObject
{
	friend class GdlFeatureDefn;

public:
	//	Constructor:
	GdlFeatureSetting()
	{
		m_nValue = 0;
		m_fHasValue = false;
	}

	//	Getters:
	int Value()				{ return m_nValue; }
	StrAnsi Name()			{ return m_staName; }
	utf16 NameTblId()		{ return m_wNameTblId; }

	//	Setters:
	void SetName(StrAnsi sta)		{ m_staName = sta; }
	void SetValue(int n)			{ m_nValue = n; m_fHasValue = true; }
	void AddExtName(utf16 wLangID, StrUni stu)
	{
		m_vextname.Push(GdlExtName(stu, wLangID));
	}
	void AddExtName(utf16 wLangID, GdlExpression * pexp)
	{
		GdlStringExpression * pexpString = dynamic_cast<GdlStringExpression*>(pexp);
		Assert(pexpString);

		StrUni stu;
		stu = pexpString->ConvertToUnicode();
		m_vextname.Push(GdlExtName(stu, wLangID));
	}
	void SetNameTblId(utf16 w)		{ m_wNameTblId = w; }

protected:
	//	Instance variables:
	StrAnsi				m_staName;
	Vector<GdlExtName>	m_vextname;
	int					m_nValue;
	utf16				m_wNameTblId;
	bool				m_fHasValue;
};



/*----------------------------------------------------------------------------------------------
Class: GdlFeatureDefn
Description: A feature and its associated information
Hungarian: feat
----------------------------------------------------------------------------------------------*/

class GdlFeatureDefn : public GdlDefn
{
	friend class GdlFeatureSetting;

public:
	enum {
		kfidStdStyles = 0,	// whatever
		kfidStdLang = 1,
	};

//	enum {	// standard style values
//		kstvPlain		= 0,
//		kstvBold		= 1,
//		kstvItalic		= 2,
//		kstvUnderscore	= 4,
//		kstvStrikeThru	= 8,
//		kstvCompressed	=16
//	};
//	enum { kstvLim = kstvPlain };	// for now, we don't support multiple styles

	//	Constructors & destructors:
	GdlFeatureDefn()
	{
		m_fStdStyles = false;
		m_fStdLang = false;
		m_fIDSet = false;
		m_fDefaultSet = false;
		m_fFatalError = false;
		m_pfsetDefault = NULL;
		m_wNameTblId = 0xFFFF;
	}

	~GdlFeatureDefn()
	{
		for (int i = 0; i < m_vpfset.Size(); ++i)
			delete m_vpfset[i];
	}

	//	Setters:
	void SetName(StrAnsi sta)	{ m_staName = sta; }
	void SetID(int n)			{ m_nID = n; m_fIDSet = true; }
	void SetDefault(int n)		{ m_nDefault = n; m_fDefaultSet = true; }
	void AddExtName(utf16 wLangID, StrUni stu)
	{
		m_vextname.Push(GdlExtName(stu, wLangID));
	}
	void AddExtName(utf16 wLangID, GdlExpression * pexp)
	{
		GdlStringExpression * pexpString = dynamic_cast<GdlStringExpression*>(pexp);
		Assert(pexpString);

		StrUni stu = pexpString->ConvertToUnicode();
		m_vextname.Push(GdlExtName(stu, wLangID));
	}
	void MarkAsLanguageFeature()
	{
		m_fStdLang = true;
		m_nID = kfidStdLang;
	}
	utf16 SetNameTblIds(utf16 wFirst); // return next id to use

	//	Getters:
	StrAnsi Name()
	{
		return m_staName;
	}
	int NumberOfSettings()
	{
		return m_vpfset.Size();
	}
	int ID()
	{
		return m_nID;
	}
	bool IsLanguageFeature()
	{
		return m_fStdLang;
	}
	utf16 NameTblId()	{ return m_wNameTblId; }
	bool NameTblInfo(Vector<StrUni> * pvstuExtNames, Vector<utf16> * pvwLangIds, 
		Vector<utf16> * pvwNameTblIds);


	GdlFeatureSetting * FindSetting(StrAnsi sta);
	GdlFeatureSetting * FindOrAddSetting(StrAnsi, GrpLineAndFile & lnf);
	GdlFeatureSetting * FindSettingWithValue(int n);

public:
	//	Pre-compiler:
	bool ErrorCheck();
	void SetStdStyleFlag();
	void FillInBoolean();
	void ErrorCheckContd();
	void CalculateDefault();
	void AssignInternalID(int n)
	{
		m_nInternalID = n;
	}
	void RecordDebugInfo();

	//	Compiler
	int InternalID()
	{
		return m_nInternalID;
	}

	void OutputSettings(GrcBinaryStream * pbstrm);

protected:
	//	Instance variables:
	StrAnsi						m_staName;
	int							m_nID;
	Vector<GdlExtName>			m_vextname;
	Vector<GdlFeatureSetting *>	m_vpfset;
	int					 		m_nDefault;

	bool m_fIDSet;
	bool m_fDefaultSet;		// if still false at the end, we need to
							// figure out what the default should be

	bool m_fStdStyles;		// true for the one feature that corresponds
							// to the set of standard styles

	bool m_fStdLang;		// true if this is the special system "lang" feature whose
							// possible values are language IDs.

	//	for compiler:
	bool m_fFatalError;		// fatal error in this feature definition
	int	m_nInternalID;
	GdlFeatureSetting * m_pfsetDefault;
	utf16 m_wNameTblId;
};

#endif // FEATURES_INCLUDED