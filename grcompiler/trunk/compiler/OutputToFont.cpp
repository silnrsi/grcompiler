/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: OutputToFont.cpp
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    Methods to write the tables in the TT font file.
-------------------------------------------------------------------------------*//*:End Ignore*/

/***********************************************************************************************
	Include files
***********************************************************************************************/
#include "main.h"

#include <time.h>

#pragma hdrstop
#undef THIS_FILE
DEFINE_THIS_FILE

/***********************************************************************************************
	Forward declarations
***********************************************************************************************/
static DWORD PadLong(DWORD ul); // 
static unsigned int CalcCheckSum(const void  * pluTable, size_t cluSize);
static int CompareDirEntries(const void *,  const void *); // compare fn for qsort()

using namespace TtfUtil;
using namespace Sfnt;

/***********************************************************************************************
	Methods
***********************************************************************************************/
/*----------------------------------------------------------------------------------------------
	Copy all tables from src file to dst file and add our custom tables.
	Adding: Silf, Gloc, Glat, Feat, and possibly Sile.

	If we are outputting a separate control file, we are basically copying the minimal font,
	making a few changes (to name, cmap, and OS/2 tables), and adding 5 tables. Otherwise
	we are copying the original source font, making a few changes (to the name table), and
	adding 4 tables.

	Return 0 on success, non-zero otherwise.
	TODO AlanW: Output Sild table (table with debug strings).
----------------------------------------------------------------------------------------------*/
int GrcManager::OutputToFont(char * pchSrcFileName, char * pchDstFileName,
	utf16 * pchDstFontFamily, bool fModFontName, utf16 * pchSrcFontFamily)
{
	const int kcExtraTables = 5; // count of tables to add to font
	std::ifstream strmSrc(pchSrcFileName, std::ios::binary);
	GrcBinaryStream bstrmDst(pchDstFileName); // this deletes pre-existing file

	OffsetSubTable offsetTableSrc;

	OffsetSubTable offsetTableMin;
	uint8* pDirEntryMin;
	uint16 cTableMinOut;

	uint16 cTableCopy;
	std::ifstream * pstrmCopy;
	OffsetSubTable::Entry * pDirEntryCopy;

	OffsetSubTable * pOffsetTableOut;
	uint16 cTableOut;

	unsigned int luMasterChecksumSrc = 0;
	unsigned int rgnCreateTime[2];
	unsigned int rgnModifyTime[2];

	// Read offset table and directory entries.
    size_t lOffset, lSize;
    size_t iTableCmapSrc, iTableCmapLen;
    size_t iTableOS2Src, iTableOS2Len;
    size_t iTableHeadSrc, iTableHeadLen;
    OffsetSubTable::Entry *pDirEntrySrc;

    if (!GetHeaderInfo(lOffset, lSize)) return 1;
    strmSrc.read((char *)&offsetTableSrc, lSize + lOffset);
    if (!GetTableDirInfo(&offsetTableSrc, lOffset, lSize)) return 1;
    uint16 cTableSrc = read(offsetTableSrc.num_tables);
    pDirEntrySrc = (OffsetSubTable::Entry *)new uint8[lSize];
    strmSrc.read((char *)pDirEntrySrc, lSize);
    if (!GetTableInfo(ktiCmap, &offsetTableSrc, pDirEntrySrc, iTableCmapSrc, iTableCmapLen)) return 2;
    if (!GetTableInfo(ktiOs2, &offsetTableSrc, pDirEntrySrc, iTableOS2Src, iTableOS2Len)) return 2;
    if (!GetTableInfo(ktiHead, &offsetTableSrc, pDirEntrySrc, iTableHeadSrc, iTableHeadLen)) return 2;

	// Read features from input font, if any. This is so we can avoid duplicating existing
	// feature strings in the name table.
	size_t iTableFeatSrc, iTableFeatLen;
	if (GetTableInfo(ktiFeat, &offsetTableSrc, pDirEntrySrc, iTableFeatSrc, iTableFeatLen))
	{
		size_t iTableNameSrc, iTableNameLen;
		GetTableInfo(ktiName, &offsetTableSrc, pDirEntrySrc, iTableNameSrc, iTableNameLen);
		ReadSourceFontFeatures(strmSrc, iTableFeatSrc, iTableFeatLen,
			iTableNameSrc, iTableNameLen);
	}

	// Count up the number of existing Graphite tables.
	int cOldGraphiteTables = 0;
	uint32 tagSilf = TableIdTag(ktiSilf);
	uint32 tagGloc = TableIdTag(ktiGloc);
	uint32 tagGlat = TableIdTag(ktiGlat);
	uint32 tagFeat = TableIdTag(ktiFeat);
	uint32 tagSill = TableIdTag(ktiSill);
	uint32 tagSile = TableIdTag(ktiSile);
	for (int iTable = 0; iTable < cTableSrc; iTable++)
	{
		uint32 tag = read(pDirEntrySrc[iTable].tag);
		if (tag == tagSilf || tag == tagGloc || tag == tagGlat || tag == tagFeat || tag == tagSill
				|| tag == tagSile)
			cOldGraphiteTables++;
	}

	// Do the same for the minimal control file.
	const char * rgchEnvVarName = "GDLPP_PREFS";
	const char * rgchMinPath = getenv(rgchEnvVarName);
	std::string staMinFile(rgchMinPath ? rgchMinPath : "");
	if (staMinFile.length() > 2)
	{
		if (staMinFile.length() > 0 && staMinFile[0] == '-' && staMinFile[1] == 'I')
			staMinFile = staMinFile.substr(2, staMinFile.length() - 2);
		if (staMinFile[staMinFile.length() - 1] != '\\')
			staMinFile.append("\\", 1);
		staMinFile.append("mingr.ttf");
	}
	else
	{
		// Can't find the minimal font; creating the stream will fail.
		staMinFile = "_bogus_.ttf";
	}
	std::ifstream strmMin(staMinFile.c_str(), std::ios::binary);
	if (SeparateControlFile())
	{
        size_t lOffset, lSize;

        if (!GetHeaderInfo(lOffset, lSize)) return 3;
		strmMin.read((char *)&offsetTableMin, lOffset + lSize);
		if (read(offsetTableMin.scaler_type) != OneFix)
			return 3;
        int cTableIn = read(offsetTableMin.num_tables);
        if (!GetTableDirInfo(&offsetTableSrc, lOffset, lSize)) return 3;
        pDirEntryMin = new uint8[lSize];
        strmSrc.read((char *)&pDirEntryMin, lSize);

		// Read the master checksum and dates from the source font head table.
		uint8 * pbTableHeadSrc = new uint8[iTableHeadLen];
        strmSrc.seekg(iTableHeadSrc);
        strmSrc.read((char *)pbTableHeadSrc, iTableHeadLen);
		luMasterChecksumSrc = TtfUtil::HeadTableCheckSum(pbTableHeadSrc);
		TtfUtil::HeadTableCreateTime(pbTableHeadSrc, &rgnCreateTime[0], &rgnCreateTime[1]);
		TtfUtil::HeadTableModifyTime(pbTableHeadSrc, &rgnModifyTime[0], &rgnModifyTime[1]);
		delete[] pbTableHeadSrc;

		cTableOut = cTableIn - cOldGraphiteTables + kcExtraTables + 1; // Six extra tables: Sile, Silf, Feat, Gloc, Glat, Sill
		// This is the file we're really copying.
		pOffsetTableOut = &offsetTableMin;
		cTableCopy = cTableIn - cOldGraphiteTables;	// number we actually copy
		cTableMinOut = cTableCopy;		// index of first Graphite table
		pstrmCopy = &strmMin;
		pDirEntryCopy = (OffsetSubTable::Entry *)pDirEntryMin;
	}
	else
	{
		//	We'll output a copy of the base font, glyphs and all.
		cTableOut = cTableSrc - cOldGraphiteTables + kcExtraTables; // Five extra tables: Silf, Feat, Gloc, Glat, Sill
		cTableMinOut = cTableSrc - cOldGraphiteTables;
		pOffsetTableOut = &offsetTableSrc;
		cTableCopy = cTableSrc - cOldGraphiteTables;	// number we actually copy
		cTableMinOut = cTableCopy;		// index of first Graphite table
		pstrmCopy = &strmSrc;
		pDirEntryCopy = pDirEntrySrc;
	}

	OffsetSubTable::Entry * pDirEntryOut;
	if (!(pDirEntryOut = new OffsetSubTable::Entry[cTableOut]))
		return 6;
	int iTableOut = 0;
	for (int iTable = 0; iTable < cTableSrc; iTable++)
	{
		uint32 tag = read(pDirEntrySrc[iTable].tag);
		if (tag == tagSilf || tag == tagGloc || tag == tagGlat || tag == tagFeat || tag == tagSill
				|| tag == tagSile)
		{}
		else
		{
			memcpy(pDirEntryOut + iTableOut, pDirEntryCopy + iTable, sizeof(OffsetSubTable::Entry));
			iTableOut++;
		}
	}
	////memcpy(pDirEntryOut, pDirEntryCopy, cTableSrc * sizeof(OffsetSubTable::Entry));

	// Offset table: adjust for extra tables and output.
	int nP2, nLog;
	uint16 suTmp;
	BinarySearchConstants(cTableOut, &nP2, &nLog);
	pOffsetTableOut->num_tables = read(cTableOut);
	suTmp = (uint16)(nP2 << 4);
	pOffsetTableOut->search_range = read(suTmp);
	suTmp = (uint16)nLog;
	pOffsetTableOut->entry_selector = read(suTmp);
	suTmp = (cTableOut << 4) - (uint16)(nP2 << 4);
	pOffsetTableOut->range_shift = read(suTmp);
	// write offset table (version & search constants)
	bstrmDst.Write((char *)pOffsetTableOut, offsetof(OffsetSubTable, table_directory));
	
	// Copy tables from input stream to output stream.
	uint32 ibTableOffset = 0;	// initialize to avoid warnings
	uint32 ibHeadOffset = 0;
	uint32 cbHeadSize = 0;
	int iNameTbl = 0;
	int iCmapTbl = -1;
	int iOS2Tbl = -1;
	uint8 * pbTable;
	ibTableOffset = PadLong(offsetof(OffsetSubTable, table_directory) + cTableOut * sizeof(OffsetSubTable::Entry));
//	bstrmDst.SetPosition(ibTableOffset);
    bstrmDst.Write((char *)pDirEntryOut, cTableOut * sizeof(OffsetSubTable::Entry));
	int iOut = 0;
	for (int iSrc = 0; iSrc < cTableCopy + cOldGraphiteTables; iSrc++)
	{
		// read table
		uint32 ibOffset, cbSize;
		cbSize = read(pDirEntryCopy[iSrc].length); // dir entries are word aligned already
		ibOffset = read(pDirEntryCopy[iSrc].offset);
		if (!(pbTable = new uint8[cbSize]))
			return 7;
		pstrmCopy->seekg(ibOffset);
		pstrmCopy->read((char *)pbTable, cbSize);

		uint32 tag = read(pDirEntryCopy[iSrc].tag);

		if (tag == tagSilf || tag == tagGloc || tag == tagGlat || tag == tagFeat || tag == tagSill
				|| tag == tagSile)
		{
			// Old Graphite table--ignore.
			delete [] pbTable;
			continue;
		}
		
		else if (SeparateControlFile() && TableIdTag(ktiOs2) == tag)
		{
			// merge the OS/2 tables from the minimal and source fonts
			iOS2Tbl = iOut;
			uint8 * pbTableSrc;
			int cbSizeSrc = read(pDirEntrySrc[iTableOS2Src].length);
			int ibOffsetSrc = read(pDirEntrySrc[iTableOS2Src].offset);
			if (!(pbTableSrc = new uint8[cbSizeSrc]))
				return 8;
			strmSrc.seekg(ibOffsetSrc);
			strmSrc.read((char *)pbTableSrc, cbSizeSrc);

			bstrmDst.SetPosition(ibTableOffset);
			if (!OutputOS2Table(pbTableSrc, cbSizeSrc, pbTable, cbSize, &bstrmDst, &cbSize))
				return 9;
			pDirEntryOut[iOut].length = read(cbSize);

			delete[] pbTableSrc;
		}
		
		else if (SeparateControlFile() && TableIdTag(ktiCmap) == tag)
		{
			// generate a new cmap
			iCmapTbl = iOut;
			uint8 * pbTableSrc;
			int cbSizeSrc = read(pDirEntrySrc[iTableCmapSrc].length);
			int ibOffsetSrc = read(pDirEntrySrc[iTableCmapSrc].offset);
			if (!(pbTableSrc = new uint8[cbSizeSrc]))
				return 10;
			strmSrc.seekg(ibOffsetSrc);
			strmSrc.read((char *)pbTableSrc, cbSizeSrc);

			bstrmDst.SetPosition(ibTableOffset);
			OutputCmapTable(pbTableSrc, cbSizeSrc, &bstrmDst, &cbSize);
			pDirEntryOut[iOut].length = read(cbSize);

			delete[] pbTableSrc;
		}
		else
		{
			// add the feature names and change the font name in the name table
			if (TableIdTag(ktiName) == tag)
			{
				iNameTbl = iOut;
				if (!AddFeatsModFamily((fModFontName ? pchDstFontFamily : NULL), &pbTable, &cbSize))
					return 11;
				pDirEntryOut[iOut].length = read(cbSize);
			}

			// remember offset and size of head table to adjust the file checksum later
			if (TableIdTag(ktiHead) == tag)
			{
				ibHeadOffset = ibTableOffset;
				cbHeadSize = cbSize;
			}

			// write table
			bstrmDst.SetPosition(ibTableOffset); // assumes seeking past EOF will fill with zeroes
			bstrmDst.Write((char *)pbTable, cbSize);
		}

		// adjust table's directory entry, offset will change since table directory is bigger
		pDirEntryOut[iOut].offset = read(ibTableOffset);
		iOut++;

		ibTableOffset = bstrmDst.SeekPadLong(ibTableOffset + cbSize);
		delete [] pbTable;
	}

	// Output our tables - Output*() methods handle 4 byte table alignment.
	// Create directory entries for our tables.
	// We can't set checksum because can't read table from GrcBinaryStream class (write only).
	uint32 ibOffset,ibOffset2;
	uint32 cbSize, cbSize2;
	int nCurTable = cTableMinOut;
	bstrmDst.SetPosition(ibTableOffset);

	// Gloc
	OutputGlatAndGloc(&bstrmDst, (int *)&ibOffset, (int *)&cbSize, 
		(int *)&ibOffset2, (int *)&cbSize2);
	pDirEntryOut[nCurTable].tag = read(TableIdTag(ktiGloc));
	pDirEntryOut[nCurTable].length = read(cbSize);
	pDirEntryOut[nCurTable].offset = read(ibOffset);
	pDirEntryOut[nCurTable++].checksum = 0L; // place holder

	// Glat
	pDirEntryOut[nCurTable].tag = read(TableIdTag(ktiGlat));
	pDirEntryOut[nCurTable].length = read(cbSize2);
	pDirEntryOut[nCurTable].offset = read(ibOffset2);
	pDirEntryOut[nCurTable++].checksum = 0L; // place holder

	// Silf
	OutputSilfTable(&bstrmDst, (int *)&ibOffset, (int *)&cbSize);
	pDirEntryOut[nCurTable].tag = read(TableIdTag(ktiSilf));
	pDirEntryOut[nCurTable].length = read(cbSize);
	pDirEntryOut[nCurTable].offset = read(ibOffset);
	pDirEntryOut[nCurTable++].checksum = 0L; // place holder

	// Feat
	OutputFeatTable(&bstrmDst, (int *)&ibOffset, (int *)&cbSize);
	pDirEntryOut[nCurTable].tag = read(TableIdTag(ktiFeat));
	pDirEntryOut[nCurTable].length = read(cbSize);
	pDirEntryOut[nCurTable].offset = read(ibOffset);
	pDirEntryOut[nCurTable++].checksum = 0L; // place holder

	// Sill
	OutputSillTable(&bstrmDst, (int *)&ibOffset, (int *)&cbSize);
	pDirEntryOut[nCurTable].tag = read(TableIdTag(ktiSill));
	pDirEntryOut[nCurTable].length = read(cbSize);
	pDirEntryOut[nCurTable].offset = read(ibOffset);
	pDirEntryOut[nCurTable++].checksum = 0L; // place holder

	if (SeparateControlFile())
	{
		// Sile
		OutputSileTable(&bstrmDst,
			pchSrcFontFamily, pchSrcFileName,
			luMasterChecksumSrc, rgnCreateTime, rgnModifyTime,
			(int *)&ibOffset, (int *)&cbSize);
		pDirEntryOut[nCurTable].tag = read(TableIdTag(ktiSile));
		pDirEntryOut[nCurTable].length = read(cbSize);
		pDirEntryOut[nCurTable].offset = read(ibOffset);
		pDirEntryOut[nCurTable++].checksum = 0L; // place holder
	}

	Assert(nCurTable == cTableOut);

	// Fix up file checksums in table directory for our tables and any we modified.
	uint32 luCheckSum;
	for (int iOut = 0; iOut < cTableOut; iOut++)
	{
		if (iOut >= cTableCopy // one of our new Graphite tables
			|| iOut == iNameTbl || iOut == iOS2Tbl || iOut == iCmapTbl)
		{	// iterate over our tables
			cbSize = PadLong(read(pDirEntryOut[iOut].length)); // find table size (tables are padded)
			if (!(pbTable = new uint8[cbSize]))
				return 12;
			bstrmDst.seekg(read(pDirEntryOut[iOut].offset));
			bstrmDst.read((char *)pbTable, cbSize);
			luCheckSum = CalcCheckSum(pbTable, cbSize);
			pDirEntryOut[iOut].checksum = read(luCheckSum);
			delete [] pbTable;
		}
	}

	// calc file checksum
	luCheckSum = 0;
	for (int iOut = 0; iOut < cTableOut; iOut++)
	{
		luCheckSum += read(pDirEntryOut[iOut].checksum);
	}
	luCheckSum += CalcCheckSum(pDirEntryOut, sizeof(OffsetSubTable::Entry) * cTableOut);
	luCheckSum += CalcCheckSum(pOffsetTableOut, offsetof(OffsetSubTable, table_directory));
	luCheckSum = 0xB1B0AFBA - luCheckSum; // adjust checksum for head table
	
	// sort directory entries
	::qsort((void *)pDirEntryOut, cTableOut, sizeof(OffsetSubTable::Entry), CompareDirEntries);

	// write directory entries
	bstrmDst.SetPosition(offsetof(OffsetSubTable, table_directory));
	for (int i = 0; i < cTableOut; i++)
	{
		bstrmDst.write((char *)(pDirEntryOut + i), sizeof(OffsetSubTable::Entry)); 
	}

	// write adjusted checksum to head table
	bstrmDst.seekg(ibHeadOffset);
	if (!(pbTable = new uint8[cbHeadSize]))
		return 13;
	bstrmDst.read((char *)pbTable, cbHeadSize); // read the head table
	((FontHeader *)pbTable)->check_sum_adjustment = read(luCheckSum);
	bstrmDst.seekp(ibHeadOffset);
	bstrmDst.write((char *)pbTable, cbHeadSize); // overwrite the head table
	delete [] pbTable;

	delete [] pDirEntrySrc;
	if (SeparateControlFile())
		delete [] pDirEntryMin;
	delete[] pDirEntryOut;

	return 0;
}


