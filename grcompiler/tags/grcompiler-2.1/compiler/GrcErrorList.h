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
Class: GrcErrorList
Description: An error generated during the post-parser or pre-compiler.
Hungarian: err
----------------------------------------------------------------------------------------------*/

class GrcErrorItem
{
	friend class GrcErrorList;

public:
	GrcErrorItem(bool fFatal, GrpLineAndFile & lnf, StrAnsi staMsg, GdlObject * pgdlObj)
		:	m_fFatal(fFatal),
			m_fMsgIncludesFatality(false),
			m_lnf(lnf),
			m_staMsg(staMsg),
			m_pgdlObject(pgdlObj)
	{
	}

	bool Equivalent(GrcErrorItem * perr)
	{
		return ( //// m_pgdlObject == perr->m_pgdlObject &&
			m_staMsg == perr->m_staMsg &&
			m_lnf == perr->m_lnf &&
			m_fFatal == perr->m_fFatal);
	}

	int PreProcessedLine()
	{
		return m_lnf.PreProcessedLine();
	}

protected:
	//	instance variables:
	GdlObject *		m_pgdlObject;
	StrAnsi			m_staMsg;
	bool			m_fFatal;
	bool			m_fMsgIncludesFatality;		// don't add label "warning" or "error--the
												// message itself includes the information
	GrpLineAndFile	m_lnf;
};

