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
#include <cassert>
#include "main.h"

#ifdef _MSC_VER
#pragma hdrstop
#endif
#undef THIS_FILE
DEFINE_THIS_FILE

/***********************************************************************************************
	Tables, Passes, and Rules
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Do the pre-compilation tasks for the tables, passes, and rules. Return false if
	compilation cannot continue due to an unrecoverable error.
----------------------------------------------------------------------------------------------*/
bool GrcManager::PreCompileRules(GrcFont * pfont)
{
	int cpassValid;	// number of valid passes

	if (!m_prndr->CheckTablesAndPasses(this, &cpassValid))
		return false;

	// Before prepending ANYs, calculate the space contextual flags.
	m_prndr->CalculateSpaceContextuals(pfont);

	//	Fix up the rules that have items in the context before the first modified item.
	Symbol psymAnyClass = m_psymtbl->FindSymbol("ANY");
	if (!m_prndr->FixRulePreContexts(psymAnyClass))
		return false;

	//	In preparation for the next step, create the fsm class vectors for each pass.
	m_prgvpglfcFsmClasses = new std::vector<GdlGlyphClassDefn *>[cpassValid];

	if (!AssignClassInternalIDs())
		return false;

	if (!m_prndr->CheckRulesForErrors(m_pgax, pfont))
		return false;

	if (!m_prndr->CheckLBsInRules())
		return false;

	m_prndr->RewriteSlotAttrAssignments(this, pfont);

	this->DetermineTableVersion();

	return true;
}


/**********************************************************************************************/
/*----------------------------------------------------------------------------------------------
	Determine the table version needed depending on various factors.
----------------------------------------------------------------------------------------------*/
void GrcManager::DetermineTableVersion()
{
	int fxdVersionNeeded;
	bool fFixPassConstraints = true;	// remains true if the only thing that is incompatible are
										// the pass constraints
	int fxdRequested = SilfTableVersion();
	SetCompilerVersionFor(fxdRequested);
	int fxdCompilerVersionNeeded = m_fxdCompilerVersion;
	if (!CompatibleWithVersion(fxdRequested, &fxdVersionNeeded, &fxdCompilerVersionNeeded,
			&fFixPassConstraints))
	{
		if (fFixPassConstraints && fxdRequested <= 0x00030000 && fxdVersionNeeded > 0x00030000)
		{
			if (UserSpecifiedVersion())
			{
				// Converting pass constraints to rule constraints should take care of the
				// incompatibility.
				m_prndr->MovePassConstraintsToRules(fxdRequested);
			}
			else
			{
				g_errorList.AddWarning(3501, NULL,
					"Version ",
					VersionString(fxdRequested),
					" of the Silf table is inadequate to handle pass constraints; version ",
					VersionString(fxdVersionNeeded),
					" will be generated.");
				FixSilfTableVersion(fxdVersionNeeded);
			}
		}
		else 
		{
			if (UserSpecifiedVersion())
			{
				g_errorList.AddWarning(3501, NULL,
					"Version ",
					VersionString(fxdRequested),
					" of the Silf table is inadequate for your specification; version ",
					VersionString(fxdVersionNeeded),
					" will be generated instead.");
			}
			FixSilfTableVersion(fxdVersionNeeded);
		}
		
	}
	else if (fxdVersionNeeded > fxdRequested)
	{
		// Eg, 2.0 -> 2.1
		FixSilfTableVersion(fxdVersionNeeded);
	}

}

/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Assign each pass a global ID number. Record a warning if the pass numbers are not
	sequential for a given table, or if there are rules in an unspecified pass. Return
	the number of valid passes (those with rules in them).
