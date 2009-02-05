/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GdlFeatures.cpp
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Implement the classes to handle the features mechanism.
-------------------------------------------------------------------------------*//*:End Ignore*/

/***********************************************************************************************
	Include files
***********************************************************************************************/
#include "main.h"

#pragma hdrstop
#undef THIS_FILE
DEFINE_THIS_FILE

/***********************************************************************************************
	Forward declarations
***********************************************************************************************/

/***********************************************************************************************
	Local Constants and static variables
***********************************************************************************************/

/***********************************************************************************************
	Methods: Post-parser
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Return the feature setting with the given name; create it if necessary.
----------------------------------------------------------------------------------------------*/
GdlFeatureSetting * GdlFeatureDefn::FindOrAddSetting(std::string sta, GrpLineAndFile & lnf)
{
	GdlFeatureSetting * pfset = FindSetting(sta);
	if (pfset)
		return pfset;
		
	pfset = new GdlFeatureSetting();
	pfset->SetName(sta);
	pfset->SetLineAndFile(lnf);
	m_vpfset.push_back(pfset);
	return pfset;
}

/*----------------------------------------------------------------------------------------------
	Return the feature setting with the given name, or NULL if none.
----------------------------------------------------------------------------------------------*/
GdlFeatureSetting * GdlFeatureDefn::FindSetting(std::string sta)
{
	for (size_t i = 0; i < m_vpfset.size(); ++i)
	{
		if (m_vpfset[i]->m_staName == sta)
			return m_vpfset[i];
	}
	return NULL;
}

/*----------------------------------------------------------------------------------------------
	Return the feature setting with the given value, or NULL if none.
----------------------------------------------------------------------------------------------*/
GdlFeatureSetting * GdlFeatureDefn::FindSettingWithValue(int n)
{
	for (size_t i = 0; i < m_vpfset.size(); i++)
	{
		if (m_vpfset[i]->m_nValue == n)
			return m_vpfset[i];
	}
	return NULL;
}


/***********************************************************************************************
	Methods: Pre-compiler
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Check for various error conditions.
----------------------------------------------------------------------------------------------*/
bool GdlFeatureDefn::ErrorCheck()
{
	if (m_fStdLang)
	{
		Assert(m_vpfset.size() == 0);
		m_fIDSet = true;
		return true;
	}

	//	Feature with no ID: fatal error
	if (m_fIDSet == false)
	{
		g_errorList.AddError(3158, this,
			"No id specified for feature ",
			m_staName);
		m_fFatalError = true;
		return false;
	}

	//	Duplicate IDs in feature settings: fatal error
	std::set<int> setnIDs;
	for (size_t ifset = 0; ifset < m_vpfset.size(); ++ifset)
	{
		int nValue = m_vpfset[ifset]->m_nValue;
		if (setnIDs.find(nValue) != setnIDs.end()) // is a member
		{
			g_errorList.AddError(3159, m_vpfset[ifset],
				"Duplicate feature setting values in ",
				m_staName);
			m_fFatalError = true;
			return false;
		}
		setnIDs.insert(nValue);
	}

	//	Feature with only one setting: warning
	if (m_vpfset.size() == 1)
		g_errorList.AddWarning(3525, this,
			"Only one setting given for feature ",
			m_staName);

	return true;
}

/*----------------------------------------------------------------------------------------------
	Determine if this style is the standard style; if so, set the flag.
----------------------------------------------------------------------------------------------*/
void GdlFeatureDefn::SetStdStyleFlag()
{
	if (m_nID == kfidStdStyles)
		m_fStdStyles = true;
}

