/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: main.cpp
Responsibility: Sharon Correll

Description:
    Main function that runs the compiler.

Compiler versions:
	1.0		- Initial
	2.0		- justification, measuring (Silf table 2.0)
	3.0?	- Silf table 3.1
	4.0		- pass constraints (Silf table 3.1)
			- large set (> 256) of replacement classes
	4.1		- mirror glyph attributes (Silf table 2.1 or 3.2)
	4.2		- large number of glyphs (> 64K) in replacement classes (Silf table 4.0);
				segsplit and extra LB flag
	4.3		- handle &= and -= for class definitions; multiple justification levels
	4.3.1	- added *skipPasses* glyph attribute and passKeySlot slot attribute for
				optimization; don't output xoffset, yoffset, or gpoint att fields
	5.0		- collision fixing (Glat table 3.0, Gloc table 1.1; feat-set operator;
				Silf table 4.1 for collision fixing, 5.0 for compression)
				
-------------------------------------------------------------------------------*//*:End Ignore*/

/***********************************************************************************************
	Include files
***********************************************************************************************/
#include "main.h"

// Version: bldinc.h holds the current copyright number and it is created by executing
// bin\mkverrsc.exe from within the bin\mkcle.bat file. The major and minor version
// numbers are hard-coded in mkcle.bat.
#ifdef GR_FW
#include "..\..\..\..\Output\Common\bldinc.h"
#endif // GR_FW

#if !defined(_WIN32) && !HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME
#include <libgen.h>
char* program_invocation_name;
char* program_invocation_short_name;
#endif

#pragma hdrstop
#undef THIS_FILE
DEFINE_THIS_FILE

/***********************************************************************************************
	Forward declarations
***********************************************************************************************/

/***********************************************************************************************
	Local Constants and static variables
***********************************************************************************************/

