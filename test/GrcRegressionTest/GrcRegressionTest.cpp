/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 2007 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GrcRegressionTest.cpp
Responsibility: Sharon Correll

Description:
    Main file for the Graphite regression test program.
-------------------------------------------------------------------------------*//*:End Ignore*/

#include "main.h"

//:>********************************************************************************************
//:>	Global variables
//:>********************************************************************************************
std::ofstream g_strmLog;	// log file output stream

int g_errorCount;

bool g_debugMode = false;
bool g_silentMode = false;
char * g_benchmarkName = NULL;
char * g_testName = NULL;
char g_logFileName[128];

int g_itcaseStart = 0;  // adjust to skip to a certain test

// Forward defintions.
int WriteLog(int);
void CopyWstringToUtf16(std::wstring textStr, gr::utf16 * utf16Buf, int bufSize);

//:>********************************************************************************************
//:>	Functions
//:>********************************************************************************************

/*----------------------------------------------------------------------------------------------
	Main function.
----------------------------------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
#ifdef _WIN32
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif // WIN32

	if (argc < 3)
	{
		std::cout << "\nUsage: GrcRegressionTest [options] benchmark-file test-file\n";
		std::cout << "\nOptions:\n";
		std::cout << "   -d       - debug mode\n";
		std::cout << "   -s       - silent mode\n";
		std::cout << "   -l file  - specify log file name\n";
		return -1;
	}

	int iargc = 1;
	while (iargc < argc)
	{
		if (strcmp(argv[iargc], "/d") == 0 || strcmp(argv[iargc], "-d") == 0)
		{
			g_debugMode = true;
		}
		else if (strcmp(argv[iargc], "/s") == 0 || strcmp(argv[iargc], "-s") == 0)
		{
			g_silentMode = true;
		}
		else if (strcmp(argv[iargc], "/l") == 0 || strcmp(argv[iargc], "-l") == 0)
		{
			iargc++;
			if (iargc < argc)
			{
				strcpy(g_logFileName, argv[iargc]);
				//g_logFileName = argv[iargc];
			}
				
		}
        else if (g_benchmarkName)
        {
			if (g_testName) {
				std::cout << "Incorrect number of arguments";
				return -1;
			}
            g_testName = argv[iargc];
        }
        else
        {
            g_benchmarkName = argv[iargc];
        }
		iargc++;
	}

	if (!g_silentMode)
		std::cout << "Graphite Compiler Regression Test\n\n";

	//	Start a new log.
	if (strlen(g_logFileName) == 0)
	{
		memset(g_logFileName, 0, 128);
	    strcpy(g_logFileName, "grcregtest.log");
	}
	g_strmLog.open(g_logFileName, std::ios_base::out | std::ios_base::app );
	if (g_strmLog.fail())
	{
		std::cout << "Unable to open log file.";
		return -1;
	}

	if (g_testName == NULL || g_benchmarkName == NULL)
	{
		std::cout << "Font files not adequately specified\n\n";
		return -1;
	}

	g_errorCount = 0;

	//WriteToLog("Graphite Compiler Regression Test\n\n");

	// *** TEST CASES ***
	/***
	const int numberOfTests = 5;
	TestCase rgtcaseList[5];

	rgtcaseList[0].m_testName.assign("Scheherazade");
	rgtcaseList[0].m_fontFileBmark.assign("SchBenchmark.ttf");
	rgtcaseList[0].m_fontFileTest.assign("SchTest.ttf");
	rgtcaseList[0].m_debug = false;
	rgtcaseList[0].m_skip = false;

	rgtcaseList[1].m_testName.assign("Charis");
	rgtcaseList[1].m_fontFileBmark.assign("CharisBenchmark.ttf");
	rgtcaseList[1].m_fontFileTest.assign("CharisTest.ttf");
	rgtcaseList[1].m_debug = false;
	rgtcaseList[1].m_skip = false;

	rgtcaseList[2].m_testName.assign("PigLatin - Silf version 2");
	rgtcaseList[2].m_fontFileBmark.assign("PigLatinBenchmark_v2.ttf");
	rgtcaseList[2].m_fontFileTest.assign("PigLatinTest_v2.ttf");
	rgtcaseList[2].m_debug = false;
	rgtcaseList[2].m_skip = false;

	rgtcaseList[3].m_testName.assign("PigLatin - Silf version 3");
	rgtcaseList[3].m_fontFileBmark.assign("PigLatinBenchmark_v3.ttf");
	rgtcaseList[3].m_fontFileTest.assign("PigLatinTest_v3.ttf");
	rgtcaseList[3].m_debug = false;
	rgtcaseList[3].m_skip = false;

	rgtcaseList[4].m_testName.assign("Padauk");
	rgtcaseList[4].m_fontFileBmark.assign("PadaukBenchmark.ttf");
	rgtcaseList[4].m_fontFileTest.assign("PadaukTest.ttf");
	rgtcaseList[4].m_debug = false;
	rgtcaseList[4].m_skip = false;

	// *** Add tests here, and increment numberOfTests. ***

	//RunTests(numberOfTests, rgtcaseList);
	****/

	// Run the test.

	WriteToLog("\nTesting ");
	WriteToLog(g_testName);
	WriteToLog(" against ");
	WriteToLog(g_benchmarkName);
	WriteToLog("...\n");
	if (!g_silentMode)
		std::cout << "Testing " << g_testName << " against " << g_benchmarkName << "...\n";

	GrcRtFileFont fontBmark(g_benchmarkName, 12.0, 96, 96);
	GrcRtFileFont fontTest(g_testName,  12.0, 96, 96);
	int g_errorCount = CompareFontTables(NULL, &fontBmark, &fontTest);
    WriteLog(g_errorCount);

	WriteToLog("\n==============================================\n");

	//g_strmLog << "\n\nTOTAL NUMBER OF ERRORS:  " << g_errorCount << "\n";
	//if (!g_silentMode)
	//	std::cout << "\n\nTOTAL NUMBER OF ERRORS:  " << g_errorCount << "\n";

	g_strmLog.close();

	return g_errorCount;
}

