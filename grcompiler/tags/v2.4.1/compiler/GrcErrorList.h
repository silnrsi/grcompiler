/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GrcErrorList.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Classes to implement the list of errors accumulated during the post-parser and pre-compiler.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef ERRORS_INCLUDED
#define ERRORS_INCLUDED

/*----------------------------------------------------------------------------------------------
Class: GrcErrorItem
Description: An error generated during the post-parser or pre-compiler.
Hungarian: err
----------------------------------------------------------------------------------------------*/

class GrcErrorItem
{
	friend class GrcErrorList;

public:
	GrcErrorItem(bool fFatal, int nID, GrpLineAndFile & lnf, std::string staMsg, GdlObject * pgdlObj)
		:	m_nID(nID),
			m_pgdlObject(pgdlObj),
			m_staMsg(staMsg),
			m_fFatal(fFatal),
			m_fMsgIncludesFatality(false),
			m_lnf(lnf)
	{
	}

	bool Equivalent(GrcErrorItem * perr)
	{
		return ( //// m_pgdlObject == perr->m_pgdlObject &&
			m_nID == perr->m_nID
			&& m_staMsg == perr->m_staMsg
			&& m_lnf == perr->m_lnf
			&& m_fFatal == perr->m_fFatal);
	}

	int PreProcessedLine()
	{
		return m_lnf.PreProcessedLine();
	}

protected:
	//	instance variables:
	int				m_nID;
	GdlObject *		m_pgdlObject;
	std::string		m_staMsg;
	bool			m_fFatal;
	bool			m_fMsgIncludesFatality;		// don't add label "warning" or "error--the
												// message itself includes the information
	GrpLineAndFile	m_lnf;
};

/*----------------------------------------------------------------------------------------------
Class: GrcErrorList
Description: Database of errors accumulated during post-parser and pre-compiler. There is only
	a single instance of this class.
Hungarian: 
----------------------------------------------------------------------------------------------*/

class GrcErrorList
{
	friend class GrcErrorItem;

public:
	GrcErrorList()
	{
		m_fFatalError = false;
	}