/***********************************************************************************************
	Methods and functions
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
    Run the compiler over the specified GDL and font files.
----------------------------------------------------------------------------------------------*/
int main(int argc, char * argv[])
{
#ifdef _WIN32 
	// CRTDebug thing
//	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
//	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF);
	// COM thing
	::CoInitialize(NULL);
#endif

#if !defined(_WIN32) && !HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME
	program_invocation_name = argv[0];
	program_invocation_short_name = basename(argv[0]);
#endif

	char * pchGdlppFile = getenv("GDLPP");
	std::string staGdlppFile(pchGdlppFile ? pchGdlppFile : "");
#ifdef _WIN32
	if (staGdlppFile.empty())
	{
		char rgchGdlppFile[1024];
		DWORD cch = GetModuleFileName(NULL, rgchGdlppFile, sizeof(rgchGdlppFile)-1);
		rgchGdlppFile[cch] = '\0';
		staGdlppFile = rgchGdlppFile;

		std::string::size_type pos = staGdlppFile.rfind('\\');
		if (pos == staGdlppFile.npos)
			pos = 0;
		staGdlppFile.erase(pos, staGdlppFile.npos);
		if (!staGdlppFile.empty())
			staGdlppFile += '\\';
		staGdlppFile += "gdlpp.exe";
	}
#else
    // On Linux just use the PATH since grcompiler has probably been run
    // via the PATH and it allows people to use things like /usr/local/bin
    if (staGdlppFile.empty())
    {
        staGdlppFile = "gdlpp";
    }
#endif

	char * pchGdlFile = NULL;
	char * pchFontFile = NULL;
	char rgchOutputFile[128];
	utf16 rgchwOutputFontFamily[128];
	memset(rgchOutputFile, 0, isizeof(char) * 128);
	memset(rgchwOutputFontFamily, 0, isizeof(utf16) * 128);

	int cargExtra = 0;
	bool fModFontName = false;

	g_cman.SetOutputDebugFiles(false, false);
	g_cman.SetSilfTableVersion(g_cman.DefaultSilfVersion(), false);
	g_cman.SetSeparateControlFile(false);
	g_cman.SetVerbose(true);
	g_cman.SetPassOptimizations(true);
	g_cman.SetOffsetAttrs(false);
	g_cman.SetCompressor(ktcNone);

	// Ignore these warnings by default:
	g_errorList.SetIgnoreWarning(510);	// Cannot find point number for coordinates...
	g_errorList.SetIgnoreWarning(3521);	// Vertical overlap between

	// on linux systems an argument starting with a / may be a path
	// so use - for options. On Windows allow both / or -
#ifdef _WIN32 
	while (argc >= 2 + cargExtra
		&& (argv[1 + cargExtra][0] == '/' || argv[1 + cargExtra][0] == '-'))
#else
	while (argc >= 2 + cargExtra && argv[1 + cargExtra][0] == '-')
#endif
	{
		HandleCompilerOptions(argv[1 + cargExtra]);
		cargExtra++;
	}
	if (g_cman.IsVerbose())
	{
		std::cout << "Graphite Compiler Version 4.9";
		#ifdef _DEBUG
			std::cout << "  [debug build]";
		#else
			std::cout << "  [release build]";
		#endif
		// \xc2\xa9 = copyright symbol
		std::cout << "\n"
			<< "Copyright (c) 2002-2014, by SIL International.  All rights reserved.\n";
	}

	if (argc < 3 + cargExtra)
	{
		std::cout << "\nusage: grcompiler [options] gdl-file input-font-file [output-font-file] [output-font-name]\n";
		std::cout << "\nOptions:\n";
		std::cout << "   -c       - compress graphite tables\n";
		std::cout << "   -d       - output XML debugger file\n";
		std::cout << "   -D       - output all debugger files\n";
		std::cout << "   -g       - permit and ignore invalid glyph definitions\n";
		std::cout << "   -nNNNN   - set name table start location\n";
		std::cout << "   -p       - omit pass-avoidance optimizations\n";
		std::cout << "   -q       - quiet mode (no messages except on error)\n";
		std::cout << "   -vN      - set Silf table version number\n";
		std::cout << "   -wNNNN   - ignore warning with the given number\n";
		std::cout << "   -wall    - display all warnings\n";
		std::cout << "   -offsets - generate xoffset, yoffset, and gpoint glyph attributes\n";
		return 2;
	}

	bool fFatalErr = false;

	SetGdlAndFontFileNames(argv[1 + cargExtra], (argc > 2 + cargExtra) ? argv[2 + cargExtra] : NULL,
		&pchGdlFile, &pchFontFile);

	g_errorList.SetFileNameFromGdlFile(pchGdlFile);

	if (argc > 3 + cargExtra)
	{
		char * pch = argv[3 + cargExtra];
		strcpy(rgchOutputFile, pch);
		if (argc > 4 + cargExtra)
		{
			pch = argv[4 + cargExtra];
			Platform_AnsiToUnicode(pch, strlen(pch), rgchwOutputFontFamily, strlen(pch));
			fModFontName = true;

			// Give a warning if the font name has something bigger than 7-bit data.
			// The font-output routines can't handle that.
			for (char * pchLp = pch; pchLp - pch < signed(strlen(pch)); pchLp++)
			{
				if ((unsigned char)(*pchLp) > 0x7F)
				{
					g_errorList.AddWarning(511, NULL, "Non-ASCII font names are not supported");
					break;
				}
			}
		}
		//else // nice idea, but don't do this for now
		//{
		//	// output file and no font family; but if the string looks more like a font name, switch
		//	if (LooksLikeFontFamily(rgchOutputFile))
		//	{
		//		//std::string sta(pch);
		//		//StrUni stu = sta;
		//		//wcscpy(rgchwOutputFontFamily, stu.Chars());
		//		::MultiByteToWideChar(CP_ACP, 0, rgchOutputFile, -1,
		//			rgchwOutputFontFamily, strlen(rgchOutputFile));
		//		rgchOutputFile[0] = 0;
		//	}
		//}
	}
	
	// Is GDL file path absolute? So will be paths in GDX.
	bool fAbsGdlFilePaths = (pchGdlFile[1] == ':' || pchGdlFile[0] == '/' || pchGdlFile[0] == '\\');

	if (strcmp(rgchOutputFile, pchFontFile) == 0)
	{
		g_errorList.AddError(142, NULL, "Input and output font files cannot be the same.");
		fFatalErr = true;
	}

	if (rgchOutputFile[0] == 0)
	{
		// Calculate output file name from the input file name.
		if (g_cman.SeparateControlFile())
			GenerateOutputControlFileName(pchFontFile, rgchOutputFile); // gtf
		else
			GenerateOutputFontFileName(pchFontFile, rgchOutputFile);   // ttf
	}

	std::string staVersion = VersionString(g_cman.SilfTableVersion());

	if (g_cman.IsVerbose())
	{
		std::cout << "Reading input font...\n\n";
	}

	GrcFont * pfont = new GrcFont(pchFontFile);
	int nFontError = pfont->Init(&g_cman);

	// Calculate output font-family name.
	utf16 rgchwInputFontFamily[128];
	if (utf16len(rgchwOutputFontFamily) > 0)
	{
		Assert(fModFontName);	//////// ????????
	}
	else if (nFontError == 0)
	{
		// Assert(fModFontName); // seems to be false in regression tests
		pfont->GetFontFamilyName(rgchwInputFontFamily, 128);
		if (g_cman.SeparateControlFile())
			GenerateOutputControlFontFamily(rgchwInputFontFamily, rgchwOutputFontFamily);
		else
			utf16cpy(rgchwOutputFontFamily, rgchwInputFontFamily);
	}
	else
	{
		std::wstring stu(L"unknown");
		//utf16cpy(rgchwOutputFontFamily, (const utf16*)stu.Chars());
		std::copy(stu.data(), stu.data() + stu.length() + 1, rgchwOutputFontFamily);
	}

	if (utf16len(rgchwOutputFontFamily) > kMaxFontNameLength)
		g_errorList.AddError(141, NULL, "Font name is longer than 32 characters.");

	//std::string staFamily((char*)rgchwOutputFontFamily);
	char rgchFamily[128];
	memset(rgchFamily, 0, sizeof(char) * 128);
	int cchw = 0;
	utf16 * pchw = rgchwOutputFontFamily;
	while (*pchw++)
		cchw++;
	Platform_UnicodeToANSI(rgchwOutputFontFamily, cchw, rgchFamily, 128);
	std::string staFamily(rgchFamily);
	if (g_cman.IsVerbose())
	{
		std::cout << "GDL file: " << pchGdlFile << "\n"
			<< "PreProcessor: " << staGdlppFile << "\n"
			<< "Input TT file: " << (pchFontFile ? pchFontFile : "none") << "\n"
			<< "Output TT file: " << rgchOutputFile << "\n"
			<< "Output font name: " << staFamily << ((fModFontName) ? "" : " (unchanged)") << "\n"
			<< "Silf table version " << (g_cman.UserSpecifiedVersion() ? "requested" : "(default)")
					<< ": " << staVersion << "\n\n";
	}

	// Simple test for illegal UTF encoding in file. GDL requires 7 bit codepoints
	gr::byte bFirst, bSecond, bThird;
	std::ifstream strmGdl;
	strmGdl.open(pchGdlFile, std::ios_base::in | std::ios_base::binary);
	if (strmGdl.fail())
	{
		fFatalErr = true;
		g_errorList.AddError(1105, NULL,
			"File ",
			pchGdlFile,
			" does not exist--compilation aborted");
	}
	
	bool fEncodingErr = false;
	if (!fFatalErr)
	{
		strmGdl >> bFirst >> bSecond >> bThird;
		strmGdl.close();

		if ((bFirst == 0xFF && bSecond == 0xFE) || (bFirst == 0xFE && bSecond == 0xFF))
		{
			fEncodingErr = true;
			g_errorList.AddError(130, NULL, "Illegal encoding in GDL file - probably UTF-16 encoding.");
		}
		else if (bFirst == 0xEF && bSecond == 0xBB && bThird == 0xBF)
		{
			fEncodingErr = true;
			g_errorList.AddError(131, NULL, "Illegal encoding in GDL file - probably UTF-8 encoding.");
		}
		else if (bFirst & 0x80 || bSecond & 0x80 || bThird & 0x80)
		{ // not really a UTF check but might as well test for illegal values here
			fEncodingErr = true;
			g_errorList.AddError(132, NULL, "Illegal encoding in GDL file - only 7 bit characters are legal.");
		}
		if (fEncodingErr)
		{
			fFatalErr = true;
			std::cout << "Illegal encoding in GDL file.\n";
			g_errorList.AddError(140, NULL,
				"Illegal encoding in GDL file");
		}
	}

	if (!fFatalErr)
	{
		// Calculate the length of the path part of the output file name.
		int cchOutputPath = strlen(rgchOutputFile);
		while (cchOutputPath > 0 && rgchOutputFile[cchOutputPath] != '\\')
			cchOutputPath--;
		char rgchOutputPath[128];
		memset(rgchOutputPath, 0, isizeof(char) * 128);
		memcpy(rgchOutputPath, rgchOutputFile, cchOutputPath); /* don't include \ */

		if (g_cman.IsVerbose())
			std::cout << "Parsing file " << pchGdlFile << "...\n";

		if (!g_cman.Parse(pchGdlFile, staGdlppFile, rgchOutputPath))
		{
			fFatalErr = true;
			std::cout << "Parsing failed.\n";
			g_errorList.AddError(139, NULL,
				"Parsing failed");
		}
	}

	if (!fFatalErr)
	{
		if (g_cman.IsVerbose())
			std::cout << "Initial processing...\n";
		if (!g_cman.PostParse())
		{
			fFatalErr = true;
			std::cout << "Initial processing failed.\n";
			g_errorList.AddError(138, NULL,
				"Initial processing failed");
		}
	}

	if (!fFatalErr)
	{
		if (nFontError != 0)
		{
			fFatalErr = true;
			if (nFontError == 7)
			{	// special case - want to avoid font copyright violations
				std::cout << "Font already contains Graphite table(s).\n";
				std::cout << "Please recompile with original (non-Graphite) font.\n";
				// similar error msg already in g_errorList
			}
			std::cout << "Could not open font--error code = " << nFontError << "\n";
			char rgch[20];
			itoa(nFontError, rgch, 10);
			g_errorList.AddError(137, NULL,
				"Could not open font--error code = ", rgch);
		}
	}

	if (!fFatalErr)
	{
		if (g_cman.IsVerbose())
			std::cout << "Checking for errors...\n";

		if (g_cman.SilfTableVersion() > g_cman.MaxSilfVersion())
		{
			g_errorList.AddError(133, NULL,
				"Invalid font table version: ",
				VersionString(g_cman.SilfTableVersion()));
		}
		if (g_cman.NameTableStart() != -1
			&& (g_cman.NameTableStart() < g_cman.NameTableStartMin()
				|| g_cman.NameTableStart() > 32767))
		{
			char rgch[20];
			itoa(g_cman.NameTableStart(), rgch, 10);
			g_errorList.AddError(134, NULL,
				"Invalid name table start ID: ", rgch,
				"; must be in range 256 - 32767.");
		}
		fFatalErr = g_errorList.AnyFatalErrors();
	}

	if (!fFatalErr)
	{
		fFatalErr = !g_cman.PreCompile(pfont);
		fFatalErr = fFatalErr || g_errorList.AnyFatalErrors();
		if (fFatalErr)
		{
			std::cout << "Compilation failed.\n";
			if (g_errorList.NumberOfErrors() == 0)
				// Make sure some error message comes out.
				g_errorList.AddError(136, NULL, "Compilation failed");
		}
	}

	if (!fFatalErr)
	{
		if (g_cman.IsVerbose())
			std::cout << "Compiling...\n";
		g_cman.Compile(pfont);
		if (g_cman.OutputDebugFiles())
		{
			g_cman.DebugEngineCode();
			g_cman.DebugRulePrecedence();
			g_cman.DebugGlyphAttributes();
			g_cman.DebugClasses();
			//g_cman.DebugOutput();
			g_cman.DebugCmap(pfont);
			if (g_cman.IsVerbose())
				std::cout << "Debug files generated.\n";
		}
		if (g_cman.OutputDebugXml())
		{
			bool f = g_cman.DebugXml(pfont, rgchOutputFile, fAbsGdlFilePaths);
			if ( g_cman.IsVerbose())
			{
				if (f)
					std::cout << "Debugger XML file generated.\n";
				else
					std::cout << "Error in generating debugger XML.\n";
			}
		}

		int nRet = g_cman.OutputToFont(pchFontFile, rgchOutputFile,
		rgchwOutputFontFamily, fModFontName, rgchwInputFontFamily);
		if (nRet == 0)
		{
			if (g_cman.IsVerbose())
				std::cout << "Compilation successful!\n";
		}
		else
		{
			std::cout << "ERROR IN WRITING FONT FILE.\n";
			char rgch[20];
			itoa(nRet, rgch, 10);
			g_errorList.AddError(135, NULL,
				"Error in writing font file--error code = ", rgch);
		}
	}

	g_errorList.SortErrors();
	g_errorList.WriteErrorsToFile(pchGdlFile, pchFontFile,
		rgchOutputFile, staFamily,
		VersionString(g_cman.SilfTableVersion()), g_cman.SeparateControlFile());

	int cerrFatal = g_errorList.NumberOfErrors();
	int cerrWarning = g_errorList.NumberOfWarnings();
	int cerrWarningGiven = g_errorList.NumberOfWarningsGiven();	// ie, not ignored
	cerrFatal = cerrFatal - cerrWarning;
	int cerrWarningIgnored = cerrWarning - cerrWarningGiven;

	if (cerrFatal > 0)
	{
		std::cout << cerrFatal << " error" << (cerrFatal > 1 ? "s " : " ");
		if (cerrWarningGiven > 0)
			std::cout << "and " << cerrWarningGiven << " warning" << (cerrWarningGiven > 1 ? "s " : " ");
		std::cout << ((cerrFatal + cerrWarningGiven > 1) ? "have" : "has")
			<< " been output to gdlerr.txt";
		if (cerrWarningIgnored > 0)
			std::cout << " (" << cerrWarningIgnored
				<< ((cerrWarningIgnored > 1) ? " warnings" : " warning") << " ignored)";
		std::cout << ".\n";
	}
	else if (cerrWarningGiven > 0)
	{
		std::cout << cerrWarningGiven << " warning"
			<< (cerrWarningGiven > 1 ? "s have" : " has") << " been output to gdlerr.txt";
		if (cerrWarningIgnored > 0)
			std::cout << " (" << cerrWarningIgnored
				<< ((cerrWarningIgnored > 1) ? " warnings" : " warning") << " ignored)";
		std::cout << ".\n";
	}
	else if (cerrWarningIgnored > 0 && g_cman.IsVerbose())
	{
		std::cout << cerrWarningIgnored
			<< ((cerrWarningIgnored > 1) ? " warnings" : " warning") << " ignored.\n";
	}

	delete pfont;

#ifdef _WIN32 // COM thing
	::CoUninitialize();
#endif

	if (g_errorList.AnyFatalErrors())
		return 1;
	else
		return 0;
}