/*----------------------------------------------------------------------------------------------
	Create a shadow version of the features based on what was in the original font (if any).
	The purpose of this is to be able to compare existing feature label strings with what is
	already in the name table and avoid unnecessary duplication.
----------------------------------------------------------------------------------------------*/
void GrcManager::ReadSourceFontFeatures(std::ifstream & strmSrc,
	size_t iTableFeatSrc, size_t iTableFeatLen, size_t iTableNameSrc, size_t iTableNameLen)
{
	// Get the contents of the name table.
	uint8 * pNameTbl = new uint8[iTableNameLen];
	strmSrc.seekg(iTableNameSrc);
	strmSrc.read((char *)pNameTbl, iTableNameLen);

	// Get and parse the contents of the Feat table.
	uint8 * pFeatTbl = new uint8[iTableFeatLen];
	strmSrc.seekg(iTableFeatSrc);
    strmSrc.read((char *)pFeatTbl, iTableFeatLen);

	uint8 * p = pFeatTbl;
	int version = read(*((uint32*)p));	p += 4;
	int cfeat = read(*((uint16*)p));	p +=2;
	// reserved
	uint16 tmp16 = read(*((uint16*)p));	p +=2;
	uint32 tmp32 = read(*((uint32*)p));	p += 4;

	std::vector<int> vncfset;
	int ifeat;
	for (ifeat = 0; ifeat < cfeat; ifeat++)
	{
		GdlFeatureDefn * pfeat = new GdlFeatureDefn();
		m_vpfeatInput.push_back(pfeat);

		pfeat->SetID(read(*((uint32*)p)));	p += 4;
		int cfset = read(*((uint16*)p));	p +=2;
		tmp16 = read(*((uint16*)p));		p +=2;	// reserved
		int offset = read(*((uint32*)p));	p += 4;
		int flags = read(*((uint16*)p));	p +=2;
		int nNameID = read(*((uint16*)p));	p +=2;

		pfeat->SetLabelFromNameTable(nNameID, 1033, pNameTbl);
		vncfset.push_back(cfset);	// remember number of settings
	}

	// feature settings
	for (ifeat = 0; ifeat < cfeat; ifeat++)
	{
		GdlFeatureDefn * pfeat = m_vpfeatInput[ifeat];
		for (int ifset = 0; ifset < vncfset[ifeat]; ifset++)
		{
			GdlFeatureSetting * pfset = new GdlFeatureSetting();
			pfeat->PushFeatureSetting(pfset);
			
			pfset->SetValue(read(*((uint16*)p)));	p +=2;
			int nNameID = (read(*((uint16*)p)));	p +=2;
			pfset->SetLabelFromNameTable(nNameID, 1033, pNameTbl);
		}
	}
}

/*----------------------------------------------------------------------------------------------
	Create a new name table with the strings associated with features and their settings in it.
	Also modify the font name. This method deletes ppNameTable's data and sets the pointer to 
	the new name table. cbNameTbl is updated with the new size. 
	pchFamilyName is the new font name. It will be stored as the font family name and 
	as part of the full font name (which includes subfamily too if this is not regular).

	The new name table may be structured differently since any name table records which point 
	to the same string will be adjusted to point to different strings (which are copies of one 
	another). This produces a simpler algorithm since one string can be copied for each 
	record. This issue was discovered close to release so large changes were rejected.
----------------------------------------------------------------------------------------------*/
bool GrcManager::AddFeatsModFamily(uint16 * pchwFamilyNameNew,
	uint8 ** ppNameTbl, uint32 * pcbNameTbl)
{
	// Get the current date, which will be used to create the unique name.
	utf16 stuDate[12];
	BuildDateString(stuDate);
	
	uint32 cbTblOld = *pcbNameTbl;

	size_t cchwFamilyName = (pchwFamilyNameNew) ? utf16len(pchwFamilyNameNew) : 0;

	// Interpret name table header:
	FontNames * pTblOld = (FontNames *)(*ppNameTbl);
	uint16 cRecords = read(pTblOld->count);
	uint16 ibStrOffset = read(pTblOld->string_offset);
	NameRecord * pRecord = pTblOld->name_record;
	
	int irecFamilyName, irecSubFamily, irecFullName, irecVendor, irecPSName, irecUniqueName,
		irecPrefFamily, irecCompatibleFull;

	uint16 ibSubFamilyOffset, cbSubFamily;
	uint16 ibVendorOffset, cbVendor;

	int cbOldTblRecords = sizeof(FontNames) + (cRecords - 1) * sizeof(NameRecord);

	// Make a list of all platform+encodings that have an English family name and therefore
	// need to be changed. (Include the Unicode platform 0 for which we don't actually know
	// the language.) Keep track of any existing platform+encodings that also need feature
	// names stored. Also calculate bytes needed to hold all strings.

	std::vector<PlatEncChange> vpecToChange;
	size_t cbOldStringData = 0;
	int nMaxNameId = 0;
	bool fUnchangedName = false;
	for (int iRecord = 0; iRecord < cRecords; iRecord++)
	{
		if (read(pRecord[iRecord].name_id) == n_family)
		{
			int nPlat = read(pRecord[iRecord].platform_id);
			int nEnc = read(pRecord[iRecord].platform_specific_id);
			int nLang = read(pRecord[iRecord].language_id);
			if ((nPlat == NameRecord::Macintosh && nLang == 0)
				|| (nPlat == NameRecord::Microsoft && nLang == LG_USENG)
				|| (nPlat == NameRecord::Unicode))
			{
				// Found an English family name or one we're treating as English--
				// change it if we're changing the names.
				PlatEncChange pecChange;
				pecChange.platformID = nPlat;
				pecChange.encodingID = nEnc;
				pecChange.engLangID = nLang;
				pecChange.cbBytesPerChar = (nPlat == NameRecord::Macintosh) ? 1 : 2;
				pecChange.fChangeName = (pchwFamilyNameNew != NULL);
				vpecToChange.push_back(pecChange);
			}
			else if ((nPlat == NameRecord::Macintosh) || (nPlat == NameRecord::Unicode)
					|| (nPlat == NameRecord::Microsoft && (nEnc == 0 || nEnc == 1 || nEnc == 10)))
			{
				// We found a family name that is not English and we will not change. But we
				// will need to output the feature names.
				PlatEncChange pecChange;
				pecChange.platformID = nPlat;
				pecChange.encodingID = nEnc;
				pecChange.engLangID = nLang;
				pecChange.cbBytesPerChar = (nPlat == NameRecord::Macintosh) ? 1 : 2;
				pecChange.fChangeName = false;
				vpecToChange.push_back(pecChange);
				fUnchangedName = true;	// give a warning
			}
			else
			{
				// Otherwise it is non-English, non-Unicode.
				fUnchangedName = true;
			}
		}
		cbOldStringData += read(pRecord[iRecord].length);
		nMaxNameId = max(nMaxNameId, int(read(pRecord[iRecord].name_id)));
	}
	size_t cbNewStringData = cbOldStringData;

	// Give a warning if there are font names we won't change.
	if (fUnchangedName)
		g_errorList.AddWarning(5503, NULL,
			"Font names that are not English and not in platform 0 (Unicode) will not be changed");

	// Do this once for all platforms:
	// Assign name table ids to the features and settings and get
	// vectors containing the external names, language ids, and name table ids
	// for all features and settings. 256 - 32767 are allowable.
	int nNameTblId = max(nMaxNameId + 1, 256);
	if (m_nNameTblStart != -1 && nNameTblId > m_nNameTblStart)
	{
		char rgch[20];
		itoa(nNameTblId, rgch, 10);
		g_errorList.AddWarning(5501, NULL,
			"Feature strings must start at ", rgch, " in the name table");
	}
	nNameTblId = max(nNameTblId, m_nNameTblStart);
	int nNameTblMinNew = nNameTblId;

	std::vector<std::wstring> vstuExtNames;
	std::vector<uint16> vnLangIds;
	std::vector<uint16> vnNameTblIds;

	size_t cchwFeatStringData = 0;
	if (!AssignFeatTableNameIds(nNameTblId, nNameTblMinNew,
		vstuExtNames, vnLangIds, vnNameTblIds,
		cchwFeatStringData, *ppNameTbl))
	{
		g_errorList.AddError(5101, NULL,
			"Insufficient space available for feature strings in name table.");
		return false; // checks for legal values
	}
	int cbFeatStringData16 = cchwFeatStringData * sizeof(utf16);
	int cbFeatStringData8 = cchwFeatStringData;
	bool f8bitFeatures = false;
	bool f16bitFeatures = false;

	Assert(vstuExtNames.size() == vnLangIds.size());
	Assert(vnLangIds.size() == vnNameTblIds.size());

	// For each platform+encoding of interest, fix up the font names and store the feature strings.

	for (int ipecChange = 0; ipecChange < signed(vpecToChange.size()); ipecChange++)
	{
		PlatEncChange * ppec = &(vpecToChange[ipecChange]);
		//int engID = ppec->engLangID;

		if (ppec->cbBytesPerChar == 1)
			f8bitFeatures = true;	// we need to output 8-bit feature strings
		else
			f16bitFeatures = true;	// we need to output 16-bit feature strings

		if (ppec->fChangeName)
		{
			// There should be (English) font names present for this platform and encoding.
			// Get pointers to the relevant records.
			irecFamilyName = irecSubFamily = irecFullName = irecVendor = irecPSName = irecUniqueName
				= irecPrefFamily = irecCompatibleFull = -1;
			if (!FindNameTblEntries(pRecord, cRecords,
				ppec->platformID, ppec->encodingID, ppec->engLangID,
				&irecFamilyName, &irecSubFamily, &irecFullName, &irecVendor, &irecPSName, &irecUniqueName,
				&irecPrefFamily, &irecCompatibleFull))
			{
				Assert(false); // how did it get in the vpecToChange list?
				continue;
			}

			Assert(pchwFamilyNameNew);

			// Generate the new full-font name, the postscript name, and the unique name.

			std::wstring stuFullName, stuPostscriptName, stuUniqueName;
			ibSubFamilyOffset = (irecSubFamily == -1) ? 0 : read(pRecord[irecSubFamily].offset) + ibStrOffset;
			cbSubFamily = (irecSubFamily == -1) ? 0 : read(pRecord[irecSubFamily].length);
			ibVendorOffset = (irecVendor == -1) ? 0 : read(pRecord[irecVendor].offset) + ibStrOffset;
			cbVendor = (irecVendor == -1) ? 0 : read(pRecord[irecVendor].length);

			// NB: Call below may allocate space which must be deleted at the end of this method.

			if (!BuildFontNames((ppec->cbBytesPerChar == 1), pchwFamilyNameNew, cchwFamilyName, stuDate,
				(uint8*)pTblOld + ibSubFamilyOffset, cbSubFamily,
				(uint8*)pTblOld + ibVendorOffset, cbVendor,
				ppec))
			{
				return false;
			}

			// Determine size adjustments for font name changes: subtract old names and add new ones.
			int dbStringDiff = 0;
			if (irecFamilyName > -1)
			{
				dbStringDiff -= read(pRecord[irecFamilyName].length);
				dbStringDiff += cchwFamilyName * ppec->cbBytesPerChar;
			}
			if (irecFullName > -1)
			{
				dbStringDiff -= read(pRecord[irecFullName].length);
				dbStringDiff += ppec->cchwFullName * ppec->cbBytesPerChar;
			}
			if (irecUniqueName > -1)
			{
				dbStringDiff -= read(pRecord[irecUniqueName].length);
				dbStringDiff += ppec->cchwUniqueName * ppec->cbBytesPerChar;
			}
			if (irecPSName > -1)
			{
				dbStringDiff -= read(pRecord[irecPSName].length);
				dbStringDiff += ppec->cchwPostscriptName * ppec->cbBytesPerChar;
			}
			if (irecPrefFamily > -1)
			{
				dbStringDiff -= read(pRecord[irecPrefFamily].length);
				dbStringDiff += cchwFamilyName * ppec->cbBytesPerChar;
			}
			if (irecCompatibleFull > -1)
			{
				dbStringDiff -= read(pRecord[irecCompatibleFull].length);
				dbStringDiff += ppec->cchwFullName * ppec->cbBytesPerChar;
			}

			cbNewStringData += dbStringDiff;
		}
		else
		{
			// Font name is not changing, but we do have to output feature strings.
			ppec->pchwFullName = NULL;
			ppec->pchwUniqueName = NULL;
			ppec->pchwPostscriptName = NULL;
			ppec->cchwFullName = 0;
			ppec->cchwUniqueName = 0;
			ppec->cchwPostscriptName = 0;
		}
	}

	// Calculate the size of the new name table.
	size_t cbTblNew = cbOldTblRecords
		+ (vstuExtNames.size() * sizeof(NameRecord) * vpecToChange.size())
								// 1 record for each feature string for each platform+encoding of interest
		+ cbNewStringData;		// adjusted for font name changes

	// We need at most one version of the 8-bit feature strings and one version of the 16-bit strings.
	if (f8bitFeatures)
		cbTblNew += cbFeatStringData8;
	if (f16bitFeatures)
		cbTblNew += cbFeatStringData16;

	// Create the new buffer.
	uint8 * pTblNew = new uint8[cbTblNew];

	// Copy directory entries and strings from source to destination,
	// modify font name and add feature names.
	if (!AddFeatsModFamilyAux((uint8 *)pTblOld, cbTblOld, pTblNew, cbTblNew, 
		vstuExtNames, vnLangIds, vnNameTblIds, 
		pchwFamilyNameNew, cchwFamilyName, vpecToChange, nNameTblMinNew))
	{
		return false;
	}

	// Set the output args & clean up.
	*pcbNameTbl = cbTblNew;
	delete [] *ppNameTbl;	// old table
	*ppNameTbl = pTblNew;

	for (size_t ipec = 0; ipec < vpecToChange.size(); ipec++)
	{
		delete vpecToChange[ipec].pchwFullName;
		delete vpecToChange[ipec].pchwUniqueName;
		delete vpecToChange[ipec].pchwPostscriptName;
	}

	return true;
}

/*----------------------------------------------------------------------------------------------
	Return a date string that can be used to generate the unique name. The format returned
	is dd-Mmm-yyyy.
----------------------------------------------------------------------------------------------*/
void GrcManager::BuildDateString(utf16 * stuDate)
{
#if 0
	// Get the current date, which will be used to create the unique name.
	__time64_t ltime;
	_time64( &ltime );
	std::wstring strTimeWchar(_wctime64(&ltime)); // format is Wed Jan 02 02:03:55 1980
	std::copy(strTimeWchar.data() + 8, strTimeWchar.data() + 10, stuDate);		// day
	stuDate[2] = '-';
	std::copy(strTimeWchar.data() + 4, strTimeWchar.data() + 7, stuDate + 3);	// month
	stuDate[6] = '-';
	std::copy(strTimeWchar.data() + 20, strTimeWchar.data() + 24, stuDate + 7);	// year
	stuDate[11] = 0;
#else
	char tempDate[20];
	time_t currentTime;
	time(&currentTime);
	strftime(tempDate, 20, "%d-%b-%Y", localtime(&currentTime));
	std::copy(tempDate, tempDate + strlen(tempDate), stuDate);
	stuDate[11] = 0;
#endif
}

