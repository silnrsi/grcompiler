/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GdlGlyphClass.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Definitions of classes of glyphs.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef CLASSES_INCLUDED
#define CLASSES_INCLUDED

class GdlGlyphDefn;
//class PseudoLess;
//typedef std::set<GdlGlyphDefn *, PseudoLess> PseudoSet; // PseudoLess isn't implemented adquately yet
typedef std::set<GdlGlyphDefn *> PseudoSet;

//class ReplClassLess;
//typedef std::set<GdlGlyphClassDefn *, ReplClassLess> ReplacementClassSet;

/*----------------------------------------------------------------------------------------------
Class: GdlGlyphAttrSetting
Description: The setting of a glyph attribute--attribute name and expression indicating
	the value.
Hungarian: glfa
----------------------------------------------------------------------------------------------*/

class GdlGlyphAttrSetting : public GdlObject
{
	friend class GdlGlyphClassDefn;

public:
	//	Constructors & destructors:
	GdlGlyphAttrSetting(Symbol psym, GdlAssignment * pasgn)
		:	m_psym(psym),
			m_pasgn(pasgn)
	{
		SetLineAndFile(pasgn->LineAndFile());
	}

	~GdlGlyphAttrSetting()
	{
		Assert(m_pasgn);
		delete m_pasgn;
	}

	Symbol GlyphSymbol()				{ return m_psym; }
	GdlAssignment * Assignment()		{ return m_pasgn; }
	GdlExpression * Expression()		{ return m_pasgn->Expression(); }

protected:
	//	Instance variables:
	Symbol			m_psym;
	GdlAssignment * m_pasgn;
};


/*----------------------------------------------------------------------------------------------
Class: GdlGlyphClassMember
Description: Abstract class subsuming GdlGlyphClassDefn and GdlGlyphDefn, ie, an element
	of a class.
Hungarian: glfd
----------------------------------------------------------------------------------------------*/
class GdlGlyphClassMember : public GdlDefn
{
public:
	GdlGlyphClassMember()
		: m_fIsSpaceGlyph(-1)
	{
	};
	virtual ~GdlGlyphClassMember()
	{
	};

	//	Pre-compiler:
	virtual void ExplicitPseudos(PseudoSet & setpglf) = 0;
	virtual int ActualForPseudo(utf16 wPseudo) = 0;
	virtual int GlyphIDCount() = 0;
	virtual unsigned int FirstGlyphInClass(bool * pfMoreThanOne) = 0;
	virtual void AssignGlyphIDsToClassMember(GrcFont *, utf16 wGlyphIDLim,
		std::map<utf16, utf16> & hmActualForPseudo,
		bool fLookUpPseudo = true) = 0;
	virtual void AssignGlyphAttrsToClassMembers(GrcGlyphAttrMatrix * pgax,
		GdlRenderer * prndr, GrcLigComponentList * plclist,
		std::vector<GdlGlyphAttrSetting *> & vpglfaAttrs, int cgid, int & igid) = 0;
	virtual void CheckExistenceOfGlyphAttr(GdlObject * pgdlAvsOrExp,
		GrcSymbolTable * psymtbl, GrcGlyphAttrMatrix * pgax, Symbol psymGlyphAttr) = 0;
	virtual void CheckCompleteAttachmentPoint(GdlObject * pgdlAvsOrExp,
		GrcSymbolTable * psymtbl, GrcGlyphAttrMatrix * pgax, Symbol psymGlyphAttr,
		bool * pfXY, bool * pfGpoint) = 0;
	virtual void CheckCompBox(GdlObject * pritset,
		GrcSymbolTable * psymtbl, GrcGlyphAttrMatrix * pgax, Symbol psymCompRef) = 0;
	virtual void StorePseudoToActualAsGlyphAttr(GrcGlyphAttrMatrix * pgax, int nAttrID,
		std::vector<GdlExpression *> & vpexpExtra) = 0;
	virtual bool IncludesGlyph(utf16) = 0;
	virtual bool HasOverlapWith(GdlGlyphClassMember * glfd, GrcFont * pfont) = 0;
	virtual bool HasBadGlyph() = 0;
	virtual bool WarnAboutBadGlyphs(bool fTop) = 0;
	virtual bool DeleteBadGlyphs() = 0;
	virtual void FlattenGlyphList(std::vector<utf16> & vgidFlattened) = 0;
	virtual bool IsSpaceGlyph(std::vector<utf16> & vwSpaceGlyphs) = 0;

public:
	//	Compiler:
	virtual void RecordInclusionInClass(GdlPass * ppass, GdlGlyphClassDefn * pglfc) = 0;
	virtual void GetMachineClasses(FsmMachineClass ** ppfsmcAssignments,
		FsmMachineClassSet & setpfsmc) = 0;