/*----------------------------------------------------------------------------------------------
    Interpret the compiler options, which are preceded by slashes  or hyphens
	in the argument list.
----------------------------------------------------------------------------------------------*/
void HandleCompilerOptions(char * arg)
{
    if (arg[1] == 'c')
    {
        g_cman.SetCompressor(ktcLZ4);
    }
    else if (arg[1] == 'd')	// XML debugger file
	{
		g_cman.SetOutputDebugFiles(true, false);
	}
	else if (arg[1] == 'D')	// all debugger files
	{
		g_cman.SetOutputDebugFiles(true, true);
	}
	else if (arg[1] == 'g')	// ignore bad glyphs
	{
		g_cman.SetIgnoreBadGlyphs(true);
	}
	else if (strcmp(arg+1, "wall") == 0)
	{
		g_errorList.ClearIgnoreWarnings();
	}
	else if (arg[1] == 'n' || arg[1] == 'v' || arg[1] == 'w')
	{
		int nValue = 0;
		char rgch[20];
		int i = 2;

		while (arg[i] >= '0' && arg[i] <= '9')
		{
			rgch[i - 2] = arg[i];
			nValue = (nValue * 10) + (arg[i] - '0');
			i++;
		}
		rgch[i - 2] = 0;

		if (arg[1] == 'n')
		{
			g_cman.SetNameTableStart(nValue);
		}
		else if (arg[1] == 'v')
		{
			int fxdVersion = nValue << 16; // put in "fixed" format

			// Give an error later if the version is invalid.

			g_cman.SetSilfTableVersion(fxdVersion, true);
		}
		else if (arg[1] == 'w')
		{
			g_errorList.SetIgnoreWarning(nValue);
		}
	}
	else if (arg[1] == 'q')
	{
		g_cman.SetVerbose(false);
	}
	else if (arg[1] == 'p')
	{
		g_cman.SetPassOptimizations(false);
	}
	else if (strcmp(arg+1, "offsets") == 0)
	{
		g_cman.SetOffsetAttrs(true);
	}

	//else if (arg[1] == 's')
	//{
	//	g_cman.SetSeparateControlFile(true);
	//}
}

