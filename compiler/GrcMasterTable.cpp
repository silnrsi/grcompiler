/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GrcMasterTable.cpp
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Master tables that are used for the post-parser.
-------------------------------------------------------------------------------*//*:End Ignore*/

/***********************************************************************************************
	Include files
***********************************************************************************************/
#include "main.h"

#ifdef _MSC_VER
#pragma hdrstop
#endif
#undef THIS_FILE
DEFINE_THIS_FILE

/***********************************************************************************************
	Forward declarations
***********************************************************************************************/

/***********************************************************************************************
	Local Constants and static variables
***********************************************************************************************/

/***********************************************************************************************
	Methods
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
    Add an item to the list. The first field of the symbol is the class or feature name
	key for the main master list.
	Arguments:
		psym		- glyph attribute or feature setting symbol
		pexpValue	- value of the above item
		nPR			- current global PointRadius setting
		munitPR		- current units for PointRadius
		fOverride	- current global AttributeOverride setting
		lnf			- line where statement occurs
----------------------------------------------------------------------------------------------*/
void GrcMasterTable::AddItem(Symbol psym, GdlExpression * pexpValue,
	int nPR, int munitPR, bool fOverride, GrpLineAndFile const& lnf,
	std::string staDescription)
{
	Symbol psymBase = psym->BaseDefnForNonGeneric();

	Assert(psymBase->FitsSymbolType(ksymtClass) || psymBase->FitsSymbolType(ksymtFeature));

	GrcMasterValueList * pmvl;

	ValueListMap::iterator hmit = m_vlistmapEntries.find(psymBase);
	if (hmit == m_vlistmapEntries.end() || hmit->second == NULL)
	//if (!m_vlistmapEntries.Retrieve(psymBase, &pmvl) || !pmvl)
	{
		pmvl = new GrcMasterValueList();
		ValueListPair hmpair;
		hmpair.first = psymBase;
		hmpair.second = pmvl;
		m_vlistmapEntries.insert(hmpair);
		//m_vlistmapEntries.Insert(psymBase, pmvl);
	}
	else
		pmvl = hmit->second;

	pmvl->AddItem(psym, pexpValue, nPR, munitPR, fOverride, lnf, staDescription);

//	GdlAssignment * pasgn;
///	
//	if (!pmvl->m_valmapEntries.Retrieve(psym, &pasgn) || !pasgn)
//	{
//		pasgn = new GdlAssignment(pexpValue, nPR, munitPR, fOverride, lnf);
//		pmvl->m_valmapEntries.Insert(psym, pasgn);
//	}
//	else
//	{
//		std::string staT = (psym->FitsSymbolType(ksymtGlyphAttr)) ?
//			"glyph attribute assignment" :
//			"feature setting";
//		g_errorList.AddWarning(2523, pexpValue,
//			"Duplicate ", staT);
//		
//		if ((lnf.PreProcessedLine() > pasgn->PreProcessedLine() && fOverride) ||
//			(lnf.PreProcessedLine() < pasgn->PreProcessedLine() && pasgn->Override()))
//			pasgn->Set(pexpValue, nPR, munitPR, fOverride, lnf);
//	}
}


/*----------------------------------------------------------------------------------------------
    Add an item to the list (ie, the name table list).
	Arguments:
		psym		- name symbol
		pexpValue	- value of the above item
		nPR			- current global PointRadius setting
		munitPR		- current units for PointRadius
		fOverride	- current global AttributeOverride setting
		lnf			- line where statement occurs
		staDescription - for error message
----------------------------------------------------------------------------------------------*/
void GrcMasterValueList::AddItem(Symbol psym, GdlExpression * pexpValue,
	int nPR, int munitPR, bool fOverride, GrpLineAndFile const& lnf,
	std::string staDescription)
{
	GdlAssignment * pasgn;
	
	ValueMap::iterator hmit = m_valmapEntries.find(psym);
	if (hmit == m_valmapEntries.end())
	//if (!m_valmapEntries.Retrieve(psym, &pasgn) || !pasgn)
	{
		pasgn = new GdlAssignment(pexpValue, nPR, munitPR, fOverride, lnf);
		ValuePair hmpair;
		hmpair.first = psym;
		hmpair.second = pasgn;
		m_valmapEntries.insert(hmpair);
		//m_valmapEntries.Insert(psym, pasgn);
	}
	else
	{
		pasgn = hmit->second;
		g_errorList.AddWarning(2524, pexpValue,
			"Duplicate ", staDescription);
		
		if ((lnf.PreProcessedLine() > pasgn->PreProcessedLine() && fOverride) ||
			(lnf.PreProcessedLine() < pasgn->PreProcessedLine() && !pasgn->Override()))
		{
			pasgn->Set(pexpValue, nPR, munitPR, fOverride, lnf);
		}
	}
}


