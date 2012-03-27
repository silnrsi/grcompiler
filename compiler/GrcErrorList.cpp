/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GrcErrorList.cpp
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:

Here are how the error and warning IDs are assigned:

global - highest error: 141 / highest warning: 511
		main.cpp
		GrpParser.g
		GrpLexer
		GrpParser
		GrcFont

parsing - highest error: 1183 / highest warning: 1513
		ParserTreeWalker
		PostParser

error checking:
	general - highest error: 2164 / highest warning: 2535
		GdlExpression
		GrcMasterTable
		GrcSymTable
		GdlNameDefn
		GdlObject
		GrcBinaryStream
		GrcEnv
		GdlRenderer
	rules - highest error: 3161 / highest warning: 3533
		ErrorCheckRules
		GdlRule
		GdlTablePass
		Fsm
	classes - highest error: 4147 / highest warning: 4517
		ErrorCheckClass
		GrcGlyphAttrMatrix
		GdlGlyphDefn
		GdlGlyphClassDefn

compilation:	highest error: 5101 / highest warning: 5505
		OutputToFont

test and debug: highest error: 6106 / highest warning: none (start at: 6500)
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

GrcErrorList g_errorList;

/***********************************************************************************************
	Methods
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Add an error or warning to the list.
----------------------------------------------------------------------------------------------*/
void GrcErrorList::AddItem(bool fFatal, int nID, GdlObject * pgdlObj, const GrpLineAndFile * plnf,
	std::string staMsg)
{
	GrpLineAndFile lnf(0, 0, "");
	if (plnf == NULL)
	{
		if (pgdlObj)
			lnf = pgdlObj->LineAndFile();
	}
	else
		lnf = *plnf;
	
	GrcErrorItem * perr = new GrcErrorItem(fFatal, nID, lnf, staMsg, pgdlObj);
	m_vperr.push_back(perr);

	if (fFatal)
		m_fFatalError = true;
}

void GrcErrorList::AddItem(bool fFatal, int nID, GdlObject * pgdlObj, const GrpLineAndFile * plnf,
	std::string * psta1, std::string * psta2, std::string * psta3, std::string * psta4,
	std::string * psta5, std::string * psta6, std::string * psta7, std::string * psta8)
{
	std::string staMsg;
	if (psta1)
		staMsg += *psta1;
	if (psta2)
		staMsg += *psta2;
	if (psta3)
		staMsg += *psta3;
	if (psta4)
		staMsg += *psta4;
	if (psta5)
		staMsg += *psta5;
	if (psta6)
		staMsg += *psta6;
	if (psta7)
		staMsg += *psta7;
	if (psta8)
		staMsg += *psta8;

	AddItem(fFatal, nID, pgdlObj, plnf, staMsg);
}

/*----------------------------------------------------------------------------------------------
	Sort the list of errors; remove duplicates.
----------------------------------------------------------------------------------------------*/
void GrcErrorList::SortErrors()
{
	std::vector<GrcErrorItem *> vperrSorted;

	GrcErrorItem * perrLast = NULL;

	while (m_vperr.size() > 0)
	{
		int iperrNext = 0;
		for (size_t iperrT = 1; iperrT < m_vperr.size(); iperrT++)
		{
			GrpLineAndFile * plnfNext = &(m_vperr[iperrNext]->m_lnf);
			///int nLineNext = m_vperr[iperrNext]->m_nLineNumber;
			std::string staNext = m_vperr[iperrNext]->m_staMsg;

			GrpLineAndFile * plnfT = &(m_vperr[iperrT]->m_lnf);
			///int nLineT = m_vperr[iperrT]->m_nLineNumber;
			std::string staT = m_vperr[iperrT]->m_staMsg;

			if (*plnfT < *plnfNext)
				iperrNext = iperrT;
			//	NB: The main purpose in sorting errors alphabetically is to get identical
			//	errors next to each other, so duplicates can be deleted.
			else if (*plnfT == *plnfNext && strcmp(staT.c_str(), staNext.c_str()) < 0)
				iperrNext = iperrT;
		}

		if (perrLast && perrLast->Equivalent(m_vperr[iperrNext]))
		{
			//	Leave out
			delete m_vperr[iperrNext];
		}
		else
		{
			vperrSorted.push_back(m_vperr[iperrNext]);
			perrLast = m_vperr[iperrNext];
		}
		m_vperr.erase(m_vperr.begin() + iperrNext);
	}

	m_vperr.clear();
	m_vperr.assign(vperrSorted.begin(), vperrSorted.end());
}