/*----------------------------------------------------------------------------------------------
    Try to be clever about the GDL-file and TTF-file arguments.
	Don't do this for now, because it can confuse the user. Just given a warning.
----------------------------------------------------------------------------------------------*/
void SetGdlAndFontFileNames(char * pchFile1, char * pchFile2,
	char ** ppchGdlFile, char ** ppchFontFile)
{
	gr::byte bFirst1, bSecond1, bThird1, bFourth1, bFirst2, bSecond2;
	std::ifstream strm1;
	strm1.open(pchFile1, std::ios_base::in | std::ios_base::binary);
	strm1 >> bFirst1 >> bSecond1 >> bThird1 >> bFourth1;
	strm1.close();

	std::ifstream strm2;
	strm2.open(pchFile2, std::ios_base::in | std::ios_base::binary);
	strm2 >> bFirst2 >> bSecond2;
	strm2.close();

	//if (bFirst1 == 0 && bThird1 == 0 && bFourth1 == 0 // && bSecond = 1 -- this can change with the version number
	//	&& bFirst2 != 0 && bSecond2 != 0)
	//{
	//	// pchFile1 looks like the beginning of a TTF file, and pchFile2 could be a GDL file.
	//	// So swap the arguments.
	//	*ppchFontFile = pchFile1;
	//	*ppchGdlFile = pchFile2;
	//}
	//else
	//{
	//	*ppchGdlFile = pchFile1;
	//	*ppchFontFile = pchFile2;
	//}

	if (bFirst1 == 0 && bThird1 == 0 && bFourth1 == 0 // && bSecond = 1 -- this can change with the version number
		&& bFirst2 != 0 && bSecond2 != 0)
	{
		g_errorList.AddWarning(512, NULL, "Did you switch the names of the GDL and TTF files?");
	}

	*ppchGdlFile = pchFile1;
	*ppchFontFile = pchFile2;
}