/*----------------------------------------------------------------------------------------------
	Run the tests.
----------------------------------------------------------------------------------------------*/
void RunTests(int numberOfTests, TestCase * ptcaseList)
{
	for (int itcase = g_itcaseStart; itcase < numberOfTests; itcase++)
	{
		TestCase * ptcase = ptcaseList + itcase;
		WriteToLog("\n----------------------------------------------\n");
		WriteToLog("Test: ");

		if (!g_silentMode)
			std::cout << "Test " << ptcase->m_testName << "...";
		WriteToLog(ptcase->m_testName);

		if (ptcase->m_skip)
		{
			if (!g_silentMode) std::cout << "skipping\n";
			WriteToLog("...skipping\n");
			continue;
		}

		WriteToLog("\n");

		RunOneTestCase(ptcase);
	}
}

/*----------------------------------------------------------------------------------------------
	Run a single test case.
----------------------------------------------------------------------------------------------*/
int RunOneTestCase(TestCase * ptcase)
{
#ifdef _MSC_VER
#if  (_WIN32_WINNT > 0x0400)
	// Break into the debugger if requested.
	if (ptcase->m_debug ) ///&& ::IsDebuggerPresent())
	{
		::DebugBreak();
	}
#endif
#endif

	GrcRtFileFont fontBmark(ptcase->m_fontFileBmark, 12.0, 96, 96);
	GrcRtFileFont fontTest( ptcase->m_fontFileTest,  12.0, 96, 96);

	int errorCount = CompareFontTables(ptcase, &fontBmark, &fontTest);

    return WriteLog(errorCount);
}


/*----------------------------------------------------------------------------------------------
	Write the error count to the log.
----------------------------------------------------------------------------------------------*/
int WriteLog(int errorCount)
{
	WriteToLog("\nError count = ");
	WriteToLog(errorCount);
	WriteToLog("\n");

	if (!g_silentMode)
	{
		if (errorCount == 0)
			std::cout << "ok\n";
		else
			std::cout << "FAILED\n";
	}

	g_errorCount += errorCount;
	return errorCount;
}

/*----------------------------------------------------------------------------------------------
	Copy a std::wstring (whose bytes can be of various sizes on different platforms)
	to a buffer of UTF16.
----------------------------------------------------------------------------------------------*/
void CopyWstringToUtf16(std::wstring textStr, gr::utf16 * utf16Buf, int bufSize)
{
	std::fill_n(utf16Buf, bufSize, 0);
	int cc = textStr.length();
	for (int i = 0; i < cc; i++)
		utf16Buf[i] = textStr[i];
}

/*----------------------------------------------------------------------------------------------
	Output information about an error.
----------------------------------------------------------------------------------------------*/
void OutputError(int & errCnt, TestCase * ptcase, std::string strErr, int i)
{
	OutputErrorAux(ptcase, strErr, i, "", -1, false, 0, 0);
	errCnt++;
}

