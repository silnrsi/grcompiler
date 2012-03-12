/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 2007 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: Main.h
Responsibility: Sharon Correll

Description:
    Header files to include in the Graphite compiler regression test program.
-------------------------------------------------------------------------------*//*:End Ignore*/

#ifdef _MSC_VER
#pragma once
#endif
#ifndef GRCOMPILER_H
#define GRCOMPILER_H 1

#define NO_EXCEPTIONS 1

// To allow call to IsDebuggerPresent:
//////#define _WIN32_WINNT WINVER

//:>********************************************************************************************
//:>	Include files
//:>********************************************************************************************
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

#include "stdafx.h"
////#include "resource.h"
////#include <hash_map>
#include <fstream>
#include <iostream>
#include <vector>
#include <functional>	// ptr_fun
////#include <algorithm>
#include <string>
#include <cstring>
#ifdef _WIN32
#include <crtdbg.h>
#endif // _WIN32
#include <assert.h>

////using std::max;
////using std::min;

#include "Generic/GrCommon.h"
#include "Generic/GrPlatform.h"

////////#include "LgCharPropsStub.h"

#include "Generic/GrConstants.h"
#include "TtfUtil.h"
// #include "graphite/Tt.h"

//#include "graphite/GrClient.h"
//#include "graphite/IGrEngine.h"
#include "FileInput.h"
//#include "graphite/Font.h"
#include "GrcRtFileFont.h"

//#include "Segment.h"
//#include "SegmentPainter.h"

#include "TestCase.h"

using namespace gr;

//:>********************************************************************************************
//:>	Functions
//:>********************************************************************************************
void RunTests(int numberOfTests, TestCase * ptcaseList);
int RunOneTestCase(TestCase * ptcase);
void OutputError(int & errCnt, TestCase * ptcase, std::string strErr, int i = -1);
void OutputErrorWithValues(int & errCnt, TestCase * ptcase, std::string strErr, int i,
	int valueFound, int valueExpected);
void OutputError(int & errCnt, TestCase * ptcase, std::string strErr1, int i1,
	std::string strErr2, int i2 = -1);
void OutputErrorAux(TestCase * ptcase, std::string strErr1, int i1, std::string strErr2, int i2,
	bool showValues, int valueFound, int valueExpected);
bool WriteToLog(std::string str, int i = -1);
bool WriteToLog(std::string str1, int i1, std::string str2, int i2 = -1);
bool WriteToLog(std::string str1, int i1, std::string str2, int i2,
	bool showValues, int valueFound, int valueExpected);
bool WriteToLog(int n);
std::wstring StringFromNameTable(const gr::byte * pNameTbl, int nLangID, int nNameID);

int CompareFontTables(TestCase * ptcase, GrcRtFileFont * pfontBmark, GrcRtFileFont * pfontTest);
void CompareSilfTables(int & errCnt, TestCase * ptcase, GrIStream & grstrmB, GrIStream & grstrmT,
	int * pchwMaxGlyphID);
void CompareClassMaps(int & ec, TestCase * ptcase, GrIStream & grstrmB, GrIStream & grstrmT);
void ComparePasses(int & ec, TestCase * ptcase, GrIStream & grstrmB, GrIStream & grstrmT,
   int fxdSilfVersionB, int fxdSilfVersionT, int cPasses,
   int lSubTableStartB, int lSubTableStartT, int * prgnPassOffsets);
int CompareFsmTables(int & ec, TestCase * ptcase,
	GrIStream & grstrmGlatB, GrIStream & grstrmGlocB,
	int fxdSilfVersionB, int fxdSilfVersionT, int iPass);
void CompareGlatAndGlocTables(int & errCnt, TestCase * ptcase, int chwMaxGlyphID,
	GrIStream & grstrmGlatB, GrIStream & grstrmGlocB,
	GrIStream & grstrmGlatT, GrIStream & gtstrmGlocT);
void CompareFeatTables(int & ec, TestCase * ptcase, GrIStream & grstrmB, GrIStream & grstrmT,
	const gr::byte * pNameTblB, const gr::byte * pNameTblT);
void CompareSillTables(int & ec, TestCase * ptcase, GrIStream & grstrmB, GrIStream & grstrmT);
int ReadVersion(GrIStream & grstrm);

#endif //!GRCOMPILER_H