/*----------------------------------------------------------------------------------------------
    Calculate the default name of the output font, based on the input font. If the
	original font is name xyz.ttf, the Graphite version will be xyz_gr.ttf. If we are
	putting the Graphite font tables into a separate control file, the file name will
	be xyz.gtf.
----------------------------------------------------------------------------------------------*/
void GenerateOutputFontFileName(char * pchFontFile, char * pchOutputFont)
{
	char * pchIn = pchFontFile;
	while (*pchIn != 0)
		pchIn++;

	while (*pchIn != '\\' && *pchIn != ':' && pchIn >= pchFontFile)
		pchIn--;
	pchIn++;

	char * pchOut = pchOutputFont;
	while (*pchIn != '.' && *pchIn != 0)
	{
		*pchOut++ = *pchIn++;
	}

	*pchOut++ = '_';
	*pchOut++ = 'g';
	*pchOut++ = 'r';

	while (*pchIn != 0)
		*pchOut++ = *pchIn++;
	*pchOut = 0;
}

/*----------------------------------------------------------------------------------------------
    Calculate the default name of the output control file, based on the input font. We remove
	the extension and append ".gtf".
----------------------------------------------------------------------------------------------*/
void GenerateOutputControlFileName(char * pchFontFile, char * pchOutputFont)
{
	char * pchIn = pchFontFile;
	while (*pchIn != 0)
		pchIn++;

	while (*pchIn != '\\' && *pchIn != ':' && pchIn >= pchFontFile)
		pchIn--;
	pchIn++;

	char * pchOut = pchOutputFont;
	while (*pchIn != '.' && *pchIn != 0)
	{
		*pchOut++ = *pchIn++;
	}

	*pchOut++ = '.';
	*pchOut++ = 'g';
	*pchOut++ = 't';
	*pchOut++ = 'f';
	*pchOut = 0;
}