void OutputErrorWithValues(int & errCnt, TestCase * ptcase, std::string strErr, int i,
	int valueFound, int valueExpected)
{
	OutputErrorAux(ptcase, strErr, i, "", -1, true, valueFound, valueExpected);
	errCnt++;
}

void OutputError(int & errCnt, TestCase * ptcase, std::string strErr1, int i1,
	std::string strErr2, int i2)
{
	OutputErrorAux(ptcase, strErr1, i1, strErr2, i2, false, 0, 0);
	errCnt++;
}

void OutputErrorAux(TestCase * ptcase,
	std::string strErr1, int i1, std::string strErr2, int i2,
	bool showValues, int valueFound, int valueExpected)
{
//	if (g_debugMode)
//		::DebugBreak();

	if (!g_silentMode)
	{
		//std::cout << ptcase->TestName() << ": ";
		std::cout << strErr1;
		if (i1 > -1)
		{
			std::cout << "[" << i1 << "]";
		}
		std::cout << strErr2;
		if (i2 > -1)
		{
			std::cout << "[" << i2 << "]";
		}
		std::cout << "\n";
	}

	WriteToLog(strErr1, i1, strErr2, i2, showValues, valueFound, valueExpected);

	WriteToLog("\n");
}

/*----------------------------------------------------------------------------------------------
	Write some text to the log file.
----------------------------------------------------------------------------------------------*/
bool WriteToLog(std::string str, int i)
{
	return WriteToLog(str, i, "", -1);
}

bool WriteToLog(std::string str1, int i1, std::string str2, int i2)
{
	if (g_strmLog.fail())
	{
		std::cout << "Error opening log file.";
		return false;
	}
	g_strmLog << str1;
	if (i1 > -1)
		g_strmLog << "[" << i1 << "]";
	g_strmLog << str2;
	if (i2 > -1)
		g_strmLog << "[" << i2 << "]";
	g_strmLog.flush();
	return true;
}

bool WriteToLog(std::string str1, int i1, std::string str2, int i2,
	bool showValues, int valueFound, int valueExpected)
{
	if (g_strmLog.fail())
	{
		std::cout << "Error opening log file.";
		return false;
	}
	g_strmLog << str1;
	if (i1 > -1)
		g_strmLog << "[" << i1 << "]";
	g_strmLog << str2;
	if (i2 > -1)
		g_strmLog << "[" << i2 << "]";
	if (showValues)
	{
		g_strmLog << "; found " << valueFound << " not " << valueExpected;
	}
	g_strmLog.flush();
	return true;
}


bool WriteToLog(int n)
{
	if (g_strmLog.fail())
	{
		std::cout << "Error opening log file.";
		return false;
	}
	g_strmLog << n;
	g_strmLog.flush();
	return true;
}

std::wstring StringFromNameTable(const gr::byte * pNameTbl, int nLangID, int nNameID)
{
	std::wstring stuName;
	stuName.erase();
	size_t lOffset = -1;
	size_t lSize = -1;

	// The Graphite compiler stores our names in either 
	// the MS (platform id = 3) Unicode (writing system id = 1) table
	// or the MS Symbol (writing system id = 0) table. Try MS Unicode first.
	// lOffset & lSize are in bytes.
	// new interface:
	if (!TtfUtil::GetNameInfo(pNameTbl, 3, 1, nLangID, nNameID, lOffset, lSize))
	{
		if (!TtfUtil::GetNameInfo(pNameTbl, 3, 0, nLangID, nNameID, lOffset, lSize))
		{
			return stuName;
		}
	}

	size_t cchw = (unsigned(lSize) / sizeof(utf16));
	utf16 * pchwName = new utf16[cchw+1]; // lSize - byte count for Uni str
	const utf16 *pchwSrcName = reinterpret_cast<const utf16*>(pNameTbl + lOffset);
    std::transform(pchwSrcName, pchwSrcName + cchw, pchwName, [] (const utf16 &cp) { return lsbf(cp); });
	pchwName[cchw] = 0;  // zero terminate
	#ifdef _WIN32
		stuName.assign((const wchar_t*)pchwName, cchw);
	#else
		wchar_t * pchwName32 = new wchar_t[cchw+1]; // lSize - byte count for Uni str
		for (int i = 0; i <= signed(cchw); i++) {
			pchwName32[i] = pchwName[i];
		}
		stuName.assign(pchwName32, cchw);
		delete [] pchwName32;
	#endif

	delete [] pchwName;
	return stuName;
}