/*----------------------------------------------------------------------------------------------
	Return true if there are any name table entries for this platform-encoding combination.

	If so, return the record indices for the various variations of the font name and other
	relevant fields for the given language (which should be English, because we assume the
	font name is in English). Even if there are no English strings, we still return true
	indicating that the feature strings need to be output for this platform+encoding.
----------------------------------------------------------------------------------------------*/
bool GrcManager::FindNameTblEntries(void * pNameTblRecord, int cNameTblRecords, 
	uint16 suPlatformId, uint16 suEncodingId, uint16 suLangId, 
	int * piFamily, int * piSubFamily, int * piFullName,
	int * piVendor, int * piPSName, int * piUniqueName,
	int * piPrefFamily, int * piCompatibleFull)
{
	bool fNamesFound = false;

	NameRecord * pRecord = (NameRecord *)(pNameTblRecord);
	for (int i = 0; i < cNameTblRecords; i++, pRecord++)
	{
		if (read(pRecord->platform_id) == suPlatformId && 
			read(pRecord->platform_specific_id) == suEncodingId)
		{
			fNamesFound = true;
			if (read(pRecord->language_id) == suLangId)
			{
				uint16 nameID = read(pRecord->name_id);
				switch (nameID)
				{
				case n_family:			*piFamily = i; break;
				case n_subfamily:		*piSubFamily = i; break;
				case n_fullname:		*piFullName = i; break;
				case n_vendor:			*piVendor = i; break;
				case n_postscript:		*piPSName = i; break;
				case n_uniquename:		*piUniqueName = i; break;
				case n_preferredfamily: *piPrefFamily = i; break;
				case n_compatiblefull:  *piCompatibleFull = i; break;
				default:
					break;
				}
			}
		}
	}
	
	return fNamesFound;
}

/*----------------------------------------------------------------------------------------------
	Generate the font names to store in the font's name table. The following are needed:

	* Family name (eg, "Doulos SIL", "Charis Literacy (A)", "Guttman David")
	* Full name (eg, "Doulos SIL", "Charis Literacy (A) BoldItalic", "Guttman David Light")
	* Unique name = Vendor name + colon + fullname + colon + day + hyphen + month + hyphen + year
		(eg, "SIL International:Charis Literacy (A) BoldItalic:20-July-2007")
	* Postscript name = strip spaces, brackets, percent, and slash from Family name, then
		append hyphen + subfammily (eg, "DoulosSIL", "CharisLiteracyA-Bold", "GuttmanDavid-Light")
	
	Also the strings must be put into Unicode Big Endian order as is required for
	the TTF name table. FOR NOW WE ARE RETURNING THEM FROM THIS METHOD AS IS.
----------------------------------------------------------------------------------------------*/

bool GrcManager::BuildFontNames(bool f8bitTable,
	uint16 * pchwFamilyName, size_t cchwFamilyName,
	utf16 * stuDate,
	uint8 * pchSubFamily, uint16 cbSubFamily,
	uint8 * pchVendor, uint16 cbVendor,
	PlatEncChange * ppec)
{
	uint16 * pchwFullName, * pchwUniqueName, * pchwPSName;
	uint16 cchwFullName, cchwUniqueName, cchwPSName;

	size_t cchwDate = utf16len(stuDate);

	Assert(pchwFamilyName);

	// TODO: properly handle the Macintosh encoding, which is not really ANSI.

	// Check for "Regular" or "Standard" subfamily
	utf16 rgchwSubFamily[128];
	bool fRegular;
	int cchwSubFamily;
	if (cbSubFamily == 0)
	{
		fRegular = true;
		cchwSubFamily = 0;
	}
	else
	{
		if (f8bitTable)
			Platform_AnsiToUnicode((char *)pchSubFamily, cbSubFamily, rgchwSubFamily, cbSubFamily);
		else
		{
			utf16ncpy(rgchwSubFamily, (utf16 *)pchSubFamily, cbSubFamily);
			TtfUtil::SwapWString(rgchwSubFamily, cbSubFamily);
		}
		cchwSubFamily = (f8bitTable) ? cbSubFamily : cbSubFamily / sizeof(utf16);
		rgchwSubFamily[cchwSubFamily] = 0;
		fRegular = utf16ncmp(rgchwSubFamily, "Regular", 7) || utf16ncmp(rgchwSubFamily, "Standard", 8);
	}

	// Get vendor name, if any.
	utf16 * rgchwVendor;
	if (cbVendor == 0)
	{
		rgchwVendor = new utf16[15];
		utf16ncpy(rgchwVendor, "Unknown Vendor", 14);
		cbVendor = (f8bitTable) ? 14 : 14 * sizeof(utf16); // pretend
	}
	else
	{
		rgchwVendor = new utf16[cbVendor + 1];
		if (f8bitTable)
			Platform_AnsiToUnicode((char *)pchVendor, cbVendor, rgchwVendor, cbVendor);
		else
		{
			utf16ncpy(rgchwVendor, (utf16 *)pchVendor, cbVendor);
			TtfUtil::SwapWString(rgchwVendor, cbVendor);
		}
	}
	int cchwVendor = (f8bitTable) ? cbVendor : cbVendor / sizeof(utf16);
	rgchwVendor[cchwVendor] = 0;

	// Build the full font name: familyname+subfamily
	// or (if subfamily = Regular/Standard) familyname
	if (fRegular)
	{	// Regular does not include the subfamily in the full font name.
		pchwFullName = new uint16[cchwFamilyName + 1];
		utf16cpy(pchwFullName, pchwFamilyName);
		cchwFullName = cchwFamilyName;
	}
	else
	{	// Other styles do include subfamily in the full font name.
		cchwFullName = cchwFamilyName + cbSubFamily / sizeof(utf16) + 1; // 1 - room for space
		pchwFullName = new uint16[cchwFullName + 1];
		if (!pchwFullName)
			return false;
		utf16ncpy(pchwFullName, pchwFamilyName, cchwFamilyName);
		pchwFullName[cchwFamilyName] = 0x0020; // space
		utf16ncpy(pchwFullName + cchwFamilyName + 1, rgchwSubFamily, cchwSubFamily);
		pchwFullName[cchwFullName] = 0;
	}
	
	// Build the Postscript name: familyname-subfamily, with certain chars stripped out.
	cchwPSName = cchwFamilyName + cchwSubFamily + 1; // 1 = hyphen
	pchwPSName = new uint16[cchwPSName + 1];
	if (!pchwPSName)
		return false;
	utf16ncpy(pchwPSName, pchwFamilyName, cchwFamilyName);
	if (cbSubFamily == 0)
		cchwPSName--; // no hyphen
	else
	{
		pchwPSName[cchwFamilyName] = 0x002D; // hyphen
		utf16ncpy(pchwPSName + cchwFamilyName + 1, rgchwSubFamily, cchwSubFamily);
	}
	pchwPSName[cchwPSName] = 0;
	// Allow only chars 33 - 126, minus the following: / % ( ) < > [ ] { }
	int cchMove = 1;
	for (utf16 * pch = pchwPSName + cchwPSName - 1; pch >= pchwPSName; pch--, cchMove++)
	{
		if (*pch < 33 || *pch > 126 || *pch == '/' || *pch == '%' || *pch == '('
			|| *pch == ')' || *pch == '<' || *pch == '>' || *pch == '[' || *pch == ']'
			|| *pch == '{' || *pch == '}')
		{
			utf16ncpy(pch, pch + 1, cchMove);
			cchwPSName--;
		}
	}
	
	// Build the unique name: vendor: fullname: date
	cchwUniqueName = cchwVendor + cchwFullName + cchwDate + 4;
	pchwUniqueName = new utf16[cchwUniqueName + 1];
	utf16ncpy(pchwUniqueName, rgchwVendor, cchwVendor);
	utf16ncpy(pchwUniqueName + cchwVendor, ": ", 2);
	utf16ncpy(pchwUniqueName + cchwVendor + 2, pchwFullName, cchwFullName);
	utf16ncpy(pchwUniqueName + cchwVendor + 2 + cchwFullName, ": ", 2);
	utf16ncpy(pchwUniqueName + cchwVendor + 2 + cchwFullName + 2, stuDate, cchwDate);
	pchwUniqueName[cchwUniqueName] = 0;
	delete[] rgchwVendor;
	
	ppec->pchwFullName = pchwFullName;
	ppec->cchwFullName = cchwFullName;
	ppec->pchwUniqueName = pchwUniqueName;
	ppec->cchwUniqueName = cchwUniqueName;
	ppec->pchwPostscriptName = pchwPSName;
	ppec->cchwPostscriptName = cchwPSName;
	
	return true;
}

/*----------------------------------------------------------------------------------------------
	Add feature and setting names to the name table. Also modify the family and full font names
	if pchwFamilyName is not null. 
	The method also copies names from pNameTbl into pNewTbl. The feature data is in the three
	vectors passed in which must parallel each other. The don't need to be sorted. 
----------------------------------------------------------------------------------------------*/
bool GrcManager::AddFeatsModFamilyAux(uint8 * pTblOld, uint32 /*cbTblOld*/, 
	uint8 * pTblNew, uint32 /*cbTblNew*/,
	std::vector<std::wstring> & vstuExtNames, std::vector<uint16> & vnLangIds,
	std::vector<uint16> & vnNameTblIds, 
	uint16 * pchwFamilyName, uint16 cchwFamilyName, std::vector<PlatEncChange> & vpec,
	int nNameTblMinNew)
{
    #pragma pack(1)
	// Struct used to store data from directory entries that need to be sorted.
	// Must match the NameRecord struct exactly.
	struct NameTblEntry
	{
		uint16 nPlatId;
		uint16 nEncId;
		uint16 nLangId;
		uint16 nNameId;
		uint16 cLength;
		uint16 ibOffset;
		static int Compare(const void * ptr1, const void * ptr2)
		{
			NameTblEntry * p1 = (NameTblEntry *) ptr1;
			NameTblEntry * p2 = (NameTblEntry *) ptr2;

			if (p1->nPlatId != p2->nPlatId)
				return read(p1->nPlatId) - read(p2->nPlatId);
			else if (p1->nEncId != p2->nEncId)
				return read(p1->nEncId) - read(p2->nEncId);
			else if (p1->nLangId != p2->nLangId)
				return read(p1->nLangId) - read(p2->nLangId);
			else
				return read(p1->nNameId) - read(p2->nNameId);
		}
	};

	FontNames * pTbl = (FontNames *)(pTblOld);
	uint16 crecOld = read(pTbl->count);
	uint16 ibStrOffsetOld = read(pTbl->string_offset);
	NameRecord * pOldRecord = pTbl->name_record;

	uint16 crecNew = crecOld + (vstuExtNames.size() * vpec.size());

	NameRecord * pNewRecord;

	// name table header
	((FontNames *)pTblNew)->format = pTbl->format;
	((FontNames *)pTblNew)->count = read(crecNew);
	uint16 ibStrOffsetNew = sizeof(FontNames) + (crecNew - 1) * sizeof(NameRecord);
	((FontNames *)pTblNew)->string_offset = read(ibStrOffsetNew);

    pNewRecord = ((FontNames *)pTblNew)->name_record;

	uint8 * pbNextString = pTblNew + ibStrOffsetNew;
	uint16 dibNew = 0; // delta offset

	utf16 rgch16[512];	// buffer for converting between wchar_t and utf16;

	// First copy the old records, changing any font names as needed.

	int ipec;

#ifndef NDEBUG
	for (ipec = 0; ipec < signed(vpec.size()) - 1; ipec++)
	{
		Assert((vpec[ipec].platformID < vpec[ipec + 1].platformID)
				|| (vpec[ipec].platformID == vpec[ipec + 1].platformID
					&& (vpec[ipec].encodingID < vpec[ipec + 1].encodingID ||
					    (vpec[ipec].encodingID == vpec[ipec + 1].encodingID &&
					     vpec[ipec].engLangID < vpec[ipec+1].engLangID))));
	}
#endif

	ipec = 0;
	int irec = 0; // scope of this variable is larger than for-loop below.
	for ( ; irec < crecOld; irec++)
	{
		PlatEncChange * ppec = &(vpec[ipec]);
		while(ipec < signed(vpec.size())
			&& (ppec->platformID < read(pOldRecord[irec].platform_id)
				|| (ppec->platformID == read(pOldRecord[irec].platform_id)
					&& ppec->encodingID < read(pOldRecord[irec].platform_specific_id))))
		{
			ipec++;
			ppec = &(vpec[ipec]);
		}

		pNewRecord[irec].platform_id = pOldRecord[irec].platform_id;
		pNewRecord[irec].platform_specific_id = pOldRecord[irec].platform_specific_id;
		pNewRecord[irec].language_id = pOldRecord[irec].language_id;
		pNewRecord[irec].name_id = pOldRecord[irec].name_id;
		pNewRecord[irec].length = pOldRecord[irec].length;
		//pNewRecord[irec].offset = ibStrOffsetNew + dibNew;

		size_t cchwStr = 0;
        uint16 cbStr = 0;
		uint8 * pbStr = NULL;
		if (ipec < signed(vpec.size())
			&& ppec->pchwFullName // this is a platform+encoding where we need to change the font
			&& ppec->platformID == read(pOldRecord[irec].platform_id)
			&& ppec->encodingID == read(pOldRecord[irec].platform_specific_id)
			&& ppec->engLangID == read(pOldRecord[irec].language_id))
		{
			uint16 nameID = read(pOldRecord[irec].name_id);
			switch (nameID)
			{
			case n_family:
			case n_preferredfamily:
				pbStr = (uint8 *)pchwFamilyName;
				cchwStr = cchwFamilyName;
				break;
			case n_fullname:
			case n_compatiblefull:
				pbStr = (uint8 *)vpec[ipec].pchwFullName;
				cchwStr = vpec[ipec].cchwFullName;
				break;
			case n_uniquename:
				pbStr = (uint8 *)vpec[ipec].pchwUniqueName;
				cchwStr = vpec[ipec].cchwUniqueName;
				break;
			case n_postscript:
				pbStr = (uint8 *)vpec[ipec].pchwPostscriptName;
				cchwStr = vpec[ipec].cchwPostscriptName;
				break;
			default:
				break;
			}
		}
		if (pbStr)
		{
			// Copy in the new string.
			cbStr = cchwStr * vpec[ipec].cbBytesPerChar;
			pNewRecord[irec].length = read(cbStr);
			if (ppec->cbBytesPerChar == sizeof(utf16))
			{
				memcpy(pbNextString, pbStr, cbStr);
				TtfUtil::SwapWString(pbNextString, cbStr / sizeof(utf16)); // make big endian
			}
			else
			{
				// TODO: properly handle Macintosh encoding
				Platform_UnicodeToANSI((utf16 *)pbStr, cchwStr, (char *)pbNextString, cchwStr);
			}
		}
		else
		{
			// Copy the old string.
			cbStr = read(pOldRecord[irec].length);
			pbStr = pTblOld + ibStrOffsetOld + read(pOldRecord[irec].offset);
			memcpy(pbNextString, pbStr, cbStr);
		}
		pNewRecord[irec].offset = read(dibNew);
		dibNew += cbStr;
		pbNextString += cbStr;
	}

	// Then add the feature strings. Do this for all platform-encodings of interest.
	// The first time through, store the string data in the buffer and record the offsets
	// (do this once for 8-bit versions and once for 16-bit). For any subsequent platforms
	// and encodings, just use the same offsets.

	std::vector<uint16> vdibOffsets8, vdibOffsets16;
	bool fStringsStored8 = false;
	bool fStringsStored16 = false;
	for (ipec = 0; ipec < signed(vpec.size()); ipec++)
	{
		PlatEncChange * ppec = &(vpec[ipec]);
		bool f8bit = (ppec->cbBytesPerChar != sizeof(utf16));
		for (size_t istring = 0; istring < vstuExtNames.size(); istring++)
		{
			pNewRecord[irec].platform_id = read(ppec->platformID);
			pNewRecord[irec].platform_specific_id = read(ppec->encodingID);
			pNewRecord[irec].language_id = read(vnLangIds[istring]);
			pNewRecord[irec].name_id = read(vnNameTblIds[istring]);
			uint16 cbStr = vstuExtNames[istring].length() * ppec->cbBytesPerChar;

			// Convert the language IDs appropriately for the platform.
			uint16 platform_id = pNewRecord[irec].platform_id;
			uint16 specific_id = pNewRecord[irec].platform_specific_id;
			uint16 language_id = pNewRecord[irec].language_id;
#if !WORDS_BIGENDIAN
			platform_id = rev16(platform_id);
			specific_id = rev16(specific_id);
			language_id = rev16(language_id);
#endif
			if (platform_id == NameRecord::Macintosh && specific_id == 0) // Roman
			{
				// TODO: Convert common Windows language codes to Macintosh
				if (language_id != LG_USENG)
				{
					char rgch[20];
					itoa(language_id, rgch, 10);
					g_errorList.AddWarning(9999, NULL,
						"Windows language ID ", rgch,
						" cannot be converted to Macintosh; 0 (English) will be used");
				}
				pNewRecord[irec].language_id = 0;	// Macintosh English
			}
			else if (platform_id == 0 && specific_id == 3) // Unicode, >= 2.0
			{
				if (language_id != LG_USENG)
				{
					char rgch[20];
					itoa(language_id, rgch, 10);
					g_errorList.AddWarning(9999, NULL,
						"Windows language ID ", rgch,
						" cannot be translated to Unicode 2.0; language 0 will be used");
				}
				pNewRecord[irec].language_id = 0;	// whatever, this is our best guess
			}

			// Convert from wchar_t to 16-bit chars.
			std::wstring stuWchar = vstuExtNames[istring];
			int cchw = stuWchar.length();
			std::copy(stuWchar.data(), stuWchar.data() + cchw, rgch16);

			if (f8bit && !fStringsStored8)
			{
				vdibOffsets8.push_back(dibNew);
				// TODO: properly handle Macintosh encoding
				Platform_UnicodeToANSI(rgch16, cbStr, (char *)pbNextString, cbStr);
				dibNew += cbStr;
				pbNextString += cbStr;
			}
			else if (!f8bit && !fStringsStored16)
			{
				vdibOffsets16.push_back(dibNew);
				memcpy(pbNextString, rgch16, cbStr);
				TtfUtil::SwapWString(pbNextString, cbStr / sizeof(utf16)); // make big endian
				dibNew += cbStr;
				pbNextString += cbStr;
			}
			// else the string has already been stored, and we've saved the offset.

			pNewRecord[irec].length = read(cbStr);
			pNewRecord[irec].offset = (f8bit) ?
				read(vdibOffsets8[istring]):
				read(vdibOffsets16[istring]);
			irec++;
		}

		(f8bit) ?
			fStringsStored8 = true:
			fStringsStored16 = true;
	}

	Assert(irec == crecNew);

	// Sort list of entries.
	::qsort(reinterpret_cast<NameTblEntry *>(pNewRecord), crecNew, sizeof(NameTblEntry), NameTblEntry::Compare);

	return true;
}