	//	Output:
	virtual void AddGlyphsToUnsortedList(std::vector<utf16> & vwGlyphs) = 0;
	virtual void AddGlyphsToSortedList(std::vector<utf16> & vwGlyphs, std::vector<int> & vnIndices) = 0;

	//	debuggers
	virtual void DebugCmapForMember(GrcFont * pfont,
		utf16 * rgchwUniToGlyphID, unsigned int * rgnGlyphIDToUni) = 0;
	virtual void DebugXmlClassMembers(std::ofstream & strmOut, 
		GdlGlyphClassDefn * pglfdParent, GrpLineAndFile lnf, int & cwGlyphIDs) = 0;

protected:
	int m_fIsSpaceGlyph;	// Probably mistakenly I thought the CalculateSpaceContextuals routine
							// was slow, hence the value is cached. It's probably not needed.

};


/*----------------------------------------------------------------------------------------------
Class: GdlGlyphClassDefn
Description: A class of glyphs and glyph attribute settings.
Hungarian: glfc
----------------------------------------------------------------------------------------------*/
class GdlGlyphClassDefn : public GdlGlyphClassMember
{
	friend class GdlGlyphDefn;
	friend class ReplClassLess;

public:
	//	Constructors & destructors:
	GdlGlyphClassDefn()
		: GdlGlyphClassMember()
	{
		
		m_fReplcmtIn = false;
		m_fReplcmtOut = false;
		m_nReplcmtInID = -1;
		m_nReplcmtOutID = -1;
		m_fHasFlatList = false;
	}

	~GdlGlyphClassDefn();

	void DeleteGlyphDefns();

	//	Getters:
	std::string Name()		{ return m_staName; }

	//	Setters:
	void SetName(std::string sta)		{ m_staName = sta; }

	void AddElement(GdlGlyphClassMember * pglfdElement, GrpLineAndFile const& lnf,
	   GlyphClassType glfct);
	void AddMember(GdlGlyphClassMember * pglfd, GrpLineAndFile const& lnf);

	void AddGlyphAttr(Symbol, GdlAssignment * pasgn);
	void AddComponent(Symbol, GdlAssignment * pasgn);

	static std::string Undefined()
	{
		return "*GCUndefined*";
	}

	//	Parser:
	GdlGlyphClassMember * AddGlyphToClass(GrpLineAndFile const& lnf,
		GlyphType glft, int nFirst);
	GdlGlyphClassMember * AddGlyphToClass(GrpLineAndFile const& lnf,
		GlyphType glft, int nFirst, int nLast);
	GdlGlyphClassMember * AddGlyphToClass(GrpLineAndFile const& lnf,
		GlyphType glft, int nFirst, int nLast, utf16 wCodePage);
	GdlGlyphClassMember * AddGlyphToClass(GrpLineAndFile const& lnf,
		GlyphType glft, std::string staPostscript);
	GdlGlyphClassMember * AddGlyphToClass(GrpLineAndFile const& lnf,
		GlyphType glft, std::string staCodepoints, utf16 wCodePage);
	GdlGlyphClassMember * AddGlyphToClass(GrpLineAndFile const& lnf,
		GlyphType glft, GdlGlyphDefn * pglfOutput, utf16 nPseudoInput);
	GdlGlyphClassMember * AddGlyphToClass(GrpLineAndFile const& lnf,
		GlyphType glft, GdlGlyphDefn * pglfOutput);
	GdlGlyphClassMember * AddClassToClass(GrpLineAndFile const& lnf,
		GdlGlyphClassDefn * pglfcMember);

