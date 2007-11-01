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

	GdlExtName(std::wstring stu, utf16 wLangID)
		:	m_stuName(stu),
			m_wLanguageID(wLangID)
	{
	}

	//	Getters:
	std::wstring Name()		{ return m_stuName; }
	utf16 LanguageID()		{ return m_wLanguageID; }

	static std::wstring s_stuNoName;

protected:
	//	Instance variables:
	std::wstring	m_stuName;
	utf16			m_wLanguageID;
};

/*----------------------------------------------------------------------------------------------
Class: GdlFeatureSetting
Description: The information for a single feature setting.
Hungarian: fset
----------------------------------------------------------------------------------------------*/

class GdlFeatureSetting : public GdlObject
{
	friend class GdlFeatureDefn;
	friend class GdlLanguageDefn;

public:
	//	Constructor:
	GdlFeatureSetting()
	{
		m_nValue = 0;
		m_fHasValue = false;
	}

	//	Getters:
	int Value()				{ return m_nValue; }
	std::string Name()		{ return m_staName; }
	utf16 NameTblId()		{ return m_wNameTblId; }

	//	Setters:
	void SetName(std::string sta)	{ m_staName = sta; }
	void SetValue(int n)			{ m_nValue = n; m_fHasValue = true; }
	void AddExtName(utf16 wLangID, std::wstring stu)
	{
		m_vextname.push_back(GdlExtName(stu, wLangID));
	}
	void AddExtName(utf16 wLangID, GdlExpression * pexp)
	{
		GdlStringExpression * pexpString = dynamic_cast<GdlStringExpression*>(pexp);
		Assert(pexpString);

		std::wstring stu = pexpString->ConvertToUnicode();
		m_vextname.push_back(GdlExtName(stu, wLangID));
	}
	void SetNameTblId(utf16 w)		{ m_wNameTblId = w; }

protected:
	//	Instance variables:
	std::string				m_staName;
	std::vector<GdlExtName>	m_vextname;
	int						m_nValue;
	utf16					m_wNameTblId;
	bool					m_fHasValue;
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
		for (size_t i = 0; i < m_vpfset.size(); ++i)
			delete m_vpfset[i];
	}

	//	Setters:
	void SetName(std::string sta)	{ m_staName = sta; }
	void SetID(unsigned int n)		{ m_nID = n; m_fIDSet = true; }
	void SetDefault(int n)			{ m_nDefault = n; m_fDefaultSet = true; }
	void AddExtName(utf16 wLangID, std::wstring stu)
	{
		m_vextname.push_back(GdlExtName(stu, wLangID));
	}
	void AddExtName(utf16 wLangID, GdlExpression * pexp)
	{
		GdlStringExpression * pexpString = dynamic_cast<GdlStringExpression*>(pexp);
		Assert(pexpString);

		std::wstring stu = pexpString->ConvertToUnicode();
		m_vextname.push_back(GdlExtName(stu, wLangID));
	}
	void MarkAsLanguageFeature()
	{
		m_fStdLang = true;
		m_nID = kfidStdLang;
	}
	utf16 SetNameTblIds(utf16 wFirst); // return next id to use

	//	Getters:
	std::string Name()
	{
		return m_staName;
	}
	int NumberOfSettings()
	{
		return m_vpfset.size();
	}
	unsigned int ID()
	{
		return m_nID;
	}
	bool IsLanguageFeature()
	{
		return m_fStdLang;
	}
	utf16 NameTblId()	{ return m_wNameTblId; }
	bool NameTblInfo(std::vector<std::wstring> & vstuExtNames, std::vector<utf16> & vwLangIds, 
		std::vector<utf16> & vwNameTblIds, size_t & cchwStringData);

	GdlFeatureSetting * FindSetting(std::string sta);
	GdlFeatureSetting * FindOrAddSetting(std::string, GrpLineAndFile & lnf);
	GdlFeatureSetting * FindSettingWithValue(int n);

public:
	//	Pre-compiler:
	bool ErrorCheck();
	void SetStdStyleFlag();
	void FillInBoolean(GrcSymbolTable * psymtbl);
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
	std::string					m_staName;
	unsigned int				m_nID;
	std::vector<GdlExtName>		m_vextname;
	std::vector<GdlFeatureSetting *>	m_vpfset;
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

/*----------------------------------------------------------------------------------------------
Class: GdlLanguageDefn
Description: A mapping of a language identifier onto a set of feature values.
Hungarian: lang
----------------------------------------------------------------------------------------------*/
class GdlLanguageDefn : public GdlDefn
{
	friend class GdlFeatureDefn;
	friend class GdlFeatureSetting;

public:
	// General:
	unsigned int Code()
	{
		unsigned int nCode;
		memcpy(&nCode, m_rgchID, sizeof(m_rgchID));
		return nCode;
	}

	void SetCode(std::string staCode)
	{
		Assert(staCode.sength() <= 4);
		staCode = staCode.substr(0, 4);
		memset(m_rgchID, 0, sizeof(m_rgchID));
		memcpy(m_rgchID, staCode.data(), staCode.size() * sizeof(char));
	}

	int NumberOfSettings()
	{
		return m_vpfset.size();
	}

	// Pre-compiler:
	void AddFeatureValue(GdlFeatureDefn * pfeat, GdlFeatureSetting * pfset,
		int nFset, GrpLineAndFile & lnf);

	// Compiler:
	void OutputSettings(GrcBinaryStream * pbstrm);

protected:
	//	Instance variables:
	char m_rgchID[4];
	std::vector<GdlFeatureDefn*> m_vpfeat;
	std::vector<GdlFeatureSetting*> m_vpfset;
	std::vector<int> m_vnFset;
};


/*----------------------------------------------------------------------------------------------
Class: GdlLangClass
Description: A collection of language maps and the feature settings for them.
Hungarian: lcls
----------------------------------------------------------------------------------------------*/
class GdlLangClass
{
	friend class GrcManager;

	GdlLangClass(std::string sta)
	{
		m_staLabel = sta;
	}

	~GdlLangClass()
	{
		for (size_t i = 0; i < m_vpexpVal.size(); i++)
			delete m_vpexpVal[i];
	}

	void AddLanguage(GdlLanguageDefn * plang)
	{
		for (size_t i = 0; i < m_vplang.size(); i++)
		{
			if (m_vplang[i] == plang)
				return;
		}
		m_vplang.push_back(plang);
	}

	void AddFeatureValue(std::string staFeat, std::string staVal,
		GdlExpression * pexpVal, GrpLineAndFile lnf)
	{
		m_vstaFeat.push_back(staFeat);
		m_vstaVal.push_back(staVal);
		m_vpexpVal.push_back(pexpVal);
		m_vlnf.push_back(lnf);
		Assert(m_vstaFeat.size() == m_vstaVal.size());
		Assert(m_vstaFeat.size() == m_vpexpVal.size());
		Assert(m_vstaFeat.size() == m_vlnf.size());
	}

	bool PreCompile(GrcManager * pcman);

protected:
	GrpLineAndFile m_lnf;
	std::string m_staLabel;
	std::vector<GdlLanguageDefn *> m_vplang;
	// These four are parallel vectors, each item corresponding to a feature assignment:
	std::vector<std::string> m_vstaFeat;		// name of feature
	std::vector<std::string> m_vstaVal;		// text of value
	std::vector<GdlExpression*> m_vpexpVal;	// expression, if not an identifer
	std::vector<GrpLineAndFile> m_vlnf;
};

#endif // FEATURES_INCLUDED
