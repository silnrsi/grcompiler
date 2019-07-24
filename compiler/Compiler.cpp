/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: Compiler.cpp
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Methods to implement the compiler, which generates the final tables and writes them to
	the output file.
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
	Methods
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Do the pre-compilation tasks for each of the main chunks of data. Return false if
	compilation cannot continue due to an unrecoverable error.
----------------------------------------------------------------------------------------------*/
bool GrcManager::PreCompile(GrcFont * pfont)
{
	if (!PreCompileFeatures(pfont))
		return false;

	if (!PreCompileLanguages(pfont)) // do after features
		return false;

	if (!PreCompileClassesAndGlyphs(pfont))
		return false;

	//	Unscale the extra ascent and extra descent.
	GdlExpression * pexp = m_prndr->ExtraAscent();
	if (pexp)
		pexp->SimplifyAndUnscale(0xFFFF, pfont);
	pexp = m_prndr->ExtraDescent();
	if (pexp)
		pexp->SimplifyAndUnscale(0xFFFF, pfont);

	if (!PreCompileRules(pfont))
		return false;

	return true;
}


bool GrcManager::Compile(GrcFont * /*pfont*/, char * pchOutputPath)
{
	if (IncludePassOptimizations())
		PassOptimizations();
	if (this->IsVerbose())
		std::cout << "[Generating FSMs: ";
	GenerateFsms(pchOutputPath);
	if (this->IsVerbose())
		std::cout << "]\n";
	CalculateContextOffsets();	// after max-rule-context has been set
	CalculateGlatVersion();		// before outputting debug files
	return false;
}


/*----------------------------------------------------------------------------------------------
	Determine what version of the Glat table is needed.
----------------------------------------------------------------------------------------------*/
void GrcManager::CalculateGlatVersion()
{
	int fxdGlatVersion = VersionForTable(ktiGlat);
	//	The version of the Glat table depends on the number of glyph attributes defined,
	//  whether compression is enabled and whether we include the glyph approximation
	//  octaboxes for collision fixing.

    if (m_prndr->HasCollisionPass() || m_tcCompressor != ktcNone)
	{
		g_errorList.AddWarning(3535, NULL, "Version 3.0 of the Glat table will be generated.");
		fxdGlatVersion = 0x00030000;
	}
	else if (m_vpsymGlyphAttrs.size() >= kMaxGlyphAttrsGlat1 && fxdGlatVersion < 0x00020000)
	{
		g_errorList.AddWarning(3531, NULL, "Version 2.0 of the Glat table will be generated.");
		fxdGlatVersion = 0x00020000;
	}
	SetTableVersion(ktiGlat, fxdGlatVersion);
}

/*----------------------------------------------------------------------------------------------
	Generate the engine code for the constraints and actions of a rule.
----------------------------------------------------------------------------------------------*/
void GdlRule::GenerateEngineCode(GrcManager * pcman, int fxdRuleVersion,
	std::vector<gr::byte> & vbActions, std::vector<gr::byte> & vbConstraints)
{
	GenerateConstraintEngineCode(pcman, fxdRuleVersion, vbConstraints);
	//	Save the size of the rule constraints from the -if- statements.
	// int cbGenConstraint = vbConstraints.size();

	//	Count the number of unmodified items at the end of the rule; these do not need to
	//	be processed as far as actions go, and the default scan advance position is just
	//	before these.
	size_t iritLimMod = m_vprit.size();
	while (iritLimMod > 0 && !dynamic_cast<GdlSetAttrItem *>(m_vprit[iritLimMod - 1]))
		iritLimMod--;

	//	Now iritLimMod is the first item that will not be modified.

	//	Also note the the first item that needs to be processed as far as constraints go is
	//	m_critPrependedAnys.
	//	The first item to be processed as far as actions go is
	//	m_critPrependedAnys + m_critPreModContext.

	bool fSetInsertToFalse = false;
	bool fBackUpOneMore = false;
	int iritFirstModItem = m_critPrependedAnys + m_critPreModContext;
	int irit;
	for (irit = m_critPrependedAnys; irit < signed(m_vprit.size()); irit++)
	{
		if (iritFirstModItem <= irit && irit < signed(iritLimMod))
		{
			m_vprit[irit]->GenerateActionEngineCode(pcman, fxdRuleVersion, vbActions, this, irit,
				&fSetInsertToFalse);
		}

		m_vprit[irit]->GenerateConstraintEngineCode(pcman, fxdRuleVersion, vbConstraints,
			irit, m_viritInput, iritFirstModItem);
	}
	if (fSetInsertToFalse)
	{
		//	Have to modify the first item beyond the scan advance position, in order to
		//	set insert = false (due to some attachment). So here we create the code to
		//	set the attribute, and then back up one to get the scan advance position back
		//	to where it should be.
		m_vprit[iritLimMod]->GenerateActionEngineCode(pcman, fxdRuleVersion, vbActions, this, irit,
			&fSetInsertToFalse);
		Assert(!fSetInsertToFalse);
		m_vprit[iritLimMod]->GenerateConstraintEngineCode(pcman, fxdRuleVersion, vbConstraints,
			irit, m_viritInput, iritFirstModItem);
		fBackUpOneMore = true;
	}

	if (vbConstraints.size() == 0)
	{ }	// vbConstraints.Push(kopRetTrue); -- no, leave empty
	else
		vbConstraints.push_back(kopPopRet);

	if (m_nOutputAdvance == -1)
	{
		if (fBackUpOneMore)
		{
			//	Return -1.
			vbActions.push_back(kopPushByte);
			vbActions.push_back(0xFF);
			vbActions.push_back(kopPopRet);
		}
		else
			//	Push return-zero, meaning don't adjust the scan position.
			vbActions.push_back(kopRetZero);
	}
	else
	{
		//	Push a command to return the amount to adjust the scan position--
		//	forward or backward.

		Assert(m_nDefaultAdvance != -1);	// calculated set in GdlRule::HasReprocessing()

		int nAdvanceOffset = m_nOutputAdvance - m_nDefaultAdvance;
		Assert((abs(nAdvanceOffset) & 0xFFFFFF00) == 0);	// check for trucation error

		if (fBackUpOneMore)
			nAdvanceOffset--;

		vbActions.push_back(kopPushByte);
		vbActions.push_back((char)nAdvanceOffset);
		vbActions.push_back(kopPopRet);
	}
}

/*----------------------------------------------------------------------------------------------
	Generate engine code for the constraints of a given rule that were in -if- statements,
	minus the final pop-and-return command.
----------------------------------------------------------------------------------------------*/
void GdlRule::GenerateConstraintEngineCode(GrcManager * /*pcman*/, int fxdRuleVersion,
	std::vector<gr::byte> & vbOutput)
{
	if (m_vpexpConstraints.size() == 0)
	{
		return;
	}

	//	'and' all the constraints together; the separate constraints come from separate
	//	-if- or -elseif- statements.
	int nBogus;
	m_vpexpConstraints[0]->GenerateEngineCode(fxdRuleVersion, vbOutput,
		-1, NULL, -1, false, -1, &nBogus);
	for (size_t ipexp = 1; ipexp < m_vpexpConstraints.size(); ipexp++)
	{
		m_vpexpConstraints[ipexp]->GenerateEngineCode(fxdRuleVersion, vbOutput,
			-1, NULL, -1, false, -1, &nBogus);
		vbOutput.push_back(kopAnd);
	}
}


/*----------------------------------------------------------------------------------------------
	Generate engine code for the constraints of a given rule item.
	Arguments:
		vbOutput			- buffer containing engine code already generated from -if-
								statements, minus the final pop-and-return command.
		viritInput			- input indices for items of this rule
		irit				- index of item
----------------------------------------------------------------------------------------------*/
void GdlRuleItem::GenerateConstraintEngineCode(GrcManager * /*pcman*/, int fxdRuleVersion,
	std::vector<gr::byte> & vbOutput,
	int irit, std::vector<int> & viritInput, int iritFirstModItem)
{
	if (!m_pexpConstraint)
	{
		return;
	}

	bool fNeedAnd = (vbOutput.size() > 0);	// need to 'and' rule item constraints with
											// -if- condition(s)

	bool fInserting = (m_psymInput->FitsSymbolType(ksymtSpecialUnderscore));
	Assert(!fInserting || dynamic_cast<GdlSubstitutionItem *>(this));

	char iritByte = viritInput[irit];
	Assert((int)iritByte == viritInput[irit]);	// no truncation error
	Assert(viritInput[irit] >= 0);	// not an inserted item

	vbOutput.push_back(kopCntxtItem);
	vbOutput.push_back(iritByte - iritFirstModItem);
	vbOutput.push_back(0); // place holder
	int ibSkipLoc = vbOutput.size();
	int nBogus;
	m_pexpConstraint->GenerateEngineCode(fxdRuleVersion, vbOutput, irit, &viritInput, irit,
		fInserting, -1, &nBogus);

	//	Go back and fill in number of bytes to skip if we are not at the 
	//	appropriate context item.
	vbOutput[ibSkipLoc - 1] = (byte)((int)vbOutput.size() - ibSkipLoc);

	if (fNeedAnd)
		vbOutput.push_back(kopAnd);
}

/*----------------------------------------------------------------------------------------------
	Generate the engine code for the constraints of a pass.
----------------------------------------------------------------------------------------------*/
void GdlPass::GenerateEngineCode(GrcManager * /*pcman*/, int fxdRuleVersion,
	std::vector<gr::byte> & vbOutput)
{
	if (m_vpexpConstraints.size() == 0)
	{
		return;
	}

	//	'and' all the constraints together; multiple constraints result from an -else if-
	//	structure.
	int nBogus;
	m_vpexpConstraints[0]->GenerateEngineCode(fxdRuleVersion, vbOutput,
		-1, NULL, -1, false, -1, &nBogus);
	for (size_t ipexp = 1; ipexp < m_vpexpConstraints.size(); ipexp++)
	{
		m_vpexpConstraints[ipexp]->GenerateEngineCode(fxdRuleVersion, vbOutput,
			-1, NULL, -1, false, -1, &nBogus);
		vbOutput.push_back(kopAnd);
	}
	vbOutput.push_back(kopPopRet);
}

/*----------------------------------------------------------------------------------------------
	Generate engine code to perform the actions for a given item.
----------------------------------------------------------------------------------------------*/
void GdlRuleItem::GenerateActionEngineCode(GrcManager * /*pcman*/, int /*fxdRuleVersion*/,
	std::vector<gr::byte> & vbOutput,
	GdlRule * /*prule*/, int /*irit*/, bool * pfSetInsertToFalse)
{
	if (*pfSetInsertToFalse)
	{
		vbOutput.push_back(kopPutCopy);
		vbOutput.push_back(0);
		GenerateInsertEqualsFalse(vbOutput);
		*pfSetInsertToFalse = false;
		vbOutput.push_back(kopNext);
	}
	else
		//	Nothing special is happening; just pass the item through unchanged.
		vbOutput.push_back(kopCopyNext);
}