	//	Pre-compiler:
	virtual void ComputeMembers()
	{	
		// Overridden by intersection and difference classes to do something interesting.
	}

	virtual void ExplicitPseudos(PseudoSet & setpglf);
	virtual int ActualForPseudo(utf16 wPseudo);
	virtual void AssignGlyphIDs(GrcFont *, utf16 wGlyphIDLim,
		std::map<utf16, utf16> & hmActualForPseudos);
	virtual void AssignGlyphIDsToClassMember(GrcFont *, utf16 wGlyphIDLim,
		std::map<utf16, utf16> & hmActualForPseudo,
		bool fLookUpPseudo = true);
	virtual int GlyphIDCount();
	void MaxJustificationLevel(int * pnJLevel);
	virtual unsigned int FirstGlyphInClass(bool * pfMoreThanOne);
	void AssignGlyphAttrsToClassMembers(GrcGlyphAttrMatrix * pgax,
		GdlRenderer * prndr, GrcLigComponentList * plclist);
	virtual void AssignGlyphAttrsToClassMembers(GrcGlyphAttrMatrix * pgax,
		GdlRenderer * prndr, GrcLigComponentList * plclist,
		std::vector<GdlGlyphAttrSetting *> & vpglfaAttrs, int cgid, int & igid);
	virtual void CheckExistenceOfGlyphAttr(GdlObject * pgdlAvsOrExp,
		GrcSymbolTable * psymtbl, GrcGlyphAttrMatrix * pgax, Symbol psymGlyphAttr);
	virtual void CheckCompleteAttachmentPoint(GdlObject * pgdlAvsOrExp,
		GrcSymbolTable * psymtbl, GrcGlyphAttrMatrix * pgax, Symbol psymGlyphAttr,
		bool * pfXY, bool * pfGpoint);
	virtual void CheckCompBox(GdlObject * pritset,
		GrcSymbolTable * psymtbl, GrcGlyphAttrMatrix * pgax, Symbol psymCompRef);
	virtual void StorePseudoToActualAsGlyphAttr(GrcGlyphAttrMatrix * pgax, int nAttrID,
		std::vector<GdlExpression *> & vpexpExtra);
	void MarkFsmClass(int nPassID, int nClassID);

	bool IsFsmClass(int ipass)
	{
		if (ipass >= signed(m_vfFsm.size()))
			return false;
		return m_vfFsm[ipass];
	}
	int FsmID(int ipass)
	{
		return m_vnFsmID[ipass];
	}

	virtual bool IncludesGlyph(utf16);

	void MarkReplcmtInputClass()			{ m_fReplcmtIn = true; }
	void MarkReplcmtOutputClass()			{ m_fReplcmtOut = true; }
	void SetReplcmtInputID(int nID)			{ m_nReplcmtInID = nID; }
	void SetReplcmtOutputID(int nID)		{ m_nReplcmtOutID = nID; }

	bool ReplcmtInputClass()				{ return m_fReplcmtIn; }
	bool ReplcmtOutputClass()				{ return m_fReplcmtOut; }
	int ReplcmtInputID()					{ return m_nReplcmtInID; }
	int ReplcmtOutputID()					{ return m_nReplcmtOutID; }

	bool CompatibleWithVersion(int fxdVersion, int * pfxdNeeded, int * pfxdCpilrNeeded);