/*----------------------------------------------------------------------------------------------
    Return the value list associated with the given symbol, which should be a class or
	feature name.
----------------------------------------------------------------------------------------------*/
GrcMasterValueList * GrcMasterTable::ValueListFor(Symbol psym)
{
	Assert(psym->FitsSymbolType(ksymtClass) || psym->FitsSymbolType(ksymtFeature));

	GrcMasterValueList * pmvl;
	ValueListMap::iterator hmit = m_vlistmapEntries.find(psym);
	if (hmit == m_vlistmapEntries.end())
		pmvl = NULL;
	else
        pmvl = hmit->second;
	//m_vlistmapEntries.Retrieve(psym, &pmvl);
	return pmvl;
}

/*----------------------------------------------------------------------------------------------
    Set up the data and settings for all the features from the
	master feature table.
----------------------------------------------------------------------------------------------*/
void GrcMasterTable::SetupFeatures(GrcSymbolTable * psymtbl)
{
	for (ValueListMap::iterator itvlistmap = EntriesBegin();
		itvlistmap != EntriesEnd();
		++itvlistmap)
	{
		Symbol psymFeatName = itvlistmap->first;  // GetKey();
		GrcMasterValueList * pmvl = itvlistmap->second; //GetValue();

		Assert(psymFeatName->FitsSymbolType(ksymtFeature));

		GdlFeatureDefn * pfeat = dynamic_cast<GdlFeatureDefn*>(psymFeatName->Data());
		Assert(pfeat);

		pmvl->SetupFeatures(pfeat, psymtbl);
	}
}