/*----------------------------------------------------------------------------------------------
	If there are no settings for this feature, fill in with the default boolean settings.
----------------------------------------------------------------------------------------------*/
void GdlFeatureDefn::FillInBoolean(GrcSymbolTable * psymtbl)
{
	if (m_fStdLang)
		return;

	bool fBoolean = false;

	if (m_vpfset.size() == 0)
	{
		GdlFeatureSetting * pfsetFalse = new GdlFeatureSetting();
		pfsetFalse->CopyLineAndFile(*this);
		pfsetFalse->m_nValue = 0;
		pfsetFalse->m_fHasValue = true;
		pfsetFalse->m_staName = "false";
		pfsetFalse->m_vextname.push_back(GdlExtName(L"False", LG_USENG));
		m_vpfset.push_back(pfsetFalse);

		GdlFeatureSetting * pfsetTrue = new GdlFeatureSetting();
		pfsetTrue->CopyLineAndFile(*this);
		pfsetTrue->m_nValue = 1;
		pfsetTrue->m_fHasValue = true;
		pfsetTrue->m_staName = "true";
		pfsetTrue->m_vextname.push_back(GdlExtName(L"True", LG_USENG));
		m_vpfset.push_back(pfsetTrue);

		if (!m_fDefaultSet)
			m_nDefault = 0;
		m_fDefaultSet = true;

		fBoolean = true;

	}
	else if (m_vpfset.size() == 2)
	{
		fBoolean = ((m_vpfset[0]->m_nValue == 0 && m_vpfset[1]->m_nValue == 1)
			|| (m_vpfset[0]->m_nValue == 1 && m_vpfset[1]->m_nValue == 0));
	}

	if (fBoolean)
	{
		// Mark the expression type as boolean, not number.
		Symbol psymFeat = psymtbl->FindSymbol(m_staName);
		Assert(psymFeat->ExpType() == kexptNumber);
		psymFeat->SetExpType(kexptBoolean);
	}
}

/*----------------------------------------------------------------------------------------------
	More error checks that should be done after creating the boolean settings.
----------------------------------------------------------------------------------------------*/
void GdlFeatureDefn::ErrorCheckContd()
{
	if (m_fStdLang)
	{
		return;
	}

	//std::set<int> setnValues;
	for (size_t ifset = 0; ifset < m_vpfset.size(); ++ifset)
	{
		//	Feature setting with no value set: warning
		if (!m_vpfset[ifset]->m_fHasValue)
		{
			g_errorList.AddWarning(3526, this,
				"Feature setting with no value specified; value will be zero: ",
				m_vpfset[ifset]->m_staName);
		}
	}
}

/*----------------------------------------------------------------------------------------------
	Calculate the default setting, if it has not been stated explicitly. The default will
	be the minimum value.
----------------------------------------------------------------------------------------------*/
void GdlFeatureDefn::CalculateDefault()
{
	if (m_fStdLang)
	{
		m_nDefault = 0;
		m_fDefaultSet = true;
		return;
	}

	if (m_fDefaultSet)
	{
		for (size_t ifset = 0; ifset < m_vpfset.size(); ++ifset)
		{
			if (m_vpfset[ifset]->m_nValue == m_nDefault)
			{
				m_pfsetDefault = m_vpfset[ifset];
				return;
			}
		}
		//	Default setting not found; make one (with no name).
		GdlFeatureSetting * pfset = new GdlFeatureSetting();
		pfset->SetValue(m_nDefault);
		m_vpfset.push_back(pfset);
		m_pfsetDefault = pfset;
		m_fDefaultSet = true;
		return;
	}

	if (m_vpfset.size() == 0)
		return;

	m_nDefault = m_vpfset[0]->m_nValue;
	m_pfsetDefault = m_vpfset[0];
	for (size_t ifset = 1; ifset < m_vpfset.size(); ++ifset)
	{
		if (m_vpfset[ifset]->m_nValue < m_nDefault)
		{
			m_nDefault = m_vpfset[ifset]->m_nValue;
			m_pfsetDefault = m_vpfset[ifset];
		}
	}
	m_fDefaultSet = true;
}

