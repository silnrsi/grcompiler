/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GrcManager.cpp
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Implements the main object that manages the compliation process.
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

GrcManager g_cman;

/***********************************************************************************************
	Methods: General
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor
----------------------------------------------------------------------------------------------*/
GrcManager::GrcManager()
{
	Init();
}

void GrcManager::Init()
{
	m_prndr = new GdlRenderer();
	m_prndr->SetLineAndFile(GrpLineAndFile(1, 1, ""));

	m_psymtbl = new GrcSymbolTable(true);

	m_venv.resize(1);

	//	Temporary structures used during parsing:
	m_mtbGlyphAttrs = new GrcMasterTable;
	m_mtbFeatures = new GrcMasterTable;
	m_mvlNameStrings = new GrcMasterValueList;

	m_pgax = NULL;
	m_plclist = NULL;

	m_prgvpglfcFsmClasses = NULL;

	m_fOutputDebugFiles = false;
	m_fOutputDebugXml = false;
	m_fBasicJust = false;

	m_nNameTblStart = -1;

	m_fxdSilfTableVersion = 0;
	m_fxdGlocTableVersion = 0;
	m_fxdGlatTableVersion = 0;
	m_fxdFeatTableVersion = 0;
	m_fxdSillTableVersion = 0;
	m_tcCompressor = ktcNone;
}


/*----------------------------------------------------------------------------------------------
	Destructor
----------------------------------------------------------------------------------------------*/
GrcManager::~GrcManager()
{
	Clear();
}

void GrcManager::Clear()
{
	if (m_prgvpglfcFsmClasses)
		delete[] m_prgvpglfcFsmClasses;

	size_t i;
	for (i = 0; i < m_vpexpModified.size(); ++i)
		delete m_vpexpModified[i];
	m_vpexpModified.clear();

	if (m_pgax)
		delete m_pgax;
	if (m_plclist)
		delete m_plclist;

	if (m_prndr)
		delete m_prndr;

	if (m_mtbGlyphAttrs)
		delete m_mtbGlyphAttrs;
	if (m_mtbFeatures)
		delete m_mtbFeatures;
	if (m_mvlNameStrings)
		delete m_mvlNameStrings;

	if (m_psymtbl)
		delete m_psymtbl;

	for (i = 0; i < this->m_vplcls.size(); i++)
		delete m_vplcls[i];
	m_vplcls.clear();

	for (i = 0; i < m_vpfeatInput.size(); ++i)
		delete m_vpfeatInput[i];
}


/***********************************************************************************************
	Methods: Getters and setters
***********************************************************************************************/
/*----------------------------------------------------------------------------------------------
	Return the number of justification levels.
----------------------------------------------------------------------------------------------*/
int GrcManager::NumJustLevels()
{
	int cRet = m_nMaxJLevel;
	if (cRet == -1)
		cRet = 1;	// no levels specified
	else if (cRet == -2)
		cRet = 0;	// no justification
	else
		cRet++;		// to account for level 0
	cRet = min(cRet, 4);
	return cRet;
}

/***********************************************************************************************
	Methods: Parser
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Push a new environment corresponding to a table statement. Records an error if the
	string is not a valid table name.
----------------------------------------------------------------------------------------------*/
GrcEnv * GrcManager::PushTableEnv(GrpLineAndFile & lnf, std::string staTableName)
{
	Symbol psymTable = SymbolTable()->FindSymbol(staTableName);
	if (!psymTable || !psymTable->FitsSymbolType(ksymtTable))
	{
		g_errorList.AddError(1181, NULL,
			"Invalid table name: ",
			staTableName,
			lnf);
		return PushGeneralEnv(lnf);	// just push a copy of current env
	}
	else
	{
		GrcEnv * penvPrev = &(m_venv[m_venv.size() - 1]);
		Symbol psymPrevTable = penvPrev->Table();
		int nPrevPass = penvPrev->Pass();
		std::pair<Symbol, int> hmpair;
		hmpair.first = psymPrevTable;
		hmpair.second = nPrevPass;
		m_hmpsymnCurrPass.insert(hmpair);
		//m_hmpsymnCurrPass.Insert(psymPrevTable, nPrevPass, true);

		GrcEnv * penvNew = PushEnvAux();

		penvNew->SetTable(psymTable);
		int nPass = 0;
		std::map<Symbol, int>::iterator hmit = m_hmpsymnCurrPass.find(psymTable);
		if (hmit != m_hmpsymnCurrPass.end())
			nPass = hmit->second;
		//m_hmpsymnCurrPass.Retrieve(psymTable, &nPass);
		penvNew->SetPass(nPass);

		return penvNew;
	}
}