----------------------------------------------------------------------------------------------*/
bool GdlRenderer::CheckTablesAndPasses(GrcManager * pcman, int * pcpassValid)
{
	int nPassNum = 0;

	GdlRuleTable * prultbl;

	m_ipassBidi = -1;

	if ((prultbl = FindRuleTable("linebreak")) != NULL)
		prultbl->CheckTablesAndPasses(pcman, &nPassNum, &m_ipassBidi);

	if ((prultbl = FindRuleTable("substitution")) != NULL)
		prultbl->CheckTablesAndPasses(pcman, &nPassNum, &m_ipassBidi);

	//if (Bidi())
	//	m_ipassBidi = nPassNum;
	//else
	//	m_ipassBidi = -1;

	if ((prultbl = FindRuleTable("justification")) != NULL)
		prultbl->CheckTablesAndPasses(pcman, &nPassNum, &m_ipassBidi);

	if ((prultbl = FindRuleTable("positioning")) != NULL)
		prultbl->CheckTablesAndPasses(pcman, &nPassNum, &m_ipassBidi);

	//	At this point nPassNum = the number of valid passes.
	*pcpassValid = nPassNum;

	if (this->Bidi() && m_ipassBidi == -1)
		m_ipassBidi = nPassNum;

	if (nPassNum >= kMaxPasses)
	{
		g_errorList.AddError(3101, NULL,
			"Number of passes (",
			std::to_string(nPassNum),
			") exceeds maximum of ",
			std::to_string(kMaxPasses - 1));
	}
	else if (nPassNum == 0)
	{
		g_errorList.AddWarning(3502, NULL,
			"No valid passes");
	}

	return true;
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::CheckTablesAndPasses(GrcManager * pcman, int *pnPassNum, int *pipassBidi)
{
	if (m_vppass.size() > 1 && m_vppass[0]->HasRules())
	{
		g_errorList.AddError(3102, this,
			m_psymName->FullName(),
			" table is multi-pass, so all rules must be explicitly placed in a pass");

		// but go ahead and treat it as the zeroth pass for now
	}

	for (auto ipass = 0U; ipass < m_vppass.size(); ++ipass)
	{
		auto const staPass = std::to_string(ipass);
		if (m_vppass[ipass]->ValidPass())
		{
			m_vppass[ipass]->AssignGlobalID(*pnPassNum);
			(*pnPassNum)++;

			int fsdcPassDir = m_vppass[ipass]->Direction();
			int fsdcScriptDir = pcman->Renderer()->ScriptDirections();
			if (fsdcPassDir != kfsdcNone && fsdcPassDir != kfsdcHorizLtr && fsdcPassDir != kfsdcHorizRtl)
			{
				g_errorList.AddError(3166, this,
					"Invalid direction for pass ",
					staPass);
			}
			else if (fsdcPassDir != kfsdcNone && fsdcScriptDir != kfsdcHorizLtr && fsdcScriptDir != kfsdcHorizRtl)
			{
				g_errorList.AddError(3167, this,
					"Direction directive invalid (on pass ",
					staPass,
					") due to font ScriptDirection setting of ",
					std::to_string(fsdcScriptDir));
			}
			else if (fsdcPassDir != kfsdcNone && fsdcScriptDir == fsdcPassDir)
			{
				g_errorList.AddWarning(3539, this,
					"Direction of pass ",
					staPass,
					" matches font's ScriptDirection and will have no effect ");
			}
			else
			{
				Assert(fsdcScriptDir >= kfsdcNone);
				Assert(fsdcScriptDir <= kfsdcHorizRtl);
				if (fsdcPassDir > kfsdcNone)
				{
					Assert(fsdcScriptDir != fsdcPassDir);
					m_vppass[ipass]->SetFlipDirection(true);
					pcman->Renderer()->SetHasFlippedPass(true);
				}
			}
		}
		else if (ipass == 0)
		{
			// okay--zeroth pass *should* be empty if there are other passes
			m_vppass[ipass]->AssignGlobalID(-1);
		}
		else
		{
			g_errorList.AddWarning(3503, this,
				"Pass ",
				staPass,
				" of ",
				m_psymName->FullName(),
				" table contains no rules");
			m_vppass[ipass]->AssignGlobalID(-1);
		}
	}
}

/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Add ANY classes to the beginning of rules to ensure that all rules in a pass have
	the same number of items before the first modified item.
----------------------------------------------------------------------------------------------*/
bool GdlRenderer::FixRulePreContexts(Symbol psymAnyClass)
{
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		m_vprultbl[iprultbl]->FixRulePreContexts(psymAnyClass);
	}
	return true;
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::FixRulePreContexts(Symbol psymAnyClass)
{
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		m_vppass[ippass]->FixRulePreContexts(psymAnyClass);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlPass::FixRulePreContexts(Symbol psymAnyClass)
{
	m_critMinPreContext = kMaxSlotsPerRule;
	m_critMaxPreContext = 0;

	//	First, calculate the maximum and minimum pre-contexts lengths for all the rules
	//	in this pass. Also record the original rule length to use as a sort key.
	for (auto prule: m_vprule)
	{
		auto crit = prule->CountRulePreContexts();
		m_critMinPreContext = min(m_critMinPreContext, crit);
		m_critMaxPreContext = max(m_critMaxPreContext, crit);
	}

	//	Now add "ANY" class slots to the beginning of each rule to make every rule have
	//	the same number of pre-context items.
	for (auto prule: m_vprule)
		prule->FixRulePreContexts(psymAnyClass, int(m_critMaxPreContext));
}

/*----------------------------------------------------------------------------------------------
	Return the number of items at the beginning of the context that are not modified.
	Also record the original rule length to use as a sort key (before it is modified
	by adding ANY classes to the beginning of the rule).
----------------------------------------------------------------------------------------------*/
size_t GdlRule::CountRulePreContexts()
{
	m_critOriginal = m_vprit.size();

	m_critPreModContext = 0;
	for (auto const &rit:  m_vprit)
	{
		if (dynamic_cast<GdlSetAttrItem *>(rit))
			return m_critPreModContext;
		m_critPreModContext++;
	}

	//	Should have hit at least on modified item.
	Assert(false);
	return m_critPreModContext;
}

/*----------------------------------------------------------------------------------------------
	Add instances of ANY slots to the beginning of the rule until the rule has the given
	number of items before the first modified item.
----------------------------------------------------------------------------------------------*/
void GdlRule::FixRulePreContexts(Symbol psymAnyClass, int critNeeded)
{
	m_critPrependedAnys = critNeeded - m_critPreModContext;
	if (m_critPrependedAnys == 0)
		return;

	for (auto iritToAdd = 0U; iritToAdd < m_critPrependedAnys; ++iritToAdd)
	{
		GdlRuleItem * prit = new GdlRuleItem(psymAnyClass);
		prit->SetLineAndFile(LineAndFile());
		prit->m_iritContextPos = iritToAdd;
		m_vprit.insert(m_vprit.begin() + iritToAdd, prit);
	}

	//	Increment the item positions following the inserted items, and adjust any slot
	//	references.
	for (auto irit = m_critPrependedAnys; irit < m_vprit.size(); ++irit)
	{
		m_vprit[irit]->IncContextPosition(int(m_critPrependedAnys));
		m_vprit[irit]->AdjustSlotRefsForPreAnys(int(m_critPrependedAnys));
	}

	//	Increment the scan advance position, if any.
	if (m_nScanAdvance > -1)
		m_nScanAdvance += int(m_critPrependedAnys);
}

/*----------------------------------------------------------------------------------------------
	Adjust slot references to take into account the fact that ANYS have been prepended
	to the beginning of the rule.
----------------------------------------------------------------------------------------------*/
void GdlRuleItem::AdjustSlotRefsForPreAnys(int critPrependedAnys)
{
	if (m_pexpConstraint)
		m_pexpConstraint->AdjustSlotRefsForPreAnys(critPrependedAnys);
}

/*--------------------------------------------------------------------------------------------*/
void GdlLineBreakItem::AdjustSlotRefsForPreAnys(int critPrependedAnys)
{
	GdlRuleItem::AdjustSlotRefsForPreAnys(critPrependedAnys);
}

/*--------------------------------------------------------------------------------------------*/
void GdlSetAttrItem::AdjustSlotRefsForPreAnys(int critPrependedAnys)
{
	GdlRuleItem::AdjustSlotRefsForPreAnys(critPrependedAnys);

	for (size_t i = 0; i < m_vpavs.size(); i++)
		m_vpavs[i]->AdjustSlotRefsForPreAnys(critPrependedAnys, this);
}

/*--------------------------------------------------------------------------------------------*/
void GdlSubstitutionItem::AdjustSlotRefsForPreAnys(int critPrependedAnys)
{
	GdlSetAttrItem::AdjustSlotRefsForPreAnys(critPrependedAnys);

	if (m_pexpSelector)
		m_pexpSelector->AdjustSlotRefsForPreAnys(critPrependedAnys);

	for (size_t ipexp = 0; ipexp < m_vpexpAssocs.size(); ipexp++)
		m_vpexpAssocs[ipexp]->AdjustSlotRefsForPreAnys(critPrependedAnys);
}

/*--------------------------------------------------------------------------------------------*/
void GdlAttrValueSpec::AdjustSlotRefsForPreAnys(int critPrependedAnys, GdlRuleItem * /*prit*/)
{
	Assert(m_psymName->FitsSymbolType(ksymtSlotAttr));

	m_pexpValue->AdjustSlotRefsForPreAnys(critPrependedAnys);
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Mark classes that are used for substitution and FSM matching, and assign them
	internal IDs.
----------------------------------------------------------------------------------------------*/
bool GrcManager::AssignClassInternalIDs()
{
	ReplacementClassSet setpglfc;
	m_prndr->MarkReplacementClasses(this, setpglfc);

	//	Now that we've given warnings about invalid glyphs, delete them from the classes.
	m_prndr->DeleteAllBadGlyphs();

	//	Now actually assign the IDs for all substitution classes in the resulting set.
	//	The first batch of sub-IDs are assigned to the output classes, the second
	//	batch to input classes. Note that some classes may have both an input
	//	and an output ID.
	int nSubID = 0;
	ReplacementClassSet::iterator itset;
	for (itset = setpglfc.begin();
		itset != setpglfc.end();
		++itset)
	{
		if ((*itset)->ReplcmtOutputClass())
		{
			(*itset)->SetReplcmtOutputID(nSubID);
			m_vpglfcReplcmtClasses.push_back(*itset);
			nSubID++;
		}
	}

	//	Next do the input classes that have only one glyph; they can also be in linear format.
	for (itset = setpglfc.begin();
		itset != setpglfc.end();
		++itset)
	{
		GdlGlyphClassDefn * pglfc = *itset;
		if (pglfc->ReplcmtInputClass() && pglfc->GlyphIDCount() <= 1)
		{
			if (pglfc->ReplcmtOutputClass())
				//	Already an output class; don't need to include it again.
				pglfc->SetReplcmtInputID(pglfc->ReplcmtOutputID());
			else
			{
				pglfc->SetReplcmtInputID(nSubID);
				m_vpglfcReplcmtClasses.push_back(*itset);
				nSubID++;
			}
		}
	}

	m_cpglfcLinear = nSubID;

	//	Finally do the input classes that have multiple glyphs. These are the classes that
	//	cannot be in linear format; they must be in indexed format (glyph ID / index pair,
	//	ordered by glyph ID).

	for (itset = setpglfc.begin();
		itset != setpglfc.end();
		++itset)
	{
		GdlGlyphClassDefn * pglfc = *itset;
		if (pglfc->ReplcmtInputClass() && pglfc->GlyphIDCount() > 1)
		{
			pglfc->SetReplcmtInputID(nSubID);
			m_vpglfcReplcmtClasses.push_back(*itset);
			nSubID++;
		}
	}

	Assert(static_cast<size_t>(nSubID) == m_vpglfcReplcmtClasses.size());

	if (nSubID >= kMaxReplcmtClasses)
	{
		g_errorList.AddError(3103, NULL,
			"Number of classes used in glyph substitution (",
			std::to_string(nSubID),
			") exceeds maximum of ",
			std::to_string(kMaxReplcmtClasses - 1));
	}

	return true;
}

/*----------------------------------------------------------------------------------------------
	Mark classes that are used for substitution and FSM matching. Also assign internal IDs
	for use in the FSMs.
----------------------------------------------------------------------------------------------*/
void GdlRenderer::MarkReplacementClasses(GrcManager * pcman,
	ReplacementClassSet & setpglfc)
{
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		m_vprultbl[iprultbl]->MarkReplacementClasses(pcman, setpglfc);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::MarkReplacementClasses(GrcManager * pcman,
	ReplacementClassSet & setpglfc)
{
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		m_vppass[ippass]->MarkReplacementClasses(pcman, setpglfc);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlPass::MarkReplacementClasses(GrcManager * pcman,
	ReplacementClassSet & setpglfc)
{
	for (size_t iprule = 0; iprule < m_vprule.size(); iprule++)
	{
		m_vprule[iprule]->MarkReplacementClasses(pcman, m_nGlobalID, setpglfc);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRule::MarkReplacementClasses(GrcManager * pcman, int nPassID,
	ReplacementClassSet & setpglfcReplace)
{
	//	Make lists of flags indicating whether each slot serves as an input replacement slot
	//	and/or an output replacement slot.
	std::vector<bool> vfInput;
	std::vector<bool> vfOutput;
	vfInput.resize(m_vprit.size(), false);
	vfOutput.resize(m_vprit.size(), false);

	for (auto irit = 0U; irit < m_vprit.size(); ++irit)
	{
		m_vprit[irit]->AssignFsmInternalID(pcman, nPassID);
		m_vprit[irit]->FindSubstitutionSlots(irit, vfInput, vfOutput);
	}

	for (auto irit = 0U; irit < m_vprit.size(); ++irit)
	{
		if (vfInput[irit])
			m_vprit[irit]->MarkClassAsReplacementClass(pcman, setpglfcReplace, true);
		if (vfOutput[irit])
			m_vprit[irit]->MarkClassAsReplacementClass(pcman, setpglfcReplace, false);
	}
}

/*----------------------------------------------------------------------------------------------
	Assign an internal ID to the input class, if any, to be used by the pass's FSM.
----------------------------------------------------------------------------------------------*/
void GdlRuleItem::AssignFsmInternalID(GrcManager * pcman, int nPassID)
{
	if (!m_psymInput)
		//	Already recorded an error--undefined class.
		return;

	if (m_psymInput->Data())
	{
		GdlGlyphClassDefn * pglfcIn = m_psymInput->GlyphClassDefnData();
		Assert(pglfcIn);
		pcman->AddToFsmClasses(pglfcIn, nPassID);
	}
	//	else insertion
}

/*----------------------------------------------------------------------------------------------
	Mark the class as being needed for the FSM for the given pass.
----------------------------------------------------------------------------------------------*/
void GrcManager::AddToFsmClasses(GdlGlyphClassDefn * pglfc, int nPassID)
{
	std::vector<GdlGlyphClassDefn *> * pvpglfcThisPass = m_prgvpglfcFsmClasses + nPassID;
	if (pglfc->IsFsmClass(nPassID))
	{
		//	Already assigned an ID for this pass.
		Assert((*pvpglfcThisPass)[pglfc->FsmID(nPassID)] == pglfc);
		return;
	}
	pglfc->MarkFsmClass(nPassID, int(pvpglfcThisPass->size()));
	pvpglfcThisPass->push_back(pglfc);
}

/*----------------------------------------------------------------------------------------------
	Record the FSM ID for the given pass in the class.
----------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::MarkFsmClass(int nPassID, int nClassID)
{
	if (m_vfFsm.size() >= unsigned(nPassID + 1))
	{
		// Once it's set it shouldn't be changed.
		Assert(!m_vfFsm[nPassID] || m_vnFsmID[nPassID] == nClassID);
	}
	else
	{
		m_vfFsm.resize(nPassID + 1, false);
		m_vnFsmID.resize(nPassID + 1, -1);
	}

	m_vfFsm[nPassID] = true;
	m_vnFsmID[nPassID] = nClassID;
}

/*----------------------------------------------------------------------------------------------
	If this item is performing an substitution, set the flags for input and output slots.
----------------------------------------------------------------------------------------------*/
void GdlRuleItem::FindSubstitutionSlots(int /*irit*/,
	std::vector<bool> & /*vfInput*/, std::vector<bool> & /*vfOutput*/)
{
	//	Do nothing.
}

/*--------------------------------------------------------------------------------------------*/
void GdlSubstitutionItem::FindSubstitutionSlots(int irit,
	std::vector<bool> & vfInput, std::vector<bool> & vfOutput)
{
	if (!m_psymOutput)
	{
		return;	// no output, therefore no substitution (if another item does a substitution
				// based on the input, that item will set the flag)
	}

	if (m_psymOutput->FitsSymbolType(ksymtClass))
	{
		if (m_pexpSelector)
		{
			//	The selector indicates the input slot.
			int nValue = m_pexpSelector->SlotNumber();
			if (nValue >= 1 && nValue <= signed(vfInput.size()))
				vfInput[nValue - 1] = true;	// selectors are 1-based
			else
				//	error condition--already handled
				return;
		}
		else if (m_psymInput->FitsSymbolType(ksymtSpecialUnderscore))
		{	// no input class, but the output class still needs an output ID.
		}
		else
			vfInput[irit] = true;

		vfOutput[irit] = true;
	}
}

/*----------------------------------------------------------------------------------------------
	Mark this item's class as a replacement class, and add it to the list.
----------------------------------------------------------------------------------------------*/
void GdlRuleItem::MarkClassAsReplacementClass(GrcManager * pcman,
	ReplacementClassSet & setpglfcReplace, bool fInput)
{
	GdlDefn * pdefn = NULL;
	if (fInput)
	{
		if (m_psymInput)
			pdefn = m_psymInput->Data();
		else
		{
			g_errorList.AddError(3104, this,
				"Item ",
				PosString(),
				" used as selector has no class specified");
			return;
		}
	}
	else // output
	{
		if (OutputSymbol())
		{
			//	This item must be a substitution item itself.
			Assert(dynamic_cast<GdlSubstitutionItem*>(this));
			Assert(OutputSymbol()->FitsSymbolType(ksymtClass));
			pdefn = OutputSymbol()->Data();
		}
	}

	if (!pdefn)
		return;	// error--undefined class

	GdlGlyphClassDefn * pglfc = dynamic_cast<GdlGlyphClassDefn *>(pdefn);
	Assert(pglfc);
	
	pglfc->FlattenMyGlyphList();

	if (pcman->IgnoreBadGlyphs())
		pglfc->WarnAboutBadGlyphs(true);

	(fInput) ? pglfc->MarkReplcmtInputClass() : pglfc->MarkReplcmtOutputClass();

	if (fInput)
	{
		int cw = pglfc->GlyphIDCount();
		if (cw > kMaxGlyphsPerInputClass)
		{
			g_errorList.AddError(3105, this,
				"Number of glyphs (",
				std::to_string(cw),
				") in class ",
				pglfc->Name(),
				" exceeds maximum of ",
				std::to_string(kMaxGlyphsPerInputClass),
				" allowed for input side of substitution");
		}

		gid16 wGlyphIDDup;
        if (pglfc->HasDuplicateGlyphs(&wGlyphIDDup))
        {
            g_errorList.AddError(3172, this,
                "Duplicates (",
                GdlGlyphDefn::GlyphIDString(wGlyphIDDup),
                ") not permitted in input class: '",
                pglfc->Name(),
                "'");
        }
	}

	setpglfcReplace.insert(pglfc);
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Do general error checking for all rules:
	* inappropriate inclusion of LHS (substitution items)
	* LB symbol in line-break pass
	* non-integer selector or association
	* inserted item used as selector/association
	* in-rule constraint on inserted item
	* LB used as selector/association
	* attribute assignment inappropriate for table type
	* inappropriate presence or absence of unit specifier
	* inappropriate operator type (eg, assignment rather than logical)
	* setting the attribute or association of a deleted item
	* mismatch between associations component.X.ref settings
	* reference to slot beyond length of rule
	* setting something other than a slot attribute
	* too many slots in the rule
	* invalid glyphs in replacement class
	* mismatch between size of class in left- and right-hand-sides
	* invalid user-definable slot attribute
----------------------------------------------------------------------------------------------*/
bool GdlRenderer::CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont)
{
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		m_vprultbl[iprultbl]->CheckRulesForErrors(pgax, pfont, this);
	}

	return true;
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
	GdlRenderer * prndr)
{
	int grfrco = kfrcoNone;

	if (m_psymName->LastFieldIs("linebreak"))
		grfrco = (kfrcoSetBreak | kfrcoSetDir | kfrcoPreBidi);
	else if (m_psymName->LastFieldIs("substitution"))
		grfrco = (kfrcoLb | kfrcoSubst | kfrcoSetCompRef | kfrcoSetDir |
					kfrcoPreBidi);   // kfrcoSetInsert | 
	else if (m_psymName->LastFieldIs("justification"))
		grfrco = (kfrcoNeedJust | kfrcoLb | kfrcoSubst | kfrcoSetCompRef |
					kfrcoSetInsert);
	else if (m_psymName->LastFieldIs("positioning"))
		grfrco = (kfrcoLb | kfrcoSetInsert | kfrcoSetPos);
	else
	{
		Assert(false);
	}

	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		m_vppass[ippass]->CheckRulesForErrors(pgax, pfont, prndr, m_psymName, grfrco);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlPass::CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
	GdlRenderer * prndr, Symbol psymTable, int grfrco)
{
	for (size_t iprule = 0; iprule < m_vprule.size(); iprule++)
	{
		m_vprule[iprule]->CheckRulesForErrors(pgax, pfont, prndr, psymTable, grfrco);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRule::CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
	GdlRenderer * prndr, Symbol psymTable, int grfrco)
{
	if (m_vprit.size() > kMaxSlotsPerRule)
	{
		g_errorList.AddError(3106, this,
			"Number of slots (",
			std::to_string(m_vprit.size()),
			") exceeds maximum of ",
			std::to_string(kMaxSlotsPerRule));
	}

	//	Create lists of flags indicating which items are line-break items, insertions,
	//	or deletions. Also create a vector giving the size of classes in the left-hand-side.
	std::vector<bool> vfLb;
	std::vector<bool> vfInsertion;
	std::vector<bool> vfDeletion;
	std::vector<bool> vfAssocs;
	size_t crit = m_vprit.size();
	vfLb.resize(crit, false);
	vfInsertion.resize(crit, false);
	vfDeletion.resize(crit, false);
	vfAssocs.resize(crit, false);
	std::vector<int> vcwClassSizes;
	vcwClassSizes.resize(crit, false);
	bool fAnyAssocs = false;

	for (auto irit = 0U; irit < crit; ++irit)
	{
		GdlRuleItem * prit = m_vprit[irit];
		GdlLineBreakItem * pritlb = dynamic_cast<GdlLineBreakItem *>(prit);
		if (pritlb)
			vfLb[irit] = true;
		if (!prit->m_psymInput)
		{	//	Invalid class
			vcwClassSizes[irit] = 0;
		}
		else
		{
			if (prit->OutputSymbol()->FitsSymbolType(ksymtSpecialUnderscore))
				vfDeletion[irit] = true;
			if (prit->m_psymInput->FitsSymbolType(ksymtSpecialUnderscore))
				vfInsertion[irit] = true;
			if (prit->m_psymInput->FitsSymbolType(ksymtClass))
			{
				GdlGlyphClassDefn * pglfc = prit->m_psymInput->GlyphClassDefnData();
				if (pglfc)		
					vcwClassSizes[irit] = pglfc->GlyphIDCount();
				else
					vcwClassSizes[irit] = 0;
				if (vcwClassSizes[irit] == 0)
					g_errorList.AddError(3162, this,
						"Empty class ", prit->m_psymInput->FullName(), " for item ", prit->PosString());
			}
			else
				vcwClassSizes[irit] = 0;

			fAnyAssocs = (fAnyAssocs || prit->AnyAssociations()
				// an @ has an implied association
				|| (prit->OutputSymbol() && prit->OutputSymbol()->FitsSymbolType(ksymtSpecialAt)));
			prit->SetAssocsVector(vfAssocs);
			if (prit->OutputSymbol() && prit->OutputSymbol()->FitsSymbolType(ksymtSpecialAt))
			{
				GdlSubstitutionItem * pritSub = dynamic_cast<GdlSubstitutionItem*>(prit);
				//int iritSub = pritSub->m_pritSelInput->m_iritContextPos;
				int iritSub = pritSub->m_pexpSelector->SlotNumber() - 1; // convert to zero-based
				vfAssocs[iritSub] = true;
			}
		}
	}

	bool fOkay = true;

	if (grfrco & kfrcoNeedJust)
	{
		// Check that there is a constraint on the rule that tests for justification status.
		CheckForJustificationConstraint();
	}

	//	Do the checks for each item.
	for (auto irit = 0U; irit < crit; ++irit)
	{
//		if (m_nScanAdvance > -1 && irit >= m_nScanAdvance &&
//			(vfInsertion[irit] || vfDeletion[irit]))
//		{
//			g_errorList.AddError(3107, m_vprit[irit],
//				"Cannot place the scan advance position (^) before insertions or deletions");
//			fOkay = false;
//		}

		if (!m_vprit[irit]->CheckRulesForErrors(pgax, pfont, prndr, this, psymTable, 
				grfrco, irit, fAnyAssocs, vfLb, 
				vfInsertion, vfDeletion, vcwClassSizes, vfAssocs))
		{
			fOkay = false;
		}
	}

	//	If all items were okay, calculate the input and ouput indices and make the
	//	adjustments.
	if (fOkay)
	{
		CalculateIOIndices();
		if (grfrco & kfrcoSetPos)	// positioning pass
			GiveOverlapWarnings(pfont, prndr->ScriptDirections());
	}
	else
	{
		m_fBadRule = true;
	}
}

/*--------------------------------------------------------------------------------------------*/
bool GdlRuleItem::CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
	GdlRenderer * prndr, GdlRule * /*prule*/, Symbol /*psymTable*/,
	int /*grfrco*/, int /*irit*/, bool /*fAnyAssocs*/,
	std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
	std::vector<int> & /*vcwClassSizes*/, std::vector<bool> & /*vfAssocs*/ )
{
	bool fOkay = true;

	//	Check constraint.
	if (m_pexpConstraint)
	{
		ExpressionType expt;
		bool fKeepChecking = m_pexpConstraint->CheckTypeAndUnits(&expt);
		if (expt != kexptBoolean && expt != kexptOne && expt != kexptZero)
		{
			g_errorList.AddWarning(3504, m_pexpConstraint,
				"Boolean value expected as result of constraint");
		}
		if (fKeepChecking)
		{
			if (!m_pexpConstraint->CheckRuleExpression(pfont, prndr, vfLb, vfIns, vfDel,
				false, false))
			{
				fOkay = false;
			}
		}
		if (fOkay)
		{
			bool fCanSub;
			SymbolSet setBogus;
			GdlExpression * pexpNew =
				m_pexpConstraint->SimplifyAndUnscale(pgax, 0, setBogus, pfont, false, &fCanSub);
			if (pexpNew && pexpNew != m_pexpConstraint)
			{
				if (fCanSub)
				{
					delete m_pexpConstraint;
					m_pexpConstraint = pexpNew;
				}
				else
					delete pexpNew;
			}
			m_pexpConstraint->CheckAttachToLookup();
		}
	}

	return fOkay;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlLineBreakItem::CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
	GdlRenderer * prndr, GdlRule * prule, Symbol psymTable,
	int grfrco, int irit, bool fAnyAssocs,
	std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
	std::vector<int> & vcwSizes, std::vector<bool> & vfAssocs)
{
	bool fOkay = true;

	if ((grfrco & kfrcoLb) == 0)
	{
		g_errorList.AddError(3108, this,
			"Line-break inappropriate in ",
			psymTable->FullName(),
			" table");
		fOkay = false;
	}

	//	method on superclass: check constraints.
	if(!GdlRuleItem::CheckRulesForErrors(pgax, pfont, prndr, prule, psymTable,
		grfrco, irit, fAnyAssocs, vfLb, vfIns, vfDel, vcwSizes, vfAssocs))
	{
		fOkay = false;
	}

	return fOkay;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlSetAttrItem::CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
	GdlRenderer * prndr, GdlRule * prule, Symbol psymTable,
	int grfrco, int irit, bool fAnyAssocs,
	std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
	std::vector<int> & vcwSizes, std::vector<bool> & vfAssocs)
{
	bool fOkay = GdlRuleItem::CheckRulesForErrors(pgax, pfont, prndr, prule, psymTable,
			grfrco, irit, fAnyAssocs, vfLb, vfIns, vfDel, vcwSizes, vfAssocs);

	for (size_t ipavs = 0; ipavs < m_vpavs.size(); ipavs++)
	{
		if (!m_vpavs[ipavs]->CheckRulesForErrors(pgax, pfont, prndr, psymTable,
			grfrco, this, irit, vfLb, vfIns, vfDel))
		{
			fOkay = false;			
		}
	}

	return fOkay;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlSubstitutionItem::CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
	GdlRenderer * prndr, GdlRule * prule, Symbol psymTable,
	int grfrco, int irit, bool fAnyAssocs,
	std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel,
	std::vector<int> & vcwClassSizes, std::vector<bool> & vfAssocs)
{
	int crit = signed(vfLb.size());

	bool fOkay = true;

	if ((grfrco & kfrcoSubst) == 0)
	{
		//g_errorList.AddError(3109, this,
		//	"Substitution (left-hand-side) not permitted in ",
		//	psymTable->FullName(),
		//	" table");
		//fOkay = false;

		g_errorList.AddWarning(3541, this,
			"Substitution in ", psymTable->FullName(), " table rule");
	}

	if (OutputSymbol()->FitsSymbolType(ksymtSpecialUnderscore))
	{
		//	Deletion
		if ((grfrco & kfrcoSubst) == 0)
		{
			g_errorList.AddError(3170, this,
				"Deletion not permitted in ",
				psymTable->FullName(),
				" table");
			fOkay = false;
		}
		else
		{
			if (m_vpavs.size())
			{
				if (m_vpavs.size() == 1 && m_vpavs[0]->AttrName()->IsPassKeySlot())
				{
					// Okay to set a deletion slot as a key slot.
				}
				else
				{
					g_errorList.AddWarning(3505, this,
						"Item ", PosString(),
						": setting attributes of a deleted item");
					for (size_t ipavs = 0; ipavs < m_vpavs.size(); ipavs++)
						delete m_vpavs[ipavs];
					m_vpavs.clear();
					fOkay = false;
				}
			}

			if (m_vpexpAssocs.size())
			{
				g_errorList.AddWarning(3506, this,
					"Item ", PosString(),
					": setting associations of a deleted item");
				for (size_t ipexp = 0; ipexp < m_vpexpAssocs.size(); ipexp++)
					delete m_vpexpAssocs[ipexp];
				m_vpexpAssocs.clear();
				fOkay = false;
			}

			if (!vfAssocs[irit])
			{
				int iritAssoc = prule->FindAutoAssocItem(true);
				GdlRuleItem * pritAssoc = (iritAssoc == -1) ? NULL : prule->Rule(iritAssoc);
				if (prule->ItemCountOriginal() == 1)
				{
					g_errorList.AddError(3168, this,
						"Rules with deletions must include at least two slots");
				}
				else if (prule->ItemCountOriginal() == this->CountFlags(vfDel) + 1
					&& iritAssoc > -1
					&& (prule->AutoAssocDone() || !pritAssoc->AnyAssociations()))
				{
					// Automatically associate the all deleted item(s) with the single non-deleted slot
					// in the rule. Also associate that item with itself.
					if (prule->AutoAssocDone())
					{
						// Did this already for a previous item.
						Assert(irit > 0);
					}
					else
					{
						auto const staAssoc = std::to_string(iritAssoc + 1 - prule->PrependedAnys());
						for (auto irit = 0U; irit < prule->NumberOfSlots(); ++irit)
						{
							GdlRuleItem * prit = prule->Item(irit);
							if (prit->OutputSymbol()->FullName() != "ANY")
							{
								pritAssoc->AddAssociation(prule->LineAndFile(), irit + 1);	// 1-based
								if (irit != iritAssoc)
									g_errorList.AddWarning(3532, this,
										"Item ", prit->PosString(),
										": slot ", staAssoc, " automatically associated with deleted item");
							}
						}
						prule->SetAutoAssocDone();
					}
				}
				else
				{
					g_errorList.AddWarning(3529, this,
						"Item ", PosString(),
						": no slot was associated with deleted item");
				}

				for (auto ipavs = 0U; ipavs < m_vpavs.size(); ++ipavs)
				{
					if (m_vpavs[ipavs]->AttrName()->IsPassKeySlot())
					{
						g_errorList.AddWarning(3540, this,
							"Cannot set passKeySlot on an inserted item");
						EraseAttrSetting(ipavs);
					}
				}
			}
		}
	}
	if (m_psymInput->FitsSymbolType(ksymtSpecialUnderscore))
	{
		//	Insertion
		if ((grfrco & kfrcoSubst) == 0)
		{
			g_errorList.AddError(3171, this,
				"Insertion not permitted in ",
				psymTable->FullName(),
				" table");
			fOkay = false;
		}
		else
		{
			if (m_vpexpAssocs.size() == 0 && !m_psymOutput->FitsSymbolType(ksymtSpecialAt))
			{
				int iritAssoc = prule->FindAutoAssocItem(false);
				GdlRuleItem * pritAssoc = (iritAssoc == -1) ? NULL : prule->Rule(iritAssoc);

				bool fExplAssocs = false;
				for (int iritTmp = 0; iritTmp < prule->ItemCountOriginal(); iritTmp++)
				{
					if (prule->Item(iritTmp)->AnyAssociations())
						fExplAssocs = true;
					// DON'T treat @ as an explicit association.
				}

				if (prule->ItemCountOriginal() == 1)
				{
					g_errorList.AddError(3169, this,
						"Rules with insertions must include at least two slots");
				}
				else if (prule->ItemCountOriginal() == this->CountFlags(vfIns) + 1
					&& iritAssoc > -1
					&& (prule->AutoAssocDone() || !fExplAssocs))
				{
					// Automatically associate all the inserted item(s) with the single non-inserted slot
					// in the rule.
					if (prule->AutoAssocDone())
					{
						// Did this already for a previous item.
						Assert(irit > 0);
					}
					else
					{
						auto const iAssoc = int(iritAssoc + 1 - prule->PrependedAnys());
						auto const staAssoc = std::to_string(iAssoc);
						for (auto irit = 0U; irit < prule->NumberOfSlots(); ++irit)
						{
							GdlRuleItem * prit = prule->Item(irit);
							if (prit->OutputSymbol()->FullName() != "ANY"
								&& irit != iritAssoc)
							{
								prit->AddAssociation(prule->LineAndFile(), iAssoc); // 1-based
								g_errorList.AddWarning(3533, this,
									"Item ", prit->PosString(),
									": inserted item automatically associated with slot ", staAssoc);
							}
						}
						prule->SetAutoAssocDone();
					}
				}
				else
				{
					g_errorList.AddWarning(3507, this,
						"Item ", PosString(),
						": inserted item was not given association");
				}
			}
			if (m_pexpConstraint)
			{
				g_errorList.AddError(3110, this,
					"Item ", PosString(),
					": cannot include constraint on inserted item");
			}
		}
	}

	//	If there are any component.X.ref settings, give a warning if they are not equal
	//	to the associations.
	std::set<int> setsrCompRef;
	for (size_t ipavs = 0; ipavs < m_vpavs.size(); ipavs++)
	{
		if (m_vpavs[ipavs]->m_psymName->IsComponentRef())
		{
			GdlSlotRefExpression * pexpsr =
				dynamic_cast<GdlSlotRefExpression *>(m_vpavs[ipavs]->m_pexpValue);
			if (pexpsr)
			{
				int sr = pexpsr->SlotNumber();
				setsrCompRef.insert(sr);
			}
		}
	}
	if (setsrCompRef.size() > 0)
	{
		std::set<int> setsrAssocs;
		for (size_t ipexp = 0; ipexp < m_vpexpAssocs.size() && fOkay; ipexp++)
		{
			int sr = m_vpexpAssocs[ipexp]->SlotNumber();
			setsrAssocs.insert(sr);
		}

		bool fOkay = (setsrCompRef.size() == setsrAssocs.size());
		for (std::set<int>::iterator it = setsrCompRef.begin(); fOkay && it != setsrCompRef.end(); ++it)
		{
			if (setsrAssocs.find(*it) == setsrAssocs.end()) // not a member
				fOkay = false;
		}

		if (!fOkay)
		{
			g_errorList.AddWarning(3508, this,
				"Item ", PosString(),
				": mismatch between associations and component references");
		}			
	}

	//	Mismatched class sizes.
	if (OutputSymbol()->FitsSymbolType(ksymtClass))
	{
		int nSel;
		if (m_pexpSelector)
			nSel = m_pexpSelector->SlotNumber() - 1;
		else
			nSel = irit;
		int cwInput = vcwClassSizes[nSel];
		GdlGlyphClassDefn * pglfc = OutputSymbol()->GlyphClassDefnData();
		if (pglfc) // otherwise, undefined class
		{
			int cwOutput = OutputSymbol()->GlyphClassDefnData()->GlyphIDCount();
			if (cwOutput == 0)
				g_errorList.AddWarning(3509, this,
					"Item ", PosString(),
					": empty class '",
					OutputSymbol()->FullName(),
					"' in right-hand-side");
			else if (cwInput == 0 && cwOutput > 1)
				g_errorList.AddWarning(3510, this,
					"Item ", PosString(),
					": class '",
					OutputSymbol()->FullName(),
					"' in rhs has multiple glyphs but selector class is empty");
			else if (cwInput == 0 && cwOutput == 1)
			{	// okay
			}
			else if (cwInput > cwOutput && cwOutput > 1)
				g_errorList.AddWarning(3511, this,
					"Item ", PosString(),
					": class '",
					OutputSymbol()->FullName(),
					"' in rhs is smaller than selector class");
			else if (cwInput < cwOutput)
				g_errorList.AddWarning(3512, this,
					"Item ", PosString(),
					": mismatched class sizes");
			else
			{
				Assert(cwOutput <= 1 || cwInput == cwOutput);
			}
		}
	}

	if (!GdlSetAttrItem::CheckRulesForErrors(pgax, pfont, prndr, prule, psymTable,
		grfrco, irit, fAnyAssocs, vfLb, vfIns, vfDel, vcwClassSizes, vfAssocs))
	{
		fOkay = false;
	}

	if (m_pexpSelector) // the lhs item to match in the substitution
	{
		int srSel = m_pexpSelector->SlotNumber();
		if (srSel < 1 || srSel > crit)
		{
			//	error condition--already handled
			fOkay = false;
		}
		else if (vfLb[srSel - 1])
		{
			g_errorList.AddError(3111, this,
				"Item ", PosString(),
				": line-break item (#) cannot serve as glyph selector");
			fOkay = false;
		}

		else if (vfIns[srSel - 1])
		{
			g_errorList.AddError(3112, this,
				"Item ", PosString(),
				": glyph selector cannot indicate an inserted item");
			fOkay = false;
		}
	}

	for (size_t ipexp = 0; ipexp < m_vpexpAssocs.size(); ipexp++)
	{
		int srAssoc = m_vpexpAssocs[ipexp]->SlotNumber(); // 1-based
		if (srAssoc < 1 || srAssoc > crit)
		{
			g_errorList.AddError(3113, this,
				"Item ", PosString(),
				": association out of range");
			fOkay = false;
		}

		else if (vfLb[srAssoc - 1])
		{
			g_errorList.AddError(3114, this,
				"Item ", PosString(),
				": association cannot be made with line-break item (#)");
			fOkay = false;
		}

		else if (vfIns[srAssoc - 1])
		{
			g_errorList.AddError(3115, this,
				"Item ", PosString(),
				": association with an inserted item");
			fOkay = false;
		}
	}

	return fOkay;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlAttrValueSpec::CheckRulesForErrors(GrcGlyphAttrMatrix * pgax, GrcFont * pfont,
	GdlRenderer * prndr, Symbol psymTable, int grfrco,
	GdlRuleItem * prit, int irit,
	std::vector<bool> & vfLb, std::vector<bool> & vfIns, std::vector<bool> & vfDel)
{
	if (!m_psymOperator->FitsSymbolType(ksymtOpAssign))
		g_errorList.AddError(3116, this,
			"Attribute assignment must use an assignment operator");

	bool fValueIsInputSlot = false;	// true if the value of an attribute indicates a slot
									// in the input stream (ie, comp.X.ref) as opposed to
									// an output slot (ie, attach.to)

	if (!m_psymName->FitsSymbolType(ksymtSlotAttr) && !m_psymName->FitsSymbolType(ksymtFeature))
	{
		if (m_psymName->FitsSymbolType(ksymtGlyphAttr))
			g_errorList.AddError(3117, this,
				"Cannot set glyph attributes in rules");
		else if (m_psymName->FitsSymbolType(ksymtGlyphMetric))
			g_errorList.AddError(3118, this,
				"Cannot set glyph metrics");
		else
			g_errorList.AddError(3119, this,
				"Cannot set anything but slot attributes and features in rules");
		return false;
	}

	bool fOkay = true;

	if (m_psymName->FitsSymbolType(ksymtFeature))
	{
		if (this->m_psymOperator->FullName() != "=")
			g_errorList.AddError(3165, this,
				"Cannot set a feature using any operator but '='");

		// Convert any feature setting in the value to an integer.
		GdlFeatureDefn * pfeat = m_psymName->FeatureDefnData();
		bool fErr = false;
		GdlExpression * pexpNew = m_pexpValue->ConvertFeatureSettingValue(pfeat, fErr);
		if (pexpNew != m_pexpValue)
		{
			delete m_pexpValue;
			m_pexpValue = pexpNew;
		}
		// Check that the calculated value is an expected setting for this feature.
		if (!fErr)
		{
			int n;
			bool fInt = m_pexpValue->ResolveToInteger(&n, false);
			if (fInt)
			{
				GdlFeatureSetting * pfsetValue = pfeat->FindSettingWithValue(n);
				if (!pfsetValue)
					g_errorList.AddWarning(3536, this,
						"Setting feature ", pfeat->Name(), " to undefined setting");
			}
			else
			{
				g_errorList.AddWarning(3537, this,
					"Cannot validate setting for feature ", pfeat->Name());
			}
		}
	}
	else if (m_psymName->IsPseudoSlotAttr())
	{
		// Ignore
		int x; x = 3;
	}
	else if (m_psymName->IsReadOnlySlotAttr())
	{
		g_errorList.AddError(3120, this,
			"The '",
			m_psymName->FullName(),
			"' attribute is read-only");
		fOkay = false;
	}

	else if (m_psymName->IsMovement())  // shift, kern, advance
	{
		if ((grfrco & kfrcoSetPos) == 0)
		{
			g_errorList.AddError(3121, this,
				"Cannot set the ",
				m_psymName->FullName(),
				" attribute in the ",
				psymTable->FullName(),
				" table");
			fOkay = false;
		}
		GdlGlyphClassDefn * pglfc = prit->OutputSymbol()->GlyphClassDefnData();
		if (pglfc && pglfc->IncludesGlyph(g_cman.LbGlyphId()))
		{
			g_errorList.AddWarning(3513, this,
				"Moving a line-break glyph will have no effect");
		}
		// else undefined class
	}

	else if (m_psymName->IsAttachment())
	{
		if ((grfrco & kfrcoSetPos) == 0)
		{
			g_errorList.AddError(3122, this,
				"Cannot set the ",
				m_psymName->FullName(),
				" attribute in the ",
				psymTable->FullName(),
				" table");
			fOkay = false;
		}
		GdlGlyphClassDefn * pglfcTmp = prit->OutputSymbol()->GlyphClassDefnData();
		if (pglfcTmp && pglfcTmp->IncludesGlyph(g_cman.LbGlyphId()))
		{
			g_errorList.AddWarning(3514, this,
				"Attaching a line-break glyph will have no effect");
		}
	}

	else if (m_psymName->LastFieldIs("breakweight"))
	{
		if ((grfrco & kfrcoSetBreak) == 0)
		{
			g_errorList.AddError(3123, this,
				"Cannot set the breakweight attribute in the ",
				psymTable->FullName(),
				" table");
			fOkay = false;
		}
	}

	else if (m_psymName->IsComponentRef())
	{
		if ((grfrco & kfrcoSetCompRef) == 0)
		{
			g_errorList.AddError(3124, this,
				"Cannot set the ",
				m_psymName->FullName(),
				" attribute in the ",
				psymTable->FullName(),
				" table");
			fOkay = false;
		}
		fValueIsInputSlot = true;
	}

	else if (m_psymName->LastFieldIs("directionality"))
	{
		if ((grfrco & kfrcoSetDir) == 0)
		{
			g_errorList.AddError(3125, this,
				"Cannot set the directionality attribute in the ",
				psymTable->FullName(),
				" table");
			fOkay = false;
		}
	}

	else if (m_psymName->LastFieldIs("insert"))
	{
		if ((grfrco & kfrcoSetInsert) == 0)
		{
			g_errorList.AddError(3126, this,
				"Cannot set the insert attribute in the ",
				psymTable->FullName(),
				" table");
			fOkay = false;
		}
	}

	else if (m_psymName->DoesJustification())
	{
		// Engine does not permit these in the substitution table.
		if (grfrco & kfrcoSubst)
		{
			g_errorList.AddError(3173, this,
				"Cannot set the ",
				m_psymName->FullName(),
				" in the ",
				psymTable->FullName(),
				" table");
		}
		else if (grfrco & kfrcoPreBidi)
		{
			if (m_psymName->LastFieldIs("width"))
			{
				g_errorList.AddWarning(3515, this,
					"Setting ",
					m_psymName->FullName(),
					" too early in the process (should be before the bidi pass)");
			}
		}
		else
		{
			if (m_psymName->LastFieldIs("stretch") || m_psymName->LastFieldIs("shrink") ||
				m_psymName->LastFieldIs("step") || m_psymName->LastFieldIs("weight"))
			{
				g_errorList.AddWarning(3516, this,
					"Setting ",
					m_psymName->FullName(),
					" too late in the process (should be before the bidi pass)");
			}
		}
	}

	else if (m_psymName->IsMeasureAttr())
	{
		// Engine does not permit these in the substitution table.
		if (grfrco & kfrcoSubst)
		{
			g_errorList.AddError(3173, this,
				"Cannot set ",
				m_psymName->FullName(),
				" in the ",
				psymTable->FullName(),
				" table");
		}
	}

	else if (m_psymName->IsCollisionAttr())
	{
		// Engine does not permit these in the substitution table.
		if (grfrco & kfrcoSubst)
		{
			g_errorList.AddError(3173, this,
				"Cannot set the ",
				m_psymName->FullName(),
				" attribute in the ",
				psymTable->FullName(),
				" table");
		}
	}

	else if (m_psymName->IsSequenceAttr())
	{
		// Engine does not permit these in the substitution table.
		if (grfrco & kfrcoSubst)
		{
			g_errorList.AddError(3173, this,
				"Cannot set the ",
				m_psymName->FullName(),
				" attribute in the ",
				psymTable->FullName(),
				" table");
		}
	}

	else if (m_psymName->IsUserDefinableSlotAttr())
	{
		int nIndex = m_psymName->UserDefinableSlotAttrIndex();
		if (nIndex < 0)
		{
			g_errorList.AddError(3127, this,
				"Invalid slot attribute: ", m_psymName->FullName());
			fOkay = false;
		}
		else if (nIndex >= kMaxUserDefinableSlotAttrs)
		{
			g_errorList.AddError(3128, this,
				"Invalid slot attribute: ", m_psymName->FullName(),
				"; maximum is ", std::to_string(kMaxUserDefinableSlotAttrs));
			fOkay = false;
		}
		else
		{
			prndr->SetNumUserDefn(nIndex);
		}
	}

	else
	{
		Assert(false);
		return false;
	}

	ExpressionType exptExpected = m_psymName->ExpType();
	Assert(exptExpected != kexptUnknown);
	ExpressionType exptFound;
	bool fKeepChecking = m_pexpValue->CheckTypeAndUnits(&exptFound);
	if (!EquivalentTypes(exptExpected, exptFound))
	{
		if (exptExpected == kexptSlotRef)
		{
			//	Make it an error, not a warning, because we can't do adequate checking below.
			g_errorList.AddError(3129, this,
				"Value for ",
				m_psymName->FullName(),
				" attribute must be a slot reference");
			fKeepChecking = false;
			fOkay = false;
		}
		else if (exptExpected == kexptMeas && exptFound == kexptNumber)
		{
			g_errorList.AddWarning(3517, this,
				"Slot attribute ",
				m_psymName->FullName(),
				" expects a scaled number");
		}
		else if (exptFound == kexptUnknown)
		{
			// Possibly they are reading a user-defined glyph attribute of unknown type.
			g_errorList.AddWarning(3538, this,
				"Cannot validate value for ",
				m_psymName->FullName());
		}
		else if (m_psymName->IsUserDefinableSlotAttr())
		{
			// any value is appropriate
		}
		else
			g_errorList.AddWarning(3518, this,
				"Assigned value has inappropriate type for '",
				m_psymName->FullName(),
				"' attribute");
	}
	
	if (fKeepChecking)
	{
		int nTmp;
		if (m_psymName->IsAttachTo() && m_pexpValue->ResolveToInteger(&nTmp, true) && nTmp == 0)
		{
			// okay - attach.to = @0 has a special meaning
		}
		else
		{
			if (!m_pexpValue->CheckRuleExpression(pfont, prndr, vfLb, vfIns, vfDel,
				true, fValueIsInputSlot))
			{
				fOkay = false;
			}
			if (m_psymName->IsAttachTo())
			{
				GdlSlotRefExpression * pexpsr = dynamic_cast<GdlSlotRefExpression *>(m_pexpValue);
				if (pexpsr)
				{
					if (pexpsr->SlotNumber() == irit + 1)
						g_errorList.AddWarning(3519, this,
							"Item ", prit->PosString(),
							": slot is being attached to itself--no attachment will result");
				}
			}
		}

		if (m_psymName->IsCollisionAttr() && m_psymName->LastFieldIs("glyph")) // collision.exclude.glyph
		{
			GdlClassMemberExpression * pexpil = dynamic_cast<GdlClassMemberExpression *>(m_pexpValue);
			if (pexpil)
			{
				pexpil->SetGlyphIndex(0); // should only be one glyph possible
				auto cValues = pexpil->ValueCount();
				if (cValues == 0)
					g_errorList.AddError(3163, this,
							"No glyphs in class ", pexpil->Name()->FullName());
				else if (cValues > 1)
					g_errorList.AddError(3164, this,
							"Single glyph definition required for collision.exclude.glyph attribute: ",
							pexpil->Name()->FullName());
			}
		}

		bool fCanSub;
		SymbolSet setBogus;
		GdlExpression * pexpNewValue =
			m_pexpValue->SimplifyAndUnscale(pgax, 0, setBogus, pfont, false, &fCanSub);
		if (pexpNewValue && pexpNewValue != m_pexpValue)
		{
			if (fCanSub)
			{
				delete m_pexpValue;
				m_pexpValue = pexpNewValue;
			}
			else
				delete pexpNewValue;
		}

		//	Use a special zero value for attach.at.gpoint and attach.with.gpoint, to 
		//	distinguish from the unspecified case.
		if ((m_psymName->IsAttachAtField() || m_psymName->IsAttachWithField()) &&
			m_psymName->LastFieldIs("gpoint"))
		{
			int n;
			m_pexpValue->ResolveToInteger(&n, false);
			if (n == 0)
				m_pexpValue->SetSpecialZero();
		}
	}

	return fOkay;
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Return true if this item sets has any associations.
----------------------------------------------------------------------------------------------*/
bool GdlRuleItem::AnyAssociations()
{
	return false;
}

bool GdlSubstitutionItem::AnyAssociations()
{
	return (m_vpexpAssocs.size() > 0);
}


/*----------------------------------------------------------------------------------------------
	Set flags indicating which items in the input are associated.
----------------------------------------------------------------------------------------------*/
void GdlRuleItem::SetAssocsVector(std::vector<bool> & vfAssocs)
{
}

void GdlSubstitutionItem::SetAssocsVector(std::vector<bool> & vfAssocs)
{
	for (int iexp = 0; iexp < signed(this->m_vpexpAssocs.size()); iexp++)
	{
		int srNumber = m_vpexpAssocs[iexp]->SlotNumber(); // 1-based
		vfAssocs[srNumber - 1] = true;
	}
}


/*----------------------------------------------------------------------------------------------
	Count up the insertiondeletion flags in the vector.
----------------------------------------------------------------------------------------------*/
int GdlSubstitutionItem::CountFlags(std::vector<bool> & vfFlags)
{
	int cFlags = 0;
	for (int i = 0; i < signed(vfFlags.size()); i++)
		cFlags = cFlags + ((vfFlags[i]) ? 1 : 0);
	return cFlags;
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Check that there is a constraint on the rule, or on at least one of its items, that
	tests justification status. This is expected in the justification table. If not,
	give a warning.
	TODO: if we implement pass constraints, these tests should be in the pass constraints.
----------------------------------------------------------------------------------------------*/
bool GdlRule::CheckForJustificationConstraint()
{
	for (size_t ipexp = 0; ipexp < m_vpexpConstraints.size(); ipexp++)
	{
		GdlExpression * pexp = m_vpexpConstraints[ipexp];
		if (pexp->TestsJustification())
			return true;
	}

	//	No such constraint on the rule itself. Look at the items.
	for (size_t irit = 0; irit < m_vprit.size(); irit++)
	{
		GdlRuleItem * prit = m_vprit[irit];
		if (prit->CheckForJustificationConstraint())
			return true;
	}

	//	No such constraint on the items. Record a warning.
	g_errorList.AddWarning(3520, this,
		"Rules in justification table should test justification status.");

	return false;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlRuleItem::CheckForJustificationConstraint()
{
	if (m_pexpConstraint)
		return m_pexpConstraint->TestsJustification();
	else
		return false;
}

/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Calculate input and output indices for each rule item, that is, indices relative to the
	input and output streams. Specifically, the input index is the rule index
	ignoring inserted items, and the output index is the rule index ignoring deleted items.
	Make the associations and slot references use the appropriate ones: component.X.ref and
	associations use input indices, and attach.to uses output indices.
	Also calculate the scan-advance value for both input and output.

	Inserted items have a negative input-index, equal to (-1 * the next item's input-index);
	deleted items have a negative output-index, equal to (-1 * the next item's output-index).
----------------------------------------------------------------------------------------------*/

void GdlRule::CalculateIOIndices()
{
	int critInput = 0;
	int critOutput = 0;

	//	Assign the indices for each item. Generate a map of indices to use in later steps.
	size_t irit;
	for (irit = 0; irit < m_vprit.size(); irit++)
		m_vprit[irit]->AssignIOIndices(&critInput, &critOutput, m_viritInput, m_viritOutput);

	//	Adjust the slot references to use the new indices.
	for (irit = 0; irit < m_vprit.size(); irit++)
		m_vprit[irit]->AdjustToIOIndices(m_viritInput, m_viritOutput);


	//	Adjust the scan-advance indicator to take the deleted items into account.
	if (m_nScanAdvance == -1)
	{
		m_nOutputAdvance = -1;
	}
	else
	{
		if (m_nScanAdvance >= signed(m_vprit.size()))
		{
			Assert(static_cast<size_t>(m_nScanAdvance) == m_vprit.size());
			m_nOutputAdvance = m_viritOutput[m_vprit.size() - 1];
			m_nOutputAdvance = (m_nOutputAdvance < 0) ?
				m_nOutputAdvance * -1 :
				m_nOutputAdvance + 1;
		}
		else
		{
			m_nOutputAdvance = m_viritOutput[m_nScanAdvance];
			if (m_nOutputAdvance < 0) // just before a deleted item
				m_nOutputAdvance = (m_nOutputAdvance * -1) - 1;
		}
	}
}


/*----------------------------------------------------------------------------------------------
	Assign each rule item an input and an output index.
----------------------------------------------------------------------------------------------*/
void GdlRuleItem::AssignIOIndices(int * pcritInput, int * pcritOutput,
	std::vector<int> & viritInput, std::vector<int> & viritOutput)
{
	m_nInputIndex = (*pcritInput)++;
	m_nOutputIndex = (*pcritOutput)++;

	viritInput.push_back(m_nInputIndex);
	viritOutput.push_back(m_nOutputIndex);
}

/*--------------------------------------------------------------------------------------------*/
void GdlSubstitutionItem::AssignIOIndices(int * pcritInput, int * pcritOutput,
	std::vector<int> & viritInput, std::vector<int> & viritOutput)
{
	if (m_psymInput && m_psymInput->FitsSymbolType(ksymtSpecialUnderscore))
	{
		//	insertion
		m_nInputIndex = (*pcritInput * -1) - 1;
		viritInput.push_back(m_nInputIndex);
	}
	else
	{
		m_nInputIndex = (*pcritInput)++;
		viritInput.push_back(m_nInputIndex);
	}
	if (m_psymOutput && m_psymOutput->FitsSymbolType(ksymtSpecialUnderscore))
	{
		//	deletion
		m_nOutputIndex = (*pcritOutput * -1) - 1;
		viritOutput.push_back(m_nOutputIndex);
	}
	else
	{
		m_nOutputIndex = (*pcritOutput)++;
		viritOutput.push_back(m_nOutputIndex);
	}
}


/*----------------------------------------------------------------------------------------------
	Modify the rule items to use either input or output indices.
----------------------------------------------------------------------------------------------*/
void GdlRuleItem::AdjustToIOIndices(std::vector<int> & viritInput, std::vector<int> & /*viritOutput*/)
{
	//	Constraints are read from the input stream.
	if (m_pexpConstraint)
		m_pexpConstraint->AdjustToIOIndices(viritInput, NULL);
}

/*--------------------------------------------------------------------------------------------*/
void GdlLineBreakItem::AdjustToIOIndices(std::vector<int> & viritInput, std::vector<int> & viritOutput)
{
	GdlRuleItem::AdjustToIOIndices(viritInput, viritOutput);
}

/*--------------------------------------------------------------------------------------------*/
void GdlSetAttrItem::AdjustToIOIndices(std::vector<int> & viritInput, std::vector<int> & viritOutput)
{
	GdlRuleItem::AdjustToIOIndices(viritInput, viritOutput);

	for (size_t i = 0; i < m_vpavs.size(); i++)
		m_vpavs[i]->AdjustToIOIndices(this, viritInput, viritOutput);
}

/*--------------------------------------------------------------------------------------------*/
void GdlSubstitutionItem::AdjustToIOIndices(std::vector<int> & viritInput, std::vector<int> & viritOutput)
{
	GdlSetAttrItem::AdjustToIOIndices(viritInput, viritOutput);

	//	for selectors: use input indices
	if (m_pexpSelector)
	{
		int sr = m_pexpSelector->SlotNumber();	// 1-based;
		m_nSelector = viritInput[sr - 1];
		Assert(m_nSelector >= 0);	// otherwise, error of using an inserted item as a selector
	}
	else
		m_nSelector = -1; // default, same item

	//	for associations: use input indices
	for (size_t ipexp = 0; ipexp < m_vpexpAssocs.size(); ipexp++)
	{
		int srAssoc = m_vpexpAssocs[ipexp]->SlotNumber(); // 1-based
		Assert(srAssoc >= 1 && static_cast<size_t>(srAssoc) <= viritInput.size());
		m_vnAssocs.push_back(viritInput[srAssoc - 1]);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlAttrValueSpec::AdjustToIOIndices(GdlRuleItem * prit,
	std::vector<int> & viritInput, std::vector<int> & viritOutput)
{
	Assert(m_psymName->FitsSymbolType(ksymtSlotAttr) || m_psymName->FitsSymbolType(ksymtFeature));

	if (m_psymName->ExpType() == kexptSlotRef)
	{
		if (m_psymName->IsComponentRef())
			m_pexpValue->AdjustToIOIndices(viritInput, NULL);
		else if (m_psymName->IsAttachment())
		{
			Assert(m_psymName->LastFieldIs("to"));	// must be attach.to
			m_pexpValue->AdjustToIOIndices(viritOutput, prit);
		}
		else
		{
			Assert(false);
		}
	}
	else
		m_pexpValue->AdjustToIOIndices(viritInput, NULL);
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Generate warning messages if there are any contiguous glyphs whose bounding boxes
	overlap (in the vertical dimension) but are not attached.
	Note that these checks are only run for positioning rules.
----------------------------------------------------------------------------------------------*/

void GdlRule::GiveOverlapWarnings(GrcFont * pfont, int grfsdc)
{
	for (int irit = 0; irit < signed(m_vprit.size()) - 1; irit++)
	{
		GdlRuleItem * prit1 = m_vprit[irit];
		GdlRuleItem * prit2 = m_vprit[irit + 1];

		if (prit1->m_iritContextPosOrig < 0 || prit1->m_iritContextPosOrig >= signed(m_vprit.size())) // eg, ANY
			continue;
		if (prit2->m_iritContextPosOrig < 0 || prit2->m_iritContextPosOrig >= signed(m_vprit.size())) // eg, ANY
			continue;

		// Figure out what these items are attached to. A value of zero means it is
		// deliberately attached to nothing.
		int nAtt1 = prit1->AttachedToSlot();
		int nAtt2 = prit2->AttachedToSlot();
		if (nAtt1 == 0 || nAtt2 == 0)
			continue;
		if (nAtt1 == prit2->m_iritContextPos + 1) // m_iritContextPos is 0-based
			continue;
		if (nAtt2 == prit1->m_iritContextPos + 1)
			continue;

		if (prit1->OverlapsWith(prit2, pfont, grfsdc))
		{
			// Give warning.
			g_errorList.AddWarning(3521, this,
				"Vertical overlap between glyphs in items ", prit1->PosString(), " and ",
				prit2->PosString(), "; attachment may be needed");
		}
	}
}

/*----------------------------------------------------------------------------------------------
	Return the slot the item is attached to, or -1 if none.
----------------------------------------------------------------------------------------------*/
int GdlRuleItem::AttachedToSlot()
{
	return -1;
}

int GdlSetAttrItem::AttachedToSlot()
{
	Assert(m_nInputIndex == m_nOutputIndex); // because this is a positioning rule
	for (size_t ipavs = 0; ipavs < m_vpavs.size(); ipavs++)
	{
		if (m_vpavs[ipavs]->m_psymName->IsAttachTo())
		{
			int nSlot;
			m_vpavs[ipavs]->m_pexpValue->ResolveToInteger(&nSlot, true);
			return nSlot;
		}
	}
	return -1;
}

/*----------------------------------------------------------------------------------------------
	Return true if any glyph in the recipient item's glyph class overlaps along the 
	vertical axis with any glyph in the argument item's glyph class.
----------------------------------------------------------------------------------------------*/
bool GdlRuleItem::OverlapsWith(GdlRuleItem * prit, GrcFont * pfont, int grfsdc)
{
	Symbol psymGlyphs1 = this->m_psymInput;
	Symbol psymGlyphs2 = prit->m_psymInput;

	GdlGlyphClassDefn * glfd1 = psymGlyphs1->GlyphClassDefnData();
	GdlGlyphClassDefn * glfd2 = psymGlyphs2->GlyphClassDefnData();

	if (!glfd1 || !glfd2)
		return false;

	if ((grfsdc & kfsdcHorizLtr || grfsdc == 0) && glfd1->HasOverlapWith(glfd2, pfont))
		return true;
	if (grfsdc & kfsdcHorizRtl && glfd2->HasOverlapWith(glfd1, pfont))
		return true;
	return false;
}

/*----------------------------------------------------------------------------------------------
	Return true if there are any overlaps along the x-axis between any glyphs in the two
	classes.
----------------------------------------------------------------------------------------------*/
bool GdlGlyphClassDefn::HasOverlapWith(GdlGlyphClassMember * pglfdLeft, GrcFont * pfont)
{
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		if (m_vpglfdMembers[iglfd]->HasOverlapWith(pglfdLeft, pfont))
			return true;
	}
	return false;
}

bool GdlGlyphDefn::HasOverlapWith(GdlGlyphClassMember * pglfdLeft, GrcFont * pfont)
{
	GdlGlyphDefn * pglfLeft = dynamic_cast<GdlGlyphDefn*>(pglfdLeft);
	if (m_glft == kglftPseudo)
	{
		return m_pglfOutput->HasOverlapWith(pglfdLeft, pfont);
	}
	else if (pglfLeft)
	{
		if (pglfLeft->m_glft == kglftPseudo)
		{
			return HasOverlapWith(pglfLeft->m_pglfOutput, pfont);
		}
		else
		{
			for (size_t iw1 = 0; iw1 < this->m_vwGlyphIDs.size(); iw1++)
			{
				utf16 w1 = this->m_vwGlyphIDs[iw1];
				if (w1 == kBadGlyph)
					continue;
				utf16 nActual = g_cman.ActualForPseudo(w1);
				if (nActual != 0)
					w1 = nActual;
				int nLsb = pfont->GetGlyphMetric(w1, kgmetLsb, this);
				for (size_t iw2 = 0; iw2 < pglfLeft->m_vwGlyphIDs.size(); iw2++)
				{
					utf16 w2 = pglfLeft->m_vwGlyphIDs[iw2];
					if (w2 == kBadGlyph)
						continue;
					utf16 nActual = g_cman.ActualForPseudo(w2);
					if (nActual != 0)
						w2 = nActual;
					int nRsb = pfont->GetGlyphMetric(w2, kgmetRsb, pglfLeft);
					if (nLsb + nRsb < 0)
						return true;
				}
			}
		}
	}
	else
	{
		GdlGlyphClassDefn * pglfcLeft = dynamic_cast<GdlGlyphClassDefn*>(pglfdLeft);
		Assert(pglfcLeft);
		for (size_t iglfd = 0; iglfd < pglfcLeft->m_vpglfdMembers.size(); iglfd++)
		{
			if (HasOverlapWith(pglfcLeft->m_vpglfdMembers[iglfd], pfont))
				return true;
		}
	}
	return false;
}


/**********************************************************************************************/
/*----------------------------------------------------------------------------------------------
	Delete all invalid glyphs.
----------------------------------------------------------------------------------------------*/
void GdlRenderer::DeleteAllBadGlyphs()
{
	for (size_t iglfc = 0; iglfc < m_vpglfc.size(); iglfc++)
		m_vpglfc[iglfc]->DeleteBadGlyphs();
}

/*----------------------------------------------------------------------------------------------
	Delete invalid glyphs from the class
----------------------------------------------------------------------------------------------*/
bool GdlGlyphClassDefn::DeleteBadGlyphs()
{
	bool fRet = false;
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		fRet = (fRet || m_vpglfdMembers[iglfd]->DeleteBadGlyphs());
	}
	return fRet;
}

bool GdlGlyphDefn::DeleteBadGlyphs()
{
	bool fRet = false;
	for (int i = signed(m_vwGlyphIDs.size()) - 1; i >=0; i--)
	{
		if (m_vwGlyphIDs[i] == kBadGlyph)
		{
			m_vwGlyphIDs.erase(m_vwGlyphIDs.begin() + i);
			fRet = true;
		}
	}
	return fRet;
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Give a warning about any invalid glyphs in the class.
----------------------------------------------------------------------------------------------*/
bool GdlGlyphClassDefn::WarnAboutBadGlyphs(bool fTop)
{
	bool fRet = false;
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		fRet = (fRet || m_vpglfdMembers[iglfd]->WarnAboutBadGlyphs(false));
	}
	if (fRet && fTop)
		g_errorList.AddWarning(3522, this,
			"Class '", m_staName, "' is used in substitution but has invalid glyphs");
	return fRet;
}

#ifdef NDEBUG
bool GdlGlyphDefn::WarnAboutBadGlyphs(bool)
#else
bool GdlGlyphDefn::WarnAboutBadGlyphs(bool fTop)
#endif
{
	Assert(!fTop);
	bool fRet = false;
	for (int i = signed(m_vwGlyphIDs.size()) - 1; i >=0; i--)
	{
		fRet = (fRet || m_vwGlyphIDs[i] == kBadGlyph);
	}
	return fRet;
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Check for the error of cross-line contextualization across more than two lines.
	Record the maximum number of slots occurring in a cross-line contextualization rule.
----------------------------------------------------------------------------------------------*/
bool GdlRenderer::CheckLBsInRules()
{
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		m_vprultbl[iprultbl]->CheckLBsInRules();
	}

	return true;
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::CheckLBsInRules()
{
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		m_vppass[ippass]->CheckLBsInRules(m_psymName);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlPass::CheckLBsInRules(Symbol psymTable)
{
	m_fLB = false;
	m_fCrossLB = false;
	m_critPreLB = 0;
	m_critPostLB = 0;
	m_fReproc = false;

	int critPreLB = 0;
	int critPostLB = 0;

	for (size_t iprule = 0; iprule < m_vprule.size(); iprule++)
	{
		if (m_vprule[iprule]->IsBadRule())
			continue;	// don't process if we've discovered errors

		bool fAnyLB = m_vprule[iprule]->CheckLBsInRules(psymTable, &critPreLB, &critPostLB);
		m_fLB = (m_fLB || fAnyLB);
		if (critPreLB > 0 && critPostLB > 0)
		{
			m_fCrossLB = true;
			m_critPreLB = max(m_critPreLB, critPreLB);
			m_critPostLB = max(m_critPostLB, critPostLB);
		}

		if (m_vprule[iprule]->HasReprocessing())
			m_fReproc = true;
	}
	Assert(!m_fCrossLB || m_fLB);
	Assert(!m_fCrossLB || m_critPreLB > 0 || m_critPostLB > 0);
}

/*--------------------------------------------------------------------------------------------*/
bool GdlRule::CheckLBsInRules(Symbol /*psymTable*/, int * pcritPreLB, int * pcritPostLB)
{
	//	Check to make sure that there are at most two line-break slots in the rule,
	//	and if there are two, they are the first and last. While we're at it, count the
	//	items before and after the line-break.

	int critLB = 0;
	int critPreTmp = 0;
	int critPostTmp = 0;
	int critPost2Tmp = 0;
	for (size_t iprit = 0; iprit < m_vprit.size(); iprit++)
	{
		if (dynamic_cast<GdlSetAttrItem *>(m_vprit[iprit]) == NULL &&
			m_vprit[iprit]->OutputSymbol()->LastFieldIs("#"))
		{
			critLB++;
		}
		else
		{
			if (signed(iprit) < m_critPrependedAnys)
			{
				// prepended ANY doesn't count
			}
			else if (critLB == 0)
				critPreTmp++;
			else if (critLB == 1)
				critPostTmp++;
			else
				critPost2Tmp++;
		}
	}

	if (critLB == 0)
	{
		//	No line-breaks in this rule.
		*pcritPreLB = 0;
		*pcritPostLB = 0;
		return false;
	}

	if (critLB > 2 || (critLB == 2 && (critPreTmp > 0 || critPost2Tmp > 0)))
	{
		g_errorList.AddError(3131, this,
			"Cross-line contextualization involving more than two lines.");
		return true;
	}

	Assert(critPost2Tmp == 0);

	*pcritPreLB = critPreTmp;
	*pcritPostLB = critPostTmp;
	return true;
}


/*----------------------------------------------------------------------------------------------
	Return true if the rule has its scan position set so that reprocessing will occur.
	As a side effect, if there was a caret in the rule, record the default scan position,
	which will be used later in outputting the rule commands.
----------------------------------------------------------------------------------------------*/
bool GdlRule::HasReprocessing()
{
	if (m_nOutputAdvance == -1)
		return false;

	//	Count the number of unmodified items at the end of the rule; these do not need to
	//	be processed, and the default scan advance position is just before these.
	size_t iritLim = m_vprit.size();
	while (iritLim > 0 && !dynamic_cast<GdlSetAttrItem *>(m_vprit[iritLim - 1]))
		iritLim--;

	if (iritLim < m_vprit.size())
	{
		GdlRuleItem * pritLim = m_vprit[iritLim];
		Assert(pritLim->m_nOutputIndex >= 0);
		m_nDefaultAdvance = pritLim->m_nOutputIndex;
	}
	else
	{
		Assert(iritLim == m_vprit.size());
		m_nDefaultAdvance = m_vprit.back()->m_nOutputIndex;
		m_nDefaultAdvance++;
		if (m_nDefaultAdvance < 0)
			// Note that when the last slot is a deletion, the ++ above in effect subtracted
			// 1, reflecting the fact that their is no output for the deletion.
			m_nDefaultAdvance = m_nDefaultAdvance * -1;
	}

	return (m_nOutputAdvance < m_nDefaultAdvance);
}


/**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Rewrite some slot attribute assignments:
	* Replace any kern assigments with the equivalent shift and advance.
----------------------------------------------------------------------------------------------*/
void GdlRenderer::RewriteSlotAttrAssignments(GrcManager * pcman, GrcFont * pfont)
{
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		m_vprultbl[iprultbl]->RewriteSlotAttrAssignments(pcman, pfont);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::RewriteSlotAttrAssignments(GrcManager * pcman, GrcFont * pfont)
{
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		m_vppass[ippass]->RewriteSlotAttrAssignments(pcman, pfont);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlPass::RewriteSlotAttrAssignments(GrcManager * pcman, GrcFont * pfont)
{
	for (size_t iprule = 0; iprule < m_vprule.size(); iprule++)
	{
		m_vprule[iprule]->RewriteSlotAttrAssignments(pcman, pfont);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRule::RewriteSlotAttrAssignments(GrcManager * pcman, GrcFont * pfont)
{
	for (size_t iprit = 0; iprit < m_vprit.size(); iprit++)
	{
		m_vprit[iprit]->RewriteSlotAttrAssignments(pcman, pfont);
	}
}


/*--------------------------------------------------------------------------------------------*/
void GdlRuleItem::RewriteSlotAttrAssignments(GrcManager * /*pcman*/, GrcFont * /*pfont*/)
{
	//	Do nothing.
}

/*--------------------------------------------------------------------------------------------*/
void GdlSetAttrItem::RewriteSlotAttrAssignments(GrcManager * pcman, GrcFont * pfont)
{
	int ipavsColFlags = -1;
	int ipavsColRange = -1;
	int ipavsColPriority = -1;

	for (auto ipavs = 0U; ipavs < m_vpavs.size(); ++ipavs)
	{
		GdlAttrValueSpec * pavsShift;
		GdlAttrValueSpec * pavsAdvance;
		bool fKern = m_vpavs[ipavs]->ReplaceKern(pcman, &pavsShift, &pavsAdvance);
		if (fKern)
		{
			delete m_vpavs[ipavs];
			m_vpavs[ipavs] = pavsShift;
			m_vpavs.insert(m_vpavs.begin() + ipavs + 1, pavsAdvance);
			ipavs++;
		}

		if (m_vpavs[ipavs]->m_psymName->FieldIs(0, "collision"))
		{
			if (m_vpavs[ipavs]->m_psymName->FieldIs(1, "flags"))
				ipavsColFlags = ipavs;
			else if (m_vpavs[ipavs]->m_psymName->FieldIs(1, "range"))
				ipavsColRange = ipavs;
			else if (m_vpavs[ipavs]->m_psymName->FieldIs(1, "priority"))
				ipavsColPriority = ipavs;
		}
	}

	MergeColRangeAndPriority(pcman, pfont, ipavsColFlags, ipavsColRange, ipavsColPriority);
}

/*----------------------------------------------------------------------------------------------
	If this a statement setting the kern attribute, generate the corresponding shift and
	advance statements, and return true. Return false otherwise.
	Specifically,
		kern.x = -10m		becomes		shift.x = -10m; adv.x = advancewidth - 10m

		kern.x += 10m		becomes		shift.x += 10m; adv.x += 10m
----------------------------------------------------------------------------------------------*/
bool GdlAttrValueSpec::ReplaceKern(GrcManager * pcman,
	GdlAttrValueSpec ** ppavsShift, GdlAttrValueSpec ** ppavsAdvance)
{
	if (!m_psymName->FieldIs(0, "kern"))
		return false;

	GrcStructName xns;
	m_psymName->GetStructuredName(&xns);
	xns.DeleteField(0);
	xns.InsertField(0, "advance");
	Symbol psymNameAdvance = pcman->SymbolTable()->FindSymbol(xns);
	if (!psymNameAdvance)
	{
		g_errorList.AddError(3132, this,
			"Invalid kern assignment");
		return false;
	}
	xns.DeleteField(0);
	xns.InsertField(0, "shift");
	Symbol psymNameShift = pcman->SymbolTable()->FindSymbol(xns);
	if (!psymNameShift)
	{
		g_errorList.AddError(3133, this,
			"Invalid kern assignment");
		return false;
	}

	GdlExpression * pexpValueShift = m_pexpValue->Clone();
	*ppavsShift = new GdlAttrValueSpec(psymNameShift, m_psymOperator, pexpValueShift);

	GdlExpression * pexpValueAdvance = m_pexpValue->Clone();

	if (m_psymOperator->FieldIs(0, "="))
	{
		//	Base 'advance' off of advancewidth (or advanceheight).
		Symbol psymAdvMetric =
			pcman->SymbolTable()->FindSymbol("advancewidth");
		if (xns.FieldEquals(1, "y"))
			psymAdvMetric = pcman->SymbolTable()->FindSymbol("advanceheight");
		Assert(psymAdvMetric);
		GdlLookupExpression * pexplook = new GdlLookupExpression(psymAdvMetric);
		Symbol psymPlus = pcman->SymbolTable()->FindSymbol("+");
		Assert(psymPlus);
		pexpValueAdvance = new GdlBinaryExpression(psymPlus, pexplook, pexpValueAdvance);
	}

	*ppavsAdvance = new GdlAttrValueSpec(psymNameAdvance, m_psymOperator, pexpValueAdvance);
	return true;
}

/*----------------------------------------------------------------------------------------------
	Merge the collision.range and collision.priority attributes into collision.flags.
	TODO: DELETE - priority is no longer defined
----------------------------------------------------------------------------------------------*/
void GdlSetAttrItem::MergeColRangeAndPriority(GrcManager * pcman, GrcFont * pfont,
	int ipavsFlags, int ipavsRange, int ipavsPriority)
{
	if (ipavsRange == -1 && ipavsPriority == -1)
		return;

	Symbol psymCollisionFlags = pcman->SymbolTable()->FindSymbol(GrcStructName("collision", "flags"));
	Symbol psymEquals = pcman->SymbolTable()->FindSymbol("=");

	GdlExpression * pexpFlagsOld = nullptr;
	GdlExpression * pexpRange = nullptr;
	GdlExpression * pexpPriority = nullptr;
	if (ipavsFlags >= 0)
		pexpFlagsOld = m_vpavs[ipavsFlags]->m_pexpValue->Clone();
	if (ipavsRange >= 0)
		pexpRange = m_vpavs[ipavsRange]->m_pexpValue->Clone();
	if (ipavsPriority >= 0)
		pexpPriority = m_vpavs[ipavsPriority]->m_pexpValue->Clone();

	Symbol psymPlus = pcman->SymbolTable()->FindSymbol("+");
	GdlExpression * pexpFlagsTemp = nullptr;
	GdlExpression * pexpFlags;
	SymbolSet setpsymBogus;
	bool fCanSub;
	if (pexpRange && pexpPriority)
	{
		pexpFlagsTemp = new GdlBinaryExpression(psymPlus, pexpRange, pexpPriority);
		if (pexpFlagsOld)
			pexpFlagsTemp = new GdlBinaryExpression(psymPlus, pexpFlagsOld, pexpFlagsTemp);
	}
	else if (pexpRange && pexpFlagsOld)
		pexpFlagsTemp = new GdlBinaryExpression(psymPlus, pexpRange, pexpFlagsOld);
	else if (pexpPriority && pexpFlagsOld)
		pexpFlagsTemp = new GdlBinaryExpression(psymPlus, pexpPriority, pexpFlagsOld);
	else if (pexpRange)
		pexpFlagsTemp = pexpRange;
	else if (pexpPriority)
		pexpFlagsTemp = pexpPriority;

	assert(pexpFlagsTemp);
	pexpFlags = pexpFlagsTemp->SimplifyAndUnscale(pcman->GlyphAttrMatrix(), -1, setpsymBogus, pfont, false, &fCanSub);
	if (pexpFlagsTemp != pexpFlags)
		delete pexpFlagsTemp;

	GdlAttrValueSpec * pavsFlagsNew = new GdlAttrValueSpec(psymCollisionFlags, psymEquals, pexpFlags);

	if (ipavsFlags >= 0)
	{
		ReplaceAttrSetting(ipavsFlags, pavsFlagsNew);
		if (ipavsRange >= 0)
			EraseAttrSetting(ipavsRange);
		if (ipavsPriority >= 0)
			EraseAttrSetting(ipavsPriority);
	}
	else if (ipavsRange >= 0)
	{
		ReplaceAttrSetting(ipavsRange, pavsFlagsNew);
		if (ipavsPriority >= 0)
			EraseAttrSetting(ipavsPriority);
	}
	else
	{
		ReplaceAttrSetting(ipavsPriority, pavsFlagsNew);
	}
}

/**********************************************************************************************/
/*----------------------------------------------------------------------------------------------
	Check that the rules and glyph attributes are compatible with the requested version of the
	Silf table. If not, return the version required.
	
	Also return the compiler version needed to support the GDL.

	This routine assumes that we can always sucessfully use a later version.
----------------------------------------------------------------------------------------------*/
bool GrcManager::CompatibleWithVersion(int fxdVersion, int * pfxdSilfNeeded,
	int * pfxdCpilrNeeded, bool * pfFixPassConstraints)
{
	*pfxdSilfNeeded = fxdVersion;

	if (fxdVersion >= kfxdMaxSilfVersion)
		return true;

	if (!m_fBasicJust)
	{
		// We need version 2.0 for basic justification.
		*pfxdSilfNeeded = max(0x00020000, *pfxdSilfNeeded);
		*pfxdCpilrNeeded = max(0x00020000, *pfxdCpilrNeeded);
	}

	if (m_vpglfcReplcmtClasses.size() >= kMaxReplcmtClassesV1_2)
	{
		// For a large set of replacement classes, we need 3.0.
		*pfxdSilfNeeded = max(0x00030000, *pfxdSilfNeeded);
		*pfxdCpilrNeeded = max(0x00030000, *pfxdCpilrNeeded);
	}

	// For now, the Glat table version does not affect the other tables.
	//if (m_vpsymGlyphAttrs.size() >= kMaxGlyphAttrsGlat1)
	//{
	//	// For a large number of glyph attributes, we need 3.0.
	//	*pfxdNeeded = max(0x00030000, *pfxdNeeded);
	//}

	if (*pfxdSilfNeeded > fxdVersion)
		*pfFixPassConstraints = false;

	bool fRet = (*pfxdSilfNeeded <= fxdVersion);

	fRet = (m_prndr->CompatibleWithVersion(fxdVersion, pfxdSilfNeeded, pfxdCpilrNeeded,
				pfFixPassConstraints) 
		&& fRet);

	return fRet;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlRenderer::CompatibleWithVersion(int fxdVersion, int * pfxdSilfNeeded,
	int * pfxdCpilrNeeded, bool * pfFixPassConstraints)
{
	bool fRet = true;

	//	Glyph atrributes:
	for (size_t ipglfc = 0; ipglfc < m_vpglfc.size(); ipglfc++)
	{
		fRet = m_vpglfc[ipglfc]->CompatibleWithVersion(fxdVersion, pfxdSilfNeeded, pfxdCpilrNeeded) 
			&& fRet;
		if (!fRet)
			*pfFixPassConstraints = false;	// something else is a problem
	}
	//	Rules:
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		fRet = m_vprultbl[iprultbl]->CompatibleWithVersion(fxdVersion, pfxdSilfNeeded, pfxdCpilrNeeded,
			pfFixPassConstraints)
			&& fRet;
	}
	return fRet;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlGlyphClassDefn::CompatibleWithVersion(int fxdVersion, int * pfxdSilfNeeded,
	int * pfxdCpilrNeeded)
{
	bool fRet = true;

	//	For each attribute assignment in the value list:
	for (size_t ipglfa = 0; ipglfa < m_vpglfaAttrs.size(); ipglfa++)
	{
		Symbol psym = m_vpglfaAttrs[ipglfa]->GlyphSymbol();
		if (psym->IsMeasureAttr() || psym->DoesJustification())
		{
			// For justification, we need Silf version 2.0.
			fRet = (fxdVersion >= 0x00020000);
			*pfxdSilfNeeded = max(*pfxdSilfNeeded, 0x00020000);
			*pfxdCpilrNeeded = max(*pfxdCpilrNeeded, 0x00020000);
		}
		else if (psym->IsMirrorAttr())
		{
			// For mirroring, we need version Silf 2.1 or 3.2.
			fRet = (fxdVersion >= 0x00020000);
			if (*pfxdSilfNeeded < 0x00030000)
				*pfxdSilfNeeded = max(*pfxdSilfNeeded, 0x00020001);
			else
				*pfxdSilfNeeded = max(*pfxdSilfNeeded, 0x00030002);
			*pfxdCpilrNeeded = max(*pfxdCpilrNeeded, 0x00040001);
		}
		else if (psym->IsCollisionAttr())
		{
			*pfxdSilfNeeded = max(*pfxdSilfNeeded, 0x00040001);
			*pfxdCpilrNeeded = max(*pfxdCpilrNeeded, 0x00050000);
		}
	}
	return fRet;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlRuleTable::CompatibleWithVersion(int fxdVersion, int * pfxdSilfNeeded,
	int * pfxdCpilrNeeded, bool * pfFixPassConstraints)
{
	bool fRet = true;
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		fRet = m_vppass[ippass]->CompatibleWithVersion(fxdVersion, pfxdSilfNeeded, pfxdCpilrNeeded,
			pfFixPassConstraints)
			&& fRet;
	}
	return fRet;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlPass::CompatibleWithVersion(int fxdVersion, int * pfxdSilfNeeded, int * pfxdCpilrNeeded,
	bool * pfFixPassConstraints)
{
	bool fRet = true;
	if (m_vpexpConstraints.size() > 0)
	{
		// Pass constraints need 3.1. (Version 3.0 outputs empty pass constraints.)
		fRet = (fxdVersion >= 0x00030001);
		*pfxdSilfNeeded = max(*pfxdSilfNeeded, 0x00030001);
		*pfxdCpilrNeeded = max(*pfxdCpilrNeeded, 0x00040000);
	}
	if (m_nCollisionFix > 0)
	{
		fRet = (fxdVersion >= 0x00040001);
		*pfxdSilfNeeded = max(*pfxdSilfNeeded, 0x00040001);
		*pfxdCpilrNeeded = max(*pfxdCpilrNeeded, 0x00050000);
	}

	for (size_t iprule = 0; iprule < m_vprule.size(); iprule++)
	{
		bool fRetTmp = m_vprule[iprule]->CompatibleWithVersion(fxdVersion, pfxdSilfNeeded, pfxdCpilrNeeded);
		if (!fRetTmp)
			*pfFixPassConstraints = false;	// something else is a problem
		fRet = fRet && fRetTmp;
	}
	return fRet;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlRule::CompatibleWithVersion(int fxdVersion, int * pfxdSilfNeeded, int * pfxdCpilrNeeded)
{
	bool fRet = true;
	for (size_t iprit = 0; iprit < m_vprit.size(); iprit++)
	{
		fRet = m_vprit[iprit]->CompatibleWithVersion(fxdVersion, pfxdSilfNeeded, pfxdCpilrNeeded)
			&& fRet;
	}
	return fRet;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlRuleItem::CompatibleWithVersion(int fxdVersion, int * pfxdSilfNeeded, int * pfxdCpilrNeeded)
{
	if (m_pexpConstraint)
	{
		return m_pexpConstraint->CompatibleWithVersion(fxdVersion, pfxdSilfNeeded, pfxdCpilrNeeded);
	}
	else
		return true;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlSetAttrItem::CompatibleWithVersion(int fxdVersion, int * pfxdSilfNeeded, int * pfxdCpilrNeeded)
{
	bool fRet = GdlRuleItem::CompatibleWithVersion(fxdVersion, pfxdSilfNeeded, pfxdCpilrNeeded);

	for (size_t ipavs = 0; ipavs < m_vpavs.size(); ipavs++)
	{
		fRet = m_vpavs[ipavs]->CompatibleWithVersion(fxdVersion, pfxdSilfNeeded, pfxdCpilrNeeded)
			&& fRet;
	}
	return fRet;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlAttrValueSpec::CompatibleWithVersion(int fxdVersion, int * pfxdSilfNeeded, int * pfxdCpilrNeeded)
{
	bool fRet = true;
	if (m_psymName->IsMeasureAttr() || m_psymName->DoesJustification())
	{
		// Measuring and justification need 2.0.
		*pfxdSilfNeeded = max(*pfxdSilfNeeded, 0x00020000);
		*pfxdCpilrNeeded = max(*pfxdCpilrNeeded, 0x00020000);
		fRet = (fxdVersion >= 0x00020000);
	}
	return fRet;
}

/**********************************************************************************************/
/*----------------------------------------------------------------------------------------------
	Convert pass constraints to rule constraints.

	This is needed when the version to be output is earlier than what is compatible with
	pass constraints.
----------------------------------------------------------------------------------------------*/
void GdlRenderer::MovePassConstraintsToRules(int fxdSilfVersion)
{
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		m_vprultbl[iprultbl]->MovePassConstraintsToRules(fxdSilfVersion);
	}	
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::MovePassConstraintsToRules(int fxdSilfVersion)
{
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		m_vppass[ippass]->MovePassConstraintsToRules(fxdSilfVersion);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlPass::MovePassConstraintsToRules(int fxdSilfVersion)
{
	if (m_vpexpConstraints.size() > 0)
	{
		g_errorList.AddWarning(3530, this,
			"Pass constraints are not compatible with version ",
			VersionString(fxdSilfVersion),
			"; constraint will be applied directly to rules.");

		for (size_t iprule = 0; iprule < m_vprule.size(); iprule++)
		{
			m_vprule[iprule]->MovePassConstraintsToRule(m_vpexpConstraints);
		}
	}
	// Delete pass constraint(s).
	for (size_t i = 0; i < m_vpexpConstraints.size(); ++i)
		delete m_vpexpConstraints[i];
	m_vpexpConstraints.clear();

}

/*--------------------------------------------------------------------------------------------*/
void GdlRule::MovePassConstraintsToRule(std::vector<GdlExpression *> & m_vpexpPassConstr)
{
	for (size_t ipexp = 0; ipexp < m_vpexpPassConstr.size(); ipexp++)
	{
		m_vpexpConstraints.push_back(m_vpexpPassConstr[ipexp]->Clone());
	}
}

/*----------------------------------------------------------------------------------------------
	Return the original number of items in the rule. Don't include ANY items.
----------------------------------------------------------------------------------------------*/
int GdlRule::ItemCountOriginal()
{
	int crit = 0;
	for (size_t irit = 0; irit <m_vprit.size(); irit++)
	{
		GdlRuleItem * prit = m_vprit[irit];
		if (prit->OutputSymbol()->FullName() != "ANY")
			crit++;
	}
	return crit;
}

/*----------------------------------------------------------------------------------------------
	Return the index of the single item that deletions or insertions can be associated
	with. Return -1 if there is more than one such item, in which case we can not do an automatic
	association.
----------------------------------------------------------------------------------------------*/
int GdlRule::FindAutoAssocItem(bool fDelete)
{
	int iritResult = -1;
	for (auto irit = 0U; irit < m_vprit.size(); ++irit)
	{
		GdlRuleItem * prit = m_vprit[irit];
		if (prit->OutputSymbol()->FullName() == "ANY")
			continue;
		GdlSubstitutionItem * pritSub = dynamic_cast<GdlSubstitutionItem *>(prit);
		if (pritSub == NULL
			|| (fDelete && !pritSub->OutputSymbol()->FitsSymbolType(ksymtSpecialUnderscore))
			|| (!fDelete && !pritSub->InputSymbol()->FitsSymbolType(ksymtSpecialUnderscore)))
		{
			if (iritResult != -1)
			{
				iritResult = -1;
				break;	// more than one possibility;
			}
			else
				iritResult = irit;
		}
	}
	return iritResult;
}


/*----------------------------------------------------------------------------------------------
	Calculate the space-contextual values for each pass, indicating the kind of situations in
	which we have spaces in rules. This is needed to allow applications that render on a 
	word-by-word basis to adjust their behavior to take these kinds of rules into account.
----------------------------------------------------------------------------------------------*/
void GdlRenderer::CalculateSpaceContextuals(GrcFont * pfont)
{
	// Add the glyph IDs of all characters that are considered spaces.
	std::vector<utf16> vwSpaceGlyphs;
	utf16 w = pfont->GlyphFromCmap(0x0020, this);	// standard space
	vwSpaceGlyphs.push_back(w);
	// add any other space characters

	m_spcon = kspconNone;
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		m_vprultbl[iprultbl]->CalculateSpaceContextuals(&m_spcon, vwSpaceGlyphs);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::CalculateSpaceContextuals(SpaceContextuals * pspconSoFar,
		std::vector<utf16> & vwSpaceGlyphs)
{
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		m_vppass[ippass]->CalculateSpaceContextuals(pspconSoFar, vwSpaceGlyphs);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlPass::CalculateSpaceContextuals(SpaceContextuals * pspconSoFar,
		std::vector<utf16> & vwSpaceGlyphs)
{
	// Full auto-kerning has the same effect of putting spaces in the middle of rule.
	// This is the most lenient setting, so there is no reason to examine every rule.
	if (m_nAutoKern == kakFull)
	{
		*pspconSoFar = kspconAnywhere;
	}
	else
	{
		for (size_t iprule = 0; iprule < m_vprule.size(); iprule++)
		{
			m_vprule[iprule]->CalculateSpaceContextuals(pspconSoFar, vwSpaceGlyphs);
		}
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRule::CalculateSpaceContextuals(SpaceContextuals * pspconSoFar,
		std::vector<utf16> & vwSpaceGlyphs)
{
	if (*pspconSoFar == kspconAnywhere)
		return;	// most lenient setting possible

	if (m_vprit.size() == 1)
	{
		// Single space slot.
		if (m_vprit[0]->IsSpaceItem(vwSpaceGlyphs))
		{
			if ((int)(*pspconSoFar) <= (int)kspconNone)
				*pspconSoFar = kspconSingleOnly;
		}
		// else no change
	}
	else
	{
		bool fFirst = false;
		bool fLast = false;
		bool fOther = false;
		if (m_vprit[0]->IsSpaceItem(vwSpaceGlyphs))
			fFirst = true;
		if (m_vprit[m_vprit.size() - 1]->IsSpaceItem(vwSpaceGlyphs))
			fLast = true;

		for (size_t iprit = 1; iprit < m_vprit.size() - 1; iprit++)
		{
			if (m_vprit[iprit]->IsSpaceItem(vwSpaceGlyphs))
			{
				fOther = true;
				break;
			}
		}

		if (fOther)
			*pspconSoFar = kspconAnywhere;

		else if (*pspconSoFar == kspconEdgeOnly)
		{}	// no change

		else if (fFirst && fLast)
			*pspconSoFar = kspconEdgeOnly;

		else if ((fFirst && *pspconSoFar == kspconLastOnly)
				|| (fLast && *pspconSoFar == kspconFirstOnly))
			*pspconSoFar = kspconEdgeOnly;

		else if (fFirst)
			*pspconSoFar = kspconFirstOnly;

		else if (fLast)
			*pspconSoFar = kspconLastOnly;

		// else no change
	}
}

/*--------------------------------------------------------------------------------------------*/
bool GdlRuleItem::IsSpaceItem(std::vector<utf16> & vwSpaceGlyphs)
{
	GdlGlyphClassDefn * pglfc = m_psymInput->GlyphClassDefnData();
	if (pglfc)
		return pglfc->IsSpaceGlyph(vwSpaceGlyphs);
	else
		return false;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlGlyphClassDefn::IsSpaceGlyph(std::vector<utf16> & vwSpaceGlyphs)
{
	if (m_fIsSpaceGlyph == -1)
	{
		for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
		{
			if (m_vpglfdMembers[iglfd]->IsSpaceGlyph(vwSpaceGlyphs))
			{
				m_fIsSpaceGlyph = 1;
				return true;
				// break;
			}
		}
		m_fIsSpaceGlyph = 0;
	}
	return (bool)m_fIsSpaceGlyph;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlGlyphDefn::IsSpaceGlyph(std::vector<utf16> & vwSpaceGlyphs)
{
	if (m_fIsSpaceGlyph == -1)
	{
		for (size_t iw = 0; iw < m_vwGlyphIDs.size(); iw++)
		{
			for (size_t iwSp = 0; iwSp < vwSpaceGlyphs.size(); iwSp++)
			{
				if (m_vwGlyphIDs[iw] == vwSpaceGlyphs[iwSp])
				{
					m_fIsSpaceGlyph = 1;
					return true;
					//break;
				}
			}
		}
		m_fIsSpaceGlyph = 0;
	}
	return (bool)m_fIsSpaceGlyph;
}