/*----------------------------------------------------------------------------------------------
	Initialize a GdlFeatureDefn with its settings from the
	master feature table.
----------------------------------------------------------------------------------------------*/
void GrcMasterValueList::SetupFeatures(GdlFeatureDefn * pfeat, GrcSymbolTable * psymtbl)
{
	std::string fName = pfeat->Name();

	GdlExpression * pexpDefault = NULL;

	for (ValueMap::iterator itvalmap = EntriesBegin();
		itvalmap != EntriesEnd();
		++itvalmap)
	{
		Symbol psym = itvalmap->first; // GetKey();
		Assert(psym->FitsSymbolType(ksymtFeatSetting));
		GdlAssignment * pasgnValue = itvalmap->second; // GetValue();
		GdlExpression * pexp = pasgnValue->m_pexp;

		Assert(psym->FieldIs(0, fName));

		if (psym->FieldIs(1, "id"))
		{
			//	feature.id
			if (psym->FieldCount() >= 2)
			{
				unsigned int nID;
				if (!pexp->ResolveToFeatureID(&nID))
					g_errorList.AddError(2147, pexp,
						"Feature id must be an integer or string of 4 characters or less");
				else if (nID == GdlFeatureDefn::kfidStdLang)
				{
					g_errorList.AddError(2148, pexp,
						"Feature ID ", std::to_string(nID), " is a reserved value");
					pfeat->SetID(nID);	// set it anyway, to avoid extra error message
				}
				else
				{
					if (psym->FieldCount() == 2) // main ID, not alternate
					{
						if (pfeat->ID() != 0)
							g_errorList.AddError(2168, pexp, "Duplicate main feature ID");
						else {
							pfeat->SetID(nID);
							pfeat->SetHasPublicID(true);
						}
					}
					else // alternate
					{
						std::string staAltName = psym->FieldAt(2);
						if (staAltName.substr(0, 6) != "hidden")
							g_errorList.AddWarning(2537, pexp,
								"Alternate feature ID should be listed using 'id.hidden'");
					}
					pfeat->AddAltID(nID);

					// This is something of a kludge. Create a symbol that looks like "featname__abcd" where abcd is the
					// string version of the ID.
					GdlStringExpression * pexpString = dynamic_cast<GdlStringExpression *>(pexp);
					if (pexpString) {
						psym->CreateFeatAltIDSymbol(psymtbl, pfeat, pexpString);
					}
				}
			}
			else
				g_errorList.AddError(2149, pexp,
					"Invalid feature id statement");
		}
		else if (psym->FieldIs(1, "default"))
		{
			//	feature.default
			if (psym->FieldCount() == 2)
				pexpDefault = pexp;	// save for later
			else
				g_errorList.AddError(2150, pexp,
					"Invalid feature default statement");
		}

		else if (psym->FieldIs(1, "settings"))
		{
			//	feature.settings....
			GdlFeatureSetting * pfset = pfeat->FindOrAddSetting(std::string(psym->FieldAt(2)),
				psym->LineAndFile());

			if (psym->FieldIs(3, "name"))
			{
				int nLangID;
				if (psym->FieldCount() != 5)
				{
					g_errorList.AddWarning(2525, pexp,
						"Invalid feature name statement");
					nLangID = LG_USENG;
				}
				else
				{
					std::string strLang = psym->FieldAt(4);
					if (strLang[0] == '0' && strLang[1] == 'x')
						nLangID = strtoul(strLang.data() + 2, NULL, 16);	// 0x0409
					else if (strLang[0] == 'x')
						nLangID = strtoul(strLang.data() + 1, NULL, 16);	// x0409
					else
						nLangID = atoi(strLang.data());							// 1033
					if (nLangID == 0 && psym->FieldAt(4) != "0")
					{
						g_errorList.AddWarning(2526, pexp,
							"Invalid language ID: ",
							psym->FieldAt(4),
							"--should be an integer");
						nLangID = LG_USENG;
					}
				}
				if (!pexp->TypeCheck(kexptString))
					g_errorList.AddWarning(2527, pexp,
						"Feature setting name must be a string");
				else
					pfset->AddExtName((utf16)nLangID, pexp);
			}

			else if (psym->FieldIs(3, "value"))
			{
				int nValue;
				if (!pexp->ResolveToInteger(&nValue, false))
					g_errorList.AddError(2151, pexp,
						"Feature value must be an integer");
				else
				{
					if (!pexp->TypeCheck(kexptNumber, kexptZero, kexptOne))
						g_errorList.AddWarning(2528, pexp,
							"Feature setting value should not be a scaled number");
					pfset->SetValue(nValue);
				}
			}

			else
				g_errorList.AddError(2152, pexp,
					"Invalid feature settings field");
		}

		else if (psym->FieldIs(1, "name"))
		{
			//	feature.name.langID
			int nLangID;
			if (psym->FieldCount() != 3)
			{
				g_errorList.AddWarning(2529, pexp,
					"Invalid feature name statement");
				nLangID = LG_USENG;
			}
			else
			{
				std::string strLang = psym->FieldAt(2);
				if (strLang[0] == '0' && strLang[1] == 'x')
					nLangID = strtoul(strLang.data() + 2, NULL, 16);	// eg, 0x0409
				else if (strLang[0] == 'x')
					nLangID = strtoul(strLang.data() + 1, NULL, 16);	// eg, x0409
				else
					nLangID = atoi(strLang.data());						// eg, 1033
				if (nLangID == 0 && psym->FieldAt(2) != "0")
				{
					g_errorList.AddWarning(2530, pexp,
						"Invalid language ID: ",
						psym->FieldAt(2),
						"--should be an integer");
					nLangID = LG_USENG;
				}
			}
			if (!pexp->TypeCheck(kexptString))
				g_errorList.AddWarning(2531, pexp,
						"Feature name must be a string.");
			else
				pfeat->AddExtName((utf16)nLangID, pexp);
		}

		else
			g_errorList.AddError(2153, pexp, "Invalid feature statement");

	}

	// Now that all the settings are defined, handle the default.
	int nDefault;
	if (pexpDefault)
	{
		if (pexpDefault->ResolveToInteger(&nDefault, false))
		{
			if (!pexpDefault->TypeCheck(kexptNumber, kexptZero, kexptOne))
				g_errorList.AddWarning(2532, pexpDefault,
					"Feature setting value should not be a scaled number");
			if (!pfeat->FindSettingWithValue(nDefault))
			{
				if (pfeat->NumberOfSettings() == 0 && (nDefault == 0 || nDefault == 1))
				{
					// Boolean default--okay.
				}
				else
				{
					g_errorList.AddWarning(2533, pexpDefault,
						"Default feature setting is not among the defined values");
				}
			}
			pfeat->SetDefault(nDefault);
		}
		else 
		{
			//	A kludge: the name of a feature setting was put in a slot-ref expression,
			//	even though that's not what it is (see the tree-walker).
			GdlSlotRefExpression * pexpsr =
				dynamic_cast<GdlSlotRefExpression *>(pexpDefault);
			if (!pexpsr || pexpsr->Alias() == "")
			{
				g_errorList.AddError(2154, pexpDefault,
					"Invalid default feature setting");
			}
			else
			{
				GdlFeatureSetting * pfsetDefault =
					pfeat->FindSetting(pexpsr->Alias());
				if (!pfsetDefault)
					g_errorList.AddError(2155, pexpDefault,
						"Default feature setting is undefined: ",
						pexpsr->Alias());
				else
					pfeat->SetDefault(pfsetDefault->Value());
			}
		}
	}
}