/*----------------------------------------------------------------------------------------------
	Push a new environment corresponding to a pass statement.
----------------------------------------------------------------------------------------------*/
GrcEnv * GrcManager::PushPassEnv(GrpLineAndFile & lnf, int nPass)
{
	GrcEnv * penvNew = PushEnvAux();

	penvNew->SetPass(nPass);

	Symbol psymTable = penvNew->Table();

	std::pair<Symbol, int> hmpair;
	hmpair.first = psymTable;
	hmpair.second = nPass;
	m_hmpsymnCurrPass.insert(hmpair);
	//m_hmpsymnCurrPass.Insert(psymTable, nPass, true);

	if (!psymTable->FitsSymbolType(ksymtTableRule))
	{
		g_errorList.AddError(1182, NULL,
			psymTable->FullName(),
			" table cannot hold passes",
			lnf);
	}

	return penvNew;
}


/*----------------------------------------------------------------------------------------------
	Push a new environment corresponding to an environment statement.
----------------------------------------------------------------------------------------------*/
GrcEnv * GrcManager::PushGeneralEnv(GrpLineAndFile & /*lnf*/)
{
	return PushEnvAux();
}


/*----------------------------------------------------------------------------------------------
	Push a new environment initialized from the previous one. Return a pointer to the new one.
----------------------------------------------------------------------------------------------*/
GrcEnv * GrcManager::PushEnvAux()
{
	GrcEnv envToCopy(m_venv.back());	// make a local copy, because the act of pushing can cause
										// the vector to resize, losing the item to copy
	m_venv.push_back(envToCopy);
	return &(m_venv.back());
}


/*----------------------------------------------------------------------------------------------
	Pop the top environment. Return a pointer to the new top.
	Arguments:
		nLine			- line number
		staStmt			- for error message: "table", "pass", "environment"
----------------------------------------------------------------------------------------------*/
GrcEnv * GrcManager::PopEnv(GrpLineAndFile & lnf, std::string staStmt)
{
	//	There should never be less than one environment on the stack, since the recipient
	//	is initialized with one.
	if (m_venv.size() <= 1)
	{
		g_errorList.AddError(1183, NULL,
			"End",
			staStmt,
			" encountered without balancing ",
			staStmt,
			" statement",
			lnf);
		return &(m_venv.back());
	}

	m_venv.pop_back();

	GrcEnv * penv = &(m_venv.back());
	Symbol psymTable = penv->Table();
	int nPass = penv->Pass();

	m_hmpsymnCurrPass.erase(psymTable);
	std::pair<Symbol, int> hmpair;
	hmpair.first = psymTable;
	hmpair.second = nPass;
	m_hmpsymnCurrPass.insert(hmpair);
    //m_hmpsymnCurrPass.Insert(psymTable, nPass, true);	// true: overwrite previous value

	return penv;
}


/*----------------------------------------------------------------------------------------------
	Add a class with the given name.
----------------------------------------------------------------------------------------------*/
GdlGlyphClassDefn * GrcManager::AddGlyphClass(GrpLineAndFile const& lnf, std::string staClassName)
{
	GrcStructName xns(staClassName);
	Symbol psymClass = m_psymtbl->AddClassSymbol(xns, lnf);
	GdlGlyphClassDefn * pglfc = dynamic_cast<GdlGlyphClassDefn*>(psymClass->Data());
	Assert(pglfc);
	m_prndr->AddGlyphClass(pglfc);
	pglfc->SetName(staClassName);
	return pglfc;
}

/*----------------------------------------------------------------------------------------------
	Add an anonymous class; assign it a generated name.
----------------------------------------------------------------------------------------------*/
GdlGlyphClassDefn * GrcManager::AddAnonymousClass(GrpLineAndFile const& lnf)
{
	Symbol psymClass = m_psymtbl->AddAnonymousClassSymbol(lnf);
	GdlGlyphClassDefn * pglfc = dynamic_cast<GdlGlyphClassDefn*>(psymClass->Data());
	Assert(pglfc);
	m_prndr->AddGlyphClass(pglfc);
	pglfc->SetName(std::string(psymClass->FieldAt(0)));
	return pglfc;
}


/*----------------------------------------------------------------------------------------------
	Set up the GDL objects from the master tables.
----------------------------------------------------------------------------------------------*/
void GrcManager::ProcessMasterTables()
{
	m_mtbFeatures->SetupFeatures();
	m_mtbGlyphAttrs->SetupGlyphAttrs();
	m_mvlNameStrings->SetupNameDefns(m_prndr->NameAssignmentsMap());
}


/***********************************************************************************************
	Methods: For test procedures
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Delete the context of the GrcManager and reinitialize so we can use it for another
	test procedure.
----------------------------------------------------------------------------------------------*/
void GrcManager::test_Recycle()
{
	Clear();
	Init();
}
