/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 2007 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: TestCase.h
Responsibility: Sharon Correll

Description:
    Definition of TestCase class for Graphite Compiler regression test program.
-------------------------------------------------------------------------------*//*:End Ignore*/

#ifdef _MSC_VER
#pragma once
#endif
#ifndef TESTCASE_H
#define TESTCASE_H 1

#define NO_EXCEPTIONS 1

class TestCase
{
public:
	TestCase()
	{
		m_fxdSilfVersion = 0x00010000;
		m_fxdFeatVersion = 0x00010000;
		m_fxdGlocVersion = 0x00010000;
		m_fxdGlatVersion = 0x00010000;
		m_fxdSillVersion = 0x00010000;
	}

	std::string m_testName;
	std::string m_fontFileBmark;
	std::string m_fontFileTest;
	bool m_debug;		// break into the debugger when running this test
	bool m_skip;		// easy way to temporarily skip the test

	// Font table versions expected in the benchmark file:
	int m_fxdSilfVersion;
	int m_fxdFeatVersion;
	int m_fxdGlocVersion;
	int m_fxdGlatVersion;
	int m_fxdSillVersion;
};


#endif // !TESTCASE_H