/*----------------------------------------------------------------------------------------------
	Return the number of errors for the given line, along with the index of the first one.
	Assumes the list is sorted.
----------------------------------------------------------------------------------------------*/
int GrcErrorList::ErrorsAtLine(int nLine)
{
	int iBogus;
	return ErrorsAtLine(nLine, &iBogus);
}

int GrcErrorList::ErrorsAtLine(int nLine, int * piperrFirst)
{
	int iperrFirst = 0;
	while (iperrFirst < signed(m_vperr.size()) && m_vperr[iperrFirst]->PreProcessedLine() < nLine)
		iperrFirst++;

	if (iperrFirst >= signed(m_vperr.size()) || nLine < m_vperr[iperrFirst]->PreProcessedLine())
	{
		*piperrFirst = -1;
		return 0;
	}

	int cperr = 1;
	while (iperrFirst + cperr < signed(m_vperr.size()) &&
		m_vperr[iperrFirst + cperr]->PreProcessedLine() == nLine)
	{
		cperr++;
	}

	*piperrFirst = iperrFirst;
	return cperr;
}

/*----------------------------------------------------------------------------------------------
	Write the errors to a text file.
----------------------------------------------------------------------------------------------*/
void GrcErrorList::WriteErrorsToFile(std::string staFileName,
	std::string staGdlFile, std::string staInputFontFile,
	std::string staOutputFile, std::string staOutputFamily, std::string staVersion,
	bool fSepCtrlFile)
{
	std::ofstream strmOut;
	strmOut.open(staFileName.c_str());
	if (strmOut.fail())
	{
		g_errorList.AddError(106, NULL,
			"Error in writing to file ",
			std::string(staFileName));
		return;
	}

	WriteErrorsToStream(strmOut, staGdlFile, staInputFontFile, staOutputFile, staOutputFamily,
		staVersion, fSepCtrlFile);
	strmOut.close();
}

/*----------------------------------------------------------------------------------------------
	Write the errors to an output stream.
----------------------------------------------------------------------------------------------*/
void GrcErrorList::WriteErrorsToStream(std::ostream& strmOut,
	std::string staGdlFile, std::string staInputFontFile,
	std::string staOutputFile, std::string staOutputFamily, std::string staVersion, bool fSepCtrlFile)
{
	strmOut << "Graphite Compilation Results\n\n";
	strmOut << "GDL file: " << staGdlFile << "\n";
	strmOut << "Input font file: " << staInputFontFile << "\n";
	strmOut << "Output font file: " << staOutputFile << "\n";
	strmOut << "Output font family: " << staOutputFamily << "\n";
	strmOut << "Silf table version: " << staVersion << "\n";
	strmOut << "Create separate control file: " << (fSepCtrlFile ? "yes" : "no") << "\n";
	strmOut << "\n*******************************************************\n\n";

	int cError = 0;
	int cWarning = 0;
	int cWarningIgnored = 0;
	for (size_t iperr = 0; iperr < m_vperr.size(); iperr++)
	{
		GrcErrorItem * perr = m_vperr[iperr];

		if (!perr->m_fFatal && IgnoreWarning(perr->m_nID))
		{
			cWarningIgnored++;
			continue;	// ignore this warning
		}

		if (perr->m_lnf.File() != "" ||
			(perr->m_lnf.OriginalLine() != 0 &&
				perr->m_lnf.OriginalLine() != kMaxFileLineNumber))
		{
			strmOut << perr->m_lnf.File() << "(" << perr->m_lnf.OriginalLine() << ") : ";
		}

		if (!perr->m_fMsgIncludesFatality)
		{
			strmOut << ((perr->m_fFatal) ? "error(" : "warning(") << perr->m_nID << "): ";
		}
		(perr->m_fFatal) ? cError++ : cWarning++;

		strmOut << perr->m_staMsg.data() << "\n";
	}

	if (m_vperr.size() > 0)
		strmOut << "\n*******************************************************\n\n";
	if (AnyFatalErrors())
	{
		strmOut << "Compilation failed";
	}
	else
	{
		WriteTableVersionsGenerated(strmOut);
		strmOut << "Compilation succeeded";
	}
	strmOut << " - " << cError << " error" << ((cError == 1) ? ", " : "s, ")
		<< cWarning << " warning" << ((cWarning == 1) ? "" : "s");
	if (cWarningIgnored > 0)
		strmOut << " (" << cWarningIgnored << " warning" << ((cWarningIgnored == 1) ? "" : "s")
			<< " ignored)";
}

