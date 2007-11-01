/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: Fsm.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Defines the class that are used to generate the finite state machines.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef FSM_INCLUDED
#define FSM_INCLUDED


typedef std::set<GdlGlyphClassDefn *> SourceClassSet;	// hungarian: scs
typedef std::vector<FsmMachineClass *> * MachineClassList;	// hungarian: mcl

void OutputNumber(std::ostream& strmOut, int nValue, int nSpaces);

/*----------------------------------------------------------------------------------------------
Class: FsmMachineClass
Description: A machine class corresponds to one column in the FSM matrix.
Hungarian: fsmc
----------------------------------------------------------------------------------------------*/
class FsmMachineClass
{
	friend class FsmTable;
	friend class GdlPass;
	friend class FsmcLess;

public:
	FsmMachineClass(int nPass)
	{
		m_nPass = nPass;
	}

	int Column()
	{
		return m_ifsmcColumn;
	}

	void SetColumn(int ifsmc)
	{
		m_ifsmcColumn = ifsmc;
	}

	void SetSourceClasses(SourceClassSet * pscs)
	{
		for (SourceClassSet::iterator itscs = pscs->begin();
			itscs != pscs->end();
			++itscs)
		{
			m_scs.insert(*itscs);
		}
	}

	void AddGlyph(utf16 w);

	int Key(int ipass);

	bool MatchesSources(SourceClassSet *);
	bool MatchesOneSource(GdlGlyphClassDefn *);

	//	Output:
	int NumberOfRanges();
	utf16 OutputRange(utf16 wGlyphID, GrcBinaryStream * pbstrm);

protected:
	int m_nPass;

	SourceClassSet m_scs;	// source-class-set corresponding to this machine-class
	std::vector<utf16> m_wGlyphs;
	int m_ifsmcColumn;	// column this machine class is assigned to
	std::string staDebug;

	// Comparison function for set manipulation:
	bool operator<(const FsmMachineClass & fsmc) const
	{
		if (this == &fsmc)
			return false; // equal, not less than
		if (this->m_wGlyphs.size() != fsmc.m_wGlyphs.size())
			return (this->m_wGlyphs.size() < fsmc.m_wGlyphs.size());
		for (size_t iw = 0; iw < m_wGlyphs.size(); iw++)
		{
			if (this->m_wGlyphs[iw] != fsmc.m_wGlyphs[iw])
				return (this->m_wGlyphs[iw] < fsmc.m_wGlyphs[iw]);
		}
		return false; // equal, not less than
	}
};


// Functor class for set manipulation
class FsmcLess
{
	friend class FsmMachineClass;
public:
	bool operator()(const FsmMachineClass * pfsmc1, const FsmMachineClass * pfsmc2) const
	{
		return (*pfsmc1 < *pfsmc2);
	}
};

typedef std::set<FsmMachineClass *, FsmcLess> FsmMachineClassSet;
//typedef std::set<FsmMachineClass *> FsmMachineClassSet;

/*----------------------------------------------------------------------------------------------
Class: FsmState
Description: A row in the FSM table.
Hungarian: fstate
----------------------------------------------------------------------------------------------*/
class FsmState
{
	friend class FsmTable;
	friend class GdlPass;

public:
	FsmState(int ccol, int critSlots, int ifsIndex)
	{
		Init(ccol, critSlots, ifsIndex);
	}

	FsmState()
	{
		Assert(false);
		m_cfsmc = 0;
		m_prgiCells = NULL;
		m_pfstateMerged = NULL;
	}

	void Init(int cfsmc, int critSlots, int ifsIndex)
	{
		m_cfsmc = cfsmc;
		m_prgiCells = new int[cfsmc];
		memset(m_prgiCells, 0, cfsmc * sizeof(int));

		m_critSlotsMatched = critSlots;
		m_ifsWorkIndex = ifsIndex;
		m_ifsFinalIndex = -1;
		m_pfstateMerged = NULL;
	}

	~FsmState()
	{
		if (m_prgiCells)
			delete[] m_prgiCells;
	}

	int SlotsMatched()
	{
		return m_critSlotsMatched;
	}

	int WorkIndex()
	{
		return m_ifsWorkIndex;
	}