/*----------------------------------------------------------------------------------------------
	Write the OS/2 table to the control file, using the table from the minimal font but
	copying key fields from the source font.

		0-1		USHORT	version	= 0x0003 
		2-3		SHORT	xAvgCharWidth   
		4-5		USHORT	usWeightClass   
		6-7		USHORT	usWidthClass   
		8-9		USHORT	fsType   
		10-29	SHORT	various offsets and sizes [10]   
		30-31	SHORT	sFamilyClass   
		32-41	BYTE	panose[10]   
		42-57	ULONG	ulUnicodeRange 
		58-61	CHAR	achVendID[4]   
		62-63	USHORT	fsSelection   
		64-65	USHORT	usFirstCharIndex   
		66-67	USHORT	usLastCharIndex
		...plus more stuff we don't care about
----------------------------------------------------------------------------------------------*/
#ifdef NDEBUG
bool GrcManager::OutputOS2Table(uint8 * pOs2TblSrc, uint32 /*cbOs2TblSrc*/,
#else
bool GrcManager::OutputOS2Table(uint8 * pOs2TblSrc, uint32 cbOs2TblSrc,
#endif
	uint8 * pOs2TblMin, uint32 cbOs2TblMin,
	GrcBinaryStream * pbstrm, uint32 * pcbSizeRet)
{
	Assert(cbOs2TblSrc >= 68); // number of bytes we may need to fiddle with
	Assert(cbOs2TblMin >= 68);
	uint8 * prgOs2TblNew = new uint8[cbOs2TblMin];
	if (!prgOs2TblNew)
		return false;

	// Initialize with minimal font table.
	memcpy(prgOs2TblNew, pOs2TblMin, isizeof(uint8) * cbOs2TblMin);

	// Overwrite key fields with information from the source font.

	// weight class: 16 bits (2 bytes)
	memcpy(prgOs2TblNew + 4, pOs2TblSrc + 4, isizeof(uint16));

	// fsType: 16 bits (2 bytes)
	memcpy(prgOs2TblNew + 8, pOs2TblSrc + 8, isizeof(uint16));

	// family class: 16 bits (2 bytes)
	memcpy(prgOs2TblNew + 30, pOs2TblSrc + 30, isizeof(uint16));

	// char range: 4 32-bit values (16 bytes)
	memcpy(prgOs2TblNew + 42, pOs2TblSrc + 42, isizeof(uint8)*4 * 4);

	// max Unicode value: 16 bits (2 bytes)
	memcpy(prgOs2TblNew + 64, pOs2TblSrc + 64, isizeof(uint16));

	// min Unicode value: 16 bits (2 bytes)
	memcpy(prgOs2TblNew + 66, pOs2TblSrc + 66, isizeof(uint16));

	// Write the result onto the output stream.
	pbstrm->Write((char *)prgOs2TblNew, cbOs2TblMin);
	*pcbSizeRet = cbOs2TblMin;

	delete[] prgOs2TblNew;

	return true;
}

/*----------------------------------------------------------------------------------------------
	Write the cmap table to the control file. We are outputting a copy of the cmap in the
	source file, with every supported character pointing at some bogus glyph (ie, square box).
----------------------------------------------------------------------------------------------*/
bool GrcManager::OutputCmapTable(uint8 * pCmapTblSrc, uint32 /*cbCmapTblSrc*/,
	GrcBinaryStream * pbstrm, uint32 * pcbSizeRet)
{
	long lPosStart = pbstrm->Position();

	//	See if we can get the USC-4 subtable.
	void * pCmap_3_10 = TtfUtil::FindCmapSubtable(pCmapTblSrc, 3, 10);
	
	//	Regardless, we should be able to find a 16-bit table.
	//	First try UTF-16 table: platform 3 encoding 1
	int nEncID = 1;
	void * pCmap_3_1 = TtfUtil::FindCmapSubtable(pCmapTblSrc, 3, 1);
	if (pCmap_3_1 == NULL)
	{
		// Try the platform 3 encoding 0 table instead (symbol font)
		pCmap_3_1 = TtfUtil::FindCmapSubtable(pCmapTblSrc, 3, 0);
		if (pCmap_3_1)
			nEncID = 0;
	}

	// Note that even if we found the 3-10 table, we're supposed to store a 3-1 table
	// as well for backward-compatibility;
	// see http://www.microsoft.com/typography/OTSPEC/cmap.htm#language

	// version
	pbstrm->WriteShort(0);

	// number of subtables
	long lPosTableCnt = pbstrm->Position();
	if (pCmap_3_10)
		pbstrm->WriteShort(2);	// both formats
	else
		pbstrm->WriteShort(1);	// just 16-bit

	// subtable info
	pbstrm->WriteShort(3);		// platform ID
	pbstrm->WriteShort(nEncID);	// encoding ID: 1 or 0
	long lPosOffset31 = pbstrm->Position();
	pbstrm->WriteInt(0);	// offset

	long lPos310Dir = pbstrm->Position();
	long lPosOffset310 = 0;
	if (pCmap_3_10)
	{
		pbstrm->WriteShort(3);	// platform ID
		pbstrm->WriteShort(10);	// encoding ID
		lPosOffset310 = pbstrm->Position();
		pbstrm->WriteInt(0);	// offset: fill in later
	}
	else
	{
		// Just in case we find out we have to output a 3-10 table after all, leave
		// space for it in the directory.
		pbstrm->WriteShort(0);
		pbstrm->WriteShort(0);
		pbstrm->WriteInt(0);
	}

	int ibOffset31 = pbstrm->Position() - lPosStart;
	bool fNeed310;
	OutputCmap31Table(
		(pCmap_3_1) ? pCmap_3_1 : pCmap_3_10,
		pbstrm, (pCmap_3_1 == NULL), &fNeed310);
	// fill in the offset
	long lPosEnd = pbstrm->Position();
	pbstrm->SetPosition(lPosOffset31);
	pbstrm->WriteInt(ibOffset31);

	if (fNeed310)
	{
		// We'll have to output a 3-10 table after all. Go back and put information
		// about it in the directory.
		pbstrm->SetPosition(lPosTableCnt);
		pbstrm->WriteShort(2);	// both formats

		pbstrm->SetPosition(lPos310Dir);
		pbstrm->WriteShort(3);	// platform ID
		pbstrm->WriteShort(10);	// encoding ID
		lPosOffset310 = pbstrm->Position();
		pbstrm->WriteInt(0);	// offset: fill in later
	}

	pbstrm->SetPosition(lPosEnd);

	if (pCmap_3_10 || fNeed310)
	{
		int ibOffset310 = pbstrm->Position() - lPosStart;
		OutputCmap310Table((pCmap_3_10) ? pCmap_3_10 : pCmap_3_1,
			pbstrm, (pCmap_3_10 == NULL));
		// fill in the offset
		lPosEnd = pbstrm->Position();
		pbstrm->SetPosition(lPosOffset310);
		pbstrm->WriteInt(ibOffset310);
		pbstrm->SetPosition(lPosEnd);
	}

	*pcbSizeRet = lPosEnd - lPosStart;
	return true;
}

/*----------------------------------------------------------------------------------------------
	Write the format 4.0 cmap (platform 3 encoding 1) subtable to the control file.
	Make every glyph point at the square box.
	Return the number of bytes in the table.
	Generally it is also reading from a 3-1 table as well, but if we don't have one, we
	may need to generate a 3-1 table from a 3-10 table (format 12).

	We can't output a 4.0 format to include more than 8190 [= (65535 - 14) / 8)]
	codepoints, because the length field is only 16 bits, and because of the lack of
	compression we use up 8 bytes per codepoint! So if we have more than that number, we
	have to truncate the 3-1 table, and also make sure to output a 3-10 table which can be
	quite a bit longer.
----------------------------------------------------------------------------------------------*/
int GrcManager::OutputCmap31Table(void * pCmapSubTblSrc,
	GrcBinaryStream * pbstrm, bool fFrom310, bool * pfNeed310)
{
	long lPosStart = pbstrm->Position();

	int cch = 0;	// number of codepoints in the map

	//	format
	pbstrm->WriteShort(4);

	//	length: fill in later
	int lPosLen = pbstrm->Position();
	pbstrm->WriteShort(0);

	//	language (irrelevant except on Macintosh)
	pbstrm->WriteShort(0);

	//	search variables: fill in later
	int lPosSearch = pbstrm->Position();
	pbstrm->WriteShort(0);
	pbstrm->WriteShort(0);
	pbstrm->WriteShort(0);
	pbstrm->WriteShort(0);

	//	Generate the list of supported characters.

	utf16 rgchChars[65535];
	std::vector<int> vichGroupStart;
	//std::vector<int> vcchGroupSize;
	int cGroups = 0;
	bool fTruncated = false;
	int key = 0; // for optimizing next-codepoint lookup
	int ch = (fFrom310) ?
		TtfUtil::Cmap310NextCodepoint(pCmapSubTblSrc, 0, &key) :
		TtfUtil::Cmap31NextCodepoint(pCmapSubTblSrc, 0, &key);
	while (ch < 0xFFFF)
	{
		rgchChars[cch] = ch;
		if (cch == 0 || ch != rgchChars[cch - 1] + 1)
		{
			vichGroupStart.push_back(cch);
			cGroups++;
		}

		cch++;
		ch = (fFrom310) ?
			TtfUtil::Cmap310NextCodepoint(pCmapSubTblSrc, ch, &key) :
			TtfUtil::Cmap31NextCodepoint(pCmapSubTblSrc, ch, &key);

		// If we're over the limit of what the subtable can handle, truncate.
		int cbNeeded = 14	// header
			+ (cGroups * 8)	// group information
			+ (cch * 2)		// glyphIdArray
			+ 10;			// final FFFF
		if (cbNeeded >= 0xFFFF) // 65535 is limit on subtable length
		{
			if (vichGroupStart[cGroups - 1] == cch - 1)
			{
				vichGroupStart.pop_back();
				cGroups--;
			}
			cch--;
			ch = 0xFFFF;
			fTruncated = true;
		}
	}
	rgchChars[cch] = 0xFFFF;  // this must always be last
	vichGroupStart.push_back(cch);
	cGroups++;
	cch++;

	//	Output the ranges of characters.

	//	End codes
	int iGroup;
	for (iGroup = 0; iGroup < cGroups - 1; iGroup++)
		pbstrm->WriteShort(rgchChars[vichGroupStart[iGroup+1] - 1]);
	pbstrm->WriteShort(0xFFFF);

	//	reserved pad
	pbstrm->WriteShort(0);

	//	Start codes
	for (iGroup = 0; iGroup < cGroups; iGroup++)
		pbstrm->WriteShort(rgchChars[vichGroupStart[iGroup]]);

	//	id delta: not used; pad with zeros
	for (iGroup = 0; iGroup < cGroups; iGroup++)
		pbstrm->WriteShort(0);

	//	id range array: each element holds the offset (in bytes) from the element
	//	itself down to the corresponding glyphIdArray element (this works because
	//	glyphIdArray immediately follows idRangeArray).
	int cchSoFar = 0;
	for (iGroup = 0; iGroup < cGroups - 1; iGroup++)
	{
		int nStart = rgchChars[vichGroupStart[iGroup]];
		int nEnd = rgchChars[vichGroupStart[iGroup+1] - 1];

		int nRangeOffset = (cGroups - iGroup + cchSoFar) * 2; // *2 for bytes, not wchars
		pbstrm->WriteShort(nRangeOffset);

		cchSoFar += (nEnd - nStart + 1);
	}
	pbstrm->WriteShort((1 + cchSoFar) * 2);
	cchSoFar++;

	//	glyphIdArray: all characters point to our special square box,
	//	except for the space character.
	for (int ich = 0; ich < cch; ich++)
	{
		if (rgchChars[ich] == kchwSpace)
			pbstrm->WriteShort(2);
		else if (rgchChars[ich] == 0xFFFF) // invalid
			pbstrm->WriteShort(0);
		else
			pbstrm->WriteShort(3);
	}

	long lPosEnd = pbstrm->Position();

	//	Fill in the length.
	int cb = lPosEnd - lPosStart;
	pbstrm->SetPosition(lPosLen);
	pbstrm->WriteShort(cb);

	//	Fill in the search variables; note that these are byte-based and so are multiplied by 2,
	//	unlike what is done in the Graphite tables.
	int nPowerOf2, nLog;
	BinarySearchConstants(cGroups, &nPowerOf2, &nLog);
	pbstrm->SetPosition(lPosSearch);
	pbstrm->WriteShort(cGroups << 1);	// * 2
	pbstrm->WriteShort(nPowerOf2 << 1); // * 2
	pbstrm->WriteShort(nLog);
	pbstrm->WriteShort((cGroups - nPowerOf2) << 1);  // * 2

	pbstrm->SetPosition(lPosEnd);

	if (fTruncated)
	{
		g_errorList.AddWarning(5502, NULL,
			"cmap format 4 subtable truncated--does not match format 12 subtable");
	}

	*pfNeed310 = fTruncated;

	return cb;
}

#if 0
// Old approach that uses one segment per codepoint.
int GrcManager::OutputCmap31Table(void * pCmapSubTblSrc,
	GrcBinaryStream * pbstrm, bool fFrom310, bool * pfNeed310)
{
	//	Maximum number of codepoints this table can hold, given the lack of compression
	//	we can do.
	const int cchMax = 8188; // = ((65535 - 14) / 8) - 2

	long lPosStart = pbstrm->Position();

	int cch = 0;	// number of items in the map

	//	format
	pbstrm->WriteShort(4);

	//	length: fill in later
	int lPosLen = pbstrm->Position();
	pbstrm->WriteShort(0);

	//	language (irrelevant except on Macintosh)
	pbstrm->WriteShort(0);

	//	search variables: fill in later
	int lPosSearch = pbstrm->Position();
	pbstrm->WriteShort(0);
	pbstrm->WriteShort(0);
	pbstrm->WriteShort(0);
	pbstrm->WriteShort(0);

	//	Generate the list of supported characters.

	utf16 rgchChars[65535];
	int key = 0; // for optimizing next-codepoint lookup
	int ch = (fFrom310) ?
		TtfUtil::Cmap310NextCodepoint(pCmapSubTblSrc, 0, &key) :
		TtfUtil::Cmap31NextCodepoint(pCmapSubTblSrc, 0, &key);
	while (ch < 0xFFFF)
	{
		rgchChars[cch] = ch;
		cch++;
		ch = (fFrom310) ?
			TtfUtil::Cmap310NextCodepoint(pCmapSubTblSrc, ch, &key) :
			TtfUtil::Cmap31NextCodepoint(pCmapSubTblSrc, ch, &key);
	}
	rgchChars[cch] = 0xFFFF;  // this must always be last
	cch++;

	//	Only output the maximum we can handle; see comment above.
	int cchOutput;
	if (cch > cchMax)
	{
		cchOutput = cchMax;
		//	If we're truncating this table, we need a 3-10 table that can
		//	handle it all properly.
		*pfNeed310 = true;
		rgchChars[cchOutput - 1] = 0xFFFF; // must be the last character
	}
	else
	{
		cchOutput = cch;
		*pfNeed310 = false;
		Assert(rgchChars[cchOutput - 1] == 0xFFFF);
	}

	//	Output the ranges of characters. Each character is in its own range.

	//	End codes
	int ich;
	for (ich = 0; ich < cchOutput; ich++)
		pbstrm->WriteShort(rgchChars[ich]);

	//	reserved pad
	pbstrm->WriteShort(0);

	//	Start codes
	for (ich = 0; ich < cchOutput; ich++)
		pbstrm->WriteShort(rgchChars[ich]);

	//	id delta: set so that all the mappings point to glyph 0
	for (ich = 0; ich < cchOutput; ich++)
	{
		if (rgchChars[ich] == kchwSpace)
			pbstrm->WriteShort((rgchChars[ich] * - 1) + 2); // space is glyph #2; TODO: make this match minimal font
		else
			pbstrm->WriteShort(rgchChars[ich] * -1);
	}

	//	id range array: not used
	for (ich = 0; ich < cchOutput; ich++)
		pbstrm->WriteShort(0);

	long lPosEnd = pbstrm->Position();

	//	Fill in the length.
	int cb = lPosEnd - lPosStart;
	pbstrm->SetPosition(lPosLen);
	pbstrm->WriteShort(cb);

	//	Fill in the search variables; note that these are byte-based and so are multiplied by 2,
	//	unlike what is done in the Graphite tables.
	int nPowerOf2, nLog;
	BinarySearchConstants(cchOutput, &nPowerOf2, &nLog);
	pbstrm->SetPosition(lPosSearch);
	pbstrm->WriteShort(cchOutput << 1);	// * 2
	pbstrm->WriteShort(nPowerOf2 << 1); // * 2
	pbstrm->WriteShort(nLog);
	pbstrm->WriteShort((cchOutput - nPowerOf2) << 1);  // * 2

	pbstrm->SetPosition(lPosEnd);

	return cb;
}
#endif

/*----------------------------------------------------------------------------------------------
	Write the format 12 (platform 3 encoding 10) cmap subtable to the control file.
	Make every glyph point at the square box. Return the number of bytes in the table.

	Normally we will be generating the 3-10 table from the original 3-10 table. But in the
	case of a source font with a very large number of glyphs but no 3-10 table, we will
	generate it from the 3-1 table.
----------------------------------------------------------------------------------------------*/
int GrcManager::OutputCmap310Table(void * pCmapSubTblSrc, GrcBinaryStream * pbstrm,
	bool fFrom31)
{
	long lPosStart = pbstrm->Position();

	int cch = 0;	// number of items in the map

	//	format
	pbstrm->WriteShort(12);

	//	reserved
	pbstrm->WriteShort(0);

	//	length: fill in later
	int lPosLen = pbstrm->Position();
	pbstrm->WriteInt(0);

	//	language (irrelevant except on Macintosh)
	pbstrm->WriteInt(0);

	//	number of groups: fill in later
	int lPosNum = pbstrm->Position();
	pbstrm->WriteInt(0);

	//	Output the ranges of characters. Each character is in its own group.

	unsigned int chLast = (fFrom31) ? 0xFFFF : 0x10FFFF;

	int key = 0; // for optimizing next-codepoint lookup
	unsigned int ch = (fFrom31) ?
		TtfUtil::Cmap31NextCodepoint(pCmapSubTblSrc, 0, &key) :
		TtfUtil::Cmap310NextCodepoint(pCmapSubTblSrc, 0, &key);
	while (ch != chLast)
	{
		//	start code
		pbstrm->WriteInt(ch);
		//	end code
		pbstrm->WriteInt(ch);
		//	glyph id
		if (ch == kchwSpace)
			pbstrm->WriteInt(2); // space is glyph #2; TODO: make this match minimal font
		else
			pbstrm->WriteInt(0);

		cch++;

		ch = (fFrom31) ?
			TtfUtil::Cmap31NextCodepoint(pCmapSubTblSrc, ch, &key) :
			TtfUtil::Cmap310NextCodepoint(pCmapSubTblSrc, ch, &key);
	}

	int lPosEnd = pbstrm->Position();

	//	Fill in the length.
	int cb = lPosEnd - lPosStart;
	pbstrm->SetPosition(lPosLen);
	pbstrm->WriteInt(cb);

	//	Fill in the number of groups.
	pbstrm->SetPosition(lPosNum);
	pbstrm->WriteInt(cch);

	pbstrm->SetPosition(lPosEnd);

	return cb;
}

/*----------------------------------------------------------------------------------------------
	Write the Sile table onto the stream.
----------------------------------------------------------------------------------------------*/
void GrcManager::OutputSileTable(GrcBinaryStream * pbstrm,
	 utf16 * pchSrcFontFamily, char * pchSrcFileName, unsigned int luMasterChecksum,
	 unsigned int * pnCreateTime, unsigned int * pnModifyTime,
	 int * pibOffset, int * pcbSize)
{
	*pibOffset = pbstrm->Position();

	// version
	int fxd = VersionForTable(ktiSile);
	pbstrm->WriteInt(fxd);

	// master check sum from source font
	pbstrm->WriteInt(luMasterChecksum);

	// create and modify times of source font
	pbstrm->WriteInt(pnCreateTime[0]);
	pbstrm->WriteInt(pnCreateTime[1]);
	pbstrm->WriteInt(pnModifyTime[0]);
	pbstrm->WriteInt(pnModifyTime[1]);

	// source font family name
	int cchFontFamily = utf16len(pchSrcFontFamily);
	pbstrm->WriteShort(cchFontFamily);
	for (int ich = 0; ich < cchFontFamily; ich++)
		pbstrm->WriteShort(pchSrcFontFamily[ich]);
	pbstrm->WriteByte(0);

	// source font file
	int cchFile = strlen(pchSrcFileName);
	pbstrm->WriteShort(cchFile);
	pbstrm->Write(pchSrcFileName, cchFile);
	pbstrm->WriteByte(0);

	// handle size and padding
	long lPos = pbstrm->Position();
	*pcbSize = lPos - *pibOffset;
	pbstrm->SeekPadLong(lPos);
}

/*----------------------------------------------------------------------------------------------
	Write the Gloc and Glat tables onto the stream.
----------------------------------------------------------------------------------------------*/
void GrcManager::OutputGlatAndGloc(GrcBinaryStream * pbstrm,
	int * pnGlocOffset, int * pnGlocSize, int * pnGlatOffset, int * pnGlatSize)
{
	int * prgibGlyphOffsets = new int[m_cwGlyphIDs + 1];
	memset(prgibGlyphOffsets, 0, isizeof(int)  * (m_cwGlyphIDs + 1));

	Symbol psymBw = m_psymtbl->FindSymbol("breakweight");
	int nAttrIdBw = psymBw->InternalID();
	//Symbol psymJStr = m_psymtbl->FindSymbol(GrcStructName("justify", "0", "stretch"));
	Symbol psymJStr = m_psymtbl->FindSymbol(GrcStructName("justify", "stretch"));
	int nAttrIdJStr = psymJStr->InternalID();

	//	Output the Glat table, recording the offsets in the array.

	*pnGlatOffset = pbstrm->Position();
	int fxdGlatVersion = VersionForTable(ktiGlat);
	//	The version of the Glat table really just depends on the number of glyph attributes defined.
	if (m_vpsymGlyphAttrs.size() >= kMaxGlyphAttrsGlat1 && fxdGlatVersion < 0x00020000)
	{
		g_errorList.AddWarning(3531, NULL, "Version 2.0 of the Glat table will be generated.");
		fxdGlatVersion = 0x00020000;
	}
	SetTableVersion(ktiGlat, fxdGlatVersion);
	pbstrm->WriteInt(fxdGlatVersion);	// version number

	int cbOutput = 4;	// first glyph starts after the version number

	int wGlyphID;
	for (wGlyphID = 0; wGlyphID < m_cwGlyphIDs; wGlyphID++)
	{
		prgibGlyphOffsets[wGlyphID] = cbOutput;

		//	Convert breakweight values depending on the table version to output.
		ConvertBwForVersion(wGlyphID, nAttrIdBw);

		//	Split any large stretch values into two 16-bit words.
		SplitLargeStretchValue(wGlyphID, nAttrIdJStr);
		
		int nAttrIDMin = 0;
		while (nAttrIDMin < signed(m_vpsymGlyphAttrs.size()))
		{
			int nValue;

			//	Skip undefined and zero-valued attributes.
			while (nAttrIDMin < signed(m_vpsymGlyphAttrs.size()) &&
				FinalAttrValue(wGlyphID, nAttrIDMin) == 0)
			{
				nAttrIDMin++;
			}

			int nAttrIDLim = nAttrIDMin;
			std::vector<int> vnValues;
			while (nAttrIDLim < signed(m_vpsymGlyphAttrs.size()) &&
				(nAttrIDLim - nAttrIDMin + 1) < 256 &&
				((nValue = FinalAttrValue(wGlyphID, nAttrIDLim)) != 0))
			{
				nAttrIDLim++;
				//	TODO SharonC: Test that the value fits in 16 bits.
				vnValues.push_back(nValue);
			}

			//	We've found a run of non-zero attributes. Output them.

			Assert(nAttrIDLim - nAttrIDMin == static_cast<int>(vnValues.size()));
			
			if (nAttrIDLim > nAttrIDMin)
			{
				int cbHeader;
				if (fxdGlatVersion < 0x00020000)
				{
					Assert(nAttrIDLim - nAttrIDMin < 256);
					pbstrm->WriteByte(nAttrIDMin);
					pbstrm->WriteByte(nAttrIDLim - nAttrIDMin);
					cbHeader = 2;
				}
				else
				{
					Assert(nAttrIDLim - nAttrIDMin < 65536);
					pbstrm->WriteShort(nAttrIDMin);
					pbstrm->WriteShort(nAttrIDLim - nAttrIDMin);
					cbHeader = 4;
				}
				for (size_t i = 0; i < vnValues.size(); i++)
					pbstrm->WriteShort(vnValues[i]);
				cbOutput += (vnValues.size() * 2) + cbHeader;
			}

			nAttrIDMin = nAttrIDLim;
		}
	}

	//	Final offset to give total length.
	prgibGlyphOffsets[m_cwGlyphIDs] = cbOutput;

	// handle size and padding
	int nTmp = pbstrm->Position();
	*pnGlatSize = nTmp - *pnGlatOffset;
	pbstrm->SeekPadLong(nTmp);

	//	Output the Gloc table.
	*pnGlocOffset = pbstrm->Position();

	int fxd = VersionForTable(ktiGloc);
	pbstrm->WriteInt(fxd);	// version number
	SetTableVersion(ktiGloc, fxd);

	//	flags
	utf16 wFlags = 0;
	bool fNeedLongFormat = (cbOutput >= 0x0000FFFF);
	bool fAttrNames = false;
	if (fNeedLongFormat)
		wFlags |= 0x0001;
	if (fAttrNames)
		wFlags |= 0x0002;
	pbstrm->WriteShort(wFlags);

	//	number of attributes
	pbstrm->WriteShort(m_vpsymGlyphAttrs.size());

	//	offsets
	for (wGlyphID = 0; wGlyphID <= m_cwGlyphIDs; wGlyphID++)
	{
		if (fNeedLongFormat)
			pbstrm->WriteInt(prgibGlyphOffsets[wGlyphID]);
		else
			pbstrm->WriteShort(prgibGlyphOffsets[wGlyphID]);

		// Just in case, since incrementing 0xFFFF will produce zero.
		if (wGlyphID == 0xFFFF)
			break;
	}

	// handle size and padding
	nTmp = pbstrm->Position();
	*pnGlocSize = nTmp - *pnGlocOffset;
	pbstrm->SeekPadLong(nTmp);

	//	debug names
	Assert(!fAttrNames);

	delete[] prgibGlyphOffsets;
}


/*----------------------------------------------------------------------------------------------
	Return the final attribute value, resolved to an integer.
----------------------------------------------------------------------------------------------*/
int GrcManager::FinalAttrValue(utf16 wGlyphID, int nAttrID)
{
	if (m_cpsymBuiltIn <= nAttrID && nAttrID < m_cpsymBuiltIn + m_cpsymComponents)
	{
		Assert(!m_pgax->Defined(wGlyphID, nAttrID));

		if (!m_plclist->FindComponentFor(wGlyphID, nAttrID))
			return 0;
		else
			// What does this really mean??
			return m_cpsymBuiltIn + m_cpsymComponents + (kFieldsPerComponent * nAttrID);
	}
	else
	{
		if (!m_pgax->Defined(wGlyphID, nAttrID))
			return 0;
		GdlExpression * pexpValue = m_pgax->GetExpression(wGlyphID, nAttrID);
		if (pexpValue == NULL)
			return 0;
		int nValue;
		bool f = pexpValue->ResolveToInteger(&nValue, false);
		Assert(f);
		if (!f)
			return 0;
		return nValue;
	}
}

/*----------------------------------------------------------------------------------------------
	Convert the breakweight value to the one appropriate for the version of the table
	being generated.
----------------------------------------------------------------------------------------------*/
void GrcManager::ConvertBwForVersion(int wGlyphID, int nAttrIdBw)
{
	int lb = FinalAttrValue(wGlyphID, nAttrIdBw);
	int lbOut;
	if (VersionForTable(ktiSilf) < 0x00020000)
	{
		switch(lb)
		{
		//case klbWsBreak:		lbOut = klbv1WordBreak; break;
		case klbWordBreak:		lbOut = klbv1WordBreak; break;
		case klbLetterBreak:	lbOut = klbv1LetterBreak; break;
		case klbClipBreak:		lbOut = klbv1ClipBreak; break;
		default:
			lbOut = lb;
		}
	}
	else
	{
		switch(lb)
		{
		//case klbWsBreak:		lbOut = klbv2WsBreak; break;
		case klbWordBreak:		lbOut = klbv2WordBreak; break;
		case klbLetterBreak:	lbOut = klbv2LetterBreak; break;
		case klbClipBreak:		lbOut = klbv2ClipBreak; break;
		default:
			lbOut = lb;
		}
	}
	if (lb != lbOut)
	{
		GdlExpression * pexpOld;
		int nPR;
		int munitPR;
		bool fOverride, fShadow;
		GrpLineAndFile lnf;
		m_pgax->Get(wGlyphID, nAttrIdBw,
			&pexpOld, &nPR, &munitPR, &fOverride, &fShadow, &lnf);

		GdlExpression * pexpNew = new GdlNumericExpression(lbOut);
		m_vpexpModified.push_back(pexpNew);
		pexpNew->CopyLineAndFile(*pexpOld);

		m_pgax->Set(wGlyphID, nAttrIdBw,
			pexpNew, nPR, munitPR, true, false, lnf);
	}
}

/*----------------------------------------------------------------------------------------------
	Convert the specification version number requested by the user into the actual
	version for the given table.

	@param	ti		- table index
----------------------------------------------------------------------------------------------*/
int GrcManager::VersionForTable(int ti)
{
	return VersionForTable(ti, SilfTableVersion());
}

int GrcManager::VersionForTable(int ti, int fxdSpecVersion)
{
	switch (ti)
	{
	case ktiGloc:
	case ktiGlat:
		return 0x00010000;
	case ktiFeat:
		return m_fxdFeatVersion;
	case ktiSile:
		return 0x00010000;
	case ktiSill:
		return 0x00010000;
	case ktiSilf:
		if (fxdSpecVersion == 0x00010000)
			return 0x00010000;
		else if (fxdSpecVersion <= kfxdMaxSilfVersion)
			return fxdSpecVersion;
		else
			// No version specified, or an invalid version:
			return g_cman.DefaultSilfVersion();
	default:
		Assert(false);
	}
	return fxdSpecVersion;
}

/*----------------------------------------------------------------------------------------------
	Return the version of the action commands to use for the rules.
----------------------------------------------------------------------------------------------*/
int GrcManager::VersionForRules()
{
	return VersionForTable(ktiSilf); // somebody these may be different
}

/*----------------------------------------------------------------------------------------------
	Set and return the version of the Silf table that is needed to handle the size of
	the class map (replacement class data).
----------------------------------------------------------------------------------------------*/
int GrcManager::SilfVersionForClassMap(int fxdSilfSpecVersion)
{
	
	int cbSpaceNeeded;	// # of bytes needed = max offset

	//	number of classes
	cbSpaceNeeded = 4;

	//	the offsets to classes themselves, assuming short ints
	cbSpaceNeeded += (m_vpglfcReplcmtClasses.size() + 1) * 2;

	//	Space needed for the class glyph lists

	for (int ipglfc = 0; ipglfc < m_cpglfcLinear; ipglfc++)
	{
		GdlGlyphClassDefn * pglfc = m_vpglfcReplcmtClasses[ipglfc];

		Assert(pglfc->ReplcmtOutputClass() || pglfc->GlyphIDCount() <= 1);
		//Assert(pglfc->ReplcmtOutputID() == cTmp);

		std::vector<utf16> vwGlyphs;
		pglfc->GenerateOutputGlyphList(vwGlyphs);

		cbSpaceNeeded += vwGlyphs.size() * 2;
	}

	//	indexed classes (input)
	for (int ipglfc = m_cpglfcLinear; ipglfc < signed(m_vpglfcReplcmtClasses.size()); ipglfc++)
	{
		GdlGlyphClassDefn * pglfc = m_vpglfcReplcmtClasses[ipglfc];

		Assert(pglfc->ReplcmtInputClass());
		//Assert(pglfc->ReplcmtInputID() == cTmp);

		std::vector<utf16> vwGlyphs;
		std::vector<int> vnIndices;
		pglfc->GenerateInputGlyphList(vwGlyphs, vnIndices);
		int n = signed(vwGlyphs.size());
		cbSpaceNeeded += 8;	// search constants
		cbSpaceNeeded += vwGlyphs.size() * 4;
	}

	if (cbSpaceNeeded > 0xFFFF)
	{
		// Offsets won't all fit in short ints; we need long ints.
		SetSilfTableVersion(0x00040000, false);
		if (fxdSilfSpecVersion != 0x00040000)
		{
			if (UserSpecifiedVersion())
				g_errorList.AddWarning(5504, NULL,
					"Version ",
					VersionString(fxdSilfSpecVersion),
					" of the Silf table is inadequate for your specification; version ",
					VersionString(0x00040000),
					" will be generated instead.");
			else
				g_errorList.AddWarning(5505, NULL,
					"Version ",
					VersionString(0x00040000),
					" of the Silf table will be generated.");

		}
		return 0x00040000;
	}
	else
	{
		return fxdSilfSpecVersion;
	}

}

/*----------------------------------------------------------------------------------------------
	Split any really large stretch values into two 16-bit words. Store the high word in the
	stretchHW attribute. These are guaranteed to follow the list of stretch attributes (see
	GrcSymbolTable::AssignInternalGlyphAttrIDs).
----------------------------------------------------------------------------------------------*/
void GrcManager::SplitLargeStretchValue(int wGlyphID, int nAttrIdJStr)
{
	int cJLevels = NumJustLevels();

	for (int nJLev = 0; nJLev < cJLevels; nJLev++)
	{
		unsigned int nStretch = FinalAttrValue(wGlyphID, nAttrIdJStr + nJLev);
		if (nStretch > 0x0000FFFF)
		{
			unsigned int nStretchLW = nStretch & 0x0000FFFF;
			unsigned int nStretchHW = (nStretch & 0xFFFF0000) >> 16;

			int nAttrIdJStrHW = nAttrIdJStr + cJLevels;

			GdlExpression * pexpOld;
			int nPR;
			int munitPR;
			bool fOverride, fShadow;
			GrpLineAndFile lnf;
			m_pgax->Get(wGlyphID, nAttrIdJStr,
				&pexpOld, &nPR, &munitPR, &fOverride, &fShadow, &lnf);

			GdlExpression * pexpNew = new GdlNumericExpression(nStretchLW);
			m_vpexpModified.push_back(pexpNew);
			pexpNew->CopyLineAndFile(*pexpOld);
			m_pgax->Set(wGlyphID, nAttrIdJStr,
				pexpNew, nPR, munitPR, true, false, lnf);

			pexpNew = new GdlNumericExpression(nStretchHW);
			m_vpexpModified.push_back(pexpNew);
			pexpNew->CopyLineAndFile(*pexpOld);
			m_pgax->Set(wGlyphID, nAttrIdJStrHW,
				pexpNew, nPR, munitPR, true, false, lnf);
		}
	}
}

/*----------------------------------------------------------------------------------------------
	Write the Silf table and related subtables to the stream.
----------------------------------------------------------------------------------------------*/
void GrcManager::OutputSilfTable(GrcBinaryStream * pbstrm, int * pnSilfOffset, int * pnSilfSize)
{
	*pnSilfOffset = pbstrm->Position();

	int fxdSilfVersion = VersionForTable(ktiSilf);
	fxdSilfVersion = SilfVersionForClassMap(fxdSilfVersion);
	SetTableVersion(ktiSilf, fxdSilfVersion);

	//	version number
	pbstrm->WriteInt(fxdSilfVersion);

	SetCompilerVersionFor(fxdSilfVersion);

	//	compiler version
	if (fxdSilfVersion >= 0x00030000)
	{	
		pbstrm->WriteInt(CompilerVersion());
	}

	//	number of sub-tables
	pbstrm->WriteShort(1);
	if (fxdSilfVersion >= 0x00030000)
	{
		//	reserved - pad bytes
		pbstrm->WriteShort(0);
		//	offset of zeroth table relative to the start of this table
		pbstrm->WriteInt(isizeof(int) * 3 + isizeof(utf16) * 2);
	}
	else if (fxdSilfVersion >= 0x00020000)
	{
		//	reserved - pad bytes
		pbstrm->WriteShort(0);
		//	offset of zeroth table relative to the start of this table
		pbstrm->WriteInt(isizeof(int) * 2 + isizeof(utf16) * 2);
	}
	else
		//	offset of zeroth table relative to the start of this table
		pbstrm->WriteInt(isizeof(int) + isizeof(utf16) + isizeof(int));

	//	Sub-table

	long lTableStartSub = pbstrm->Position();

	//	rule version - right now this is the same as the table version
	if (fxdSilfVersion >= 0x00030000)
		pbstrm->WriteInt(fxdSilfVersion);

	long lOffsetsPos = pbstrm->Position();
	if (fxdSilfVersion >= 0x00030000)
	{
		// Place holders for offsets to passes and pseudo-glyphs.
		pbstrm->WriteShort(0);
		pbstrm->WriteShort(0);
	}

	//	maximum valid glyph ID
	pbstrm->WriteShort(m_cwGlyphIDs - 1);

	//	extra ascent
	GdlNumericExpression * pexp = m_prndr->ExtraAscent();
	if (pexp)
		pbstrm->WriteShort(pexp->Value());
	else
		pbstrm->WriteShort(0);
	//	extra descent
	pexp = m_prndr->ExtraDescent();
	if (pexp)
		pbstrm->WriteShort(pexp->Value());
	else
		pbstrm->WriteShort(0);

	//	number of passes
	int cpass, cpassLB, cpassSub, cpassPos, cpassJust, ipassBidi;
	m_prndr->CountPasses(&cpass, &cpassLB, &cpassSub, &cpassJust, &cpassPos, &ipassBidi);
	pbstrm->WriteByte(cpass);
	//	first substitution pass
	pbstrm->WriteByte(cpassLB);
	//	first positioning pass
	pbstrm->WriteByte(cpassLB + cpassSub + cpassJust);
	//	first justification pass
	if (fxdSilfVersion < 0x00020000)
		pbstrm->WriteByte(cpass);
	else
		pbstrm->WriteByte(cpassLB + cpassSub);
	//	index of bidi pass
	pbstrm->WriteByte(ipassBidi);

	//	line-break flags
	pbstrm->WriteByte(m_prndr->LineBreakFlags() & (fxdSilfVersion < 0x00030000 ? 1 : 3));
	//	ranges for cross-line-boundary contextualization
	pbstrm->WriteByte(m_prndr->PreXlbContext());
	pbstrm->WriteByte(m_prndr->PostXlbContext());

	//	the fake glyph attribute that is used to store the actual glyph ID for pseudo-glyphs
	Symbol psym = m_psymtbl->FindSymbol("*actualForPseudo*");
	pbstrm->WriteByte(psym->InternalID());
	//	breakweight attribute ID
	psym = m_psymtbl->FindSymbol("breakweight");
	pbstrm->WriteByte(psym->InternalID());
	//	directionality attribute ID
	psym = m_psymtbl->FindSymbol("directionality");
	pbstrm->WriteByte(psym->InternalID());
	if (fxdSilfVersion >= 0x00020000)
	{
		// mirror attribute ID
		psym = m_psymtbl->FindSymbol(GrcStructName("mirror", "glyph"));
		pbstrm->WriteByte(psym->InternalID());

		// reserved (pad byte)
		pbstrm->WriteByte(0);

		if (m_fBasicJust)
		{
			pbstrm->WriteByte(0);	// number of levels
		}
		else
		{
			//	number of levels
			pbstrm->WriteByte(1);
			//	justify.0.stretch attribute ID
			GrcStructName xnsJStretch("justify", "stretch");
			psym = m_psymtbl->FindSymbol(xnsJStretch);
			Assert(psym);
			pbstrm->WriteByte(psym->InternalID());
			//	justify.0.shrink attribute ID
			GrcStructName xnsJShrink("justify", "shrink");
			psym = m_psymtbl->FindSymbol(xnsJShrink);
			Assert(psym);
			pbstrm->WriteByte(psym->InternalID());
			//	justify.0.step attribute ID
			GrcStructName xnsJStep("justify", "step");
			psym = m_psymtbl->FindSymbol(xnsJStep);
			Assert(psym);
			pbstrm->WriteByte(psym->InternalID());
			//	justify.0.weight attribute ID
			GrcStructName xnsJWeight("justify", "weight");
			psym = m_psymtbl->FindSymbol(xnsJWeight);
			//Assert(psym);
			pbstrm->WriteByte(psym ? psym->InternalID() : -1);
			//	runto
			pbstrm->WriteByte(0);
			//	reserved (pad bytes)
			pbstrm->WriteByte(0);
			pbstrm->WriteByte(0);
			pbstrm->WriteByte(0);
		}
	}

	//	number of initial attributes that represent ligature components
	pbstrm->WriteShort(m_cpsymComponents);

	//	number of user-defined slot attributes
	pbstrm->WriteByte(m_prndr->NumUserDefn());
	//	max number of ligature components
	pbstrm->WriteByte(m_prndr->NumLigComponents());

	//	directions supported
	gr::byte  grfsdc = m_prndr->ScriptDirections();
	grfsdc = (grfsdc == 0) ? static_cast<gr::byte>(kfsdcHorizLtr) : grfsdc;	// supply default--left-to-right
	pbstrm->WriteByte(grfsdc);
	//	reserved (pad bytes)
	pbstrm->WriteByte(0);
	pbstrm->WriteByte(0);
	pbstrm->WriteByte(0);

	if (fxdSilfVersion >= 0x00020000)
	{
		//	reserved (pad byte)
		pbstrm->WriteByte(0);
		//	number of critical features
		pbstrm->WriteByte(0);
		//	TODO: include list of critical features
		//m_prndr->OutputCriticalFeatures(pbstrm, m_vCriticalFeatures);

		//	reserved (pad byte)
		pbstrm->WriteByte(0);
	}

	//	number of scripts supported
	int cScriptTags = m_prndr->NumScriptTags();
	pbstrm->WriteByte(cScriptTags);
	//	array of script tags
	int i;
	for (i = 0; i < cScriptTags; i++)
		pbstrm->WriteInt(m_prndr->ScriptTag(i));

	//	line break glyph ID
	pbstrm->WriteShort(m_wLineBreak);

	//	array of offsets to passes, relative to the start of this subtable--fill in later;
	//	for now, write (cpass + 1) place holders
	long lPassOffsetsPos = pbstrm->Position();
	long nPassOffset = lPassOffsetsPos - lTableStartSub;
	for (i = 0; i <= cpass; i++)
		pbstrm->WriteInt(0);

	//	number of pseudo mappings and search constants
	long nPseudoOffset = pbstrm->Position() - lTableStartSub;
	int n = signed(m_vwPseudoForUnicode.size());
	int nPowerOf2, nLog;
	BinarySearchConstants(n, &nPowerOf2, &nLog);
	pbstrm->WriteShort(n);
	pbstrm->WriteShort(nPowerOf2);
	pbstrm->WriteShort(nLog);
	pbstrm->WriteShort(n - nPowerOf2);

	//	array of unicode-to-pseudo mappings
	for (i = 0; i < signed(m_vwPseudoForUnicode.size()); i++)
	{
		if (fxdSilfVersion < 0x00020000)
			pbstrm->WriteShort(m_vnUnicodeForPseudo[i]);
		else
			pbstrm->WriteInt(m_vnUnicodeForPseudo[i]);
		pbstrm->WriteShort(m_vwPseudoForUnicode[i]);
	}

	//	replacement classes
	m_prndr->OutputReplacementClasses(fxdSilfVersion, m_vpglfcReplcmtClasses, m_cpglfcLinear,
		pbstrm);

	//	passes
	std::vector<int> vnPassOffsets;
	m_prndr->OutputPasses(this, pbstrm, lTableStartSub, vnPassOffsets);
	Assert(vnPassOffsets.size() == static_cast<size_t>(cpass + 1));

	//	Now go back and fill in the offsets.

	long lSavePos = pbstrm->Position();

	if (fxdSilfVersion >= 0x00030000)
	{
		pbstrm->SetPosition(lOffsetsPos);
		pbstrm->WriteShort(nPassOffset);
		pbstrm->WriteShort(nPseudoOffset);
	}

	pbstrm->SetPosition(lPassOffsetsPos);
	for (i = 0; i < signed(vnPassOffsets.size()); i++)
		pbstrm->WriteInt(vnPassOffsets[i]);

	pbstrm->SetPosition(lSavePos);

	// handle size and padding
	*pnSilfSize = lSavePos - *pnSilfOffset;
	pbstrm->SeekPadLong(lSavePos);
}


/*----------------------------------------------------------------------------------------------
	Write the list of replacement clases to the output stream. First write the classes that
	can be written in linear format (output classes), and then write the ones that are ordered
	by glyph ID and need a map to get the index. Note that some classes may serve as both
	input and output classes and are written twice.
	Arguments:
		fxdSilfVersion	- if >= 4.0, output long offsets
		vpglfc			- list of replacement classes (previously generated)
		cpgflcLinear	- number of classes that can be in linear format
		pbstrm			- output stream
----------------------------------------------------------------------------------------------*/
void GdlRenderer::OutputReplacementClasses(int fxdSilfVersion,
	std::vector<GdlGlyphClassDefn *> & vpglfcReplcmt, int cpglfcLinear,
	GrcBinaryStream * pbstrm)
{
	long lClassMapStart = pbstrm->Position();

	//	number of classes
	pbstrm->WriteShort(vpglfcReplcmt.size());
	//	number that can be in linear format
	pbstrm->WriteShort(cpglfcLinear);

	//	offsets to classes--fill in later; for now output (# classes + 1) place holders
	std::vector<int> vnClassOffsets;
	long lOffsetPos = pbstrm->Position();
	int ipglfc;
	for (ipglfc = 0; ipglfc <= signed(vpglfcReplcmt.size()); ipglfc++)
	{
		if (fxdSilfVersion < 0x00040000)
			pbstrm->WriteShort(0);
		else
			pbstrm->WriteInt(0);
	}

	//	linear classes (output)
	int cTmp = 0;
	for (ipglfc = 0; ipglfc < cpglfcLinear; ipglfc++)
	{
		GdlGlyphClassDefn * pglfc = vpglfcReplcmt[ipglfc];

		vnClassOffsets.push_back(pbstrm->Position() - lClassMapStart);

		Assert(pglfc->ReplcmtOutputClass() || pglfc->GlyphIDCount() <= 1);
		//Assert(pglfc->ReplcmtOutputID() == cTmp);

		std::vector<utf16> vwGlyphs;
		pglfc->GenerateOutputGlyphList(vwGlyphs);
		//	number of items and search constants
		//int n = vwGlyphs.Size();
		//int nPowerOf2, nLog;
		//BinarySearchConstants(n, &nPowerOf2, &nLog);
		//pbstrm->WriteShort(n);
		//pbstrm->WriteShort(nPowerOf2);
		//pbstrm->WriteShort(nLog);
		//pbstrm->WriteShort(n - nPowerOf2);
		//	glyph list
		for (size_t iw = 0; iw < vwGlyphs.size(); iw++)
			pbstrm->WriteShort(vwGlyphs[iw]);

		cTmp++;
	}

	//	indexed classes (input)
	for (ipglfc = cpglfcLinear; ipglfc < signed(vpglfcReplcmt.size()); ipglfc++)
	{
		GdlGlyphClassDefn * pglfc = vpglfcReplcmt[ipglfc];

		vnClassOffsets.push_back(pbstrm->Position() - lClassMapStart);

		Assert(pglfc->ReplcmtInputClass());
		Assert(pglfc->ReplcmtInputID() == cTmp);

		std::vector<utf16> vwGlyphs;
		std::vector<int> vnIndices;
		pglfc->GenerateInputGlyphList(vwGlyphs, vnIndices);
		//	number of items and search constants
		int n = signed(vwGlyphs.size());
		int nPowerOf2, nLog;
		BinarySearchConstants(n, &nPowerOf2, &nLog);
		pbstrm->WriteShort(n);
		pbstrm->WriteShort(nPowerOf2);
		pbstrm->WriteShort(nLog);
		pbstrm->WriteShort(n - nPowerOf2);
		//	glyph list
		for (size_t iw = 0; iw < vwGlyphs.size(); iw++)
		{
			pbstrm->WriteShort(vwGlyphs[iw]);
			pbstrm->WriteShort(vnIndices[iw]);
		}

		cTmp++;
	}

	//	final offset giving length of block
	vnClassOffsets.push_back(pbstrm->Position() - lClassMapStart);

	//	Now go back and fill in the offsets.
	long lSavePos = pbstrm->Position();

	pbstrm->SetPosition(lOffsetPos);
	for (size_t i = 0; i < vnClassOffsets.size(); i++)
	{
		if (fxdSilfVersion < 0x00040000)
			pbstrm->WriteShort(vnClassOffsets[i]);
		else
			pbstrm->WriteInt(vnClassOffsets[i]);
	}

	pbstrm->SetPosition(lSavePos);
}


/*----------------------------------------------------------------------------------------------
	Generate a list of all the glyphs in the class, ordered by index (ie, ordered as listed
	in the program).
----------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::GenerateOutputGlyphList(std::vector<utf16> & vwGlyphs)
{
	AddGlyphsToUnsortedList(vwGlyphs);
}


/*----------------------------------------------------------------------------------------------
	Add all the glyphs to the list in the order they were defined.
----------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::AddGlyphsToUnsortedList(std::vector<utf16> & vwGlyphs)
{
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		m_vpglfdMembers[iglfd]->AddGlyphsToUnsortedList(vwGlyphs);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphDefn::AddGlyphsToUnsortedList(std::vector<utf16> & vwGlyphs)
{
	for (size_t iw = 0; iw < m_vwGlyphIDs.size(); iw++)
	{
		vwGlyphs.push_back(m_vwGlyphIDs[iw]);
	}
}


/*----------------------------------------------------------------------------------------------
	Generate a list of all the glyphs in the class, ordered by glyph ID. These will be
	output in linear format.
----------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::GenerateInputGlyphList(std::vector<utf16> & vwGlyphs, std::vector<int> & vnIndices)
{
	AddGlyphsToSortedList(vwGlyphs, vnIndices);
}


/*----------------------------------------------------------------------------------------------
	Add all the glyphs to the list, keeping the list sorted.
----------------------------------------------------------------------------------------------*/
void GdlGlyphClassDefn::AddGlyphsToSortedList(std::vector<utf16> & vwGlyphs, std::vector<int> & vnIndices)
{
	for (size_t iglfd = 0; iglfd < m_vpglfdMembers.size(); iglfd++)
	{
		m_vpglfdMembers[iglfd]->AddGlyphsToSortedList(vwGlyphs, vnIndices);
	}
}

/*--------------------------------------------------------------------------------------------*/
void GdlGlyphDefn::AddGlyphsToSortedList(std::vector<utf16> & vwGlyphs, std::vector<int> & vnIndices)
{
	Assert(vwGlyphs.size() == vnIndices.size());

	for (size_t iw = 0; iw < m_vwGlyphIDs.size(); iw++)
	{
		int nNextIndex = signed(vwGlyphs.size());

		utf16 w = m_vwGlyphIDs[iw];
		if (vwGlyphs.size() == 0 ||
			w > vwGlyphs.back())	// common case
		{
			vwGlyphs.push_back(w);
			vnIndices.push_back(nNextIndex);
		}
		else
		{
			int iLow = 0;
			int iHigh = signed(vwGlyphs.size());

			while (iHigh - iLow > 1)
			{
				int iMid = (iHigh + iLow) >> 1;	// divide by 2
				if (w == vwGlyphs[iMid])
				{
					iLow = iMid;
					iHigh = iMid + 1;
				}
				else if (w < vwGlyphs[iMid])
					iHigh = iMid;
				else
					iLow = iMid;
			}

			if (w <= vwGlyphs[iLow])
			{
				vwGlyphs.insert(vwGlyphs.begin() + iLow, w);
				vnIndices.insert(vnIndices.begin() + iLow, nNextIndex);
			}
			else
			{
				Assert(static_cast<size_t>(iHigh) == vwGlyphs.size() || w < vwGlyphs[iHigh]);
				vwGlyphs.insert(vwGlyphs.begin() + iLow + 1, w);
				vnIndices.insert(vnIndices.begin() + iLow + 1, nNextIndex);
			}
		}
	}
}


/*----------------------------------------------------------------------------------------------
	Count the passes for output to the font table.
----------------------------------------------------------------------------------------------*/
void GdlRenderer::CountPasses(int * pcpass, int * pcpassLB, int * pcpassSub,
	int * pcpassJust, int * pcpassPos, int * pipassBidi)
{
	GdlRuleTable * prultbl;

	if ((prultbl = FindRuleTable("linebreak")) != NULL)
		*pcpassLB = prultbl->CountPasses();
	else
		*pcpassLB = 0;

	if ((prultbl = FindRuleTable("substitution")) != NULL)
		*pcpassSub = prultbl->CountPasses();
	else
		*pcpassSub = 0;

	if ((prultbl = FindRuleTable("justification")) != NULL)
		*pcpassJust = prultbl->CountPasses();
	else
		*pcpassJust = 0;

	if ((prultbl = FindRuleTable("positioning")) != NULL)
		*pcpassPos = prultbl->CountPasses();
	else
		*pcpassPos = 0;

	*pcpass = *pcpassLB + *pcpassSub + *pcpassJust + *pcpassPos;

	if (Bidi())
		*pipassBidi = *pcpassLB + *pcpassSub;	
	else
		*pipassBidi = -1;
}


/*--------------------------------------------------------------------------------------------*/
int GdlRuleTable::CountPasses()
{
	int cRet = 0;

	for (size_t ipass = 0; ipass < m_vppass.size(); ++ipass)
	{
		if (m_vppass[ipass]->HasRules())
			cRet++;
	}
	return cRet;
}


/*----------------------------------------------------------------------------------------------
	Output the passes to the stream.
----------------------------------------------------------------------------------------------*/
void GdlRenderer::OutputPasses(GrcManager * pcman, GrcBinaryStream * pbstrm, long lTableStart,
	std::vector<int> & vnOffsets)
{
	GdlRuleTable * prultbl;

	if ((prultbl = FindRuleTable("linebreak")) != NULL)
		prultbl->OutputPasses(pcman, pbstrm, lTableStart, vnOffsets);

	if ((prultbl = FindRuleTable("substitution")) != NULL)
		prultbl->OutputPasses(pcman, pbstrm, lTableStart, vnOffsets);

	if ((prultbl = FindRuleTable("justification")) != NULL)
		prultbl->OutputPasses(pcman, pbstrm, lTableStart, vnOffsets);

	if ((prultbl = FindRuleTable("positioning")) != NULL)
		prultbl->OutputPasses(pcman, pbstrm, lTableStart, vnOffsets);

	//	Push one more offset so the last pass can figure its length.
	vnOffsets.push_back(pbstrm->Position() - lTableStart);
}


/*--------------------------------------------------------------------------------------------*/
void GdlRuleTable::OutputPasses(GrcManager * pcman, GrcBinaryStream * pbstrm, long lTableStart,
	std::vector<int> & vnOffsets)
{
	for (size_t ipass = 0; ipass < m_vppass.size(); ++ipass)
	{
		if (m_vppass[ipass]->HasRules())
		{
			vnOffsets.push_back(pbstrm->Position() - lTableStart);
			m_vppass[ipass]->OutputPass(pcman, pbstrm, lTableStart);
		}
	}
}


/*----------------------------------------------------------------------------------------------
	Output the contents of the pass to the stream.
----------------------------------------------------------------------------------------------*/
void GdlPass::OutputPass(GrcManager * pcman, GrcBinaryStream * pbstrm, int lTableStart)
{
	long lPassStart = pbstrm->Position();

	int fxdSilfVersion = pcman->VersionForTable(ktiSilf);
	int fxdRuleVersion = fxdSilfVersion; // someday these may be different

	int nOffsetToPConstraint = 0;
	long lOffsetToPConstraintPos = 0;

	int nOffsetToConstraint = 0;
	long lOffsetToConstraintPos = 0;

	int nOffsetToAction = 0;
	long lOffsetToActionPos = 0;

	int nOffsetToDebugArrays = 0;
	long lOffsetToDebugArraysPos = 0;

	//	flags; TODO SharonC: figure out what should be in here
	pbstrm->WriteByte(0);
	//	MaxRuleLoop
	pbstrm->WriteByte(m_nMaxRuleLoop);
	//	max rule context
	pbstrm->WriteByte(m_nMaxRuleContext);
	//	MaxBackup
	pbstrm->WriteByte(m_nMaxBackup);
	//	number of rules
	pbstrm->WriteShort(m_vprule.size());

	long lFsmOffsetPos = pbstrm->Position();
	if (fxdSilfVersion >= 0x00020000)
	{
		// offset to row information, or (<=v2) reserved
		pbstrm->WriteShort(0);
		// pass constraint byte count--fill in later
		lOffsetToPConstraintPos = pbstrm->Position();
		pbstrm->WriteInt(0);
	}

	//	offset to rule constraint code--fill in later
	lOffsetToConstraintPos = pbstrm->Position();
	pbstrm->WriteInt(0);
	//	offset to action code--fill in later
	lOffsetToActionPos = pbstrm->Position();
	pbstrm->WriteInt(0);
	//	offset to debug strings--fill in later
	lOffsetToDebugArraysPos = pbstrm->Position();
	pbstrm->WriteInt(0);

	long nFsmOffset = pbstrm->Position() - lPassStart;

	//	number of FSM rows
	pbstrm->WriteShort(NumStates());
	//	number of transitional states
	pbstrm->WriteShort(NumTransitionalStates());
	//	number of success states
	pbstrm->WriteShort(NumSuccessStates());
	//	number of columns
	pbstrm->WriteShort(m_pfsm->NumberOfColumns());

	//	number of glyph sub-ranges
	int n = TotalNumGlyphSubRanges();
	pbstrm->WriteShort(n);
	//	glyph sub-range search constants
	int nPowerOf2, nLog;
	BinarySearchConstants(n, &nPowerOf2, &nLog);
	pbstrm->WriteShort(nPowerOf2);
	pbstrm->WriteShort(nLog);
	pbstrm->WriteShort(n - nPowerOf2);

	//	glyph sub-ranges: for each glyph, find the machine class that includes it, if any,
	//	and output the range.
	utf16 w = 0;
	while (w < pcman->NumGlyphs())
	{
		for (size_t i = 0; i < m_vpfsmc.size(); i++)
		{
			//	If this machine class includes this glyph, output the range on the stream
			//	and return the last glyph in the range. Otherwise return 0xFFFF.
			utf16 wLast = m_vpfsmc[i]->OutputRange(w, pbstrm);
			if (wLast != 0xFFFF)
			{
				w = wLast;
				break;
			}
		}
		w++;
	}

	//	rule list and offsets
	std::vector<int> vnOffsets;
	std::vector<int> vnRuleList;
	GenerateRuleMaps(vnOffsets, vnRuleList);
	int i;
	for (i = 0; i < signed(vnOffsets.size()); i++)
		pbstrm->WriteShort(vnOffsets[i]);
	for (i = 0; i < signed(vnRuleList.size()); i++)
		pbstrm->WriteShort(vnRuleList[i]);

	//	minRulePreContext
	pbstrm->WriteByte(m_critMinPreContext);
	//	maxRulePreContext
	pbstrm->WriteByte(m_critMaxPreContext);
	//	start states
	Assert(m_critMaxPreContext - m_critMinPreContext + 1 == static_cast<int>(m_vrowStartStates.size()));
	Assert(m_vrowStartStates[0] == 0);
	for (i = 0; i < signed(m_vrowStartStates.size()); i++)
		pbstrm->WriteShort(m_vrowStartStates[i]);

	//	rule sort keys
	for (i = 0; i < signed(m_vprule.size()); i++)
	{
		pbstrm->WriteShort(m_vprule[i]->SortKey());
	}

	//	pre-context item counts
	for (i = 0; i < signed(m_vprule.size()); i++)
	{
		pbstrm->WriteByte(m_vprule[i]->NumberOfPreModContextItems());
	}

	//	action and constraint offsets--fill in later;
	//	for now, write (# rules + 1) place holders
	std::vector<int> vnActionOffsets;
	std::vector<int> vnConstraintOffsets;
	long lCodeOffsets = pbstrm->Position();
	if (fxdSilfVersion >= 0x00020000)
	{
		// reserved - pad byte
		pbstrm->WriteByte(0);
		lCodeOffsets = pbstrm->Position();
		// pass constraint byte count
		pbstrm->WriteShort(0);
	}
	for (i = 0; i <= signed(m_vprule.size()); i++)
	{
		pbstrm->WriteShort(0);
		pbstrm->WriteShort(0);
	}

	//	transition table for states
	OutputFsmTable(pbstrm);

	//	constraint and action code

	size_t ib;

	int cbPassConstraint;
	if (fxdSilfVersion >= 0x00020000)
	{
		// reserved - pad byte
		pbstrm->WriteByte(0);

		nOffsetToPConstraint = pbstrm->Position() - lTableStart;
		std::vector<gr::byte> vbPassConstr;
		this->GenerateEngineCode(pcman, fxdRuleVersion, vbPassConstr);
		for (ib = 0; ib < vbPassConstr.size(); ib++)
			pbstrm->WriteByte(vbPassConstr[ib]);
		cbPassConstraint = vbPassConstr.size();
	}
	else
	{
		nOffsetToPConstraint = pbstrm->Position() - lTableStart;
		cbPassConstraint = 0;
	}


	//	ENHANCE: divide GenerateEngineCode into two methods.

	nOffsetToConstraint = pbstrm->Position() - lTableStart;

	//	Output a dummy byte just to keep any constraint from having zero as its offset,
	//	because we are using zero as a indicator that there are no constraints.
	pbstrm->WriteByte(0);

	std::vector<gr::byte> vbConstraints;
	std::vector<gr::byte> vbActions;
	int irule;
	for (irule = 0; irule < signed(m_vprule.size()); irule++)
	{
		vbConstraints.clear();
		m_vprule[irule]->GenerateEngineCode(pcman, fxdRuleVersion, vbActions, vbConstraints);
		if (vbConstraints.size() == 0)
			vnConstraintOffsets.push_back(0);
		else
		{
			vnConstraintOffsets.push_back(pbstrm->Position() - nOffsetToConstraint - lTableStart);
			for (ib = 0; ib < vbConstraints.size(); ib++)
				pbstrm->WriteByte(vbConstraints[ib]);
		}
	}
	vnConstraintOffsets.push_back(pbstrm->Position() - nOffsetToConstraint - lTableStart);

	nOffsetToAction = pbstrm->Position() - lTableStart;

	for (irule = 0; irule < signed(m_vprule.size()); irule++)
	{
		vbActions.clear();
		vnActionOffsets.push_back(pbstrm->Position() - nOffsetToAction - lTableStart);
		m_vprule[irule]->GenerateEngineCode(pcman, fxdRuleVersion, vbActions, vbConstraints);
		for (size_t ib = 0; ib < vbActions.size(); ib++)
			pbstrm->WriteByte(vbActions[ib]);
	}
	vnActionOffsets.push_back(pbstrm->Position() - nOffsetToAction - lTableStart);

	Assert(vnConstraintOffsets.size() == m_vprule.size() + 1);
	Assert(vnActionOffsets.size() == m_vprule.size() + 1);

	//	TODO: output debugger strings
	nOffsetToDebugArrays = 0;	// pbstrm->Position() - lTableStart;

	//	Now go back and fill in the offsets.

	long lSavePos = pbstrm->Position();

	if (fxdSilfVersion >= 0x00020000)
	{
		pbstrm->SetPosition(lOffsetToPConstraintPos);
		pbstrm->WriteInt(nOffsetToPConstraint);
	}
	pbstrm->SetPosition(lOffsetToConstraintPos);
	pbstrm->WriteInt(nOffsetToConstraint);
	pbstrm->SetPosition(lOffsetToActionPos);
	pbstrm->WriteInt(nOffsetToAction);
	pbstrm->SetPosition(lOffsetToDebugArraysPos);
	pbstrm->WriteInt(nOffsetToDebugArrays);

	pbstrm->SetPosition(lCodeOffsets);
	if (fxdSilfVersion >= 0x00020000)
		pbstrm->WriteShort(cbPassConstraint);
	for (i = 0; i < signed(vnConstraintOffsets.size()); i++)
		pbstrm->WriteShort(vnConstraintOffsets[i]);
	for (i = 0; i < signed(vnActionOffsets.size()); i++)
		pbstrm->WriteShort(vnActionOffsets[i]);

	if (fxdSilfVersion >= 0x00030000)
	{
		pbstrm->SetPosition(lFsmOffsetPos);
		pbstrm->WriteShort(nFsmOffset);
	}

	pbstrm->SetPosition(lSavePos);
}


/*----------------------------------------------------------------------------------------------
	If this machine class includes the given glyph ID, output the range on the stream
	and return the last glyph in the range; otherwise return 0xFFFF. Note that
	due to the way this method is used, we can assme the given glyph ID is the first glyph
	in the range.
----------------------------------------------------------------------------------------------*/
utf16 FsmMachineClass::OutputRange(utf16 wGlyphID, GrcBinaryStream * pbstrm)
{
	for (size_t iMin = 0; iMin < m_wGlyphs.size(); iMin++)
	{
		if (m_wGlyphs[iMin] == wGlyphID)
		{
			//	This machine class includes the glyph. Search for the end of the range of
			//	contiguous glyphs.
			int iLim = iMin + 1;
			while (iLim < signed(m_wGlyphs.size()) && m_wGlyphs[iLim] == m_wGlyphs[iLim - 1] + 1)
				iLim++;
			//	Write to the stream;
			pbstrm->WriteShort(m_wGlyphs[iMin]);
			pbstrm->WriteShort(m_wGlyphs[iLim - 1]);
			pbstrm->WriteShort(m_ifsmcColumn);
			return m_wGlyphs[iLim - 1];		// last glyph in range
		}
		else if (m_wGlyphs[iMin] > wGlyphID)
			return 0xFFFF;

	}
	return 0xFFFF;
}


/*----------------------------------------------------------------------------------------------
	Generate the lists of rule maps for states in the FSM. The second argment are the list
	of rules, the first argument are the offsets into the list for each success state.
	For instance, if you have the following success states:
		s3:  r1, r2, r5
		s4:  r0
		s5:  r3, r4
		s6:  r6
	you'll get the following:
		vnOffsets               vnRuleLists
		    0						1
		    3						2
		    4						5
			6						0
			7						3
									4
									6
	(States 0 - 2 are omitted from the lists; only success states are included.)
----------------------------------------------------------------------------------------------*/
void GdlPass::GenerateRuleMaps(std::vector<int> & vnOffsets, std::vector<int> & vnRuleList)
{
	size_t ifsLim = m_vifsFinalToWork.size();
	for (size_t ifs = 0; ifs < ifsLim; ifs++)
	{
		FsmState * pfstate = m_pfsm->StateAt(m_vifsFinalToWork[ifs]);

		Assert(!pfstate->HasBeenMerged());

		if (pfstate->NumberOfRulesSucceeded() > 0)
		{
			vnOffsets.push_back(vnRuleList.size());
			//	Make a sorted list of all the rule indices (this allows the rules to be
			//	tried in the order that they appeared in the source file).
			std::vector<int> virule;
			for (std::set<int>::iterator itset = pfstate->m_setiruleSuccess.begin();
				itset != pfstate->m_setiruleSuccess.end();
				++itset)
			{
				for (size_t iirule = 0; iirule <= virule.size(); iirule++)
				{
					if (iirule == virule.size())
					{
						virule.push_back(*itset);
						break;
					}
					else if (*itset < virule[iirule])
					{
						virule.insert(virule.begin() + iirule, *itset);
						break;
					}
				}
			}
			//	Now put them into the vector.
			for (size_t iirule = 0; iirule < virule.size(); iirule++)
				vnRuleList.push_back(virule[iirule]);
		}
		else
		{
			//	All non-success states should be together at the beginning of the table.
			Assert(vnRuleList.size() == 0);
		}
	}

	//	Push a final offset, so that the last state can figure its length.
	vnOffsets.push_back(vnRuleList.size());
}


/*----------------------------------------------------------------------------------------------
	Output the transition table itself.
----------------------------------------------------------------------------------------------*/
void GdlPass::OutputFsmTable(GrcBinaryStream * pbstrm)
{
	int cfsmc = m_pfsm->NumberOfColumns();
	size_t ifsLim = m_vifsFinalToWork.size();
	for (size_t ifs = 0; ifs < ifsLim; ifs++)
	{
		FsmState * pfstate = m_pfsm->StateAt(m_vifsFinalToWork[ifs]);

		Assert(!pfstate->HasBeenMerged());

		if (pfstate->AllCellsEmpty())
		{
			//	We've hit the end of the transitional states--quit.
			break;
		}

		for (int ifsmc = 0; ifsmc < cfsmc; ifsmc++)
		{
			int ifsmcValue = pfstate->CellValue(ifsmc);
			if (m_pfsm->RawStateAt(ifsmcValue)->HasBeenMerged())
				ifsmcValue = m_pfsm->RawStateAt(ifsmcValue)->MergedState()->WorkIndex();
			ifsmcValue = m_vifsWorkToFinal[ifsmcValue];

			//	Optimize: do some fancy footwork on the state number.
			pbstrm->WriteShort(ifsmcValue);
		}
	}
}


/*----------------------------------------------------------------------------------------------
	Convenient wrapper to call the same method in GdlRenderer.
----------------------------------------------------------------------------------------------*/
bool GrcManager::AssignFeatTableNameIds(utf16 wFirstNameId, utf16 wNameIdMinNew,
	std::vector<std::wstring> & vstuExtNames, std::vector<utf16> & vwLangIds,
	std::vector<utf16> & vwNameTblIds,
	size_t & cchwStringData, uint8 * pNameTbl)
{
	return m_prndr->AssignFeatTableNameIds(wFirstNameId, wNameIdMinNew,
		vstuExtNames, vwLangIds, vwNameTblIds, cchwStringData, pNameTbl, m_vpfeatInput);
}

/*----------------------------------------------------------------------------------------------
	Assign name table IDs to each feature and all settings. The IDs will be assigned 
	sequentially beginning at wFirstNameId. Note that each string does NOT get its own ID.
	A given feature can have several strings (names) each with a differnt lang id 
	but all get the same name ID.

	Return three vectors which each contain one element for each feature and setting.
	The vectors are parallel. Elements numbered n contains the name string, lang ID, and 
	name table ID for a given feature or setting.
----------------------------------------------------------------------------------------------*/
bool GdlRenderer::AssignFeatTableNameIds(utf16 wFirstNameId, utf16 wNameIdMinNew,
	std::vector<std::wstring> & vstuExtNames, std::vector<utf16> & vwLangIds,
	std::vector<utf16> & vwNameTblIds,
	size_t & cchwStringData, uint8 * pNameTbl, std::vector<GdlFeatureDefn *> & vpfeatInput)
{
	if (wFirstNameId > 32767)
		return false; // max allowed value

	int nNameIdNoName = -1;
	int nPlatId, nEncId, nLangId;
	char * rgchNoName = "NoName";
	TtfUtil::GetNameIdForString(pNameTbl, nPlatId, nEncId, nLangId, nNameIdNoName,
		rgchNoName, 6);

	utf16 wNameTblId = wFirstNameId;
	for (size_t ifeat = 0; ifeat < m_vpfeat.size(); ifeat++)
	{
		wNameTblId = m_vpfeat[ifeat]->SetNameTblIds(wNameTblId, pNameTbl, vpfeatInput);
		if (wNameTblId > 32767)
			return false;
		m_vpfeat[ifeat]->NameTblInfo(vstuExtNames, vwLangIds, vwNameTblIds, cchwStringData,
			wNameIdMinNew, nNameIdNoName);
	}
	return true;	
}

/*----------------------------------------------------------------------------------------------
	Write the "Feat" table and related subtables to the stream.
----------------------------------------------------------------------------------------------*/
void GrcManager::OutputFeatTable(GrcBinaryStream * pbstrm, int * pnFeatOffset, int * pnFeatSize)
{
	*pnFeatOffset = pbstrm->Position();

	//	version number
	int fxdFeatVersion = VersionForTable(ktiFeat);
	SetTableVersion(ktiFeat, fxdFeatVersion);
	pbstrm->WriteInt(fxdFeatVersion);

	m_prndr->OutputFeatTable(pbstrm, *pnFeatOffset, fxdFeatVersion);

	// handle size and padding
	int nTmp = pbstrm->Position();
	*pnFeatSize = nTmp - *pnFeatOffset;
	pbstrm->SeekPadLong(nTmp);
}

/*---------------------------------------------------------------------------------------------*/

void GdlRenderer::OutputFeatTable(GrcBinaryStream * pbstrm, long lTableStart,
	int fxdVersion)
{
	std::vector<int> vnOffsets;
	std::vector<long> vlOffsetPos;

	//	number of features
	pbstrm->WriteShort(m_vpfeat.size());

	//	reserved
	pbstrm->WriteShort(0);
	pbstrm->WriteInt(0);

	size_t ifeat;
	for (ifeat = 0; ifeat < m_vpfeat.size(); ifeat++)
	{
		//	feature id
		if (fxdVersion >= 0x00020000)
			pbstrm->WriteInt(m_vpfeat[ifeat]->ID());
		else
			pbstrm->WriteShort(m_vpfeat[ifeat]->ID());

		//	number of settings
		pbstrm->WriteShort(m_vpfeat[ifeat]->NumberOfSettings());

		if (fxdVersion >= 0x00020000)
			pbstrm->WriteShort(0); // pad bytes

		//	offset to setting--fill in later
		vlOffsetPos.push_back(pbstrm->Position());
		pbstrm->WriteInt(0);

		//	flags
		pbstrm->WriteShort(0x8000);	// all our features are mutually exclusive

		//	name index for feature name
		pbstrm->WriteShort(m_vpfeat[ifeat]->NameTblId());
	}

	for (ifeat = 0; ifeat < m_vpfeat.size(); ifeat++)
	{
		vnOffsets.push_back(pbstrm->Position() - lTableStart);
		m_vpfeat[ifeat]->OutputSettings(pbstrm);
	}

	Assert(vnOffsets.size() == m_vpfeat.size());

	//	Now fill in the offsets.

	long lSavePos = pbstrm->Position();

	for (ifeat = 0; ifeat < vnOffsets.size(); ifeat++)
	{
		pbstrm->SetPosition(vlOffsetPos[ifeat]);
		pbstrm->WriteInt(vnOffsets[ifeat]);
	}

	pbstrm->SetPosition(lSavePos);
}

/*---------------------------------------------------------------------------------------------*/

void GdlFeatureDefn::OutputSettings(GrcBinaryStream * pbstrm)
{
	//	first output the default setting
	if (m_pfsetDefault)
	{
		pbstrm->WriteShort(m_pfsetDefault->Value());
		pbstrm->WriteShort(m_pfsetDefault->NameTblId());	// name index
	}
	else if (m_vpfset.size() == 0)
	{
		//	no settings (eg, 'lang' feature); write 0 as the default
		pbstrm->WriteShort(0);
		pbstrm->WriteShort(32767); // no name index - output largest legal value
	}

	for (size_t ifset = 0; ifset < m_vpfset.size(); ifset++)
	{
		if (m_vpfset[ifset] != m_pfsetDefault)
		{
			pbstrm->WriteShort(m_vpfset[ifset]->Value());
			pbstrm->WriteShort(m_vpfset[ifset]->NameTblId());	// name index
		}
	}
}


/*----------------------------------------------------------------------------------------------
	Write the "Sill" table and related subtables to the stream.
----------------------------------------------------------------------------------------------*/
void GrcManager::OutputSillTable(GrcBinaryStream * pbstrm, int * pnSillOffset, int * pnSillSize)
{
	*pnSillOffset = pbstrm->Position();

	//	version number
	int fxd = VersionForTable(ktiSill);
	SetTableVersion(ktiSill, fxd);
	pbstrm->WriteInt(fxd);

	m_prndr->OutputSillTable(pbstrm, *pnSillOffset);

	// handle size and padding
	int nTmp = pbstrm->Position();
	*pnSillSize = nTmp - *pnSillOffset;
	pbstrm->SeekPadLong(nTmp);
}

/*---------------------------------------------------------------------------------------------*/

void GdlRenderer::OutputSillTable(GrcBinaryStream * pbstrm, long lTableStart)
{
	// Note: if the format of the Sill table changes, the CheckLanguageFeatureSize method
	// needs to be changed to match.

	std::vector<int> vnOffsets;
	std::vector<long> vlOffsetPos;

	//	search constants
	int n = m_vplang.size();
	int nPowerOf2, nLog;
	BinarySearchConstants(n, &nPowerOf2, &nLog);
	pbstrm->WriteShort(n); // number of languages
	pbstrm->WriteShort(nPowerOf2);
	pbstrm->WriteShort(nLog);
	pbstrm->WriteShort(n - nPowerOf2);

	int ilang;
	for (ilang = 0; ilang < signed(m_vplang.size()); ilang++)
	{
		//	language ID
		unsigned int nCode = m_vplang[ilang]->Code();
		char rgchCode[4];
		memcpy(rgchCode, &nCode, 4);
		pbstrm->Write(rgchCode, 4);

		//	number of settings
		pbstrm->WriteShort(m_vplang[ilang]->NumberOfSettings());

		//	offset to setting--fill in later
		vlOffsetPos.push_back(pbstrm->Position());
		pbstrm->WriteShort(0);
	}
	// Extra bogus entry to make it easy to find length of last.
	pbstrm->WriteInt(0x80808080);
	pbstrm->WriteShort(0);
	vlOffsetPos.push_back(pbstrm->Position());
	pbstrm->WriteShort(0);

	for (ilang = 0; ilang < signed(m_vplang.size()); ilang++)
	{
		vnOffsets.push_back(pbstrm->Position() - lTableStart);
		m_vplang[ilang]->OutputSettings(pbstrm);
	}
	vnOffsets.push_back(pbstrm->Position() - lTableStart); // offset of bogus entry gives length of last real one

	Assert(vnOffsets.size() == m_vplang.size() + 1);

	//	Now fill in the offsets.

	long lSavePos = pbstrm->Position();

	for (ilang = 0; ilang < signed(vnOffsets.size()); ilang++)
	{
		pbstrm->SetPosition(vlOffsetPos[ilang]);
		pbstrm->WriteShort(vnOffsets[ilang]);
	}

	pbstrm->SetPosition(lSavePos);
}

/*---------------------------------------------------------------------------------------------*/

void GdlLanguageDefn::OutputSettings(GrcBinaryStream * pbstrm)
{
	Assert(m_vpfeat.size() == m_vnFset.size());
	for (size_t ifset = 0; ifset < m_vpfset.size(); ifset++)
	{
		pbstrm->WriteInt(m_vpfeat[ifset]->ID());	// feature ID
		pbstrm->WriteShort(m_vnFset[ifset]);		// value
		pbstrm->WriteShort(0);						// pad
	}
}

/*----------------------------------------------------------------------------------------------
	Calculate the standard search constants for the given number n:
		- max power of 2 < n
		- log-base-2(the number above)
----------------------------------------------------------------------------------------------*/
void BinarySearchConstants(int n, int * pnPowerOf2, int * pnLog)
{
	Assert(n >= 0);

	if (n == 0)
	{
		*pnPowerOf2 = 0;
		*pnLog = 0;
		return;
	}

	*pnPowerOf2 = 1;
	*pnLog = 0;

	while ((*pnPowerOf2 << 1) <= n)
	{
		*pnPowerOf2 = *pnPowerOf2 << 1;
		*pnLog = *pnLog + 1;
	}
}


/*----------------------------------------------------------------------------------------------
	Write a byte to the output stream.
----------------------------------------------------------------------------------------------*/
void GrcBinaryStream::WriteByte(int iOutput)
{
	Assert(((iOutput & 0xFFFFFF00) == 0xFFFFFF00) ||
		((iOutput & 0xFFFFFF00) == 0x00000000));

	char b = iOutput & 0x000000FF;
	write(&b, 1);
}

/*----------------------------------------------------------------------------------------------
	Write a short integer to the output stream.
----------------------------------------------------------------------------------------------*/
void GrcBinaryStream::WriteShort(int iOutput)
{
	Assert(((iOutput & 0xFFFF0000) == 0xFFFF0000) ||
		((iOutput & 0xFFFF0000) == 0x00000000));

	//	big-endian format:
	char b1 = (iOutput & 0x000000FF);
	char b2 = (iOutput & 0x0000FF00) >> 8;
	write(&b2, 1);
	write(&b1, 1);
}

/*----------------------------------------------------------------------------------------------
	Write a standard integer to the output stream.
----------------------------------------------------------------------------------------------*/
void GrcBinaryStream::WriteInt(int iOutput)
{
	//	big-endian format:
	char b1 = (iOutput & 0x000000FF);
	char b2 = (iOutput & 0x0000FF00) >> 8;
	char b3 = (iOutput & 0x00FF0000) >> 16;
	char b4 = (iOutput & 0xFF000000) >> 24;

	write(&b4, 1);
	write(&b3, 1);
	write(&b2, 1);
	write(&b1, 1);
}

/*----------------------------------------------------------------------------------------------
	Seek to ibOffset then add zero padding for long alignment.
	Return padded location.
----------------------------------------------------------------------------------------------*/
long GrcBinaryStream::SeekPadLong(long ibOffset)
{
	int cPad = ((ibOffset + 3) & ~3) - ibOffset;
	seekp(ibOffset);
	if (cPad)
		write("\0\0\0", cPad);
	return tellp();
}

/*----------------------------------------------------------------------------------------------
	Write a byte to the output stream.
----------------------------------------------------------------------------------------------*/
void GrcSubStream::WriteByte(int iOutput)
{
	Assert(((iOutput & 0xFFFFFF00) == 0xFFFFFF00) ||
		((iOutput & 0xFFFFFF00) == 0x00000000));

	char b = iOutput & 0x000000FF;
	m_strm.write(&b, 1);
}

/*----------------------------------------------------------------------------------------------
	Write a short integer to the output stream.
----------------------------------------------------------------------------------------------*/
void GrcSubStream::WriteShort(int iOutput)
{
	Assert(((iOutput & 0xFFFF0000) == 0xFFFF0000) ||
		((iOutput & 0xFFFF0000) == 0x00000000));

	//	big-endian format:
	char b1 = (iOutput & 0x000000FF);
	char b2 = (iOutput & 0x0000FF00) >> 8;
	m_strm.write(&b2, 1);
	m_strm.write(&b1, 1);
}

/*----------------------------------------------------------------------------------------------
	Write a standard integer to the output stream.
----------------------------------------------------------------------------------------------*/
void GrcSubStream::WriteInt(int iOutput)
{
	//	big-endian format:
	char b1 = (iOutput & 0x000000FF);
	char b2 = (iOutput & 0x0000FF00) >> 8;
	char b3 = (iOutput & 0x00FF0000) >> 16;
	char b4 = (iOutput & 0xFF000000) >> 24;

	m_strm.write(&b4, 1);
	m_strm.write(&b3, 1);
	m_strm.write(&b2, 1);
	m_strm.write(&b1, 1);
}

/*----------------------------------------------------------------------------------------------
	Adjust argument for 4 byte padding.
----------------------------------------------------------------------------------------------*/
DWORD PadLong(DWORD ul)
{
	return (ul + 3) & ~3;
}

/*----------------------------------------------------------------------------------------------
	Compare two table directory entries. Used by qsort().
----------------------------------------------------------------------------------------------*/
int CompareDirEntries(const void * ptr1, const void * ptr2)
{
	unsigned int lTmp1 = read(((OffsetSubTable::Entry *)ptr1)->tag);
	unsigned int lTmp2 = read(((OffsetSubTable::Entry *)ptr2)->tag);
	return (lTmp1 - lTmp2);
}

/*----------------------------------------------------------------------------------------------
	Calculate a checksum. cluSize is the byte count of the table pointed at by pluTable.
	The table must be padded to a length that is a multiple of four. cluSize includes the 
	padding. The table is treated as a sequence of longs which are summed together.
----------------------------------------------------------------------------------------------*/

unsigned int CalcCheckSum(const void * pluTable, size_t cluSize)
{
	Assert(!(cluSize & 0x00000003));
	unsigned int  luCheckSum = 0;
    const uint32 *        element = static_cast<const uint32 *>(pluTable);
	const uint32 *const   end = element + cluSize / sizeof(uint32);
    for (;element != end; ++element)
		luCheckSum += read(*element);

    return luCheckSum;
}

/***********************************************************************************************
	Debuggers
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Test the serialization process.
----------------------------------------------------------------------------------------------*/
void GrcManager::DebugOutput()
{
	GrcBinaryStream bstrm("testfont.ttt");

	bstrm.WriteInt(0);
	bstrm.WriteInt(0);
	bstrm.WriteInt(0);
	bstrm.WriteInt(0);

	int nGlocOffset, nGlocSize;
	int nGlatOffset, nGlatSize;
	int nSilOffset, nSilSize;
	int nFeatOffset, nFeatSize;

	OutputGlatAndGloc(&bstrm, &nGlocOffset, &nGlocSize, &nGlatOffset, &nGlatSize);
	OutputSilfTable(&bstrm, &nSilOffset, &nSilSize);
	OutputFeatTable(&bstrm, &nFeatOffset, &nFeatSize);

	long lSavePos = bstrm.Position();

	bstrm.SetPosition(0);
	bstrm.WriteInt(nSilOffset);
	bstrm.WriteInt(nGlocOffset);
	bstrm.WriteInt(nGlatOffset);
	bstrm.WriteInt(nFeatOffset);

	bstrm.SetPosition(lSavePos);
}
