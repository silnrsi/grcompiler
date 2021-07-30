/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GdlRenderer.cpp
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Implementation of the top-level GDL thing.
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
	Methods: General
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor
----------------------------------------------------------------------------------------------*/
GdlRenderer::GdlRenderer()
{
	m_fAutoPseudo = true;
	m_nBidi = -1;		// not set
	m_ipassBidi = -2;	// not set
	m_fHasFlippedPass = false;
	m_pexpXAscent = NULL;
	m_pexpXDescent = NULL;
	m_grfsdc = kfsdcNone;
	m_cnUserDefn = 0;
	m_cnComponents = 0;
}


/*----------------------------------------------------------------------------------------------
	Destructor
----------------------------------------------------------------------------------------------*/
GdlRenderer::~GdlRenderer()
{
	//	Delete all the GdlGlyphDefns (simple glyphs) stored in the classes,
	//	then delete all the classes at the top level.
	size_t i;
	for (i = 0; i < m_vpglfc.size(); ++i)
		m_vpglfc[i]->DeleteGlyphDefns();
	for (i = 0; i < m_vpglfc.size(); ++i)
		delete m_vpglfc[i];

	for (i = 0; i < m_vprultbl.size(); ++i)
		delete m_vprultbl[i];

	for (i = 0; i < m_vpfeat.size(); ++i)
		delete m_vpfeat[i];

	for (i = 0; i < m_vplang.size(); ++i)
		delete m_vplang[i];

	for (NameDefnMap::iterator itmap = m_hmNameDefns.begin();
		itmap != m_hmNameDefns.end();
		++itmap)
	{
		delete itmap->second;
	}

	if (m_pexpXAscent)
		delete m_pexpXAscent;
	if (m_pexpXDescent)
		delete m_pexpXDescent;
}

/*----------------------------------------------------------------------------------------------
	Add a language to the list. Keep them in sorted order. Return false if the language
	was already present.
----------------------------------------------------------------------------------------------*/
bool GdlRenderer::AddLanguage(GdlLanguageDefn * plang)
{
	uintptr_t iplangLo = 0;
	uintptr_t iplangHi = m_vplang.size();
	while (true)
	{
		auto iplangMid = (iplangLo + iplangHi) >> 1; // div by 2
		if (iplangMid >= m_vplang.size())
		{
			m_vplang.push_back(plang);
			return true;
		}

		unsigned int nCodeThis = plang->Code();
		unsigned int nCodeThat = m_vplang[iplangMid]->Code();
		int cmp = strcmp((char*)&nCodeThis, (char*)&nCodeThat);

		if (cmp == 0)
			return false; // already present
		if (iplangHi - iplangLo == 1)
		{
			m_vplang.insert(m_vplang.begin() + iplangLo + ((cmp<0) ? 0 : 1), plang);
			return true;
		}
		if (cmp < 0)
			iplangHi = iplangMid;
		else
			iplangLo = iplangMid;
	}
}


/***********************************************************************************************
	Methods: Parser
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Return the rule table with the given name; create it if it does not exist. Assumes that
	the name is a valid one for tables, but not necessarily for rule tables.
----------------------------------------------------------------------------------------------*/
GdlRuleTable * GdlRenderer::GetRuleTable(GrpLineAndFile & lnf, std::string staTableName)
{
	GdlRuleTable * prultbl = FindRuleTable(staTableName);

	if (prultbl)
		return prultbl;

	//	Create a new one, if permissible.

	Symbol psymTableName = g_cman.SymbolTable()->FindSymbol(staTableName);
	Assert(psymTableName);
	if (!psymTableName || !psymTableName->FitsSymbolType(ksymtTableRule))
	{
		g_errorList.AddError(2162, NULL,
			"The ",
			staTableName,
			" table cannot hold rules",
			lnf);
		return NULL;
	}

	prultbl = new GdlRuleTable(psymTableName);
	prultbl->SetLineAndFile(lnf);
	m_vprultbl.push_back(prultbl);

	return prultbl;
}


/*----------------------------------------------------------------------------------------------
	Return the table with the given name, or NULL if it does not exist.
----------------------------------------------------------------------------------------------*/
GdlRuleTable * GdlRenderer::FindRuleTable(std::string staTableName)
{
	Symbol psymTableName = g_cman.SymbolTable()->FindSymbol(staTableName);
	if (!psymTableName)
		return NULL;

	return FindRuleTable(psymTableName);
}

GdlRuleTable * GdlRenderer::FindRuleTable(Symbol psymTableName)
{
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		if (m_vprultbl[iprultbl]->NameSymbol() == psymTableName)
			return m_vprultbl[iprultbl];
	}
	
	return NULL;
}

/***********************************************************************************************
	Methods: Pre-compiler
***********************************************************************************************/

void GdlRenderer::SetNumUserDefn(size_t c)
{
	m_cnUserDefn = max(m_cnUserDefn, c+1);
}

void GdlRenderer::SetNumLigComponents(size_t c)
{
	m_cnComponents = max(m_cnComponents, c);
}