	int CellValue(int ifsmc)
	{
		Assert(m_prgiCells);
		return m_prgiCells[ifsmc];
	}

	void SetCellValue(int ifsmc, int ifsValue)
	{
		Assert(m_prgiCells);
		m_prgiCells[ifsmc] = ifsValue;
	}

	int NumberOfRulesMatched()
	{
		return m_setiruleMatched.size();
	}

	bool RuleMatched(int irule)
	{
		return (m_setiruleMatched.find(irule) != m_setiruleMatched.end()); // is a member
	}

	void AddRuleToMatchedList(int irule)
	{
		if (m_setiruleMatched.find(irule) == m_setiruleMatched.end()) // not a member
			m_setiruleMatched.insert(irule);
	}

	int NumberOfRulesSucceeded()
	{
		return m_setiruleSuccess.size();
	}

	bool RuleSucceeded(int irule)
	{
		return (m_setiruleSuccess.find(irule) != m_setiruleSuccess.end()); // is a member
	}

	void AddRuleToSuccessList(int irule)
	{
		if (m_setiruleSuccess.find(irule) == m_setiruleSuccess.end())  // not a member
			m_setiruleSuccess.insert(irule);
	}

	bool StatesMatch(FsmState * pfState);

	bool HasBeenMerged()
	{
		return (m_pfstateMerged != NULL);
	}

	FsmState * MergedState()
	{
		return m_pfstateMerged;
	}

	void SetMergedState(FsmState * pfsmc)
	{
		m_pfstateMerged = pfsmc;
	}

	void SetFinalIndex(int n)
	{
		m_ifsFinalIndex = n;
	}

	bool AllCellsEmpty()
	{
		for (int ifsmc = 0; ifsmc < m_cfsmc; ifsmc++)
		{
			if (m_prgiCells[ifsmc] != 0)
				return false;
		}
		return true;
	}

protected:
	int m_critSlotsMatched;	// number of slots matched at this state
	int m_cfsmc;			// number of machine classes, ie columns
	int m_ifsWorkIndex;		// working index of this state, as the FSM was originally
							// generated (currently just used for debugging)
	int m_ifsFinalIndex;	// adjusted index for final output form of FSM (-1 for merged
							// states)

	std::set<int> m_setiruleMatched;	// indices of rules matched by this state
	std::set<int> m_setiruleSuccess;	// indices of rules for which this is a success state

	int * m_prgiCells;	// the cells of the state, holding the index of the state
						// to transition to


	FsmState * m_pfstateMerged;		// pointer to an earlier state that is identical to this
									// one (if any), so this state is considered merged with
									// that one


public:
	//	Debuggers:
	void DebugFsmState(std::ostream & strmOut, int ifs);
};



/*----------------------------------------------------------------------------------------------
Class: FsmTable
Description: A single finite state machine.
Hungarian: fsm
----------------------------------------------------------------------------------------------*/
class FsmTable
{
	friend class GdlPass;

public:
	FsmTable(int nPass, int cfsmc)
	{
		m_nPass = nPass;
		m_cfsmc = cfsmc;
	}

	~FsmTable()
	{
		for (size_t ipfstate = 0; ipfstate < m_vpfstate.size(); ipfstate++)
			delete m_vpfstate[ipfstate];
	}

	void AddState(int critSlotsMatched)
	{
		FsmState * pfstateNew = new FsmState(m_cfsmc, critSlotsMatched, m_vpfstate.size());
		m_vpfstate.push_back(pfstateNew);
	}

	FsmState * StateAt(int ifs)
	{
		FsmState * pfstateRet = RawStateAt(ifs);
		if (pfstateRet->HasBeenMerged())
			return pfstateRet->MergedState();

		return pfstateRet;
	}

	FsmState * RawStateAt(int ifs)
	{
		return m_vpfstate[ifs];
	}

	int RawNumberOfStates()
	{
		return signed(m_vpfstate.size());
	}

	int NumberOfColumns()
	{
		return m_cfsmc;
	}

protected:
	int m_nPass;	// the pass this table pertains to
	int m_cfsmc;	// the number of machine classes, ie columns, in the table
	std::vector<FsmState *> m_vpfstate;	// the rows
};


#endif // !FSM_INCLUDED