/*----------------------------------------------------------------------------------------------
    Calculate the font name that will be put into the output control file,
	based on the input font name. We append "Graphite" to the font name.
----------------------------------------------------------------------------------------------*/
void GenerateOutputControlFontFamily(utf16 * pchFontName, utf16 * pchOutputName)
{
	wchar_t rgchw[120];
	std::copy(pchFontName, pchFontName + 120, rgchw);
	std::wstring stu(rgchw);
	stu.append(L" Graphite");
	std::copy(stu.data(), stu.data() + stu.length() + 1, pchOutputName);
	//utf16cpy(pchOutputName, (const utf16*)stu.Chars());
	//pchOutputName[stu.Length() + 1] = 0;
}

/*----------------------------------------------------------------------------------------------
    Return true if what is supposedly the output file name looks more like the family name;
	ie, if it has spaces in it and doesn't have '.' or '\'
----------------------------------------------------------------------------------------------*/
bool LooksLikeFontFamily(char * pch)
{
	bool fSpaces = false;
	while (*pch != 0)
	{
		if (*pch == ' ')
			fSpaces = true;
		else if (*pch == '\\' || *pch == '.')
			return false;
		pch++;
	}
	return fSpaces;
}

/*----------------------------------------------------------------------------------------------
    Generate a string containing the version number, eg: 0x00030002 -> "3.2"
----------------------------------------------------------------------------------------------*/
std::string VersionString(int fxdVersion)
{
	std::string sta = "";
	char rgch[20];
	itoa(fxdVersion >> 16, rgch, 10);
	sta += rgch;
	sta += ".";
	itoa(fxdVersion & 0x0000FFFF, rgch, 10);
	sta += rgch;
	return sta;
}
