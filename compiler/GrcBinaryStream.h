/*--------------------------------------------------------------------*//*:Ignore this sentence.
Copyright (C) 1999, 2001 SIL International. All rights reserved.

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: GrcBinaryStream.h
Responsibility: Sharon Correll
Last reviewed: Not yet.

Description:
-------------------------------------------------------------------------------*//*:End Ignore*/
#ifdef _MSC_VER
#pragma once
#endif
#ifndef GRC_BINSTRM_INCLUDED
#define GRC_BINSTRM_INCLUDED

#include <string>
#include <sstream>
using std::fstream;

/*----------------------------------------------------------------------------------------------
Class: GrcBinaryStream
Description: Our stream for writing to the TrueType font.
Hungarian: bstrm
----------------------------------------------------------------------------------------------*/

class GrcBinaryStream : public fstream
{
public:
	GrcBinaryStream(const char * stFileName)
		: fstream(stFileName, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc)
	{
	}

	~GrcBinaryStream()
	{
	}

public:
    template<typename T> void WriteByte(T);
	template<typename T> void WriteShort(T);
	template<typename T> void WriteInt(T);
	
    void Write(void * pbTable, size_t cbSize)
	{
		write(static_cast<char *>(pbTable), cbSize);
	}

	offset_t Position()
	{
		return offset_t(tellp());
	}

	void SetPosition(offset_t lPos)
	{
		seekp(offset_t(lPos));
	}

	offset_t SeekPadLong(offset_t ibOffset);

	void Close(void)
	{
		close();
	}
};


/*----------------------------------------------------------------------------------------------
Class: GrcSubStream
Description: A substream that will eventually be output on the main stream.
Hungarian: substrm
----------------------------------------------------------------------------------------------*/
class GrcSubStream
{
public:
	template<typename T> void WriteByte(T);
	template<typename T> void WriteShort(T);
	template<typename T> void WriteInt(T);

public:
	std::ostream m_strm;
};

/*----------------------------------------------------------------------------------------------
Class: GrcDiversion
Description: Divert output on a given stream a temporary in-memory buffer in constructor.
  Restore i/o to streams original target and write buffer on call to undivert() or the destructor.
Hungarian: dstrm
----------------------------------------------------------------------------------------------*/
class GrcDiversion : public std::stringbuf
{
    std::streambuf * m_sbSaved;
    std::iostream   & m_strm;

    // Disable copying of any sort
    GrcDiversion(const GrcDiversion &);
    GrcDiversion & operator = (const GrcDiversion &);

public:
    GrcDiversion(std::iostream & strm, const std::string sInitial="")
        : std::stringbuf(std::ios::in | std::ios::out | std::ios::ate), m_sbSaved(0), m_strm(strm)
    {
        divert(sInitial);
    }

    ~GrcDiversion()
    {
        undivert();
    }

    void divert(const std::string sInitial="")
    {
        undivert();
        m_strm.flush();
        str(sInitial);
        m_sbSaved = m_strm.rdbuf(this);
    }

    void undivert()
    {
        if (m_sbSaved)
        {
            m_strm.flush();
            m_strm.rdbuf(m_sbSaved);
            m_strm.write(str().data(), str().size());
            m_strm.flush();
            str("");
            m_sbSaved = 0;
        }
    }
};

#endif // !GRC_BINSTRM_INCLUDED