/*----------------------------------------------------------------------------------------------
	Set up the glyph attributes for all the classes from the master glyph
	attribute table.
----------------------------------------------------------------------------------------------*/
void GrcMasterTable::SetupGlyphAttrs()
{
	for (ValueListMap::iterator itvlistmap = EntriesBegin();
		itvlistmap != EntriesEnd();
		++itvlistmap)
	{
		Symbol psymClassName = itvlistmap->first;   // GetKey();
		GrcMasterValueList * pmvl = itvlistmap->second;  // GetValue();

		Assert(psymClassName->FitsSymbolType(ksymtClass));

		GdlGlyphClassDefn * pglfc = dynamic_cast<GdlGlyphClassDefn*>(psymClassName->Data());
		Assert(pglfc);

		pmvl->SetupGlyphAttrs(pglfc);
	}
}

/*----------------------------------------------------------------------------------------------
	Initialize a single GlyphClassDefn with its settings from master glyph table.
----------------------------------------------------------------------------------------------*/
void GrcMasterValueList::SetupGlyphAttrs(GdlGlyphClassDefn * pglfc)
{
	std::vector<Symbol> vpsymProcessed;

	for (ValueMap::iterator itvalmap = EntriesBegin();
		itvalmap != EntriesEnd();
		++itvalmap)
	{
		Symbol psym = itvalmap->first; // GetKey();
		GdlAssignment * pasgnValue = itvalmap->second; // GetValue();
		GdlExpression * pexp = pasgnValue->m_pexp;
//		int nPR = pasgnValue->m_nPR;
//		int munitPR = pasgnValue->m_munitPR;
//		int nStmtNo = pasgnValue->m_nStmtNo;
//		bool fOverride = pasgnValue->m_fOverride;

		Assert(psym->FieldIs(0, pglfc->Name()));

		if (psym->FieldIs(1, "directionality"))
		{
			if (psym->FieldCount() == 2)
			{
				pglfc->AddGlyphAttr(psym, pasgnValue);
				vpsymProcessed.push_back(psym);
			}
			else
				g_errorList.AddError(2156, pexp,
					"Invalid use of directionality attribute");
		}
		else if (psym->FieldIs(1, "breakweight"))
		{
			if (psym->FieldCount() == 2)
			{
				pglfc->AddGlyphAttr(psym, pasgnValue);
				vpsymProcessed.push_back(psym);
			}
			else
				g_errorList.AddError(2157, pexp,
					"Invalid use of breakweight attribute");
		}
		else if (psym->FieldIs(1, "component"))
		{
			if (psym->FieldCount() == 4 &&
				(psym->FieldIs(3, "left") ||
					psym->FieldIs(3, "bottom") ||
					psym->FieldIs(3, "right") ||
					psym->FieldIs(3, "top")))
			{
				pglfc->AddComponent(psym, pasgnValue);
				vpsymProcessed.push_back(psym);
			}
			else
				g_errorList.AddError(2158, pexp, "Invalid use of component attribute");
		}
		else
		{
			if (psym->FitsSymbolType(ksymtGlyphMetric))
				//	We should never get here, but just in case.
				g_errorList.AddError(2159, pexp, "Cannot set the value of a glyph metric.");
			else
			{
				//	User-defined glyph attribute.
				pglfc->AddGlyphAttr(psym, pasgnValue);
				vpsymProcessed.push_back(psym);
			}
		}
	}

	//	Now delete all the assignments that have been successfully processed from the map,
	//	since they are now "owned" by the class definition. (Otherwise we get double destruction.)

	for (size_t ipsym = 0; ipsym < vpsymProcessed.size(); ipsym++)
	{
		m_valmapEntries.erase(vpsymProcessed[ipsym]);
		//m_valmapEntries.Delete(vpsymProcessed[ipsym]);
	}
}


