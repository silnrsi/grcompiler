/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GdlNameDefn.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
    GdlNameDefn contains collections of informative strings.
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef NAMEDEFN_INCLUDED
#define NAMEDEFN_INCLUDED

/*----------------------------------------------------------------------------------------------
Class: GdlNameDefn
Description: Each GdlNameDefn contains a string (or a collection strings in multiple languages)
	giving a piece of information about the renderer (version, author, comment, etc.).
	These are stored in the GdlRenderer, keyed under the numeric identifier for the name.
Hungarian: ndefn
----------------------------------------------------------------------------------------------*/

class GdlNameDefn : public GdlObject
{
public:
	GdlExtName * ExtName(int i)
	{
		return &m_vextname[i];
	}

	void AddExtName(utf16 wLangID, std::wstring stu)
	{
		m_vextname.push_back(GdlExtName(stu, wLangID));
	}
	void AddExtName(utf16 wLangID, GdlExpression * pexp)
	{
		GdlStringExpression * pexpString = dynamic_cast<GdlStringExpression*>(pexp);
		Assert(pexpString);

		std::wstring stu = pexpString->ConvertToUnicode();
		m_vextname.push_back(GdlExtName(stu, wLangID));
	}

	void DeleteExtName(int i)
	{
		Assert(i < static_cast<int>(m_vextname.size()));
		m_vextname.erase(m_vextname.begin() + i);
	}

	size_t NameCount()
	{
		 return m_vextname.size();
	}

protected:
	std::vector<GdlExtName>	m_vextname;
};


typedef std::map<int, GdlNameDefn *> NameDefnMap;



#endif // NAMEDEFN_INCLUDED