/*----------------------------------------------------------------------------------------------
	Output the table version numbers to the error file.
----------------------------------------------------------------------------------------------*/
void GrcErrorList::WriteTableVersionsGenerated(std::ostream& strmOut)
{
	strmOut << "Table versions generated:\n";

	int fxd = g_cman.TableVersion(ktiSilf);
	strmOut << "  Silf: " << VersionString(fxd) << "\n";
	fxd = g_cman.TableVersion(ktiGloc);
	strmOut << "  Gloc: " << VersionString(fxd) << "\n";
	fxd = g_cman.TableVersion(ktiGlat);
	strmOut << "  Glat: " << VersionString(fxd) << "\n";
	fxd = g_cman.TableVersion(ktiFeat);
	strmOut << "  Feat: " << VersionString(fxd) << "\n";
	fxd = g_cman.TableVersion(ktiSill);
	strmOut << "  Sill: " << VersionString(fxd) << "\n\n";

	fxd = g_cman.CompilerVersion();
	strmOut << "Minimal compiler version required: " << VersionString(fxd) << "\n";

	strmOut << "\n*******************************************************\n\n";
}

/*----------------------------------------------------------------------------------------------
	Return the number of non-fatal errors.
----------------------------------------------------------------------------------------------*/
int GrcErrorList::NumberOfWarnings()
{
	int cerrRet = 0;
	for (size_t ierr = 0; ierr < m_vperr.size(); ierr++)
	{
		if (!IsFatal(ierr))
			cerrRet++;
	}
	return cerrRet;
}


/*----------------------------------------------------------------------------------------------
	Return the number of non-fatal errors that were not ignored.
----------------------------------------------------------------------------------------------*/
int GrcErrorList::NumberOfWarningsGiven()
{
	int cerrRet = 0;
	for (size_t ierr = 0; ierr < m_vperr.size(); ierr++)
	{
		if (!IsFatal(ierr) && !IgnoreWarning(m_vperr[ierr]->m_nID))
			cerrRet++;
	}
	return cerrRet;
}


/*----------------------------------------------------------------------------------------------
	Store or remove an indication that the given warning should be ignored in the output.
----------------------------------------------------------------------------------------------*/
void GrcErrorList::SetIgnoreWarning(int nWarning, bool f)
{
	int iFound = -1;
	for (size_t i = 0; i < m_vnIgnoreWarnings.size(); i++)
	{
		if (m_vnIgnoreWarnings[i] == nWarning)
		{
			iFound = i;
			break;
		}
	}
	if (f)
	{
		if (iFound == -1)
			m_vnIgnoreWarnings.push_back(nWarning);
	}
	else
	{
		if (iFound > -1)
			m_vnIgnoreWarnings.erase(m_vnIgnoreWarnings.begin() + iFound);
	}
}

/*----------------------------------------------------------------------------------------------
	Store or remove an indication that the given warning should be ignored in the output.
----------------------------------------------------------------------------------------------*/
bool GrcErrorList::IgnoreWarning(int nWarning)
{
	for (size_t i = 0; i < m_vnIgnoreWarnings.size(); i++)
	{
		if (m_vnIgnoreWarnings[i] == nWarning)
			return true;
	}
	return false;
}

/*----------------------------------------------------------------------------------------------
	Global functions for use by the parser.
----------------------------------------------------------------------------------------------*/
void AddGlobalError(bool fFatal, int nID, std::string msg, int nLine)
{
	if (fFatal)
		g_errorList.AddError(nID, NULL, msg.c_str(), GrpLineAndFile(nLine, 0, ""));
	else
		g_errorList.AddWarning(nID, NULL, msg.c_str(), GrpLineAndFile(nLine, 0, ""));
}


void AddGlobalError(bool fFatal, int nID, std::string msg, GrpLineAndFile const& lnf)
{
	if (fFatal)
		g_errorList.AddError(nID, NULL, msg.c_str(), lnf);
	else
		g_errorList.AddWarning(nID, NULL, msg.c_str(), lnf);
}


/*----------------------------------------------------------------------------------------------
	Set the error file name to use the same path as the GDL file.
----------------------------------------------------------------------------------------------*/
void GrcErrorList::SetFileNameFromGdlFile(char * pchGdlFile)
{
	char * pchEnd = pchGdlFile + strlen(pchGdlFile);
	while (*pchEnd != '\\')
	{
		if (pchEnd <= pchGdlFile)
		{
			m_strErrFile = "gdlerr.txt";
			return;
		}
		pchEnd--;
	}
	m_strErrFile.assign(pchGdlFile, (pchEnd - pchGdlFile + 1));
	m_strErrFile.append("gdlerr.txt");
}