/*--------------------------------------------------------------------------------------------*/
void GdlSetAttrItem::GenerateActionEngineCode(GrcManager * pcman, int fxdRuleVersion,
	std::vector<gr::byte> & vbOutput,
	GdlRule * /*prule*/, int irit, bool * pfSetInsertToFalse)
{
	if (m_vpavs.size() == 0 && !*pfSetInsertToFalse)
		vbOutput.push_back(kopCopyNext);
	else
	{
		int nIIndex = m_nInputIndex;
		nIIndex = (nIIndex < 0) ? (nIIndex + 1) * -1 : nIIndex;

		vbOutput.push_back(kopPutCopy);
		vbOutput.push_back(0);
		if (*pfSetInsertToFalse)
			GenerateInsertEqualsFalse(vbOutput);
		*pfSetInsertToFalse = GenerateAttrSettingCode(pcman, fxdRuleVersion, vbOutput,
			irit, nIIndex);
		vbOutput.push_back(kopNext);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlSubstitutionItem::GenerateActionEngineCode(GrcManager * pcman, int fxdRuleVersion,
	std::vector<gr::byte> & vbOutput,
	GdlRule * prule, int irit, bool * pfSetInsertToFalse)
{
	bool fInserting = (m_psymInput->FitsSymbolType(ksymtSpecialUnderscore));
	bool fDeleting = (m_psymOutput->FitsSymbolType(ksymtSpecialUnderscore));
	Assert(!fInserting || !fDeleting);

	int nIIndex = m_nInputIndex;
	nIIndex = (nIIndex < 0) ? (nIIndex + 1) * -1 : nIIndex;
	if (fInserting)
		//	Because we haven't "got" the current slot yet (when inserting we do a peek,
		//	not a "get").
		nIIndex--;

	//	Generate the code to insert, delete, replace, etc.
	if (fDeleting)
		//	Note that it's kind of strange to be setting attributes or associations for 
		//	deleted objects, but it's not an error and we've already given a warning.
		vbOutput.push_back(kopDelete);
	else
	{
		if (fInserting)
			vbOutput.push_back(kopInsert);

		if (m_psymOutput->FitsSymbolType(ksymtSpecialAt))
		{
			//	Direct copy.
			int bOffset = (m_nSelector == -1) ? 0 : m_nSelector - nIIndex;
			Assert((abs(bOffset) & 0xFFFFFF00) == 0);	// check for truncation error
			vbOutput.push_back(kopPutCopy);
			vbOutput.push_back((char)bOffset);
		}
		else
		{
			Assert(m_psymOutput->FitsSymbolType(ksymtClass));
			GdlGlyphClassDefn * pglfcOutput = m_psymOutput->GlyphClassDefnData();
			Assert(pglfcOutput);

			int op;
			int nSel = (m_pexpSelector) ? m_pexpSelector->SlotNumber() - 1 : irit;
			GdlRuleItem * pritSel = prule->Item(nSel);
			Symbol psymSel = pritSel->m_psymInput;
			GdlGlyphClassDefn * pglfcSel = psymSel->GlyphClassDefnData();
			//	We're not doing a substitution based on correspondences within classes, but
			//	rather a simple replacement, under the following circumstances:
			if (psymSel->FitsSymbolType(ksymtSpecialUnderscore))
				//	(a) there is no selector class
				op = kopPutGlyph;
			else if (pglfcSel && pglfcSel->GlyphIDCount() == 0)
				//	(b) the selector class has no glyphs
				op = kopPutGlyph;
			else if (pglfcOutput && pglfcOutput->GlyphIDCount() <= 1)
				//	(c) there is only one glyph in the output class
				op = kopPutGlyph;
			else
			{
				//	Otherwise we're doing a replacement of a glyph from the selector class
				//	with the corresponding glyph from the output class.
				Assert(pglfcSel);
				op = kopPutSubs;
			}
			if (fxdRuleVersion <= 0x00020000)
			{
				// Use old 8-bit versions of these commands.
				switch (op)
				{
				case kopPutGlyph:	op = kopPutGlyphV1_2;	break;
				case kopPutSubs:	op = kopPutSubsV1_2;	break;
				default: break;
				}
			}
			vbOutput.push_back(op);

			int nOutputID = pglfcOutput->ReplcmtOutputID();
			Assert(nOutputID >= 0);

			switch (op)
			{
			case kopPutGlyph:
				vbOutput.push_back(nOutputID >> 8);
				vbOutput.push_back(nOutputID & 0x000000FF);
				break;
			case kopPutGlyphV1_2:
				vbOutput.push_back(nOutputID);
				break;
			case kopPutSubs:
			case kopPutSubsV1_2:
				{
					int nSelIO = (m_nSelector == -1) ? nIIndex : m_nSelector;
					int bSelOffset = nSelIO - nIIndex;
					Assert((abs(bSelOffset) & 0xFFFF0000) == 0);	// check for truncation error

					Assert(pglfcSel->ReplcmtInputID() >= 0);

					vbOutput.push_back((char)bSelOffset);

					int nInputID = pglfcSel->ReplcmtInputID();
					if (op == kopPutSubsV1_2)
					{
						vbOutput.push_back(nInputID);
						vbOutput.push_back(nOutputID);
					}
					else
					{
						vbOutput.push_back(nInputID >> 8);
						vbOutput.push_back(nInputID & 0x000000FF);
						vbOutput.push_back(nOutputID >> 8);
						vbOutput.push_back(nOutputID & 0x000000FF);
					}
					break;
				}
			default:
				Assert(false);
				break;
			}
		}
	}

	//	Generate the code to set the associations.
	if (m_vnAssocs.size() > 0)
	{
		vbOutput.push_back(kopAssoc);
		vbOutput.push_back((byte)m_vnAssocs.size());
		for (size_t in = 0; in < m_vnAssocs.size(); in++)
		{
			Assert(m_vnAssocs[in] >= 0);	// can't associate with an inserted item
			int bAssocOffset = m_vnAssocs[in] - nIIndex;
			Assert((abs(bAssocOffset) & 0xFFFFFF00) == 0);	// check for truncation error

			vbOutput.push_back((char)bAssocOffset);
		}
	}

	//	Generate the code to set the attributes.
	if (*pfSetInsertToFalse)
		GenerateInsertEqualsFalse(vbOutput);

	*pfSetInsertToFalse = GenerateAttrSettingCode(pcman, fxdRuleVersion, vbOutput, irit, nIIndex);

	//	Go on to the next slot.
	vbOutput.push_back(kopNext);
}


/*----------------------------------------------------------------------------------------------
	Generate engine code to set slot attributes. Return true if we need to set
	insert = false on the following item (because this item makes a forward attachment).
----------------------------------------------------------------------------------------------*/
bool GdlSetAttrItem::GenerateAttrSettingCode(GrcManager * pcman, int fxdRuleVersion,
	std::vector<gr::byte> & vbOutput,
	int irit, int nIIndex)
{
	bool fAttachForward = false;
	for (size_t ipavs = 0; ipavs < m_vpavs.size(); ipavs++)
	{
		if (m_vpavs[ipavs]->GenerateAttrSettingCode(pcman, fxdRuleVersion, vbOutput,
			irit, nIIndex, AttachTo()))
		{
			fAttachForward = true;
		}
	}
	return fAttachForward;
}

/*--------------------------------------------------------------------------------------------*/
bool GdlAttrValueSpec::GenerateAttrSettingCode(GrcManager * pcman, int fxdRuleVersion,
	std::vector<gr::byte> & vbOutput,
	int irit, int nIIndex, int iritAttachTo)
{
	bool fAttachForward = false;

	int nBogus;

	Assert(m_psymName->FitsSymbolType(ksymtSlotAttr) || m_psymName->FitsSymbolType(ksymtFeature));
	Assert(m_pexpValue);
	ExpressionType expt = m_psymName->ExpType();
	Assert(expt == kexptSlotRef || expt == kexptNumber
		|| expt == kexptMeas || expt == kexptBoolean || expt == kexptGlyphID);
	std::string staOp = m_psymOperator->FullName();
	int slat = m_psymName->FitsSymbolType(ksymtSlotAttr) ? m_psymName->SlotAttrEngineCodeOp() : kslatBogus;

	if (m_psymName->IsPseudoSlotAttr())
	{
		// Ignore
	}
	else if (m_psymName->FitsSymbolType(ksymtFeature))
	{
		GdlFeatureDefn * pfeat = m_psymName->FeatureDefnData();
		Assert(staOp == "=");

		m_pexpValue->GenerateEngineCode(fxdRuleVersion, vbOutput, irit, NULL, nIIndex,
			false, -1, &nBogus);

		vbOutput.push_back(kopFeatSet);
		vbOutput.push_back(pfeat->InternalID());
	}
	else if (m_psymName->IsIndexedSlotAttr())	// eg, component.XXX.ref, user1
	{
		m_pexpValue->GenerateEngineCode(fxdRuleVersion, vbOutput,
			irit, NULL, nIIndex, false, -1, &nBogus);

		if (m_psymName->IsComponentRef() || pcman->VersionForTable(ktiSilf) < 0x00020000)
		{
			Assert(staOp == "=");
			vbOutput.push_back(kopIAttrSetSlot);
		}
		else if (m_psymName->IsUserDefinableSlotAttr())
		{
			if (staOp == "=")
				vbOutput.push_back(kopIAttrSet);
			else if (staOp == "+=")
				vbOutput.push_back(kopIAttrAdd);
			else if (staOp == "-=")
				vbOutput.push_back(kopIAttrSub);
//			else if (staOp == "&=")
//				vbOutput.push_back(kopIAttrBitAnd);
//			else if (staOp == "|=")
//				vbOutput.push_back(kopIAttrBitOr);
			else
			{
				Assert(false);
			}
		}
		vbOutput.push_back(slat);
		vbOutput.push_back(pcman->SlotAttributeIndex(m_psymName));
	}
	else if (expt == kexptSlotRef)
	{
		Assert(staOp == "=");
		int nValue;
		if (slat == kslatAttTo && iritAttachTo == -1)
		{
			// attach.to = @0 means no attachment
		}
		else
		{
			m_pexpValue->GenerateEngineCode(fxdRuleVersion, vbOutput, irit, NULL, nIIndex,
				false, iritAttachTo, &nValue);

			vbOutput.push_back(kopAttrSetSlot);
			vbOutput.push_back(slat);

			if (slat == kslatAttTo)
			{
				if (nValue < 0)
					GdlRuleItem::GenerateInsertEqualsFalse(vbOutput);	// for this slot
				else if (nValue > 0)
					fAttachForward = true;	// generate insert = false for next slot
			}
		}
	}
	else
	{
		bool fAttachAt =
			(slat == kslatAttAtX || slat == kslatAttAtY || slat == kslatAttAtGpt ||
				slat == kslatAttAtXoff || slat == kslatAttAtYoff);

		int op = kopNop;
		if (staOp == "=")
			op = kopAttrSet;
		else if (staOp == "+=")
			op = kopAttrAdd;
		else if (staOp == "-=")
			op = kopAttrSub;
//		else if (staOp == "&=")
//			op = kopAttrBitAnd;
//		else if (staOp == "|=")
//			op = kopAttrBitOr;
		else
		{
			Assert(false);
		}

		m_pexpValue->GenerateEngineCode(fxdRuleVersion, vbOutput, irit, NULL, nIIndex,
			fAttachAt, iritAttachTo, &nBogus);

		vbOutput.push_back(op);
		vbOutput.push_back(slat);
	}

	return fAttachForward;
}

/*----------------------------------------------------------------------------------------------
	Generate the extra "insert = false" for attachments.
----------------------------------------------------------------------------------------------*/
void GdlRuleItem::GenerateInsertEqualsFalse(std::vector<gr::byte> & vbOutput)
{
	vbOutput.push_back(kopPushByte);
	vbOutput.push_back(0);	// false;
	vbOutput.push_back(kopAttrSet);
	vbOutput.push_back(kslatInsert);
}


/*----------------------------------------------------------------------------------------------
	Return the component ID for a given ligature component symbol, or the number cooresponding
	to the index of the user-definable slot attribute.
----------------------------------------------------------------------------------------------*/
int GrcManager::SlotAttributeIndex(Symbol psym)
{
	if (psym->IsComponentRef())
	{
		Symbol psymBase = psym->BaseLigComponent();
		if (!psymBase->IsGeneric())
			psymBase = psymBase->Generic();
		Assert(psymBase->IsGeneric());

		int nIDRet = psymBase->InternalID();
		Assert(nIDRet > -1);
		return nIDRet;
	}
	else if (psym->IsUserDefinableSlotAttr())
	{
		return psym->UserDefinableSlotAttrIndex();
	}
	else
	{
		//	No other kinds of indexed attributes so far.
		Assert(false);
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	Analyze the passes to calculate the maximum number of characters before and after
	the official range of a segment that could cause the segment to become invalid.
	Also set a simple flag if any line-break items occur in any rule.
----------------------------------------------------------------------------------------------*/
void GrcManager::CalculateContextOffsets()
{
	m_prndr->CalculateContextOffsets();
}

/*--------------------------------------------------------------------------------------------*/
void GdlRenderer::CalculateContextOffsets()
{
	m_fLineBreak = false;
	m_fLineBreakB4Just = false;
	m_critPreXlbContext = 0;
	m_critPostXlbContext = 0;

	GdlRuleTable * prultblSub = FindRuleTable("substitution");
	GdlRuleTable * prultblJust= FindRuleTable("justification");
	GdlRuleTable * prultblPos = FindRuleTable("positioning");

	//	Don't need to do this for the linebreak table, since conceptually it occurs
	//	before the linebreaks have been made.

	if (prultblSub)
		prultblSub->CalculateContextOffsets(&m_critPreXlbContext, &m_critPostXlbContext,
			&m_fLineBreak, false, NULL, NULL);

	m_fLineBreakB4Just = m_fLineBreak;

	if (prultblJust)
		prultblJust->CalculateContextOffsets(&m_critPreXlbContext, &m_critPostXlbContext,
			&m_fLineBreak, false, prultblSub, NULL);

	if (prultblPos)
		prultblPos->CalculateContextOffsets(&m_critPreXlbContext, &m_critPostXlbContext,
			&m_fLineBreak, true, prultblSub, prultblJust);
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::CalculateContextOffsets(int * pcPreXlbContext, int * pcPostXlbContext,
	bool * pfLineBreak, bool fPos, GdlRuleTable * prultbl1, GdlRuleTable * prultbl2)
{
	if (*pcPreXlbContext == kInfiniteXlbContext && *pcPostXlbContext == kInfiniteXlbContext)
	{
		Assert(*pfLineBreak);
		*pfLineBreak = true;
		return;
	}

	for (size_t ipass = 0; ipass < m_vppass.size(); ipass++)
	{
		GdlPass * ppass = m_vppass[ipass];

		if (ppass->HasLineBreaks())
			*pfLineBreak = true;

		//	If no cross-line-boundary rules in this pass, ignore it.
		if (!ppass->HasCrossLineContext())
			continue;

		if (ppass->HasReprocessing())
		{
			//	This pass has reprocessing occurring: return values indicating that we
			//	can't determine a context limit.
			*pcPreXlbContext = kInfiniteXlbContext;
			*pcPostXlbContext = kInfiniteXlbContext;
			return;
		}

		int cPreTmp = ppass->MaxPreLBSlots();
		int cPostTmp = ppass->MaxPostLBSlots();

		//	Loop backwards through all the passes in this table, calculating the ranges.
		for (int ipassPrev = ipass; ipassPrev-- > 0; )
		{
			GdlPass * ppassPrev = m_vppass[ipassPrev];
			if (fPos)
			{
				cPreTmp = std::max(cPreTmp, ppassPrev->MaxPreLBSlots());
				cPostTmp = std::max(cPostTmp, ppassPrev->MaxPostLBSlots());
			}
			else
			{
				if (ppassPrev->HasReprocessing())
				{
					//	Previous pass has reprocessing occurring: return values indicating that
					//	we can't determine a context limit.
					*pcPreXlbContext = kInfiniteXlbContext;
					*pcPostXlbContext = kInfiniteXlbContext;
					return;
				}
				//	For the substitution table, multiply the range by the max number of context
				//	items in the pass.
				cPreTmp = cPreTmp * ppassPrev->MaxRuleContext();
				cPostTmp = cPostTmp * ppassPrev->MaxRuleContext();
			}
		}

		//	Loop backwards through the previous table(s) also.
		for (int itbl = 2; itbl > 0; itbl--)
		{
			GdlRuleTable * prultblPrev = ((itbl == 2) ? prultbl2 : prultbl1);
			if (prultblPrev)
			{
				for (int ipassPrev = prultblPrev->NumberOfPasses(); ipassPrev-- > 0; )
				{
					GdlPass * ppassPrev = prultblPrev->m_vppass[ipassPrev];

					if (ppassPrev->HasReprocessing())
					{
						//	Previous pass has reprocessing occurring:
						//	return values indicating that
						//	we can't determine a context limit.
						*pcPreXlbContext = kInfiniteXlbContext;
						*pcPostXlbContext = kInfiniteXlbContext;
						return;
					}
					cPreTmp = cPreTmp * ppassPrev->MaxRuleContext();
					cPostTmp = cPostTmp * ppassPrev->MaxRuleContext();
				}
			}
		}

		*pcPreXlbContext = max(*pcPreXlbContext, cPreTmp);
		*pcPostXlbContext = max(*pcPostXlbContext, cPostTmp);
	}
}


/***********************************************************************************************
	Debuggers
***********************************************************************************************/
/*----------------------------------------------------------------------------------------------
	Output a list of rules ordered by precedence.
----------------------------------------------------------------------------------------------*/
void GrcManager::DebugRulePrecedence(char * pchOutputPath)
{

	std::string staOutputFilename(pchOutputPath);
	staOutputFilename.append("/dbg_ruleprec.txt");

	std::ofstream strmOut;
	strmOut.open(staOutputFilename.data());
	if (strmOut.fail())
	{
		g_errorList.AddWarning(6501, NULL,
			"Error in writing to file ", staOutputFilename.data());
		return;
	}

	if (g_errorList.AnyFatalErrors())
		strmOut << "Fatal errors--compilation aborted";
	else
	{
		strmOut << "RULE PRECEDENCE\n\n";
		m_prndr->DebugRulePrecedence(this, strmOut);
	}

	strmOut.close();
}
/*--------------------------------------------------------------------------------------------*/
void GdlRenderer::DebugRulePrecedence(GrcManager * pcman, std::ostream & strmOut)
{
	GdlRuleTable * prultbl;

	if ((prultbl = FindRuleTable("linebreak")) != NULL)
		prultbl->DebugRulePrecedence(pcman, strmOut, m_ipassBidi);

	if ((prultbl = FindRuleTable("substitution")) != NULL)
		prultbl->DebugRulePrecedence(pcman, strmOut, m_ipassBidi);

	if ((prultbl = FindRuleTable("justification")) != NULL)
		prultbl->DebugRulePrecedence(pcman, strmOut, m_ipassBidi);

	if ((prultbl = FindRuleTable("positioning")) != NULL)
		prultbl->DebugRulePrecedence(pcman, strmOut, m_ipassBidi);
}

void GdlRuleTable::DebugRulePrecedence(GrcManager * pcman, std::ostream & strmOut, int ipassBidi)
{
	strmOut << "\nTABLE: " << m_psymName->FullName() << "\n";
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		if (ipassBidi - 1 == m_vppass[ippass]->GlobalID())
			strmOut << "\nPASS " << ipassBidi << ": bidi\n";

		m_vppass[ippass]->DebugRulePrecedence(pcman, strmOut);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlPass::DebugRulePrecedence(GrcManager * pcman, std::ostream & strmOut)
{
	if (m_vprule.size() == 0)
	{
		strmOut << "\nPASS: " << PassDebuggerNumber() << " - no rules\n";
		return;
	}

	Assert(PassDebuggerNumber() != 0);

	strmOut << "\nPASS: " << PassDebuggerNumber() << " (GDL #" << this->m_nNumber << ")\n";

	// Sort rules by their precedence: primarily by the number of items matched (largest first),
	// and secondarily by their location in the file (rule number--smallest first).
	std::vector<int> viruleSorted;
	std::vector<int> vnKeys;
	for (size_t irule1 = 0; irule1 < m_vprule.size(); irule1++)
	{
		int nSortKey1 = m_vprule[irule1]->SortKey();
		size_t iirule2;
		for (iirule2 = 0; iirule2 < viruleSorted.size(); iirule2++)
		{
			int nSortKey2 = vnKeys[iirule2];
			if (nSortKey1 > nSortKey2 ||
				(nSortKey1 == nSortKey2 && signed(irule1) < viruleSorted[iirule2]))
			{
				// Insert it.
				viruleSorted.insert(viruleSorted.begin() + iirule2, irule1);
				vnKeys.insert(vnKeys.begin() + iirule2, nSortKey1);
				break;
			}
		}
		if (iirule2 >= viruleSorted.size())
		{
			viruleSorted.push_back(irule1);
			vnKeys.push_back(nSortKey1);
		}

		Assert(viruleSorted.size() == irule1 + 1);
	}

	int nPassNum = PassDebuggerNumber();
	for (size_t iirule = 0; iirule < m_vprule.size(); iirule++)
	{
		strmOut << "\n" << iirule << " - RULE " << nPassNum << "." << viruleSorted[iirule] << ", ";
		m_vprule[viruleSorted[iirule]]->LineAndFile().WriteToStream(strmOut, true);
		strmOut << ":  ";

		m_vprule[viruleSorted[iirule]]->RulePrettyPrint(pcman, strmOut, false);
		strmOut << "\n\n";
	}
}

/*----------------------------------------------------------------------------------------------
	Output a text version of the engine code to the stream.
----------------------------------------------------------------------------------------------*/
void GrcManager::DebugEngineCode(char * pchOutputPath)
{
	std::string staOutputFilename(pchOutputPath);
	staOutputFilename.append("/dbg_enginecode.txt");

	std::ofstream strmOut;
	strmOut.open(staOutputFilename.data());
	if (strmOut.fail())
	{
		g_errorList.AddWarning(6502, NULL,
			"Error in writing to file ", staOutputFilename.data());
		return;
	}

	if (g_errorList.AnyFatalErrors())
		strmOut << "Fatal errors--compilation aborted";
	else
	{
		strmOut << "ENGINE CODE FOR RULES\n\n";
		m_prndr->DebugEngineCode(this, strmOut);
	}

	strmOut.close();
}

/*--------------------------------------------------------------------------------------------*/
void GdlRenderer::DebugEngineCode(GrcManager * pcman, std::ostream & strmOut)
{
	GdlRuleTable * prultbl;

	int fxdRuleVersion = pcman->VersionForRules();

	if ((prultbl = FindRuleTable("linebreak")) != NULL)
		prultbl->DebugEngineCode(pcman, fxdRuleVersion, strmOut);

	if ((prultbl = FindRuleTable("substitution")) != NULL)
		prultbl->DebugEngineCode(pcman, fxdRuleVersion, strmOut);

	if (m_ipassBidi > -1)
		strmOut << "\nPASS " << m_ipassBidi + 1 << ": bidi\n";

	if ((prultbl = FindRuleTable("justification")) != NULL)
		prultbl->DebugEngineCode(pcman, fxdRuleVersion, strmOut);

	if ((prultbl = FindRuleTable("positioning")) != NULL)
		prultbl->DebugEngineCode(pcman, fxdRuleVersion, strmOut);
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::DebugEngineCode(GrcManager * pcman, int fxdRuleVersion, std::ostream & strmOut)
{
	strmOut << "\nTABLE: " << m_psymName->FullName() << "\n";
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		m_vppass[ippass]->DebugEngineCode(pcman, fxdRuleVersion, strmOut);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlPass::DebugEngineCode(GrcManager * pcman, int fxdRuleVersion, std::ostream & strmOut)
{
	int nPassNum = PassDebuggerNumber();
	strmOut << "\nPASS: " << nPassNum << "\n";

	std::vector<gr::byte> vbPassConstraints;
	GenerateEngineCode(pcman, fxdRuleVersion, vbPassConstraints);
	if (vbPassConstraints.size() == 0)
	{
		strmOut << "\nPASS CONSTRAINTS: none\n";
	}
	else
	{
		strmOut << "\nPASS CONSTRAINTS:\n";
		GdlRule::DebugEngineCode(vbPassConstraints, fxdRuleVersion, strmOut);
	}

	if (m_vprule.size() == 0)
		strmOut << "\nNO RULES\n";

	for (size_t iprul = 0; iprul < m_vprule.size(); iprul++)
	{
		strmOut << "\nRULE " << nPassNum << "." << iprul << ", ";
		m_vprule[iprul]->LineAndFile().WriteToStream(strmOut, true);
		strmOut << ":  ";

		m_vprule[iprul]->RulePrettyPrint(pcman, strmOut, false);
		strmOut << "\n";
		m_vprule[iprul]->DebugEngineCode(pcman, fxdRuleVersion, strmOut);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRule::DebugEngineCode(GrcManager * pcman, int fxdRuleVersion, std::ostream & strmOut)
{
	std::vector<gr::byte> vbActions;
	std::vector<gr::byte> vbConstraints;

	GenerateEngineCode(pcman, fxdRuleVersion, vbActions, vbConstraints);

	strmOut << "\nACTIONS:\n";
	DebugEngineCode(vbActions, fxdRuleVersion, strmOut);

	if (vbConstraints.size() == 0)
	{
		strmOut << "\nCONSTRAINTS: none\n";
	}
	else
	{
		strmOut << "\nCONSTRAINTS:\n";
		DebugEngineCode(vbConstraints, fxdRuleVersion, strmOut);
	}
}

void GdlRule::DebugEngineCode(std::vector<gr::byte> & vb, int /*fxdRuleVersion*/, std::ostream & strmOut)
{
	int ib = 0;
	while (ib < signed(vb.size()))
	{
		int op = vb[ib++];
		strmOut << EngineCodeDebugString(op);

		int cbArgs = 0;
		int slat;
		unsigned int nUnsigned;
		int nSigned;
		signed short int nSignedShort;
		int gmet;
		int pstat;
		switch (op)
		{
		case kopNop:				cbArgs = 0;		break;
		case kopPushByte:			cbArgs = 1;		break;
		case kopPushByteU:
			nUnsigned = (unsigned int)vb[ib++];
			strmOut << " " << nUnsigned;
			break;
		case kopPushShort:
			nSignedShort = (signed short int)vb[ib++];
			nSignedShort = (nSignedShort << 8) + vb[ib++];
			nSigned = (signed int)nSignedShort;
			strmOut << " " << nSigned;
			break;
		case kopPushShortU:
			nUnsigned = (unsigned int)vb[ib++];
			nUnsigned = (nUnsigned << 8) + vb[ib++];
			strmOut << " " << nUnsigned;
			break;
		case kopPushLong:
			nUnsigned = (int)vb[ib++];
			nUnsigned = (nUnsigned << 8) + vb[ib++];
			nUnsigned = (nUnsigned << 8) + vb[ib++];
			nUnsigned = (nUnsigned << 8) + vb[ib++];
			nSigned = (signed int)nUnsigned;
			strmOut << " " << nSigned;
			break;
		case kopAdd:				cbArgs = 0;		break;
		case kopSub:				cbArgs = 0;		break;
		case kopMul:				cbArgs = 0;		break;
		case kopDiv:				cbArgs = 0;		break;
		case kopBitAnd:				cbArgs = 0;		break;
		case kopBitOr:				cbArgs = 0;		break;
		case kopBitNot:				cbArgs = 0;		break;
		case kopMin:				cbArgs = 0;		break;
		case kopMax:				cbArgs = 0;		break;
		case kopNeg:				cbArgs = 0;		break;
		case kopTrunc8:				cbArgs = 0;		break;
		case kopTrunc16:			cbArgs = 0;		break;
		case kopCond:				cbArgs = 0;		break;
		case kopAnd:				cbArgs = 0;		break;
		case kopOr:					cbArgs = 0;		break;
		case kopNot:				cbArgs = 0;		break;
		case kopEqual:				cbArgs = 0;		break;
		case kopNotEq:				cbArgs = 0;		break;
		case kopLess:				cbArgs = 0;		break;
		case kopGtr:				cbArgs = 0;		break;
		case kopLessEq:				cbArgs = 0;		break;
		case kopGtrEq:				cbArgs = 0;		break;
		case kopNext:				cbArgs = 0;		break;
		case kopNextN:				cbArgs = 1;		break;	// N
		case kopCopyNext:			cbArgs = 0;		break;
		case kopPutGlyphV1_2:
			nUnsigned = (unsigned int)vb[ib++];	// output class
			strmOut << " " << nUnsigned;
			cbArgs = 0;
			break;
		case kopPutSubsV1_2:
			nSigned = (signed int)vb[ib++];		// selector
			strmOut << " " << nSigned;
			nUnsigned = (unsigned int)vb[ib++];	// input class
			strmOut << " " << nUnsigned;
			nUnsigned = (unsigned int)vb[ib++];	// output class
			strmOut << " " << nUnsigned;
			cbArgs = 0;
			break;
		case kopPutCopy:			cbArgs = 1;		break;	// selector
		case kopInsert:				cbArgs = 0;		break;
		case kopDelete:				cbArgs = 0;		break;

		case kopPutGlyph:
			nSignedShort = (signed short int)vb[ib++];
			nSignedShort = (nSignedShort << 8) + vb[ib++];
			nSigned = (signed int)nSignedShort;
			strmOut << " " << nSigned;	// output class
			break;

		case kopPutSubs3:
			nSigned = (int)vb[ib++];
			strmOut << " " << nSigned;	// slot offset
			nSignedShort = (signed short int)vb[ib++];
			nSignedShort = (nSignedShort << 8) + vb[ib++];
			nSigned = (signed int)nSignedShort;
			strmOut << " " << nSigned;	// input class
			// fall through
		case kopPutSubs2:
			nSigned = (int)vb[ib++];
			strmOut << " " << nSigned;	// slot offset
			nSignedShort = (signed short int)vb[ib++];
			nSignedShort = (nSignedShort << 8) + vb[ib++];
			nSigned = (signed int)nSignedShort;
			strmOut << " " << nSigned;	// input class
			// fall through
		case kopPutSubs:
			nSigned = (int)vb[ib++];
			strmOut << " " << nSigned;	// slot offset
			nSignedShort = (signed short int)vb[ib++];
			nSignedShort = (nSignedShort << 8) + vb[ib++];
			nSigned = (signed int)nSignedShort;
			strmOut << " " << nSigned;	// input class
			nSignedShort = (signed short int)vb[ib++];
			nSignedShort = (nSignedShort << 8) + vb[ib++];
			nSigned = (signed int)nSignedShort;
			strmOut << " " << nSigned;	// output class
			break;

		case kopAssoc:
			cbArgs = vb[ib++];
			strmOut << " " << cbArgs;
			break;
		case kopCntxtItem:			cbArgs = 2;		break;

		case kopAttrSet:
		case kopAttrAdd:
		case kopAttrSub:
//		case kopAttrBitAnd:
//		case kopAttrBitOr:
		case kopAttrSetSlot:
			slat = vb[ib++];
			strmOut << " " << SlotAttributeDebugString(slat);
			cbArgs = 0;
			break;
		case kopIAttrSet:
		case kopIAttrAdd:
		case kopIAttrSub:
//		case kopIAttrBitAnd:
//		case kopIAttrBitOr:
		case kopIAttrSetSlot:
			slat = vb[ib++];
			strmOut << " " << SlotAttributeDebugString(slat);
			cbArgs = 1;
			break;
		case kopPushSlotAttr:
			slat = vb[ib++];
			strmOut << " " << SlotAttributeDebugString(slat);
			cbArgs = 1;	// selector
			break;
		case kopPushISlotAttr:
			slat = vb[ib++];
			strmOut << " " << SlotAttributeDebugString(slat);
			cbArgs = 2;	// selector, index
			break;
		case kopPushGlyphAttr:
		case kopPushAttToGlyphAttr:
			nSignedShort = (signed short int)vb[ib++];
			nSignedShort = (nSignedShort << 8) + vb[ib++];
			nSigned = (signed int)nSignedShort;
			strmOut << " " << nSigned;	// glyph attribute
			cbArgs = 1;					// selector
			break;
		case kopPushGlyphAttrV1_2:
		case kopPushAttToGAttrV1_2:
			nUnsigned = (unsigned int)vb[ib++]; // glyph attribute
			strmOut << " " << nUnsigned;
			cbArgs = 1;							// selector
			break;
		case kopPushGlyphMetric:
		case kopPushAttToGlyphMetric:
			gmet = vb[ib++];
			strmOut << " " << GlyphMetricDebugString(gmet);
			cbArgs = 2;	// selector, cluster
			break;
		case kopPushFeat:			cbArgs = 2;		break;	// feature internal ID, selector
		//case kopPushIGlyphAttr:	cbArgs = 2;		break;	// glyph attr, index
		case kopPushProcState:
			pstat = vb[ib++];
			strmOut << " " << ProcessStateDebugString(pstat);
			cbArgs = 0;
			break;
		case kopPushVersion:		cbArgs = 0;		break;
		case kopPopRet:				cbArgs = 0;		break;
		case kopRetZero:			cbArgs = 0;		break;
		case kopRetTrue:			cbArgs = 0;		break;
		case kopSetBits:
			nUnsigned = (signed short int)vb[ib++];
			nUnsigned = (nUnsigned << 8) + vb[ib++];
			nSigned = (signed int)nUnsigned;
			strmOut << " " << nSigned;
			nUnsigned = (signed short int)vb[ib++];
			nUnsigned = (nUnsigned << 8) + vb[ib++];
			nSigned = (signed int)nUnsigned;
			strmOut << " " << nSigned;
			cbArgs = 0;
			break;

		case kopFeatSet:			cbArgs = 1;		break;	// feature internal ID

		default:
			Assert(false);
			cbArgs = 0;
		}

		// This loop handles only 8-bit signed values.
		for (int iTmp = 0; iTmp < cbArgs; iTmp++)
		{
			int n = (char)vb[ib++];
			strmOut << " " << n;
		}
		strmOut << "\n";
	}
}


/*----------------------------------------------------------------------------------------------
	Return the text equivalent of the given slot attribute.
----------------------------------------------------------------------------------------------*/
std::string GdlRule::SlotAttributeDebugString(int slat)
{
	std::string sta("bad-slot-attr-");
	switch (slat)
	{
	case kslatAdvX:				return "advance_x";
	case kslatAdvY:				return "advance_y";
	case kslatAttTo:			return "attach_to";
	case kslatAttAtX:			return "attach_at_x";
	case kslatAttAtY:			return "attach_at_y";
	case kslatAttAtGpt:			return "attach_at_gpoint";
	case kslatAttAtXoff:		return "attach_at_xoffset";
	case kslatAttAtYoff:		return "attach_at_yoffset";
	case kslatAttWithX:			return "attach_with_x";
	case kslatAttWithY:			return "attach_with_y";
	case kslatAttWithGpt:		return "attach_with_gpoint";
	case kslatAttWithXoff:		return "attach_with_xoffset";
	case kslatAttWithYoff:		return "attach_with_yoffset";
	case kslatAttLevel:			return "attach_level";
	case kslatBreak:			return "break";
	case kslatCompRef:			return "comp_ref";
	case kslatDir:				return "dir";
	case kslatInsert:			return "insert";
	case kslatPosX:				return "pos_x";
	case kslatPosY:				return "pos_y";
	case kslatShiftX:			return "shift_x";
	case kslatShiftY:			return "shift_y";
	case kslatUserDefnV1:		return "user";
	case kslatUserDefn:			return "user";
	case kslatMeasureSol:		return "measure_startofline";
	case kslatMeasureEol:		return "measure_endofline";
	case kslatJ0Stretch:		return "justify_0_stretch";
	case kslatJ0Shrink:			return "justify_0_shrink";
	case kslatJ0Step:			return "justify_0_step";
	case kslatJ0Weight:			return "justify_0_weight";
	case kslatJ0Width:			return "justify_0_width";
	case kslatJ1Stretch:		return "justify_1_stretch";
	case kslatJ1Shrink:			return "justify_1_shrink";
	case kslatJ1Step:			return "justify_1_step";
	case kslatJ1Weight:			return "justify_1_weight";
	case kslatJ1Width:			return "justify_1_width";
	case kslatJ2Stretch:		return "justify_2_stretch";
	case kslatJ2Shrink:			return "justify_2_shrink";
	case kslatJ2Step:			return "justify_2_step";
	case kslatJ2Weight:			return "justify_2_weight";
	case kslatJ2Width:			return "justify_2_width";
	case kslatJ3Stretch:		return "justify_3_stretch";
	case kslatJ3Shrink:			return "justify_3_shrink";
	case kslatJ3Step:			return "justify_3_step";
	case kslatJ3Weight:			return "justify_3_weight";
	case kslatJ3Width:			return "justify_3_width";
	case kslatSegSplit:			return "segsplit";
	case kslatColFlags:			return "col_flags";
	case kslatColMargin:		return "col_margin";
	case kslatColMarginWt:		return "col_marginweight";
	case kslatColMinX:			return "col_min_x";
	case kslatColMinY:			return "col_min_y";
	case kslatColMaxX:			return "col_max_x";
	case kslatColMaxY:			return "col_max_y";
	case kslatColExclGlyph:		return "col_excl_glyph";
	case kslatColExclOffX:		return "col_excl_off_x";
	case kslatColExclOffY:		return "col_excl_off_y";
	case kslatColFixX:			return "col_fix_x";
	case kslatColFixY:			return "col_fix_y";
//	case kslatColOrderClass:	return "col_order_class";
//	case kslatColOrderEnforce:	return "col_order_enforce";
	case kslatSeqClass:			return "seq_class";
	case kslatSeqProxClass:		return "seq_proxClass";
	case kslatSeqOrder:			return "seq_order";
	case kslatSeqAboveXoff:		return "seq_above_xoff";
	case kslatSeqAboveWt:		return "seq_above_wt";
	case kslatSeqBelowXlim:		return "seq_below_xlim";
	case kslatSeqBelowWt:		return "seq_below_wt";
	case kslatSeqValignHt:		return "seq_valign_ht";
	case kslatSeqValignWt:		return "seq_valign_wt";

	default:
		Assert(false);
		char rgch[20];
		itoa(slat, rgch, 10);
		sta += rgch;
		return sta;
	}
}


/*----------------------------------------------------------------------------------------------
	Return the text equivalent of the given glyph metric.
----------------------------------------------------------------------------------------------*/
std::string GdlRule::GlyphMetricDebugString(int gmet)
{
	std::string sta("bad-glyph-metric-");
	switch (gmet)
	{
	case kgmetLsb:				return "lsb";
	case kgmetRsb:				return "rsb";
	case kgmetBbTop:			return "bb_top";
	case kgmetBbBottom:			return "bb_bottom";
	case kgmetBbLeft:			return "bb_left";
	case kgmetBbRight:			return "bb_right";
	case kgmetBbHeight:			return "bb_height";
	case kgmetBbWidth:			return "bb_width";
	case kgmetAdvWidth:			return "aw";
	case kgmetAdvHeight:		return "ah";
	case kgmetAscent:			return "ascent";
	case kgmetDescent:			return "descent";
	default:
		Assert(false);
		char rgch[20];
		itoa(gmet, rgch, 10);
		sta += rgch;
		return sta;
	}
}


/*----------------------------------------------------------------------------------------------
	Return the text equivalent of the given engine code operator.
----------------------------------------------------------------------------------------------*/
std::string GdlRule::EngineCodeDebugString(int op)
{
	std::string sta("bad-engine-op-");
	switch (op)
	{
	case kopNop:					return "Nop";
	case kopPushByte:				return "PushByte";
	case kopPushByteU:				return "PushByteU";
	case kopPushShort:				return "PushShort";
	case kopPushShortU:				return "PushShortU";
	case kopPushLong:				return "PushLong";
	case kopAdd:					return "Add";
	case kopSub:					return "Sub";
	case kopMul:					return "Mul";
	case kopDiv:					return "Div";
	case kopMin:					return "Min";
	case kopMax:					return "Max";
	case kopNeg:					return "Neg";
	case kopTrunc8:					return "Trunc8";
	case kopTrunc16:				return "Trunc16";
	case kopCond:					return "Cond";
	case kopAnd:					return "And";
	case kopOr:						return "Or";
	case kopNot:					return "Not";
	case kopBitAnd:					return "BitAnd";
	case kopBitOr:					return "BitOr";
	case kopBitNot:					return "BitNot";
	case kopEqual:					return "Equal";
	case kopNotEq:					return "NotEq";
	case kopLess:					return "Less";
	case kopGtr:					return "Gtr";
	case kopLessEq:					return "LessEq";
	case kopGtrEq:					return "GtrEq";
	case kopNext:					return "Next";
	case kopNextN:					return "NextN";
	case kopCopyNext:				return "CopyNext";
	case kopPutGlyph:				return "PutGlyph";
	case kopPutGlyphV1_2:			return "PutGlyph(V1&2)";
	case kopPutSubsV1_2:			return "PutSubs(V1&2)";
	case kopPutSubs:				return "PutSubs";
	case kopPutSubs2:				return "PutSubs2";
	case kopPutSubs3:				return "PutSubs3";
	case kopPutCopy:				return "PutCopy";
	case kopInsert:					return "Insert";
	case kopDelete:					return "Delete";
	case kopAssoc:					return "Assoc";
	case kopCntxtItem:				return "CntxtItem";
	case kopAttrSet:				return "AttrSet";
	case kopAttrAdd:				return "AttrAdd";
	case kopAttrSub:				return "AttrSub";
//	case kopAttrBitAnd:				return "AttrBitAnd";
//	case kopAttrBitOr:				return "AttrBitOr";
	case kopAttrSetSlot:			return "AttrSetSlot";
	case kopIAttrSetSlot:			return "IAttrSetSlot";
	case kopPushSlotAttr:			return "PushSlotAttr";
	case kopPushISlotAttr:			return "PushISlotAttr";
	case kopPushGlyphAttr:			return "PushGlyphAttr";
	case kopPushGlyphAttrV1_2:		return "PushGlyphAttr(V1&2)";
	case kopPushGlyphMetric:		return "PushGlyphMetric";
	case kopPushFeat:				return "PushFeat";
	case kopPushAttToGlyphAttr:		return "PushAttToGlyphAttr";
	case kopPushAttToGAttrV1_2:		return "PushAttToGlyphAttr(V1&2)";
	case kopPushAttToGlyphMetric:	return "PushAttToGlyphMetric";
	case kopPushIGlyphAttr:			return "PushIGlyphAttr";
	case kopPushVersion:			return "PushVersion";
	case kopPopRet:					return "PopRet";
	case kopRetZero:				return "RetZero";
	case kopRetTrue:				return "RetTrue";
	case kopIAttrSet:				return "IAttrSet";
	case kopIAttrAdd:				return "IAttrAdd";
	case kopIAttrSub:				return "IAttrSub";
//	case kopIAttrBitAnd:			return "IAttrBitAnd";
//	case kopIAttrBitOr:				return "IAttrBitOr";
	case kopPushProcState:			return "PushProcState";
	case kopSetBits:				return "SetBits";
	case kopFeatSet:				return "FeatSet";
	default:
		Assert(false);
		char rgch[20];
		itoa(op, rgch, 10);
		sta += rgch;
		return sta;
	}
}

/*----------------------------------------------------------------------------------------------
	Return the text equivalent of the given process-state.
----------------------------------------------------------------------------------------------*/
std::string GdlRule::ProcessStateDebugString(int pstat)
{
	std::string sta("bad-process-state-");
	switch (pstat)
	{
	case kpstatJustifyMode:		return "JustifyMode";
	case kpstatJustifyLevel:	return "JustifyLevel";
	default:
		Assert(false);
		char rgch[20];
		itoa(pstat, rgch, 10);
		sta += rgch;
		return sta;
	}
}

/*----------------------------------------------------------------------------------------------
	Output a list of glyph attributes.
----------------------------------------------------------------------------------------------*/
void GrcManager::DebugGlyphAttributes(char * pchOutputPath)
{
	std::string staOutputFilename(pchOutputPath);
	staOutputFilename.append("/dbg_glyphattrs.txt");

	std::ofstream strmOut;
	strmOut.open(staOutputFilename.data());
	if (strmOut.fail())
	{
		g_errorList.AddWarning(6503, NULL,
			"Error in writing to file ", staOutputFilename.data());
		return;
	}

	Symbol psymBw = m_psymtbl->FindSymbol("breakweight");
	int nAttrIdBw = psymBw->InternalID();

	Symbol psymSkipP = m_psymtbl->FindSymbol("*skipPasses*");
	int nAttrIdSkipP = psymSkipP->InternalID();
	int nAttrIdSkipP2 = 0;
	Symbol psymSkipP2 = m_psymtbl->FindSymbol("*skipPasses2*");
	if (psymSkipP2)
		nAttrIdSkipP2 = psymSkipP2->InternalID();
	int cpass = this->m_prndr->NumberOfPasses();

	//Symbol psymJStr = m_psymtbl->FindSymbol(GrcStructName("justify", "0", "stretch"));
	Symbol psymJStr = m_psymtbl->FindSymbol(GrcStructName("justify", "stretch"));
	int nAttrIdJStr = psymJStr->InternalID();

	if (g_errorList.AnyFatalErrors())
		strmOut << "Fatal errors--compilation aborted";
	else
	{
		strmOut << "GLYPH ATTRIBUTE IDS\n\n";
		for (size_t nAttrID = 0; nAttrID < m_vpsymGlyphAttrs.size(); nAttrID++)
		{
			if (m_vpsymGlyphAttrs[nAttrID]->InternalID() == static_cast<int>(nAttrID))
			{
				strmOut << nAttrID << ": "
					<< m_vpsymGlyphAttrs[nAttrID]->FullName() << "\n";
			}
			// else we have something like justify.stretch which is unused
		}
		strmOut << "\n\n\nGLYPH ATTRIBUTE VALUES\n\n";

		for (int wGlyphID = 0; wGlyphID < m_cwGlyphIDs; wGlyphID++)
		{
			// Convert breakweight values depending on the table version to output.
			ConvertBwForVersion(wGlyphID, nAttrIdBw);

			//	Split any large stretch values into two 16-bit words.
			SplitLargeStretchValue(wGlyphID, nAttrIdJStr);
		
			bool fAnyNonZero = false;

			for (size_t nAttrID = 0; nAttrID < m_vpsymGlyphAttrs.size(); nAttrID++)
			{
				int nValue = FinalAttrValue(wGlyphID, nAttrID);

				//	Skip undefined and zero-valued attributes.
				if (nValue == 0)
					continue;

				if (fAnyNonZero == false)
				{
					strmOut << wGlyphID << "  [";
					DebugHex(strmOut, wGlyphID);
					strmOut << "]\n";
				}

				fAnyNonZero = true;

				strmOut << "   " << m_vpsymGlyphAttrs[nAttrID]->FullName()
					<< " = ";
				if (m_vpsymGlyphAttrs[nAttrID]->LastFieldIs("gpoint") &&
					nValue == kGpointZero)
				{
					strmOut << "zero" << "\n";
				}
				else
				{
					strmOut  << nValue;
					if (nAttrID == nAttrIdSkipP || nAttrID == nAttrIdSkipP2)
					{
						int iStart = 0;
						int iStop = cpass;
						if (cpass > kPassPerSPbitmap)
							iStop = kPassPerSPbitmap;
						if (nAttrID == nAttrIdSkipP2)
						{
							iStart = kPassPerSPbitmap;
							iStop = cpass;
						}

						strmOut << " [";
						DebugHex(strmOut, nValue);
						strmOut << "  / ";
						// Print out bits in order of passes (low to high).
						int tValue = nValue;
						for (int ipass = iStart; ipass < iStop; ipass++)
						{
							int n = int((tValue & 0x0001) != 0);
							strmOut << " " << n;
							tValue = tValue >> 1;
						}
						strmOut << "]";
					}
					else if (nValue > 9 || nValue < 0)
					{
						strmOut << " [";
						DebugHex(strmOut, nValue);
						strmOut << "]";
					}
					strmOut << "\n";
				}
			}

			if (fAnyNonZero)
				strmOut << "\n\n";
		}
	}

	strmOut.close();
}


/*----------------------------------------------------------------------------------------------
	Generate a list of glyph attributes (whose indices in the vector match their internal IDs).
----------------------------------------------------------------------------------------------*/
void GrcSymbolTable::GlyphAttrList(std::vector<Symbol> & vpsym)
{
	for (SymbolTableMap::iterator it = EntriesBegin();
		it != EntriesEnd();
		++it)
	{
		Symbol psym = it->second; // GetValue();
		//Symbol psym = it.GetValue();

		if (psym->m_psymtblSubTable)
			psym->m_psymtblSubTable->GlyphAttrList(vpsym);

		else if (psym->IsGeneric() && psym->FitsSymbolType(ksymtGlyphAttr))
		{
			if (psym->InternalID() >= 0)
			{
				while (signed(vpsym.size()) <= psym->InternalID())
					vpsym.push_back(NULL);

				vpsym[psym->InternalID()] = psym;
			}
		}
	}
}


/*----------------------------------------------------------------------------------------------
	Generate a pretty-print description of the rule (similar to the original syntax).
----------------------------------------------------------------------------------------------*/
void GdlRule::RulePrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml)
{
	size_t cEndif = 0;
	for (size_t iexp = 0; iexp < m_vpexpConstraints.size(); iexp++)
	{
		strmOut << "if (";
		m_vpexpConstraints[iexp]->PrettyPrint(pcman, strmOut, fXml);
		strmOut << ") ";
		cEndif++;
	}

	//	Loop through all the items to see if we need a LHS or a context.
	bool fLhs = false;
	bool fContext = (m_nScanAdvance != -1);
	int irit;
	for (irit = 0; irit < signed(m_vprit.size()) ; irit++)
	{
		GdlRuleItem * prit = m_vprit[irit];
		GdlSubstitutionItem * pritsub = dynamic_cast<GdlSubstitutionItem *>(prit);
		if (pritsub)
			fLhs = true;

		GdlSetAttrItem * pritset = dynamic_cast<GdlSetAttrItem *>(prit);
		if (!pritset)
			fContext = true;
		else if (prit->m_pexpConstraint)
			fContext = true;
	}

	if (fLhs)
	{
		for (irit = 0; irit < signed(m_vprit.size()) ; irit++)
		{
			m_vprit[irit]->LhsPrettyPrint(pcman, this, irit, strmOut, fXml);
		}
		if (fXml)
			strmOut << "&gt;  ";
		else
			strmOut << ">  ";
	}

	for (irit = 0; irit < signed(m_vprit.size()) ; irit++)
	{
		m_vprit[irit]->RhsPrettyPrint(pcman, this, irit, strmOut, fXml);
	}

	if (fContext)
	{
		strmOut << " /  ";
		for (irit = 0; irit < signed(m_vprit.size()) ; irit++)
		{
			if (m_nScanAdvance == irit)
				strmOut << "^  ";
			m_vprit[irit]->ContextPrettyPrint(pcman, this, irit, strmOut, fXml);
		}
	}

	strmOut << ";";

	for (size_t i = 0; i < cEndif; i++)
		strmOut << " endif; ";
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleItem::LhsPrettyPrint(GrcManager * /*pcman*/, GdlRule * /*prule*/, int /*irit*/,
	std::ostream & /*strmOut*/, bool /*fXml*/)
{
	//	Do nothing.
}

void GdlSetAttrItem::LhsPrettyPrint(GrcManager * /*pcman*/, GdlRule * /*prule*/, int /*irit*/,
	std::ostream & strmOut, bool /*fXml*/)
{
	strmOut << m_psymInput->FullAbbrev();
	strmOut << "  ";
}

void GdlSubstitutionItem::LhsPrettyPrint(GrcManager * /*pcman*/, GdlRule * /*prule*/, int /*irit*/,
	std::ostream & strmOut, bool /*fXml*/)
{
	strmOut << m_psymInput->FullAbbrev();
	strmOut << "  ";
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleItem::RhsPrettyPrint(GrcManager * /*pcman*/, GdlRule * /*prule*/, int /*irit*/,
	std::ostream & /*strmOut*/, bool /*fXml*/)
{
	//	Do nothing.
}

void GdlSetAttrItem::RhsPrettyPrint(GrcManager * pcman, GdlRule * prule, int irit,
	std::ostream & strmOut, bool fXml)
{
	strmOut << m_psymInput->FullAbbrev();
	AttrSetterPrettyPrint(pcman, prule, irit, strmOut, fXml);
	strmOut << "  ";
}

void GdlSubstitutionItem::RhsPrettyPrint(GrcManager * pcman, GdlRule * prule, int irit,
	std::ostream & strmOut, bool fXml)
{
	strmOut << m_psymOutput->FullAbbrev();

	if (m_pexpSelector)
	{
		if (m_psymOutput->Data())
			strmOut << "$";
		strmOut << m_pexpSelector->SlotNumber();
	}

	if (m_vpexpAssocs.size() > 0)
	{
		strmOut << ":";
		if (m_vpexpAssocs.size() > 1)
			strmOut << "(";
		int iexp;
		for (iexp = 0; iexp < signed(m_vpexpAssocs.size()) - 1; iexp++)
			strmOut << m_vpexpAssocs[iexp]->SlotNumber() << " ";
		strmOut << m_vpexpAssocs[iexp]->SlotNumber();
		if (m_vpexpAssocs.size() > 1)
			strmOut << ")";
	}
	AttrSetterPrettyPrint(pcman, prule, irit, strmOut, fXml);
	strmOut << "  ";
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleItem::ContextPrettyPrint(GrcManager * pcman, GdlRule * /*prule*/, int /*irit*/,
	std::ostream & strmOut, bool fXml)
{
	strmOut << m_psymInput->FullAbbrev();
	ConstraintPrettyPrint(pcman, strmOut, true, fXml);
	strmOut << "  ";
}

void GdlLineBreakItem::ContextPrettyPrint(GrcManager * pcman, GdlRule * /*prule*/, int /*irit*/,
	std::ostream & strmOut, bool fXml)
{
	strmOut << "#";
	ConstraintPrettyPrint(pcman, strmOut, true, fXml);
	strmOut << "  ";
}

void GdlSetAttrItem::ContextPrettyPrint(GrcManager * pcman, GdlRule * /*prule*/, int /*irit*/,
	std::ostream & strmOut, bool fXml)
{
	strmOut << "_";
	ConstraintPrettyPrint(pcman, strmOut, true, fXml);
	strmOut << "  ";
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleItem::ConstraintPrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml,
	bool fSpace)
{
	if (m_pexpConstraint)
	{
		if (fSpace) strmOut << " ";
		strmOut << "{ ";
		m_pexpConstraint->PrettyPrint(pcman, strmOut, fXml);
		strmOut << " }";
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlSetAttrItem::AttrSetterPrettyPrint(GrcManager * pcman, GdlRule * /*prule*/, int /*irit*/,
	std::ostream & strmOut, bool fXml)
{
	if (m_vpavs.size() > 0)
	{
		bool fAtt = false;
		bool fAttAt = false;
		bool fAttWith = false;
		strmOut << " { ";

		// Do attach and collision attributes first.
		// Use embedded {} structure to take up less room.
		int ciavsAttach = 0;
		int ciavsCollision = 0;
		for (size_t iavs = 0; iavs < m_vpavs.size(); iavs++)
		{
			if (m_vpavs[iavs]->m_psymName->IsAttachment())
				ciavsAttach++;
			if (m_vpavs[iavs]->m_psymName->IsCollisionAttr())
				ciavsCollision++;
		}
		if (ciavsAttach > 0)
		{
			strmOut << " attach {";
			for (size_t iavs = 0; iavs < m_vpavs.size(); iavs++)
			{
				if (m_vpavs[iavs]->m_psymName->IsAttachment())
					m_vpavs[iavs]->PrettyPrintAttach(pcman, strmOut, fXml);
			}
			strmOut << "} ";
		}
		if (ciavsCollision > 1)
		{
			strmOut << " collision {";
			for (size_t iavs = 0; iavs < m_vpavs.size(); iavs++)
			{
				if (m_vpavs[iavs]->m_psymName->IsCollisionAttr())
				{
					strmOut << m_vpavs[iavs]->m_psymName->FullAbbrevOmit("collision");
					strmOut << " " << m_vpavs[iavs]->m_psymOperator->FullAbbrev() << " ";
					m_vpavs[iavs]->m_pexpValue->PrettyPrint(pcman, strmOut, fXml);
					strmOut << "; ";
				}
			}
			strmOut << "} ";
		}

		// Now do everything else.
		for (size_t iavs = 0; iavs < m_vpavs.size(); iavs++)
		{
			if (!m_vpavs[iavs]->m_psymName->IsAttachment()
					&& (!m_vpavs[iavs]->m_psymName->IsCollisionAttr() || ciavsCollision <= 1))
				m_vpavs[iavs]->PrettyPrint(pcman, strmOut, fXml, &fAtt, &fAttAt, &fAttWith, m_vpavs.size());
		}
		strmOut << " }";
	}
}

void GdlAttrValueSpec::PrettyPrintAttach(GrcManager * pcman, std::ostream & strmOut, bool fXml)
{
	if (m_fFlattened
		&& (m_psymName->IsAttachAtField() || m_psymName->IsAttachWithField()))
	{
		// A single statement like "attach.at = apt" has been translated into
		// "attach.at.x = apt.x, attach.at.y = apt.y, attach.at.xoffset = apt.xoffset,
		// attach.at.yoffset = apt.yoffset". Just print out one of these, say, the x.
		if (m_psymName->IsAttachXField())
		{
			if (m_psymName->IsAttachAtField())
				strmOut << "at = ";
			else
				strmOut << "with = ";
			GdlLookupExpression * pexpLookup = dynamic_cast<GdlLookupExpression *>(m_pexpValue);
			if (pexpLookup)
			{
				Symbol psym = pexpLookup->Name()->ParentSymbol();
				strmOut << psym->FullAbbrevOmit("attach");
			}
			else
				// strange...
				m_pexpValue->PrettyPrint(pcman, strmOut, fXml);
			strmOut << "; ";
		}
		return;
	}
	
	if (m_psymName->IsAttachOffsetField())
	{
		int nValue;
		if (m_pexpValue->ResolveToInteger(&nValue, false) && nValue == 0)
			// Don't bother putting out something like attach.at.xoffset = 0.
			return;
	}

	strmOut << m_psymName->FullAbbrevOmit("attach");
	strmOut << " " << m_psymOperator->FullAbbrev() << " ";
	m_pexpValue->PrettyPrint(pcman, strmOut, fXml);
	strmOut << "; ";
}

void GdlAttrValueSpec::PrettyPrint(GrcManager * pcman, std::ostream & strmOut, bool fXml,
	bool * /*pfAtt*/, bool * /*pfAttAt*/, bool * /*pfAttWith*/, int /*cpavs*/)
{
	strmOut << m_psymName->FullAbbrev();
	strmOut << " " << m_psymOperator->FullAbbrev() << " ";
	m_pexpValue->PrettyPrint(pcman, strmOut, fXml);
	strmOut << "; ";
}


/*----------------------------------------------------------------------------------------------
	Output a list of all the classes and their members.
----------------------------------------------------------------------------------------------*/
void GrcManager::DebugClasses(char * pchOutputPath)
{
	std::string staOutputFilename(pchOutputPath);
	staOutputFilename.append("/dbg_classes.txt");

	std::ofstream strmOut;
	strmOut.open(staOutputFilename.data());
	if (1 + 1 == 2)  ///// strmOut.fail())
	{
		g_errorList.AddWarning(6504, NULL,
			"Error in writing to file ", staOutputFilename.data());
		return;
	}

	if (g_errorList.AnyFatalErrors())
		strmOut << "Fatal errors--compilation aborted";
	else
	{
		m_prndr->DebugClasses(strmOut, m_vpglfcReplcmtClasses, m_cpglfcLinear);
	}

	strmOut.close();
}
		
void GdlRenderer::DebugClasses(std::ostream & strmOut,
	std::vector<GdlGlyphClassDefn *> & vpglfcReplcmt, int cpglfcLinear)
{
	strmOut << "LINEAR (OUTPUT) CLASSES";

	//	linear classes (output)
	int cTmp = 0;
	int ipglfc;
	for (ipglfc = 0; ipglfc < cpglfcLinear; ipglfc++)
	{
		GdlGlyphClassDefn * pglfc = vpglfcReplcmt[ipglfc];

		Assert(pglfc->ReplcmtOutputClass() || pglfc->GlyphIDCount() <= 1);
		//Assert(pglfc->ReplcmtOutputID() == cTmp);

		strmOut << "\n\n";
		strmOut << "Class #" << ipglfc << ": ";
		strmOut << pglfc->Name();

		std::vector<utf16> vwGlyphs;
		pglfc->GenerateOutputGlyphList(vwGlyphs);

		//	glyph list
		for (size_t iw = 0; iw < vwGlyphs.size(); iw++)
		{
			if (iw % 10 == 0)
			{
				strmOut << "\n" << iw << ":";
			}
			strmOut << "   ";
			GrcManager::DebugHex(strmOut, vwGlyphs[iw]);
		}

		cTmp++;
	}

	strmOut << "\n\n\nINDEXED (INPUT) CLASSES";

	//	indexed classes (input)
	for (ipglfc = cpglfcLinear; ipglfc < signed(vpglfcReplcmt.size()); ipglfc++)
	{
		GdlGlyphClassDefn * pglfc = vpglfcReplcmt[ipglfc];

		Assert(pglfc->ReplcmtInputClass());
		Assert(pglfc->ReplcmtInputID() == cTmp);

		strmOut << "\n\n";
		strmOut << "Class #" << ipglfc << ": ";
		strmOut << pglfc->Name();

		std::vector<utf16> vwGlyphs;
		std::vector<int> vnIndices;
		pglfc->GenerateInputGlyphList(vwGlyphs, vnIndices);
		//	glyph list
		for (size_t iw = 0; iw < vwGlyphs.size(); iw++)
		{
			if (iw % 5 == 0)
			{
				strmOut << "\n";
			}
			GrcManager::DebugHex(strmOut, vwGlyphs[iw]);
			strmOut << " :";
			if (vnIndices[iw] < 1000) strmOut << " ";
			if (vnIndices[iw] < 100)  strmOut << " ";
			if (vnIndices[iw] < 10)   strmOut << " ";
			strmOut << vnIndices[iw];
			strmOut << "    ";
		}

		cTmp++;
	}
}

/*----------------------------------------------------------------------------------------------
	Output the contents of the -cmap-, the mapping from unicode-to-glyph ID and vice-versa.
	Also include any pseudo-glyphs.
----------------------------------------------------------------------------------------------*/
void GrcManager::DebugCmap(GrcFont * pfont, char * pchOutputPath)
{
	bool fSuppPlaneChars = pfont->AnySupplementaryPlaneChars();

	std::string staOutputFilename(pchOutputPath);
	staOutputFilename.append("/dbg_cmap.txt");

	std::ofstream strmOut;
	strmOut.open(staOutputFilename.data());
	if (strmOut.fail())
	{
		g_errorList.AddWarning(6505, NULL,
			"Error in writing to file ", staOutputFilename.data());
		return;
	}

	int nFirstPseudo = 0x10000;
	for (size_t iw = 0; iw < m_vwPseudoForUnicode.size(); iw++)
		nFirstPseudo = min(nFirstPseudo, static_cast<int>(m_vwPseudoForUnicode[iw]));

	if (g_errorList.AnyFatalErrors())
	{
		strmOut << "Fatal errors--compilation aborted";
	}
	else
	{
		int cnUni = pfont->NumUnicode();
		utf16 * rgchwUniToGlyphID = new utf16[cnUni];
		unsigned int * rgnGlyphIDToUni = new unsigned int[0x10000];

		std::vector<unsigned int> vnXUniForPsd;
		std::vector<utf16> vwXPsdForUni;

		CmapAndInverse(pfont, cnUni, rgchwUniToGlyphID, rgnGlyphIDToUni,
			vnXUniForPsd, vwXPsdForUni);

		unsigned int nUni;
		utf16 wGlyphID;

		strmOut << "UNICODE => GLYPH ID MAPPINGS\n\n";

		int iXPsd = 0; // extra pseudos
		GrcFont::iterator fit(pfont);
		int iUni;
		for (iUni = 0, fit = pfont->Begin();
			fit != pfont->End();
			++fit, ++iUni)
		{
			nUni = *fit;
			wGlyphID = rgchwUniToGlyphID[iUni];
			Assert(wGlyphID != 0);

			while (iXPsd < signed(vnXUniForPsd.size()) && vnXUniForPsd[iXPsd] < nUni)
			{
				// insert extra pseudos that are not in the cmap
				WriteCmapItem(strmOut, vnXUniForPsd[iXPsd], fSuppPlaneChars, vwXPsdForUni[iXPsd], 
					true, true, false);
				iXPsd++;
			}
			WriteCmapItem(strmOut, nUni, fSuppPlaneChars, wGlyphID, true,
				wGlyphID >= nFirstPseudo, true);
		}

		// Sort the extra pseudos by glyph ID.
		for (int i1 = 0; i1 < signed(vwXPsdForUni.size()) - 1; i1++)
			for (size_t i2 = i1 + 1; i2 < vwXPsdForUni.size(); i2++)
				if (vwXPsdForUni[i1] > vwXPsdForUni[i2])
				{
					// Swap
					utf16 wTmp = vwXPsdForUni[i1];
					vwXPsdForUni[i1] = vwXPsdForUni[i2];
					vwXPsdForUni[i2] = wTmp;
					unsigned int nTmp = vnXUniForPsd[i1];
					vnXUniForPsd[i1] = vnXUniForPsd[i2];
					vnXUniForPsd[i2] = nTmp;
				}

		strmOut << "\n\n\nGLYPH ID => UNICODE MAPPINGS\n\n";

		iXPsd = 0;
		for (wGlyphID = 0; wGlyphID < 0xFFFF; wGlyphID++)
		{
			if (wGlyphID == m_wLineBreak)
			{
				strmOut << wGlyphID;
				if (fSuppPlaneChars) strmOut << "    ";
				strmOut << "                 [line-break]\n";
			}
			else if (wGlyphID == m_wPhantom)
			{
				strmOut << wGlyphID;
				if (fSuppPlaneChars) strmOut << "    ";
				strmOut << "                 [phantom]\n";
			}
			else if (iXPsd < signed(vwXPsdForUni.size()) && vwXPsdForUni[iXPsd] == wGlyphID)
			{
				// Pseudo-glyph where the Unicode value is not in the cmap.
				if (vnXUniForPsd[iXPsd] != 0)
					WriteCmapItem(strmOut, vnXUniForPsd[iXPsd], fSuppPlaneChars,
						wGlyphID, false, true, false);
				iXPsd++;
			}
			else
			{
				nUni = rgnGlyphIDToUni[wGlyphID];
				if (nUni != 0)
					WriteCmapItem(strmOut, nUni, fSuppPlaneChars,
						wGlyphID, false, wGlyphID >= nFirstPseudo, true);
			}
		}

		delete[] rgchwUniToGlyphID;
		delete[] rgnGlyphIDToUni;
	}

	strmOut.close();
}

void GrcManager::WriteCmapItem(std::ofstream & strmOut,
	unsigned int nUnicode, bool fSuppPlaneChars, utf16 wGlyphID, bool fUnicodeToGlyph,
	bool fPseudo, bool fInCmap)
{
	if (fUnicodeToGlyph)
	{
		DebugUnicode(strmOut, nUnicode, fSuppPlaneChars);
//		if (wGlyphID < 0x0100)
//			strmOut << " '" << (char)wGlyphID << "'";
//		else
//			strmOut << "    ";

		strmOut << "  => ";

		if (wGlyphID < 10)		strmOut << " ";
		if (wGlyphID < 100)		strmOut << " ";
		if (wGlyphID < 1000)	strmOut << " ";
		if (wGlyphID < 10000)	strmOut << " ";
		strmOut << wGlyphID << "  (";
		DebugHex(strmOut, wGlyphID);
		strmOut << ")";

		if (fPseudo)
		{
			if (fInCmap)
			{
				if (wGlyphID >= m_wFirstAutoPseudo)
					strmOut << "  [auto-pseudo]";
				else
					strmOut << "  [pseudo]";
			}
			else
				strmOut << "  [pseudo; not in cmap]";
		}

		strmOut << "\n";
	}
	else	// glyph to Unicode
	{
		strmOut << wGlyphID;	//DebugHex(strmOut, wGlyphID);
		if (wGlyphID < 10)		strmOut << " ";
		if (wGlyphID < 100)		strmOut << " ";
		if (wGlyphID < 1000)	strmOut << " ";
		if (wGlyphID < 10000)	strmOut << " ";
		strmOut << " =>  ";

		DebugUnicode(strmOut, nUnicode, fSuppPlaneChars);
		if (nUnicode < 0x0100)
			strmOut << "  '" << (char)nUnicode << "'";
		else
			strmOut << "     ";

		if (fPseudo)
		{
			if (fInCmap)
			{
				if (wGlyphID >= m_wFirstAutoPseudo)
					strmOut << "  [auto-pseudo]";
				else
					strmOut << "  [pseudo]";
			}
			else
				strmOut << "  [pseudo; not in cmap]";
		}

		strmOut << "\n";
	}
}


void GdlRenderer::DebugCmap(GrcFont * pfont, utf16 * rgchwUniToGlyphID, unsigned int * rgnGlyphIDToUni)
{
	for (size_t ipglfc = 0; ipglfc < m_vpglfc.size(); ipglfc++)
		m_vpglfc[ipglfc]->DebugCmap(pfont, rgchwUniToGlyphID, rgnGlyphIDToUni);
}


void GdlGlyphClassDefn::DebugCmap(GrcFont * pfont,
	utf16 * rgchwUniToGlyphID, unsigned int * rgnGlyphIDToUni)
{
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
		m_vpglfdMembers[iglfd]->DebugCmapForMember(pfont, rgchwUniToGlyphID, rgnGlyphIDToUni);
}


void GdlGlyphClassDefn::DebugCmapForMember(GrcFont * /*pfont*/,
	utf16 * /*rgchwUniToGlyphID*/, unsigned int * /*rgnGlyphIDToUni*/)
{
	//	Do nothing; this class will be handled separately at the top level.
}

void GdlGlyphDefn::DebugCmapForMember(GrcFont * pfont,
	utf16 * rgchwUniToGlyphID, unsigned int * rgnGlyphIDToUni)
{
	Assert(m_vwGlyphIDs.size() > 0);

	// TODO: check for kBadGlyph values

	//unsigned int n;
	//unsigned int nUnicode;
	//utf16 w;
	//utf16 wGlyphID;
	//utf16 wFirst; // wLast;

	switch (m_glft)
	{
	case kglftGlyphID:
		break;

	case kglftUnicode:
//		Assert(m_nFirst <= m_nLast);
//		for (n = m_nFirst; n <= m_nLast; ++n)
//		{
//			wGlyphID = pfont->GlyphFromCmap(n, this);
//			if (wGlyphID != 0)
//			{
//				rgchwUniToGlyphID[n] = wGlyphID;
//				rgnGlyphIDToUni[wGlyphID] = n;
//			}
//
//			// Just in case, since incrementing 0xFFFFFFFF will produce zero.
//			if (n == 0xFFFFFFFF)
//				break;
//		}
		
		break;

	case kglftPostscript:
		break;

	case kglftCodepoint:
//		char rgchCdPg[20];
//		itoa(m_wCodePage, rgchCdPg, 10);
//		wFirst = (utf16)m_nFirst;
//		wLast = (utf16)m_nLast;
//		if (wFirst == 0 && wLast == 0)
//		{
//			for (int ich = 0; ich < m_sta.Length(); ich++)
//			{
//				char rgchCdPt[2];
//				rgchCdPt[0] = m_sta.GetAt(ich);
//				nUnicode = pfont->UnicodeFromCodePage(m_wCodePage, m_sta[ich], this);
//				Assert(nUnicode != 0);
//				if ((wGlyphID = g_cman.PseudoForUnicode(nUnicode)) == 0)
//					wGlyphID = pfont->GlyphFromCmap(nUnicode, this);
//				Assert(wGlyphID != 0);
//				rgchwUniToGlyphID[nUnicode] = wGlyphID;
//				rgnGlyphIDToUni[wGlyphID] = nUnicode;
//			}
//		}
//		else
//		{
//			Assert(wFirst <= wLast);
//			for (w = wFirst; w <= wLast; w++)
//			{
//				nUnicode = pfont->UnicodeFromCodePage(m_wCodePage, w, this);
//				Assert(nUnicode != 0);
//				if ((wGlyphID = g_cman.PseudoForUnicode(nUnicode)) == 0)
//					wGlyphID = pfont->GlyphFromCmap(nUnicode, this);
//				Assert(wGlyphID != 0);
//				rgchwUniToGlyphID[nUnicode] = wGlyphID;
//				rgnGlyphIDToUni[wGlyphID] = nUnicode;
//
//				// Just in case, since incrementing 0xFFFF will produce zero.
//				if (w == 0xFFFF)
//					break;
//			}
//		}
		break;

	case kglftPseudo:
		Assert(m_nFirst == 0);
		Assert(m_nLast == 0);
		Assert(m_pglfOutput);
		//	While we're at it, handle the output glyph ID.
		m_pglfOutput->DebugCmapForMember(pfont, rgchwUniToGlyphID, rgnGlyphIDToUni);

		if (m_nUnicodeInput != 0)
		{
			//	It is the assigned glyph ID which is the 'contents' of this glyph defn.
//			rgchwUniToGlyphID[m_nUnicodeInput - pfont->MinUnicode()] = m_wPseudo;
			//rgnGlyphIDToUni[m_wPseudo] = m_nUnicodeInput;
		}
		break;
		
	default:
		Assert(false);
	}
}

/*----------------------------------------------------------------------------------------------
	Generate the cmap and inverse cmap. Used by the debugger output routines.
----------------------------------------------------------------------------------------------*/
void GrcManager::CmapAndInverse(GrcFont * pfont, 
	int cnUni, utf16 * rgchwUniToGlyphID, unsigned int * rgnGlyphIDToUni,
	std::vector<unsigned int> & vnXUniForPsd, std::vector<utf16> & vwXPsdForUni)
{
	memset(rgchwUniToGlyphID, 0, (cnUni * isizeof(utf16)));
	memset(rgnGlyphIDToUni, 0, (0x10000 * isizeof(int)));

	pfont->GetGlyphsFromCmap(rgchwUniToGlyphID);

	// Generate the inverse cmap. Also overwrite the glyph IDs for any pseudos.
	int iUni;
	GrcFont::iterator fit(pfont);
	int iUniPsd = 0;
	for (iUni = 0, fit = pfont->Begin();
		fit != pfont->End();
		++fit, ++iUni)
	{
		unsigned int nUni = *fit;

		// Handle pseudos.
		while (iUniPsd < signed(m_vnUnicodeForPseudo.size()) && nUni > m_vnUnicodeForPseudo[iUniPsd])
		{
			// Put any Unicode -> pseudo mappings where the Unicode is not in the cmap into 
			// a separate list.
			vnXUniForPsd.push_back(m_vnUnicodeForPseudo[iUniPsd]);
			vwXPsdForUni.push_back(m_vwPseudoForUnicode[iUniPsd]);
			iUniPsd++;
		}
		if (iUniPsd < signed(m_vnUnicodeForPseudo.size()) && m_vnUnicodeForPseudo[iUniPsd] == nUni)
		{
			// Pseudo: overwrite glyph ID.
			rgchwUniToGlyphID[iUni] = m_vwPseudoForUnicode[iUniPsd];
			iUniPsd++;
		}
		utf16 wGlyph = rgchwUniToGlyphID[iUni];
		rgnGlyphIDToUni[wGlyph] = nUni;
	}
	Assert(iUni == cnUni);
}

/*----------------------------------------------------------------------------------------------
	Output XML to be used by the engine debugger.
----------------------------------------------------------------------------------------------*/
bool GrcManager::DebugXml(GrcFont * pfont, char * pchOutputFilename, bool fAbsGdlFilePaths)
{
	// Current working directory, for calculating file paths in GDX file:
	char rgchCurWkDir[128];
	char * pchBogus = getcwd(rgchCurWkDir, 128); // Linux requires assignment

	// Calculate the name of the debugger-xml file. It is the name of the font file, but with
	// a .gdx extension.
	int cchLen = strlen(pchOutputFilename);
	char * pchOut = pchOutputFilename;
	char rgchDbgXmlFile[128];
	char rgchOutputPath[128];
	memset(rgchDbgXmlFile, 0, 128 * sizeof(char));
	memset(rgchOutputPath, 0, 128 * sizeof(char));
	memcpy(rgchDbgXmlFile, pchOutputFilename, cchLen * sizeof(char));
	char * pchXml = rgchDbgXmlFile + cchLen;
	while (pchXml != rgchDbgXmlFile && *pchXml != '.')	// remove the extension
		*pchXml-- = 0;
	*pchXml = 0;	// '.'
	memcpy(rgchOutputPath, rgchDbgXmlFile, cchLen * sizeof(char));

	// Remove the rest of the filename from the path.
	char * pchPath = rgchOutputPath + (pchXml - rgchDbgXmlFile);
	while (pchPath != rgchOutputPath && *pchPath != '/' && *pchPath != '\\')
		*pchPath-- = 0;
	*pchPath = 0;

	/*****
	// Output to the current directory:
	while (*pchOut != 0) {
		if (*pchOut == '/' || *pchOut == '\\') {
			pchXml = rgchDbgXmlFile;
			pchOut++;
		} else
			*pchXml++ = *pchOut++;
	}
	pchXml--;
	while (*pchXml != '.')
		pchXml--;
	*****/

	*pchXml++ = '.';
	*pchXml++ = 'g';
	*pchXml++ = 'd';
	*pchXml++ = 'x';
	*pchXml = 0;

	std::string staPathToCur;
	if (fAbsGdlFilePaths)
		staPathToCur.assign("");
	else
		// Generate the path from the GDX file to the current working directory.
		// This must be prepended on to the source code file names in the GDX file.
		staPathToCur = pathFromOutputToCurrent(rgchCurWkDir, rgchOutputPath);

	std::ofstream strmOut;
	strmOut.open(rgchDbgXmlFile);
	if (strmOut.fail())
	{
		g_errorList.AddWarning(6501, NULL,
			"Error in writing to file ", rgchDbgXmlFile);
		return false;
	}

	if (g_errorList.AnyFatalErrors())
		strmOut << "Fatal errors--compilation aborted";
	else
	{
		strmOut << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			<< "<graphite>\n\n";

		// Glyphs and glyph attributes
		this->DebugXmlGlyphs(pfont, strmOut, staPathToCur);

		// Classes and members
		this->m_prndr->DebugXmlClasses(strmOut, staPathToCur);

		// Features
		m_prndr->DebugXmlFeatures(strmOut, staPathToCur);
		
		// Rules
		m_prndr->DebugXmlRules(this, strmOut, staPathToCur);

		strmOut << "</graphite>\n";
	}

	strmOut.close();
	return true;
}

/*--------------------------------------------------------------------------------------------*/
void GrcManager::DebugXmlGlyphs(GrcFont * pfont, std::ofstream & strmOut,
	std::string staPathToCur)
{
	// Glyph attribute definitions

	strmOut << "  <glyphAttrs>\n";

	Symbol psymActual = m_psymtbl->FindSymbol("*actualForPseudo*");
	unsigned int nAttrIdActual = psymActual->InternalID();
	Symbol psymBw = m_psymtbl->FindSymbol("breakweight");
	unsigned int nAttrIdBw = psymBw->InternalID();
	Symbol psymDir = m_psymtbl->FindSymbol("directionality");
	unsigned int nAttrIdDir = psymDir->InternalID();
	Symbol psymSkipP = NULL;
	unsigned int nAttrIdSkipP = 0;
	Symbol psymSkipP2 = NULL;
	unsigned int nAttrIdSkipP2 = 0;
	if (this->IncludePassOptimizations())
	{
		psymSkipP = m_psymtbl->FindSymbol("*skipPasses*");
		nAttrIdSkipP = psymSkipP->InternalID();
		psymSkipP2 = m_psymtbl->FindSymbol("*skipPasses2*");
		if (psymSkipP2)
			nAttrIdSkipP2 = psymSkipP2->InternalID();
	}

	//Symbol psymJStr = m_psymtbl->FindSymbol(GrcStructName("justify", "0", "stretch"));
	Symbol psymJStr = m_psymtbl->FindSymbol(GrcStructName("justify", "stretch"));
	int nAttrIdJStr = psymJStr->InternalID();

	for (size_t nAttrID = 0; nAttrID < m_vpsymGlyphAttrs.size(); nAttrID++)
	{
		Symbol psymGlyphAttr = m_vpsymGlyphAttrs[nAttrID];

		//if ((psymGlyphAttr->IsMirrorAttr() || nAttrID == nAttrIdDir)
		//		&& !m_prndr->Bidi())
		//	continue;

		if (psymGlyphAttr->InternalID() == static_cast<int>(nAttrID))
		{
			std::string staExpType = ExpressionDebugString(psymGlyphAttr->ExpType());
			if (nAttrID == nAttrIdBw)
				staExpType = "bw";
			else if (nAttrID == nAttrIdDir)
				staExpType = "dircode";
			else if (nAttrID == nAttrIdActual)
				staExpType = "gid";
			else if (nAttrID == nAttrIdSkipP || nAttrID == nAttrIdSkipP2)
				staExpType = "bitmap";
			else if (psymGlyphAttr->IsPointField())
				staExpType = "point";
			else if (psymGlyphAttr->IsComponentBoxField())
				staExpType = "comp";

			strmOut << "    <glyphAttr name=\"" << psymGlyphAttr->FullName() 
				<< "\" attrId=\"" << nAttrID 
				<< "\" type=\"" << staExpType
				<< "\" />\n";
		}
		// else we have something like justify.stretch which is unused
	}

	strmOut << "  </glyphAttrs>\n\n";

	// Glyphs and their attribute values

	// Generate the cmap.
	int cnUni = pfont->NumUnicode();
	utf16 * rgchwUniToGlyphID = new utf16[cnUni];
	unsigned int * rgnGlyphIDToUni = new unsigned int[0x10000];
	std::vector<unsigned int> vnXUniForPsd;
	std::vector<utf16> vwXPsdForUni;
	CmapAndInverse(pfont, cnUni, rgchwUniToGlyphID, rgnGlyphIDToUni,
		vnXUniForPsd, vwXPsdForUni);

	strmOut << "  <glyphs>\n";

	std::vector<std::string> vstaSingleMemberClasses;
	std::vector<std::string> vstaSingleMemberClassFiles;
	std::vector<int> vnSingleMemberClassLines;
	m_prndr->RecordSingleMemberClasses(vstaSingleMemberClasses,
		vstaSingleMemberClassFiles, vnSingleMemberClassLines, staPathToCur);

	int fxdGlatVersion = TableVersion(ktiGlat);

	for (int wGlyphID = 0; wGlyphID < m_cwGlyphIDs; wGlyphID++)
	{
		// Convert breakweight values depending on the table version to output.
		////ConvertBwForVersion(wGlyphID, nAttrIdBw);

		//	Split any large stretch values into two 16-bit words.
		////SplitLargeStretchValue(wGlyphID, nAttrIdJStr);

		strmOut << "    <glyph glyphid=\"" << wGlyphID;
		if (vstaSingleMemberClasses.size() > (unsigned)wGlyphID && vstaSingleMemberClasses[wGlyphID] != "")
			strmOut << "\" className=\"" << vstaSingleMemberClasses[wGlyphID];
		if (rgnGlyphIDToUni[wGlyphID] != 0)
		{
			strmOut << "\" usv=\""; 
			DebugUnicode(strmOut, rgnGlyphIDToUni[wGlyphID], false);
		}
		if (vstaSingleMemberClassFiles.size() > (unsigned)wGlyphID && vstaSingleMemberClassFiles[wGlyphID] != "")
			strmOut << "\" inFile=\"" << vstaSingleMemberClassFiles[wGlyphID]
				<< "\" atLine=\"" << vnSingleMemberClassLines[wGlyphID];
		strmOut<< "\"" << ">\n";
	
		for (size_t nAttrID = 0; nAttrID < m_vpsymGlyphAttrs.size(); nAttrID++)
		{
			int nValue = FinalAttrValue(wGlyphID, nAttrID);

			if (nAttrID == nAttrIdActual && nValue == 0)
				continue;

			if ((m_vpsymGlyphAttrs[nAttrID]->IsMirrorAttr() || nAttrID == nAttrIdDir)
					&& m_prndr->Bidi() == 0)
				// Ignore mirror and directionality attribute for non-bidi.
				continue;

			// Get the original expression where this attribute was set.
			GdlExpression * pexp;
			int nPR;
			int munitPR;
			bool fOverride, fShadow;
			GrpLineAndFile lnf;
			m_pgax->Get(wGlyphID, nAttrID,
				&pexp, &nPR, &munitPR, &fOverride, &fShadow, &lnf);

			if (m_vpsymGlyphAttrs[nAttrID]->IsUserDefined() && !m_pgax->Defined(wGlyphID, nAttrID))
				// Attribute not defined for this glyph.
				continue;

			if (! m_prndr->HasCollisionPass() && m_vpsymGlyphAttrs[nAttrID]->IsCollisionAttr())
				// Ignore collision attributes when there is no collision pass.
				continue;

			strmOut << "      <glyphAttrValue name=\"" << m_vpsymGlyphAttrs[nAttrID]->FullName()
				<< "\" value=\"";
			if (m_vpsymGlyphAttrs[nAttrID]->LastFieldIs("gpoint") && nValue == kGpointZero)
				strmOut << "zero";
			else
				strmOut  << nValue;

			if (!lnf.NotSet())
				strmOut << "\" inFile=\"" << lnf.FileWithPath(staPathToCur)
					<< "\" atLine=\"" << lnf.OriginalLine();

			strmOut << "\" />\n";
		}

		if (fxdGlatVersion >= 0x00030000)
		{
			// Output glyph-approximation octaboxes.
			if (wGlyphID < m_wGlyphIDLim && wGlyphID < (int)m_vgbdy.size())
				m_vgbdy[wGlyphID].DebugXml(strmOut);
		}

		strmOut << "    </glyph>\n";
	}

	strmOut << "  </glyphs>\n\n";
}

/*--------------------------------------------------------------------------------------------*/
void GdlRenderer::RecordSingleMemberClasses(std::vector<std::string> & vstaSingleMemberClasses,
	std::vector<std::string> & vstaFiles, std::vector<int> & vnLines, std::string staPathToCur)
{
	for (size_t ipglfc = 0; ipglfc < m_vpglfc.size(); ipglfc++)
	{
		m_vpglfc[ipglfc]->RecordSingleMemberClasses(vstaSingleMemberClasses, vstaFiles, vnLines,
			staPathToCur);
	}
}

void GdlGlyphClassDefn::RecordSingleMemberClasses(std::vector<std::string> & vstaSingleMemberClasses,
	std::vector<std::string> & vstaFiles, std::vector<int> & vnLines, std::string staPathToCur)
{
	FlattenMyGlyphList();
	if (m_vgidFlattened.size() == 1)
	{
		utf16 gid = m_vgidFlattened[0];
		while (vstaSingleMemberClasses.size() <= gid)
		{
			vstaSingleMemberClasses.push_back("");
			vstaFiles.push_back("");
			vnLines.push_back(-1);
		}
		if (vstaSingleMemberClasses[gid] == "")
		{
			vstaSingleMemberClasses[gid] = this->Name();
			if (!m_lnf.NotSet())
			{
				vstaFiles[gid] = m_lnf.FileWithPath(staPathToCur);
				vnLines[gid] = m_lnf.OriginalLine();
			}
		}
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRenderer::DebugXmlClasses(std::ofstream & strmOut, std::string staPathToCur)		
{
	strmOut << "  <classes>\n";

	int cwGlyphIDs;
	for (size_t ipglfc = 0; ipglfc < m_vpglfc.size(); ipglfc++)
	{
		if (m_vpglfc[ipglfc]->Name() == "ANY")
			continue;

		cwGlyphIDs = 0;
		m_vpglfc[ipglfc]->DebugXmlClasses(strmOut, cwGlyphIDs, staPathToCur);
	}

	strmOut << "  </classes>\n\n";
}


void GdlGlyphClassDefn::DebugXmlClasses(std::ofstream & strmOut, int & cwGlyphIDs,
	std::string staPathToCur)
{
	strmOut << "    <class name=\"" << this->Name();
	if (m_fReplcmtIn)
		strmOut << "\" subClassIndexLhs=\"" << m_nReplcmtInID;
	if (m_fReplcmtOut)
		strmOut << "\" subClassIndexRhs=\"" << m_nReplcmtOutID;
	strmOut << "\">\n";

	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		m_vpglfdMembers[iglfd]->DebugXmlClassMembers(strmOut, staPathToCur,
			this, LineAndFileForMember(iglfd), cwGlyphIDs);
	}
	strmOut << "    </class>\n";
}


void GdlGlyphClassDefn::DebugXmlClassMembers(std::ofstream & strmOut, std::string staPathToCur,
	GdlGlyphClassDefn * pglfdParent, GrpLineAndFile lnf, int & cwGlyphIDs)
{
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		m_vpglfdMembers[iglfd]->DebugXmlClassMembers(strmOut, staPathToCur, pglfdParent, lnf,
			cwGlyphIDs);
	}
}

void GdlGlyphDefn::DebugXmlClassMembers(std::ofstream & strmOut, std::string staPathToCur,
	GdlGlyphClassDefn * /*pglfdParent*/, GrpLineAndFile lnf, int & cwGlyphIDs)
{
	for (unsigned int iw = 0; iw < m_vwGlyphIDs.size(); iw++)
	{
		strmOut << "      <member glyphid=\"" << m_vwGlyphIDs[iw] << "\" index=\"" << cwGlyphIDs;

		if (!lnf.NotSet())
			strmOut << "\" inFile=\"" << lnf.FileWithPath(staPathToCur)
				<< "\" atLine=\"" << lnf.OriginalLine();

		strmOut << "\" />\n";
		cwGlyphIDs++;
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRenderer::DebugXmlFeatures(std::ofstream & strmOut, std::string staPathToCur)
{
	strmOut << "  <features>\n";

	for (size_t ipfeat = 0; ipfeat < m_vpfeat.size(); ipfeat++)
	{
		GdlFeatureDefn * pfeat = m_vpfeat[ipfeat];
		pfeat->DebugXmlFeatures(strmOut, staPathToCur);
	}

	strmOut << "  </features>\n\n";
}

void GdlFeatureDefn::DebugXmlFeatures(std::ofstream & strmOut, std::string staPathToCur)
{
	GrpLineAndFile lnf = this->LineAndFile();
	unsigned int nID = this->ID();
	strmOut << "    <feature name=\"" << this->Name()
		<< "\" featureID=\"" << nID;
	if (nID > 0xFFFFFF)
	{
		// Output string format as well.

		char rgch[5];
		rgch[4] = 0;
		rgch[3] = (char)(nID & 0x000000FF);
		rgch[2] = (char)((nID & 0x0000FF00) >> 8);
		rgch[1] = (char)((nID & 0x00FF0000) >> 16);
		rgch[0] = (char)((nID & 0xFF000000) >> 24);
		std::string staT(rgch);

		// Why doesn't this work??
		//union {
		//	char rgch[4];
		//	unsigned int n;
		//} featid;
		//featid.n = nID;
		//// Reverse them.
		//char chTmp = featid.rgch[0];
		//featid.rgch[0] = featid.rgch[4];
		//featid.rgch[4] = chTmp;
		//chTmp = featid.rgch[2];
		//featid.rgch[2] = featid.rgch[3];
		//featid.rgch[3] = chTmp;
		//std::string staT(featid.rgch);
		
		strmOut << "\" featureIDstring=\"" << staT;
	}
	strmOut << "\" index=\"" << this->InternalID();

	if (!lnf.NotSet() && this->ID() != kfidStdLang)
		strmOut << "\" inFile=\"" << lnf.FileWithPath(staPathToCur)
			<< "\" atLine=\"" << lnf.OriginalLine();

	strmOut << "\" >\n";

	for (unsigned int ifset = 0; ifset < m_vpfset.size(); ifset++)
	{
		m_vpfset[ifset]->DebugXmlFeatures(strmOut, staPathToCur);
	}

	strmOut << "    </feature>\n";
}

void GdlFeatureSetting::DebugXmlFeatures(std::ofstream & strmOut, std::string staPathToCur)
{
	GrpLineAndFile lnf = this->LineAndFile();
	strmOut << "      <featureSetting name=\"" << this->Name()
		<< "\" value=\"" << this->Value();

	if (!lnf.NotSet())
		strmOut << "\" inFile=\"" << lnf.FileWithPath(staPathToCur)
			<< "\" atLine=\"" << lnf.OriginalLine();

	strmOut << "\" />\n";
}


/*--------------------------------------------------------------------------------------------*/
void GdlRenderer::DebugXmlRules(GrcManager * pcman, std::ofstream & strmOut,
	std::string staPathToCur)
{
	strmOut << "  <rules>\n";

	GdlRuleTable * prultbl;

	if ((prultbl = FindRuleTable("linebreak")) != NULL)
		prultbl->DebugXmlRules(pcman, strmOut, staPathToCur);

	if ((prultbl = FindRuleTable("substitution")) != NULL)
		prultbl->DebugXmlRules(pcman, strmOut, staPathToCur);

	if (RawBidi() == kFullPass)
		strmOut << "    <pass table=\"bidi\" index=\"" << m_ipassBidi + 1 << "\" />\n";

	if ((prultbl = FindRuleTable("justification")) != NULL)
		prultbl->DebugXmlRules(pcman, strmOut, staPathToCur);

	if ((prultbl = FindRuleTable("positioning")) != NULL)
		prultbl->DebugXmlRules(pcman, strmOut, staPathToCur);

	strmOut << "  </rules>\n\n";
}

void GdlRuleTable::DebugXmlRules(GrcManager * pcman, std::ofstream & strmOut,
	std::string staPathToCur)
{
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		m_vppass[ippass]->DebugXmlRules(pcman, strmOut, staPathToCur, this->NameSymbol());
	}
}

void GdlPass::DebugXmlRules(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur,
	Symbol psymTableName)
{
	if (!ValidPass())
		return;

	int temp = PassDebuggerNumber();
	Assert(PassDebuggerNumber() != 0);

	strmOut << "    <pass table=\"" << psymTableName->FullName()
		<< "\" index=\"" << PassDebuggerNumber() << "\"";
	if (m_nCollisionFix != 0)
		strmOut << " collisionFix=\"" << m_nCollisionFix << "\"";
	if (m_nAutoKern != 0)
		strmOut << " autoKern=\"" << m_nAutoKern << "\"";
	if (m_fFlipDir != 0)
		strmOut << " flipDir=\"" << int(m_fFlipDir) << "\"";
	strmOut << " >\n";

	if (m_vprule.size() > 0)
	{
		if (m_vpexpConstraints.size() > 0)
		{
			strmOut << "        <passConstraints>\n";
			for (size_t iexp = 0; iexp < m_vpexpConstraints.size(); iexp++)
			{
				GrpLineAndFile lnf = m_vpexpConstraints[iexp]->LineAndFile();
				strmOut << "          <passConstraint gdl=\"{ ";
				m_vpexpConstraints[iexp]->PrettyPrint(pcman, strmOut, true);
				strmOut << " }\" inFile=\"" << lnf.FileWithPath(staPathToCur)
					<< "\" atLine=\"" << lnf.OriginalLine() << "\" />\n";
			}
			strmOut << "        </passConstraints>\n";
		}

		for (size_t irule = 0; irule < m_vprule.size(); irule++)
		{
			m_vprule[irule]->DebugXml(pcman, strmOut, staPathToCur, PassDebuggerNumber(), irule);
		}
	}

	strmOut << "    </pass>  <!-- pass " << PassDebuggerNumber() 
				<< " (" << psymTableName->FullName() << " table) -->\n\n";
}

void GdlRule::DebugXml(GrcManager * pcman, std::ofstream & strmOut, std::string staPathToCur,
	int nPassNum, int nRuleNum)
{
	strmOut << "      <rule id=\"" << nPassNum << "." << nRuleNum
		<< "\" inFile=\"" << LineAndFile().FileWithPath(staPathToCur)
		<< "\" atLine=\"" << LineAndFile().OriginalLine()
		<< "\" preAnys=\"" << m_critPrependedAnys
		<< "\"\n            prettyPrint=\"";
	this->RulePrettyPrint(pcman, strmOut, true);
	strmOut << "\" >\n";

	if (m_vpexpConstraints.size() > 0)
	{
		strmOut << "        <ruleConstraints>\n";
		for (size_t iexp = 0; iexp < m_vpexpConstraints.size(); iexp++)
		{
			GrpLineAndFile lnf = m_vpexpConstraints[iexp]->LineAndFile();
			strmOut << "          <ruleConstraint gdl=\"{ ";
			m_vpexpConstraints[iexp]->PrettyPrint(pcman, strmOut, true);
			strmOut << " }\" inFile=\"" << lnf.FileWithPath(staPathToCur)
				<< "\" atLine=\"" << lnf.OriginalLine() << "\" />\n";
		}
		strmOut << "        </ruleConstraints>\n";
	}

	//	Loop through all the items to see if we need a LHS or a context.
	bool fLhs = false;
	bool fContext = (m_nScanAdvance != -1);
	int irit;
	for (irit = 0; irit < signed(m_vprit.size()) ; irit++)
	{
		GdlRuleItem * prit = m_vprit[irit];
		GdlSubstitutionItem * pritsub = dynamic_cast<GdlSubstitutionItem *>(prit);
		if (pritsub)
			fLhs = true;

		GdlSetAttrItem * pritset = dynamic_cast<GdlSetAttrItem *>(prit);
		if (!pritset)
			fContext = true;
		else if (prit->m_pexpConstraint)
			fContext = true;
	}

	// LHS
	if (fLhs)
	{
		strmOut << "        <lhs>\n";
		for (size_t irit = 0; irit < m_vprit.size(); irit++)
		{
			m_vprit[irit]->DebugXmlLhs(pcman, strmOut, staPathToCur);
		}
		strmOut << "        </lhs>\n";
	}

	// RHS
	strmOut << "        <rhs>\n";
	for (size_t irit = 0; irit < m_vprit.size(); irit++)
	{
		m_vprit[irit]->DebugXmlRhs(pcman, strmOut, staPathToCur);
	}
	strmOut << "        </rhs>\n";

	// Context
	if (fContext)
	{
		strmOut << "        <context>\n";
		int iritRhs = 0;
		for (size_t irit = 0; irit < m_vprit.size(); irit++)
		{
			if (m_nScanAdvance == (signed)irit)
				strmOut << "          <caret />\n";

			m_vprit[irit]->DebugXmlContext(pcman, strmOut, staPathToCur, iritRhs);
		}
		strmOut << "        </context>\n";
	}

	strmOut << "      </rule>\n";
}


void GdlRuleItem::DebugXmlLhs(GrcManager * /*pcman*/, std::ofstream & /*strmOut*/,
	std::string /*staPathToCur*/)
{
	//	Do nothing.
}

void GdlSetAttrItem::DebugXmlLhs(GrcManager * /*pcman*/, std::ofstream & strmOut,
	std::string /*staPathToCur*/)
{
	strmOut << "          <lhsSlot className=\"" << m_psymInput->FullAbbrev()
		<< "\" slotIndex=\"" << m_iritContextPos + 1 << "\" />\n";
}

void GdlSubstitutionItem::DebugXmlLhs(GrcManager * /*pcman*/, std::ofstream & strmOut,
	std::string /*staPathToCur*/)
{
	strmOut << "          <lhsSlot className=\"" << m_psymInput->FullAbbrev()
		<< "\" slotIndex=\"" << m_iritContextPos + 1 << "\" />\n";
}

void GdlRuleItem::DebugXmlRhs(GrcManager * /*pcman*/, std::ofstream & /*strmOut*/,
	std::string /*staPathToCur*/)
{
	//	Do nothing.
}

void GdlSetAttrItem::DebugXmlRhs(GrcManager * pcman, std::ofstream & strmOut,
	std::string staPathToCur)
{
	strmOut << "          <rhsSlot className=\"" << m_psymInput->FullAbbrev();
	if (m_vpavs.size() > 0)
	{
		strmOut << "\" assignmentGdl=\"";
		AttrSetterPrettyPrint(pcman, NULL, 0, strmOut, true);	// NULL and 0 are bogus but not used
	}

	strmOut << "\" slotIndex=\"" << m_iritContextPos + 1 << "\"";

	bool fNeedSlotAttrs = false;
	for (unsigned ipavs = 0; ipavs < m_vpavs.size(); ipavs++)
	{
		// For now, we only need attachment slot attributes.
		if (m_vpavs[ipavs]->m_psymName->IsAttachment())
		{
			fNeedSlotAttrs = true;
			break;
		}
	}
	if (fNeedSlotAttrs)
	{
		strmOut << ">\n            <slotAttrs";
		for (unsigned ipavs = 0; ipavs < m_vpavs.size(); ipavs++)
		{
			m_vpavs[ipavs]->DebugXml(pcman, strmOut, staPathToCur);
		}
		strmOut << " />\n          </rhsSlot>\n";
	}
	else
	{
		strmOut << "/>\n";
	}
}

void GdlSubstitutionItem::DebugXmlRhs(GrcManager * pcman, std::ofstream & strmOut,
	std::string /*staPathToCur*/)
{
	strmOut << "          <rhsSlot className=\"" << m_psymOutput->FullAbbrev();
	
	if (m_pexpSelector)
		strmOut << "\" selectorIndex=\"" << m_pexpSelector->SlotNumber();

	if (m_vpexpAssocs.size() > 0)
	{
		strmOut << "\" associations=\"";
		int iexp;
		for (iexp = 0; iexp < signed(m_vpexpAssocs.size()) - 1; iexp++)
			strmOut << m_vpexpAssocs[iexp]->SlotNumber() << " ";
		strmOut << m_vpexpAssocs[iexp]->SlotNumber();
	}

	if (m_vpavs.size() > 0)
	{
		strmOut << "\" assignmentGdl=\"";
		AttrSetterPrettyPrint(pcman, NULL, 0, strmOut, true);	// NULL and 0 are bogus but not used
	}
	strmOut << "\" slotIndex=\"" << m_iritContextPos + 1 << "\" />\n";

}

void GdlRuleItem::DebugXmlContext(GrcManager * pcman, std::ofstream & strmOut,
	std::string /*staPathToCur*/, int & /*iritRhs*/)
{
	strmOut << "          <contextSlot type=\"class\" className=\"" << m_psymInput->FullAbbrev() 
		<< "\"";
	if (this->m_pexpConstraint)
	{
		strmOut << " >\n            <constraint gdl=\"";
		ConstraintPrettyPrint(pcman, strmOut, true);
		strmOut << "\" />\n          </contextSlot>\n";
	}
	else
		strmOut << " />\n";
}

void GdlSetAttrItem::DebugXmlContext(GrcManager * pcman, std::ofstream & strmOut,
	std::string /*staPathToCur*/, int & iritRhs)
{
	iritRhs++;

	strmOut << "          <contextSlot type=\"place-holder\""
		<< " rhsIndex=\"" << iritRhs << "\"";
	if (this->m_pexpConstraint)
	{
		strmOut << " >\n            <constraint gdl=\"";
		ConstraintPrettyPrint(pcman, strmOut, true);
		strmOut << "\" />\n          </contextSlot>\n";
	}
	else
		strmOut << " />\n";
}

void GdlRuleItem::DebugXmlConstraint(GrcManager * pcman, std::ofstream & strmOut,
	std::string /*staPathToCur*/)
{
	if (m_pexpConstraint)
	{
		strmOut << "        <constraint gdl=\"";
		m_pexpConstraint->PrettyPrint(pcman, strmOut, true);
		strmOut << "\" />\n";
	}
}

void GdlLineBreakItem::DebugXmlConstraint(GrcManager * pcman, std::ofstream & strmOut,
	std::string /*staPathToCur*/)
{
	strmOut << "          <contextSlot className=\"#\"";
	ConstraintPrettyPrint(pcman, strmOut, true);
	strmOut << "\" />\n";

}

void GdlAttrValueSpec::DebugXml(GrcManager * pcman, std::ostream & strmOut, std::string /*staPathToCur*/)
{
	// For now, only output the attach attributes.
	if (m_psymName->IsAttachment())
	{
		if (m_fFlattened
			&& (m_psymName->IsAttachAtField() || m_psymName->IsAttachWithField()))
		{
			// A single statement like "attach.at = apt" has been translated into
			// "attach.at.x = apt.x, attach.at.y = apt.y, attach.at.xoffset = apt.xoffset,
			// attach.at.yoffset = apt.yoffset". Just print out one of these, say, the x.
			if (m_psymName->IsAttachXField())
			{
				if (m_psymName->IsAttachAtField())
					strmOut << " attachAt=\"";
				else
					strmOut << " attachWith=\"";
				GdlLookupExpression * pexpLookupValue = dynamic_cast<GdlLookupExpression *>(m_pexpValue);
				if (pexpLookupValue)
				{
					Symbol psym = pexpLookupValue->Name()->ParentSymbol();	// glyph attr
					strmOut << psym->LastField();
				}
				else
					// strange...
					m_pexpValue->PrettyPrint(pcman, strmOut, true);

				strmOut << "\"";
			}
		}
		else if (m_psymName->IsAttachTo())
		{
			strmOut << " attachTo=\"";
			GdlSlotRefExpression * pexpSlotValue = dynamic_cast<GdlSlotRefExpression *>(m_pexpValue);
			if (pexpSlotValue)
				strmOut << pexpSlotValue->AdjustedIndex() + 1;
			else
				// strange...
				m_pexpValue->PrettyPrint(pcman, strmOut, true);

			strmOut << "\"";
		}
	}
}

/*----------------------------------------------------------------------------------------------
	Output a number in hex format.
----------------------------------------------------------------------------------------------*/
void GrcManager::DebugHex(std::ostream & strmOut, utf16 wGlyphID)
{
	char rgch[20];
	itoa(wGlyphID, rgch, 16);
	strmOut << "0x";
	if (wGlyphID <= 0x0fff) strmOut << "0";
	if (wGlyphID <= 0x00ff) strmOut << "0";
	if (wGlyphID <= 0x000f) strmOut << "0";
	strmOut << rgch;
}

/*----------------------------------------------------------------------------------------------
	Output a Unicode codepoint in hex format.
----------------------------------------------------------------------------------------------*/
void GrcManager::DebugUnicode(std::ostream & strmOut, int nUnicode, bool f32bit)
{
	char rgch[20];
	itoa(nUnicode, rgch, 16);
	for (int ich = 0; ich < 20; ich++)
	{
		if ('a' <= rgch[ich] && rgch[ich] <= 'f')
			rgch[ich] -= 'a' - 'A';	// uppercase
		if (rgch[ich] == 0)
			break;
	}

	strmOut << "U+";
	if (f32bit)
	{
		if (nUnicode <= 0x0FFFFFFF) strmOut << "0";
		if (nUnicode <= 0x00FFFFFF) strmOut << "0";
		if (nUnicode <= 0x000FFFFF) strmOut << "0";
		if (nUnicode <= 0x0000FFFF) strmOut << "0";
	}
	if (nUnicode <= 0x0fff) strmOut << "0";
	if (nUnicode <= 0x00ff) strmOut << "0";
	if (nUnicode <= 0x000f) strmOut << "0";
	strmOut << rgch;
}

/*----------------------------------------------------------------------------------------------
	Return a string version of an expression type
----------------------------------------------------------------------------------------------*/
std::string GrcManager::ExpressionDebugString(ExpressionType expt)
{
	switch (expt)
	{
	default:
	case kexptUnknown:	return "unknown";
	case kexptZero:
	case kexptOne:
	case kexptNumber:	return "integer";
	case kexptBoolean:	return "boolean";
	case kexptMeas:		return "em-units";
	case kexptSlotRef:	return "slot-ref";
	case kexptString:	return "string";
	case kexptPoint:	return "point";
	case kexptGlyphID:	return "gid";
	}
}

/*----------------------------------------------------------------------------------------------
	Return the path from the output directory (where the font and GDX file will go) to
	the current directory. For instance, if the output directory is "C:/aaa/bbb/ccc/ddd"
	and the current directory is "C:/aaa/bbb/ccc/eee/fff", the result will be
	"../eee/fff".

	This is used for modifying the GDL file names that are put in the GDX file; they must be
	relative to that file.
----------------------------------------------------------------------------------------------*/
std::string GrcManager::pathFromOutputToCurrent(char * rgchCurDir, char * rgchOutputPath)
{
	std::string staCurDir(rgchCurDir);
	std::string staOutputPath(rgchOutputPath);
	std::string staResult;

	std::vector<std::string> vstaCurDir;
	splitPath(rgchCurDir, vstaCurDir);

	std::vector<std::string> vstaOutputPath;
	char chSep = splitPath(rgchOutputPath, vstaOutputPath);

	std::vector<std::string> vstaResultRev;

	if (rgchOutputPath[0] == '/' || rgchOutputPath[1] == ':')
	{
		// Output path is absolute.
		while (vstaCurDir.size() > 0 && vstaOutputPath.size() > 0
#ifdef _WIN32
			&& stricmp(vstaCurDir[0].data(), vstaOutputPath[0].data()) == 0)
#else
		// Paths on Linux are case-sensitive, and stricmp doesn't seem to work anyway.
			&& strcmp(vstaCurDir[0].data(), vstaOutputPath[0].data()) == 0)
#endif
		{
			vstaCurDir.erase(vstaCurDir.begin());
			vstaOutputPath.erase(vstaOutputPath.begin());
		}
		size_t ista;
		for (ista = 0; ista < vstaOutputPath.size(); ista++)
		{
			staResult.append("..");
			staResult.append(&chSep, 1);
		}
		for (ista = 0; ista < vstaCurDir.size(); ista++)
		{
			staResult.append(vstaCurDir[ista]);
			staResult.append(&chSep, 1);
		}
	}
	else
	{
		int iCWDpath = vstaCurDir.size() - 1;	// index of the current directory in the path
		for (size_t istaOut = 0; istaOut < vstaOutputPath.size(); istaOut++)
		{
			if (strcmp(vstaOutputPath[istaOut].data(), "..") == 0)
			{
				if (vstaResultRev.size() > 0
					&& strcmp(vstaResultRev[vstaResultRev.size()-1].data(), "..") == 0)
				{
					// Output path is something like 'aaa/..' - strange situation, but
					// remove the most recent directory.
					vstaResultRev.pop_back();
				}
				else
				{
					vstaResultRev.push_back(vstaCurDir[iCWDpath]);
					iCWDpath--;
				}
			}
			else
				vstaResultRev.push_back("..");
		}

		for (int ista = vstaResultRev.size() - 1; ista >= 0; ista--)
		{
			staResult.append(vstaResultRev[ista]);
			staResult.append(&chSep, 1);
		}
	}

	return staResult;
}

/*----------------------------------------------------------------------------------------------
	Split path into directory names. Return the character that is used for the separator
	(/ or \).
----------------------------------------------------------------------------------------------*/
char GrcManager::splitPath(char * rgchPath, std::vector<std::string> & vstaResult)
{
	char chSep = '/';  // unknown
	char * pch = rgchPath;
	char * pchStart = rgchPath;
	while (*pch != 0 || pch > pchStart)
	{
		if (*pch == 0 || *pch == '/' || *pch == '\\')
		{
			if (*pch == '\\')
				chSep = '\\';
			else if (*pch == '/')
				chSep = '/';

			char rgchBuf[64];
			memset(rgchBuf, 0, 64);
			memcpy(rgchBuf, pchStart, (pch - pchStart) * sizeof(char));
			std::string staBuf(rgchBuf);
			if (staBuf.length() > 0)
				vstaResult.push_back(staBuf);
			pchStart = pch + 1;
		}
		if (*pch != 0)
			pch++;
	}
	return chSep;
}

/*----------------------------------------------------------------------------------------------
	Enable the font to skip passes when the stream does not include any of the key glyphs.
----------------------------------------------------------------------------------------------*/
void GrcManager::PassOptimizations()
{
	// The *skipPasses* bitmap has already been initialized to 1111111... for all glyphs.

	Symbol psymSkipP = m_psymtbl->FindSymbol("*skipPasses*");
	unsigned int nAttrIdSkipP = psymSkipP->InternalID();

	m_prndr->PassOptimizations(m_pgax, m_psymtbl, nAttrIdSkipP);
}

/*--------------------------------------------------------------------------------------------*/
void GdlRenderer::PassOptimizations(GrcGlyphAttrMatrix * pgax, GrcSymbolTable * psymtbl,
	unsigned int nAttrIdSkipP)
{
	for (size_t iprultbl = 0; iprultbl < m_vprultbl.size(); iprultbl++)
	{
		m_vprultbl[iprultbl]->PassOptimizations(pgax, psymtbl, nAttrIdSkipP);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::PassOptimizations(GrcGlyphAttrMatrix * pgax,  GrcSymbolTable * psymtbl,
	unsigned int nAttrIdSkipP)
{
	for (size_t ippass = 0; ippass < m_vppass.size(); ippass++)
	{
		m_vppass[ippass]->PassOptimizations(pgax, psymtbl, nAttrIdSkipP);
	}
}
/*--------------------------------------------------------------------------------------------*/
void GdlPass::PassOptimizations(GrcGlyphAttrMatrix * pgax, GrcSymbolTable * psymtbl,
	unsigned int nAttrIdSkipP)
{
	for (size_t iprule = 0; iprule < m_vprule.size(); iprule++)
	{
		m_vprule[iprule]->PassOptimizations(pgax, psymtbl, nAttrIdSkipP, this->GlobalID());
	}
}
/*--------------------------------------------------------------------------------------------*/
void GdlRule::PassOptimizations(GrcGlyphAttrMatrix * pgax, GrcSymbolTable * psymtbl,
	unsigned int nAttrIdSkipP, int nPassID)
{
	//	Find a "key" slot for this rule: the first slot to be modified via substitution,
	//	deletion, or attribute setting. For each glyph in the class associated with the slot,
	//	clear the *skipPasses* bit for this pass, indicating that the presence of that glyph
	//	requires the pass to be run.

	bool fInsertion = false;
	int iritFirstNonInsertion = -1;

	//	First, look for a slot that is explicitly marked.
	int iritKey = -1;
	for (unsigned int irit = 0; irit < m_vprit.size() ; irit++)
	{
		if (m_vprit[irit]->IsMarkedKeySlot())
		{
			if (iritKey == -1)
				iritKey = irit;
			else
				g_errorList.AddWarning(5708, this, "Multiple slots marked as key slot");
		}
		if (m_vprit[irit]->IsInsertionSlot())
			fInsertion = true;
		else if (iritFirstNonInsertion == -1)
			iritFirstNonInsertion = irit;
	}

	if (iritKey == -1)
	{
		//	Next, look for the first modified slot.
		for (unsigned int irit = 0; irit < m_vprit.size() ; irit++)
		{
			if (m_vprit[irit]->CanBeKeySlot())
			{
				iritKey = irit;
				break;
			}
		}
	}
	
	if (iritKey == -1 && fInsertion)
	{
		// The rule has an insertion, so use the first non-insertion slot as the key.
		iritKey = iritFirstNonInsertion;

		if (iritKey == -1)
		{
			// All slots are insertion slots. Just mark all glyphs as key for this pass.
			// (Actually this should never happen, because a rule with all insertions results in
			// an error before we get this far.)
			Symbol psym = psymtbl->FindSymbol("ANY");
			GdlGlyphClassDefn * pglfcKey = psym->GlyphClassDefnData();
			pglfcKey->MarkKeyGlyphsForPass(pgax, nAttrIdSkipP, nPassID);
		}
	}
	if (iritKey > -1)
	{
		m_vprit[iritKey]->MarkKeyGlyphsForPass(pgax, nAttrIdSkipP, nPassID);
	}
	//	Otherwise this rule has no effect. This is possible when a rule has been created
	//	to preclude another rule being run. Just ignore it for this purpose.
}

/*--------------------------------------------------------------------------------------------*/
void GdlRuleItem::MarkKeyGlyphsForPass(GrcGlyphAttrMatrix * pgax, unsigned int nAttrIdSkipP,
	int nPassID)
{
	GdlGlyphClassDefn * pglfcKey = m_psymInput->GlyphClassDefnData();
	pglfcKey->MarkKeyGlyphsForPass(pgax, nAttrIdSkipP, nPassID);
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::MarkKeyGlyphsForPass(GrcGlyphAttrMatrix * pgax, unsigned int nAttrIdSkipP,
	int nPassID)
{
	if (nPassID >= kPassPerSPbitmap)
	{
		// Use *skipPasses2*.
		nPassID -= kPassPerSPbitmap;
		nAttrIdSkipP++;
	}

	if (nPassID >= kPassPerSPbitmap)
	{
		//	More than 32 passes! - ignore this pass.
	}
	else
	{
		// Clear the *skipPasses* bit for all the glyphs in this class.

		if (!this->m_fHasFlatList)
			this->FlattenMyGlyphList();

		// Pass IDs start at 0, so pass 0 uses the lowest bit.
		unsigned int nClearBit = 0x1 << nPassID;
		unsigned int nMask = ~nClearBit;

		for (unsigned int igid = 0; igid < m_vgidFlattened.size(); igid++)
		{
			GdlExpression * pexp = pgax->GetExpression(m_vgidFlattened[igid], nAttrIdSkipP);
			Assert(pexp);
			int nValue;
			bool f = pexp->ResolveToInteger(&nValue, false);
			Assert(f); // this better be an integer!
			nValue = (unsigned int)nValue & nMask;
			GdlNumericExpression * pexpNum = dynamic_cast<GdlNumericExpression *>(pexp);
			Assert(pexpNum);
			pexpNum->SetValue(nValue);
		}
	}
}

/*----------------------------------------------------------------------------------------------
	Return true if the slot attribute settings indicate that this a key slot for a pass.
----------------------------------------------------------------------------------------------*/
bool GdlSetAttrItem::IsMarkedKeySlot()
{
	for (unsigned int iavs = 0; iavs < m_vpavs.size(); iavs++)
	{
		if (m_vpavs[iavs]->IsKeySlotAttr())
			return true;
	}
	return false;
}

bool GdlAttrValueSpec::IsKeySlotAttr()
{
	return (m_psymName->FullName() == "passKeySlot");
}