	virtual bool HasOverlapWith(GdlGlyphClassMember * glfd, GrcFont * pfont);
	virtual bool HasBadGlyph();
	virtual bool WarnAboutBadGlyphs(bool fTop);
	virtual bool DeleteBadGlyphs();
	virtual bool IsSpaceGlyph(std::vector<utf16> & vwSpaceGlyphs);

public:
	//	Compiler:
	void RecordInclusionInClass(GdlPass * ppass);
	virtual void RecordInclusionInClass(GdlPass * ppass, GdlGlyphClassDefn * pglfc);
	virtual void GetMachineClasses(FsmMachineClass ** ppfsmcAssignments,
		FsmMachineClassSet & setpfsmc);

	//	Output
	void GenerateOutputGlyphList(std::vector<utf16> & vwGlyphs);
	void GenerateInputGlyphList(std::vector<utf16> & vwGlyphs, std::vector<int> & vnIndices);
	void AddGlyphsToUnsortedList(std::vector<utf16> & vwGlyphs);
	void AddGlyphsToSortedList(std::vector<utf16> & vwGlyphs, std::vector<int> & vnIndices);

	//	debuggers
	void DebugCmap(GrcFont * pfont,
		utf16 * rgchwUniToGlyphID, unsigned int * rgnGlyphIDToUni);
	virtual void DebugCmapForMember(GrcFont * pfont,
		utf16 * rgchwUniToGlyphID, unsigned int * rgnGlyphIDToUni);
	void DebugXmlClasses(std::ofstream & strmOut, int & cwGlyphIDs);
	virtual void DebugXmlClassMembers(std::ofstream & strmOut,
		GdlGlyphClassDefn * pglfdParent, GrpLineAndFile lnf, int & cwGlyphIDs);
	void RecordSingleMemberClasses(std::vector<std::string> & vstaSingleMemberClasses);

	void FlattenMyGlyphList()
	{
		if (!m_fHasFlatList)
		{
			for (size_t i = 0; i < m_vpglfdMembers.size(); i++)
				m_vpglfdMembers[i]->FlattenGlyphList(m_vgidFlattened);
			m_fHasFlatList = true;
		}
	}
	virtual void FlattenGlyphList(std::vector<utf16> & vgidFlattened);

	GrpLineAndFile & LineAndFileForMember(int iglfd)
	{
		return m_vlnfMembers[iglfd];
	}


protected:
	//	Instance variables:
	std::string							m_staName;
	std::vector<GdlGlyphClassMember*>	m_vpglfdMembers;
	std::vector<GrpLineAndFile>			m_vlnfMembers;	// where each member was added, for debugger file

	std::vector<GdlGlyphAttrSetting*>	m_vpglfaAttrs;

	// Flat list of included glyph ids; needed to generate a key for a replacement-class set.
	std::vector<utf16> m_vgidFlattened;
	bool m_fHasFlatList;

//	std::vector<GdlGlyphAttrSetting*>	m_vpglfaComponents;
//	std::vector<std::string>			m_vstaComponentNames;	// redundant with what is in components
																// list, but more accessible
//	GdlExpression *		m_pexpDirection;
//	int					m_nDirStmtNo;
//	GdlExpression *		m_pexpBreakweight;
//	int					m_nBwStmtNo;

	//	for compiler use:
	std::vector<bool>	m_vfFsm;		// needs to be matched by the FSM, one flag for each pass
	std::vector<int>	m_vnFsmID;		// FSM class ID, one for each pass

	bool	m_fReplcmtIn;		// serves as an input class for replacement
	bool	m_fReplcmtOut;		// serves as an output class for replacement
	int		m_nReplcmtInID;		// internal ID when serving as replacement input class
	int		m_nReplcmtOutID;	// internal ID when serving as replacement output class

