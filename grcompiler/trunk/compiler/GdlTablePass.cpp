/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GdlTablePass.cpp
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Implements the classes corresponding to the tables of rules and their passes.
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
    Return the pass with the given number; create it if it does not exist. Pass 0 is the
	special unnumbered pass, for tables with only one pass.
----------------------------------------------------------------------------------------------*/
GdlPass* GdlRuleTable::GetPass(GrpLineAndFile & lnf, int ipassNumber,
	int nMaxRuleLoop, int nMaxBackup)
{
	if (ipassNumber < 0)
	{
		g_errorList.AddError(3150, NULL,
			StrAnsi("Invalid pass number"),
			lnf);
		return NULL;
	}

	while (ipassNumber >= m_vppass.Size())
		m_vppass.Push(NULL);

	if (m_vppass[ipassNumber] == NULL)
	{
		m_vppass[ipassNumber] = new GdlPass(ipassNumber, nMaxRuleLoop, nMaxBackup);
		m_vppass[ipassNumber]->SetLineAndFile(lnf);
	}

	return m_vppass[ipassNumber];
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
GdlPass::~GdlPass()
{
	int i;
	for (i = 0; i < m_vprule.Size(); ++i)
		delete m_vprule[i];

	for (i = 0; i < m_vpexpConstraints.Size(); ++i)
		delete m_vpexpConstraints[i];

	if (m_pfsm)
		delete m_pfsm;

	ClearFsmWorkSpace();
}

/*----------------------------------------------------------------------------------------------
	Clear the data structures that are used to manage the stuff for FSM generation.
----------------------------------------------------------------------------------------------*/
void GdlPass::ClearFsmWorkSpace()
{
	//	Delete all the machine-class lists in the hash map.
	for (HashMap<int, MachineClassList>::iterator it = m_hmMachineClassMap.Begin();
		it != m_hmMachineClassMap.End();
		++it)
	{
		delete *it;	// vector containing a group of machine classes with the same key;
					// notice that we store a pointer to the vector which is separately
					// allocated, not the vector itself, due to the limitations of HashMap.
	}
	m_hmMachineClassMap.Clear();

	//	Delete all the FsmMachineClasses and clear the master list.
	for (int i = 0; i < m_vpfsmc.Size(); i++)
		delete m_vpfsmc[i];
	m_vpfsmc.Clear();
}