	~GrcErrorList()
	{
		for (size_t i = 0; i < m_vperr.size(); ++i)
			delete m_vperr[i];
	}

	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, GrpLineAndFile const& lnf)
	{
		AddItem(true, nID, pgdlobj, &lnf, staMsg);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, GrpLineAndFile const& lnf)
	{
		AddItem(true, nID, pgdlobj, &lnf, &staMsg, &sta2, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3,
		GrpLineAndFile const& lnf)
	{
		AddItem(true, nID, pgdlobj, &lnf, &staMsg, &sta2, &sta3, NULL, NULL, NULL, NULL, NULL);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3, std::string sta4,
		GrpLineAndFile const& lnf)
	{
		AddItem(true, nID, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, NULL, NULL, NULL, NULL);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3, std::string sta4,
		std::string sta5,
		GrpLineAndFile const& lnf)
	{
		AddItem(true, nID, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, NULL, NULL, NULL);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3, std::string sta4,
		std::string sta5, std::string sta6,
		GrpLineAndFile const& lnf)
	{
		AddItem(true, nID, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, NULL, NULL);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3, std::string sta4,
		std::string sta5, std::string sta6, std::string sta7,
		GrpLineAndFile const& lnf)
	{
		AddItem(true, nID, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, NULL);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3, std::string sta4,
		std::string sta5, std::string sta6, std::string sta7, std::string sta8,
		GrpLineAndFile const& lnf)
	{
		AddItem(true, nID, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, &sta8);
	}


	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg)
	{
		AddItem(true, nID, pgdlobj, NULL, staMsg);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2)
	{
		AddItem(true, nID, pgdlobj, NULL, &staMsg, &sta2, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3)
	{
		AddItem(true, nID, pgdlobj, NULL, &staMsg, &sta2, &sta3, NULL, NULL, NULL, NULL, NULL);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3, std::string sta4)
	{
		AddItem(true, nID, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, NULL, NULL, NULL, NULL);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3, std::string sta4,
		std::string sta5)
	{
		AddItem(true, nID, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, NULL, NULL, NULL);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3, std::string sta4,
		std::string sta5, std::string sta6)
	{
		AddItem(true, nID, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, NULL, NULL);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3, std::string sta4,
		std::string sta5, std::string sta6, std::string sta7)
	{
		AddItem(true, nID, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, NULL);
	}
	void AddError(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3, std::string sta4,
		std::string sta5, std::string sta6, std::string sta7, std::string sta8)
	{
		AddItem(true, nID, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, &sta8);
	}


	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, GrpLineAndFile const& lnf)
	{
		AddItem(false,nID,  pgdlobj, &lnf, staMsg);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, GrpLineAndFile const& lnf)
	{
		AddItem(false, nID, pgdlobj, &lnf, &staMsg, &sta2, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3,
		GrpLineAndFile const& lnf)
	{
		AddItem(false, nID, pgdlobj, &lnf, &staMsg, &sta2, &sta3, NULL, NULL, NULL, NULL, NULL);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3,
		std::string sta4,
		GrpLineAndFile const& lnf)
	{
		AddItem(false, nID, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, NULL, NULL, NULL, NULL);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3,
		std::string sta4, std::string sta5,
		GrpLineAndFile const& lnf)
	{
		AddItem(false, nID, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, NULL, NULL, NULL);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3,
		std::string sta4, std::string sta5, std::string sta6,
		GrpLineAndFile const& lnf)
	{
		AddItem(false, nID, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, NULL, NULL);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3,
		std::string sta4, std::string sta5, std::string sta6, std::string sta7,
		GrpLineAndFile const& lnf)
	{
		AddItem(false, nID, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, NULL);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3,
		std::string sta4, std::string sta5, std::string sta6, std::string sta7, std::string sta8,
		GrpLineAndFile const& lnf)
	{
		AddItem(false, nID, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, &sta8);
	}


	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg)
	{
		AddItem(false, nID, pgdlobj, NULL, staMsg);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2)
	{
		AddItem(false, nID, pgdlobj, NULL, &staMsg, &sta2, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3)
	{
		AddItem(false, nID, pgdlobj, NULL, &staMsg, &sta2, &sta3, NULL, NULL, NULL, NULL, NULL);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3,
		std::string sta4)
	{
		AddItem(false, nID, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, NULL, NULL, NULL, NULL);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3,
		std::string sta4, std::string sta5)
	{
		AddItem(false, nID, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, NULL, NULL, NULL);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3,
		std::string sta4, std::string sta5, std::string sta6)
	{
		AddItem(false, nID, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, NULL, NULL);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3,
		std::string sta4, std::string sta5, std::string sta6, std::string sta7)
	{
		AddItem(false, nID, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, NULL);
	}
	void AddWarning(int nID, GdlObject * pgdlobj, std::string staMsg, std::string sta2, std::string sta3,
		std::string sta4, std::string sta5, std::string sta6, std::string sta7, std::string sta8)
	{
		AddItem(false, nID, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, &sta8);
	}

	void AddItem(bool fFatal, int nID, GdlObject * pgdlobj, const GrpLineAndFile *, std::string staMsg);
	void AddItem(bool fFatal, int nID, GdlObject * pgdlobj, const GrpLineAndFile *,
		std::string * psta1, std::string * psta2, std::string * psta3, std::string * psta4,
		std::string * psta5, std::string * psta6, std::string * psta7, std::string * psta8);

	void SortErrors();
	int NumberOfErrors()
	{
		return m_vperr.size();
	}
	int ErrorsAtLine(int nLine);
	int ErrorsAtLine(int nLine, int * piperrFirst);

	bool AnyFatalErrors()
	{
		return m_fFatalError;
	}

	int NumberOfWarnings();
	int NumberOfWarningsGiven();

	bool IsFatal(int iperr)
	{
		return m_vperr[iperr]->m_fFatal;
	}

	void SetIgnoreWarning(int nID, bool f = true);
	bool IgnoreWarning(int nID);

	void SetLastMsgIncludesFatality(bool f)
	{
		m_vperr.back()->m_fMsgIncludesFatality = f;
	}

	void WriteErrorsToFile(std::string staGdlFile, std::string staInputFontFile,
		std::string staOutputFile, std::string staOutputFamily, std::string staVersion, int fSepCtrlFile)
	{
		WriteErrorsToFile(m_strErrFile, staGdlFile, staInputFontFile, staOutputFile, staOutputFamily,
			staVersion, fSepCtrlFile);
	}
	void WriteErrorsToFile(std::string staErrFile, std::string staGdlFile, std::string staInputFontFile,
		std::string staOutputFile, std::string staOutputFamily,
		std::string staVersion, bool fSepCtrlFile);
	void WriteErrorsToStream(std::ostream&, std::string staGdlFile, std::string staInputFontFile,
		std::string staOutputFile, std::string staOutputFamily,
		std::string staVersion, bool fSepCtrlFile);

	void SetFileNameFromGdlFile(char * pchGdlFile);

protected:
	//	instance variables:
	bool m_fFatalError;

	std::vector<GrcErrorItem *> m_vperr;

	std::vector<int> m_vnIgnoreWarnings;

	std::string m_strErrFile;

public:
	//	For test procedures:
	bool test_ErrorIs(int iperr, std::string staMsg)
	{
		return (m_vperr[iperr]->m_staMsg == staMsg);
	}

	void test_Clear()
	{
		for (size_t i = 0; i < m_vperr.size(); ++i)
		{
			delete m_vperr[i];
		}
		m_vperr.clear();
		m_fFatalError = false;
	}
};


void AddGlobalError(bool fFatal, int nID, std::string msg, int nLine);
void AddGlobalError(bool fFatal, int nID, std::string msg, GrpLineAndFile const&);


#endif // ERRORS_INCLUDED