/*----------------------------------------------------------------------------------------------
Class: GrcErrorList
Description: Database of errors accumulated during post-parser and pre-compiler.
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
		for (int i = 0; i < m_vperr.Size(); ++i)
			delete m_vperr[i];
	}

	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, GrpLineAndFile const& lnf)
	{
		AddItem(true, pgdlobj, &lnf, staMsg);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, GrpLineAndFile const& lnf)
	{
		AddItem(true, pgdlobj, &lnf, &staMsg, &sta2, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3,
		GrpLineAndFile const& lnf)
	{
		AddItem(true, pgdlobj, &lnf, &staMsg, &sta2, &sta3, NULL, NULL, NULL, NULL, NULL);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3, StrAnsi sta4,
		GrpLineAndFile const& lnf)
	{
		AddItem(true, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, NULL, NULL, NULL, NULL);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3, StrAnsi sta4,
		StrAnsi sta5,
		GrpLineAndFile const& lnf)
	{
		AddItem(true, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, NULL, NULL, NULL);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3, StrAnsi sta4,
		StrAnsi sta5, StrAnsi sta6,
		GrpLineAndFile const& lnf)
	{
		AddItem(true, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, NULL, NULL);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3, StrAnsi sta4,
		StrAnsi sta5, StrAnsi sta6, StrAnsi sta7,
		GrpLineAndFile const& lnf)
	{
		AddItem(true, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, NULL);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3, StrAnsi sta4,
		StrAnsi sta5, StrAnsi sta6, StrAnsi sta7, StrAnsi sta8,
		GrpLineAndFile const& lnf)
	{
		AddItem(true, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, &sta8);
	}


	void AddError(GdlObject * pgdlobj, StrAnsi staMsg)
	{
		AddItem(true, pgdlobj, NULL, staMsg);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2)
	{
		AddItem(true, pgdlobj, NULL, &staMsg, &sta2, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3)
	{
		AddItem(true, pgdlobj, NULL, &staMsg, &sta2, &sta3, NULL, NULL, NULL, NULL, NULL);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3, StrAnsi sta4)
	{
		AddItem(true, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, NULL, NULL, NULL, NULL);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3, StrAnsi sta4,
		StrAnsi sta5)
	{
		AddItem(true, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, NULL, NULL, NULL);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3, StrAnsi sta4,
		StrAnsi sta5, StrAnsi sta6)
	{
		AddItem(true, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, NULL, NULL);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3, StrAnsi sta4,
		StrAnsi sta5, StrAnsi sta6, StrAnsi sta7)
	{
		AddItem(true, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, NULL);
	}
	void AddError(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3, StrAnsi sta4,
		StrAnsi sta5, StrAnsi sta6, StrAnsi sta7, StrAnsi sta8)
	{
		AddItem(true, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, &sta8);
	}


	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, GrpLineAndFile const& lnf)
	{
		AddItem(false, pgdlobj, &lnf, staMsg);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, GrpLineAndFile const& lnf)
	{
		AddItem(false, pgdlobj, &lnf, &staMsg, &sta2, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3,
		GrpLineAndFile const& lnf)
	{
		AddItem(false, pgdlobj, &lnf, &staMsg, &sta2, &sta3, NULL, NULL, NULL, NULL, NULL);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3,
		StrAnsi sta4,
		GrpLineAndFile const& lnf)
	{
		AddItem(false, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, NULL, NULL, NULL, NULL);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3,
		StrAnsi sta4, StrAnsi sta5,
		GrpLineAndFile const& lnf)
	{
		AddItem(false, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, NULL, NULL, NULL);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3,
		StrAnsi sta4, StrAnsi sta5, StrAnsi sta6,
		GrpLineAndFile const& lnf)
	{
		AddItem(false, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, NULL, NULL);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3,
		StrAnsi sta4, StrAnsi sta5, StrAnsi sta6, StrAnsi sta7,
		GrpLineAndFile const& lnf)
	{
		AddItem(false, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, NULL);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3,
		StrAnsi sta4, StrAnsi sta5, StrAnsi sta6, StrAnsi sta7, StrAnsi sta8,
		GrpLineAndFile const& lnf)
	{
		AddItem(false, pgdlobj, &lnf, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, &sta8);
	}


	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg)
	{
		AddItem(false, pgdlobj, NULL, staMsg);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2)
	{
		AddItem(false, pgdlobj, NULL, &staMsg, &sta2, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3)
	{
		AddItem(false, pgdlobj, NULL, &staMsg, &sta2, &sta3, NULL, NULL, NULL, NULL, NULL);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3,
		StrAnsi sta4)
	{
		AddItem(false, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, NULL, NULL, NULL, NULL);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3,
		StrAnsi sta4, StrAnsi sta5)
	{
		AddItem(false, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, NULL, NULL, NULL);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3,
		StrAnsi sta4, StrAnsi sta5, StrAnsi sta6)
	{
		AddItem(false, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, NULL, NULL);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3,
		StrAnsi sta4, StrAnsi sta5, StrAnsi sta6, StrAnsi sta7)
	{
		AddItem(false, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, NULL);
	}
	void AddWarning(GdlObject * pgdlobj, StrAnsi staMsg, StrAnsi sta2, StrAnsi sta3,
		StrAnsi sta4, StrAnsi sta5, StrAnsi sta6, StrAnsi sta7, StrAnsi sta8)
	{
		AddItem(false, pgdlobj, NULL, &staMsg, &sta2, &sta3, &sta4, &sta5, &sta6, &sta7, &sta8);
	}


	void AddItem(bool fFatal, GdlObject * pgdlobj, const GrpLineAndFile *, StrAnsi staMsg);
	void AddItem(bool fFatal, GdlObject * pgdlobj, const GrpLineAndFile *,
		StrAnsi * psta1, StrAnsi * psta2, StrAnsi * psta3, StrAnsi * psta4,
		StrAnsi * psta5, StrAnsi * psta6, StrAnsi * psta7, StrAnsi * psta8);

	void SortErrors();
	int NumberOfErrors()
	{
		return m_vperr.Size();
	}
	int ErrorsAtLine(int nLine);
	int ErrorsAtLine(int nLine, int * piperrFirst);

	bool AnyFatalErrors()
	{
		return m_fFatalError;
	}

	int NumberOfWarnings();

	bool IsFatal(int iperr)
	{
		return m_vperr[iperr]->m_fFatal;
	}

	void SetLastMsgIncludesFatality(bool f)
	{
		(*m_vperr.Top())->m_fMsgIncludesFatality = f;
	}

	void WriteErrorsToFile(StrAnsi staGdlFile, StrAnsi staInputFontFile,
		StrAnsi staOutputFile, StrAnsi staOutputFamily, StrAnsi staVersion, int fSepCtrlFile)
	{
		WriteErrorsToFile("gdlerr.txt", staGdlFile, staInputFontFile, staOutputFile, staOutputFamily,
			staVersion, fSepCtrlFile);
	}
	void WriteErrorsToFile(StrAnsi staErrFile, StrAnsi staGdlFile, StrAnsi staInputFontFile,
		StrAnsi staOutputFile, StrAnsi staOutputFamily,
		StrAnsi staVersion, bool fSepCtrlFile);
	void WriteErrorsToStream(std::ostream&, StrAnsi staGdlFile, StrAnsi staInputFontFile,
		StrAnsi staOutputFile, StrAnsi staOutputFamily,
		StrAnsi staVersion, bool fSepCtrlFile);

protected:
	//	instance variables:
	bool m_fFatalError;

	Vector<GrcErrorItem *> m_vperr;

public:
	//	For test procedures:
	bool test_ErrorIs(int iperr, StrAnsi staMsg)
	{
		return (m_vperr[iperr]->m_staMsg == staMsg);
	}

	void test_Clear()
	{
		for (int i = 0; i < m_vperr.Size(); ++i)
		{
			delete m_vperr[i];
		}
		m_vperr.Clear();
		m_fFatalError = false;
	}
};


void AddGlobalError(bool fFatal, std::string msg, int nLine);
void AddGlobalError(bool fFatal, std::string msg, GrpLineAndFile const&);


#endif // ERRORS_INCLUDED