/*----------------------------------------------------------------------------------------------
	Assign name table ids to the feature itself and all its settings. Each of these gets a 
	unique id which starts with nFirst and is incremented.
	Return the id that should be used next.
----------------------------------------------------------------------------------------------*/
utf16 GdlFeatureDefn::SetNameTblIds(utf16 wFirst)
{
	utf16 wNameTblId = wFirst;
	m_wNameTblId = wNameTblId++;

	size_t cnFeatSet = m_vpfset.size();
	for (size_t i = 0; i < cnFeatSet; i++)
	{
		m_vpfset[i]->SetNameTblId(wNameTblId++);
	}

	return wNameTblId;
}

std::wstring GdlExtName::s_stuNoName(L"NoName"); // static

/*----------------------------------------------------------------------------------------------
	Push onto the argument vectors information for the feature itself and all its settings.
	The three arrays must remain parallel. Retrieve all the external names and their 
	corresponding language ids and the name table ids (assigned in SetNameTblIds().
	While we're at it, keep a total of the length of the all the string data.
----------------------------------------------------------------------------------------------*/
bool GdlFeatureDefn::NameTblInfo(std::vector<std::wstring> & vstuExtNames,
	std::vector<utf16> & vwLangIds, std::vector<utf16> & vwNameTblIds, size_t & cchwStringData)
{
	// store data for the feature itself
	size_t cnExtNames = m_vextname.size();
	if (cnExtNames <= 0)
	{
		vstuExtNames.push_back(GdlExtName::s_stuNoName);
		cchwStringData += GdlExtName::s_stuNoName.length();
		vwLangIds.push_back(LG_USENG);
		vwNameTblIds.push_back(m_wNameTblId);
	}
	else
	{
		for (size_t i = 0; i < cnExtNames; i++)
		{
			vstuExtNames.push_back(m_vextname[i].Name());
			cchwStringData += m_vextname[i].Name().length();
			vwLangIds.push_back(m_vextname[i].LanguageID());
			vwNameTblIds.push_back(m_wNameTblId);
		}
	}

	// store data for all settings
	size_t cnSettings = m_vpfset.size();
	for (size_t i = 0; i < cnSettings; i++)
	{
		GdlFeatureSetting * pFeatSet = m_vpfset[i];
		cnExtNames = pFeatSet->m_vextname.size();
		if (cnExtNames <= 0)
		{
			vstuExtNames.push_back(GdlExtName::s_stuNoName);
			cchwStringData += GdlExtName::s_stuNoName.length();
			vwLangIds.push_back(LG_USENG);
			vwNameTblIds.push_back(pFeatSet->NameTblId());
		}
		else
		{
			for (size_t j = 0; j < cnExtNames; j++)
			{
				vstuExtNames.push_back(pFeatSet->m_vextname[j].Name());
				cchwStringData += pFeatSet->m_vextname[j].Name().length();
				vwLangIds.push_back(pFeatSet->m_vextname[j].LanguageID());
				vwNameTblIds.push_back(pFeatSet->NameTblId());
			}
		}
	}
	
	return true;
}

/*----------------------------------------------------------------------------------------------
	Record something about the feature in the debug database.
----------------------------------------------------------------------------------------------*/
void GdlFeatureDefn::RecordDebugInfo()
{
}

/*----------------------------------------------------------------------------------------------
	Add a default feature setting to the language definition.
----------------------------------------------------------------------------------------------*/
void GdlLanguageDefn::AddFeatureValue(GdlFeatureDefn * pfeat, GdlFeatureSetting * pfset,
	int nFset, GrpLineAndFile & lnf)
{
	for (size_t ifeat = 0; ifeat < m_vpfeat.size(); ifeat++)
	{
		if (m_vpfeat[ifeat] == pfeat)
		{
			if (m_vnFset[ifeat] == nFset)
				return;
			else 
			{
				g_errorList.AddError(3160, NULL, "Duplicate language feature setting", lnf);
				return;
			}
		}
	}
	m_vpfeat.push_back(pfeat);
	m_vpfset.push_back(pfset);
	m_vnFset.push_back(nFset);
	Assert(m_vpfeat.size() == m_vpfset.size());
	Assert(m_vpfeat.size() == m_vnFset.size());
}
