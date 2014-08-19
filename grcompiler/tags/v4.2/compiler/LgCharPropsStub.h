/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 2003 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: LgCharPropsStub.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Stub of the LgCharPropertyEngine class, and related definitions.
----------------------------------------------------------------------------------------------*/

typedef utf16 wchar;

//typedef [v1_enum] enum LgBidiCategory
typedef enum tagLgBidiCategory
{
	// Strong
	kbicL,   // Left-Right; most alphabetic chars, etc.
	kbicLRE, // Left-Right Embedding
	kbicLRO, // Left-Right Override
	kbicR,   // Right-Left; Hebrew and its punctuation
	kbicAL,  // Right-Left Arabic
	kbicRLE, // Right-Left Embedding
	kbicRLO, // Right-Left Override
	kbicPDF, // Pop Directional Format

	// Weak
	kbicEN,  //	European Number
	kbicES,  //	European Number Separator
	kbicET,  //	European Number Terminator
	kbicAN,  //	Arabic Number
	kbicCS,  //	Common Number Separator

	// Separators:
	kbicNSM, // Non-Spacing Mark
	kbicBN,  // Boundary Neutral
	kbicB,   //	Paragraph Separator
	kbicS,   //	Segment Separator

	// Neutrals:
	kbicWS,  //	Whitespace
	kbicON  //	Other Neutrals ; All other characters: punctuation, symbols
} LgBidiCategory;  // Hungarian: bic


//	Script direction codes
typedef enum tagScriptDirCode
{
	kfsdcNone			= 0,
	kfsdcHorizLtr		= 1,	// horizontal left-to-right
	kfsdcHorizRtl		= 2,	// horizontal right-to-left
	kfsdcVertFromLeft	= 4,	// vertical from left
	kfsdcVertFromRight	= 8	// vertical from right

} ScriptDirCode;	// Hungarian: sdc


// Types of line break, used by the rendering engines.
//typedef [v1_enum] enum LgLineBreak {
typedef enum tagLgLineBreak {

	klbNoBreak,		// (the segment is not broken at all; the whole run fit);
	klbWordBreak,	// (a break at a word boundary: a normal, natural place to break);
	klbHyphenBreak,	// (break a word at a known valid hyphenation point, or comparable;
	klbLetterBreak,	// (break between letters, but not at a word or normal hyphenation point),
	klbClipBreak	// (had to clip even first letter, worst possible break)
} LgLineBreak;  // Hungarian: lbrk


typedef bool ComBool;

// This class should never actually be used, it is only here to make the everything compile.
class ILgCharacterPropertyEngine
{
public:
	HRESULT get_IsSeparator(wchar /*wUnicode*/,  ComBool * /*pfIsSep*/)
	{
		GrAssert(false);
		return E_NOTIMPL;
	}
	HRESULT get_BidiCategory(wchar /*wUnicode*/,  LgBidiCategory * /*pbidi*/)
	{
		GrAssert(false);
		return E_NOTIMPL;
	}
	int Release()
	{
		return 1;
	}
};

///typedef GenSmartPtr<ILgCharacterPropertyEngine> ILgCharacterPropertyEnginePtr;
typedef ILgCharacterPropertyEngine * ILgCharacterPropertyEnginePtr;

enum {
	CLSID_LgIcuCharPropEngine = 0
};
