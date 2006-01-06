/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: PreCompiler.cpp
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Methods to implement the pre-compiler, which does error checking and adjustments.
-------------------------------------------------------------------------------*//*:End Ignore*/

/***********************************************************************************************
	Include files
***********************************************************************************************/
#include "main.h"

#pragma hdrstop
#undef THIS_FILE
DEFINE_THIS_FILE

/***********************************************************************************************
	Features
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Do the pre-compilation tasks for the feature definitions. Return false if
	compilation cannot continue due to an unrecoverable error.
----------------------------------------------------------------------------------------------*/
bool GrcManager::PreCompileFeatures(GrcFont * pfont)
{
	return m_prndr->PreCompileFeatures(this, pfont);
}

bool GdlRenderer::PreCompileFeatures(GrcManager * pcman, GrcFont * pfont)
{
	int nInternalID = 0;

	Set<int> setID;

	for (int ipfeat = 0; ipfeat < m_vpfeat.Size(); ipfeat++)
	{
		GdlFeatureDefn * pfeat = m_vpfeat[ipfeat];
		int nID = pfeat->ID();
		if (setID.IsMember(nID))
		{
			char rgch[20];
			itoa(nID, rgch, 10);
			g_errorList.AddError(pfeat, "Duplicate feature ID: ", rgch);
		}
		else
			setID.Insert(nID);

		if (pfeat->ErrorCheck())
		{
			pfeat->SetStdStyleFlag();
			pfeat->FillInBoolean(pcman->SymbolTable());
			pfeat->ErrorCheckContd();
			pfeat->CalculateDefault();
			pfeat->AssignInternalID(nInternalID);
			pfeat->RecordDebugInfo();
		}

		nInternalID++;
	}

	if (m_vpfeat.Size() > kMaxFeatures)
	{
		char rgchMax[20];
		itoa(kMaxFeatures, rgchMax, 10);
		char rgchCount[20];
		itoa(m_vpfeat.Size(), rgchCount, 10);
		g_errorList.AddError(NULL,
			"Number of features (",
			rgchCount,
			") exceeds maximum of ",
			rgchMax);
	}

	return true;
}