	// Comparing two class definitions for insertion in a set; this assumes they are used as
	// replacement classes and therefore the order of the items is signficant.
	bool ReplClassLessThan(const GdlGlyphClassDefn * pglfc) const
	{
		//if (!this->m_fHasFlatList)
		//	this->FlattenMyGlyphList();
		//if (!pglfc->m_fHasFlatList)
		//	pglfc->FlattenMyGlyphList();
		Assert(this->m_fHasFlatList);	// obsolete
		Assert(pglfc->m_fHasFlatList);

		if (this == pglfc)
			return false;

		//for (int igid = 0; igid < signed(m_vgidFlattened.size()); igid++)
		//{
		//	if (signed(pglfc->m_vgidFlattened.size()) < igid - 1)
		//		return false;
		//	if (this->m_vgidFlattened[igid] < pglfc->m_vgidFlattened[igid])
		//		return true;
		//	if (this->m_vgidFlattened[igid] > pglfc->m_vgidFlattened[igid])
		//		return false;
		//}
		//// At this point either the two classes are exactly the same, or pglfc is longer.
		//if (pglfc->m_vgidFlattened.size() > this->m_vgidFlattened.size())
		//	return true;

		// The contents of the two classes are the same; compare the names.
		return (strcmp(this->m_staName.c_str(), pglfc->m_staName.c_str()) < 0);
	}

};	// end of class GdlGlyphClassDefn


/*----------------------------------------------------------------------------------------------
Class: GdlGlyphIntersectionClassDefn
Description: A class of glyphs and glyph attribute settings that is calculated via an
	intersection of two other classes.
Hungarian: glfci
----------------------------------------------------------------------------------------------*/
class GdlGlyphIntersectionClassDefn : public GdlGlyphClassDefn
{
public:
	GdlGlyphIntersectionClassDefn()
		: GdlGlyphClassDefn()
	{
	};
	virtual ~GdlGlyphIntersectionClassDefn()
	{
	};
	void DeleteGlyphDefns();

	void AddSet(GdlGlyphClassMember * pglfd, GrpLineAndFile const& lnf);

	// Pre-compiler:
	virtual void AssignGlyphIDs(GrcFont * pfont, utf16 wGlyphIDLim,
		std::map<utf16, utf16> & hmActualForPseudo);
	virtual void ComputeMembers();

protected:
	// These sets are intersected to create the list of members:
	std::vector<GdlGlyphClassMember*>	m_vpglfdSets;
	std::vector<GrpLineAndFile>			m_vlnfSets;		// where each set was added, for debugger file
};


/*----------------------------------------------------------------------------------------------
Class: GdlGlyphDifferenceClassDefn
Description: A class of glyphs and glyph attribute settings that is calculated using the
	difference of two other classes.
Hungarian: glfcd
----------------------------------------------------------------------------------------------*/
class GdlGlyphDifferenceClassDefn : public GdlGlyphClassDefn
{
public:
	GdlGlyphDifferenceClassDefn()
		: GdlGlyphClassDefn()
	{
	};
	virtual ~GdlGlyphDifferenceClassDefn()
	{
	};
	void DeleteGlyphDefns();

	void AddMinuend(GdlGlyphClassMember * pglfd, GrpLineAndFile const& lnf);
	void AddSubtrahend(GdlGlyphClassMember * pglfd, GrpLineAndFile const& lnf);
	bool HasMinuend()
	{
		return (m_pglfdMinuend != NULL);
	}

	// Pre-compiler:
	virtual void AssignGlyphIDs(GrcFont * pfont, utf16 wGlyphIDLim,
		std::map<utf16, utf16> & hmActualForPseudo);
	virtual void ComputeMembers();

protected:
	// The subtrahend is subtracted from the minuend to create the list of members:
	GdlGlyphClassMember *	m_pglfdMinuend;
	GrpLineAndFile			m_lnfMinuend;		// where minuend was added, for debugger file

	GdlGlyphClassMember *	m_pglfdSubtrahend;
	GrpLineAndFile			m_lnfSubtrahend;	// where subtrahend was added, for debugger file
};


// Functor class for set manipulation.
class ReplClassLess
{
		friend class GdlGlyphClassDefn;
public:
	bool operator()(const GdlGlyphClassDefn * pglfc1, const GdlGlyphClassDefn * pglfc2) const
	{
		return (pglfc1->ReplClassLessThan(pglfc2));
	}
};


#endif // CLASSES_INCLUDED