/*----------------------------------------------------------------------------------------------
	Initialize a NameDefn with its values from master table.
----------------------------------------------------------------------------------------------*/
void GrcMasterValueList::SetupNameDefns(NameDefnMap & hmNameMap)
{
	for (ValueMap::iterator itvalmap = EntriesBegin();
		itvalmap != EntriesEnd();
		++itvalmap)
	{
		Symbol psym = itvalmap->first; // GetKey();
		GdlAssignment * pasgnValue = itvalmap->second; // GetValue();
		GdlExpression * pexp = pasgnValue->m_pexp;
		//int nPR = pasgnValue->m_nPR;
		//int munitPR = pasgnValue->m_munitPR;
		bool fOverride = pasgnValue->m_fOverride;

		GdlStringExpression * pexpstr = dynamic_cast<GdlStringExpression *>(pexp);

		if (psym->FieldCount() != 2)
			g_errorList.AddError(2160, pexp, "Invalid name table entry.");

		else if (!pexpstr)
			g_errorList.AddError(2161, pexp,
				"Value of name table entry must be a string");

		else
		{
			std::string staNameID = psym->FieldAt(0);
			std::string staLangID = psym->FieldAt(1);

			int nNameID = atoi(staNameID.data());
			if (nNameID == 0 && staNameID != "0")
				g_errorList.AddWarning(2534, pexp,
					"Invalid name ID: ",
					staNameID,
					"--should be an integer");
			int nLangID = atoi(staLangID.data());
			if (nLangID == 0 && staLangID != "0")
				g_errorList.AddWarning(2535, pexp,
					"Invalid language ID: ",
					staLangID,
					"--should be an integer");

			GdlNameDefn * pndefn = NULL;
			NameDefnMap::iterator itmap = hmNameMap.find(nNameID);
			if (itmap == hmNameMap.end())
			//if (!hmNameMap.Retrieve(nNameID, &pndefn))
			{
				pndefn = new GdlNameDefn();
				pndefn->SetLineAndFile(pasgnValue->LineAndFile());
				std::pair<int, GdlNameDefn *> hmpair;
				hmpair.first = nNameID;
				hmpair.second = pndefn;
				hmNameMap.insert(hmpair);
				//hmNameMap.Insert(nNameID, pndefn);
			}

			bool fFoundLang = false;
			for (auto iextname = 0U; iextname < pndefn->NameCount(); ++iextname)
			{
				if (pndefn->ExtName(iextname)->LanguageID() == nLangID)
				{
					fFoundLang = true;
					if (fOverride)
						pndefn->DeleteExtName(iextname);
					break;
				}
			}

			if (fFoundLang == false || fOverride)
			{
				pndefn->AddExtName((utf16)nLangID, pexp);
//				vpsymProcessed.Push(psym);
			}
		}
	}

	//	Now delete all the assignments that have been successfully processed from the map,
	//	since they are now "owned" by the name map. (Otherwise we get double destruction.)

//	for (std::vector<Symbol>::iterator it = vpsymProcessed.Begin();
//		it != vpsymProcessed.End();
//		it++)
//	{
//		m_valmapEntries.Delete(*it);
//	}
}
