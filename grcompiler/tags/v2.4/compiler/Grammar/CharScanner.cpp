/**
 * <b>SOFTWARE RIGHTS</b>
 * <p>
 * ANTLR 2.6.0 MageLang Insitute, 1998
 * <p>
 * We reserve no legal rights to the ANTLR--it is fully in the
 * public domain. An individual or company may do whatever
 * they wish with source code distributed with ANTLR or the
 * code generated by ANTLR, including the incorporation of
 * ANTLR, or its output, into commerical software.
 * <p>
 * We encourage users to develop software with ANTLR. However,
 * we do ask that credit is given to us for developing
 * ANTLR. By "credit", we mean that if you use ANTLR or
 * incorporate any source code into one of your programs
 * (commercial product, research project, or otherwise) that
 * you acknowledge this fact somewhere in the documentation,
 * research report, etc... If you like ANTLR and have
 * developed a nice tool with the output, please mention that
 * you developed it using ANTLR. In addition, we ask that the
 * headers remain intact in our source code. As long as these
 * guidelines are kept, we expect to continue enhancing this
 * system and expect to make other tools available as they are
 * completed.
 * <p>
 * The ANTLR gang:
 * @version ANTLR 2.6.0 MageLang Insitute, 1998
 * @author Terence Parr, <a href=http://www.MageLang.com>MageLang Institute</a>
 * @author <br>John Lilley, <a href=http://www.Empathy.com>Empathy Software</a>
 * @author <br><a href="mailto:pete@yamuna.demon.co.uk">Pete Wells</a>
 */

#include "Antlr/CharScanner.hpp"
#include "Antlr/CommonToken.hpp"

#include "Antlr/TokenStream.hpp"
#include "Antlr/ScannerException.hpp"
#include <map>
#include <cctype>
#include <iostream>
#include <cstring>
#pragma warning(disable: 4355) // used in base member initializer list

CharScannerLiteralsLess::CharScannerLiteralsLess(const CharScanner* theScanner)
: scanner(theScanner)
{}

bool CharScannerLiteralsLess::operator() (const std::string& x,const std::string& y) const
{
	if (scanner->getCaseSensitiveLiterals()) {
		return std::less<std::string>()(x,y);
	} else {
#ifdef NO_STRCASECMP
		return (_stricmp(x.c_str(),y.c_str())<0);
#else
		return (strcasecmp(x.c_str(),y.c_str())<0);
#endif
	}
}

CharScanner::CharScanner(InputBuffer& cb)
	: saveConsumedInput(true) //, caseSensitiveLiterals(true)
	, literals(CharScannerLiteralsLess(this))
	, inputState(new LexerInputState(cb))
	, commitToPath(false)
{
	setTokenObjectFactory(&CommonToken::factory);
}

CharScanner::CharScanner(InputBuffer* cb)
	: saveConsumedInput(true) //, caseSensitiveLiterals(true)
	, literals(CharScannerLiteralsLess(this))
	, inputState(new LexerInputState(cb))
	, commitToPath(false)
{
	setTokenObjectFactory(&CommonToken::factory);
}

CharScanner::CharScanner(const LexerSharedInputState& state)
	: saveConsumedInput(true) //, caseSensitiveLiterals(true)
	, literals(CharScannerLiteralsLess(this))
	, inputState(state)
	, commitToPath(false)
{
	setTokenObjectFactory(&CommonToken::factory);
}

CharScanner::~CharScanner()
{
}

void CharScanner::append(char c)
{
	if (saveConsumedInput)
		text+=c;
}

void CharScanner::append(const std::string& s)
{
	if (saveConsumedInput)
		text+=s;
}

void CharScanner::commit()
{
	inputState->getInput().commit();
}

void CharScanner::consume()
{
	if (inputState->guessing == 0) {
		if (caseSensitive) {
			append(LA(1));
		} else {
			// use input.LA(), not LA(), to get original case
			// CharScanner.LA() would toLower it.
			append(inputState->getInput().LA(1));
		}
	}
	inputState->getInput().consume();
}

/** Consume chars until one matches the given char */
void CharScanner::consumeUntil(int c)
{
	while (LA(1) != EOF_CHAR && LA(1) != c)
	{
		consume();
	}
}

/** Consume chars until one matches the given set */
void CharScanner::consumeUntil(const BitSet& set)
{
	while (LA(1) != EOF_CHAR && !set.member(LA(1))) {
		consume();
	}
}

bool CharScanner::getCaseSensitive() const
{ return caseSensitive; }

//bool CharScanner::getCaseSensitiveLiterals() const
//{ return caseSensitiveLiterals; }

bool CharScanner::getCommitToPath() const
{ return commitToPath; }

const std::string& CharScanner::getFilename() const
{ return inputState->filename; }

InputBuffer& CharScanner::getInputBuffer()
{ return inputState->getInput(); }

LexerSharedInputState CharScanner::getInputState()
{ return inputState; }

int CharScanner::getLine() const
{ return inputState->line; }

// return a copy of the current text buffer
const std::string& CharScanner::getText() const
{ return text; }

RefToken CharScanner::getTokenObject() const
{ return _returnToken; }

int CharScanner::LA(int i)
{
	if ( caseSensitive ) {
		return inputState->getInput().LA(i);
	} else {
		return toLower(inputState->getInput().LA(i));
	}
}

RefToken CharScanner::makeToken(int t)
{
	RefToken tok=tokenFactory();
	tok->setType(t);
	tok->setLine(inputState->line);
	return tok;
}

int CharScanner::mark()
{
	return inputState->getInput().mark();
}

void CharScanner::match(int c)
{
	if ( LA(1) != c ) {
		throw ScannerException(std::string("mismatched char: '") + charName(LA(1)) + "' expected '"+charName(c)+"'", inputState->line);
	}
	consume();
}

void CharScanner::match(const BitSet& b)
{
	if (!b.member(LA(1))) {
		throw ScannerException(std::string("mismatched char: '") + charName(LA(1)) + "'", inputState->line);
	}
	consume();
}

void CharScanner::match(const std::string& s)
{
	int len = s.length();
	for (int i=0; i<len; i++) {
		if ( LA(1) != s[i] ) {
			throw ScannerException(std::string("mismatched char: '") + charName(LA(1)) + "'", inputState->line);
		}
		consume();
	}
}

void CharScanner::matchNot(int c)
{
	if ( LA(1) == c ) {
		throw ScannerException(std::string("mismatched char: '") + charName(LA(1)) + "'", inputState->line);
	}
	consume();
}

void CharScanner::matchRange(int c1, int c2)
{
	if (LA(1)<c1 || LA(1)>c2) {
		throw ScannerException(std::string("char out of range: '") + charName(LA(1)) + "'", inputState->line);
	}
	consume();
}

void CharScanner::newline()
{ ++inputState->line; }

void CharScanner::panic()
{
	std::cerr << "CharScanner: panic" << std::endl;
	exit(1);
}

void CharScanner::panic(const std::string& s)
{
	std::cerr << "CharScanner: panic: " << s << std::endl;
	exit(1);
}

/** Report exception errors caught in nextToken() */
void CharScanner::reportError(const ScannerException& ex)
{
	if (getFilename() == "")
		std::cerr << "Error: " << ex.toString() << std::endl;
	else
		std::cerr << "Error in " << getFilename() << ": " << ex.toString() << std::endl;
}

/** Parser error-reporting function can be overridden in subclass */
void CharScanner::reportError(const std::string& s)
{
	if (getFilename() == "")
		std::cerr << "Error: " << s << std::endl;
	else
		std::cerr << "Error in " << getFilename() << ": " << s << std::endl;
}

/** Parser warning-reporting function can be overridden in subclass */
void CharScanner::reportWarning(const std::string& s)
{
	if (getFilename() == "")
		std::cerr << "Warning: " << s << std::endl;
	else
		std::cerr << "Warning in " << getFilename() << ": " << s << std::endl;
}

void CharScanner::resetText()
{ text=""; }

void CharScanner::rewind(int pos)
{
	inputState->getInput().rewind(pos);
}

void CharScanner::setCaseSensitive(bool t)
{
	caseSensitive = t;
}

void CharScanner::setCommitToPath(bool commit)
{
	commitToPath = commit;
}

void CharScanner::setFilename(const std::string& f)
{ inputState->filename=f; }

void CharScanner::setLine(int l)
{ inputState->line=l; }

void CharScanner::setText(const std::string& s)
{ text=s; }

void CharScanner::setTokenObjectFactory(factory_type factory)
{ tokenFactory=factory; }

// Test the token text against the literals table
// Override this method to perform a different literals test
int CharScanner::testLiteralsTable(int ttype) const
{
	std::map<std::string,int,CharScannerLiteralsLess>::const_iterator i = literals.find(text);
	if (i != literals.end())
		ttype = (*i).second;
	return ttype;
}

// Override this method to get more specific case handling
char CharScanner::toLower(char c) const
{
	return tolower(c);
}

void CharScanner::traceIn(const std::string& rname)
{
	std::cout << "enter lexer " << rname << "; c==" << LA(1) << std::endl;
}

void CharScanner::traceOut(const std::string& rname)
{
	std::cout << "exit lexer " << rname << "; c==" << LA(1) << std::endl;
}

const char* CharScanner::charName(int ch)
{
	if (ch == EOF)
		return "EOF";
	else {
		static char buf[2];
		buf[0] = static_cast<char>(ch);
		buf[1] = '\0';
		return buf;
	}
}

#ifndef NO_STATIC_CONSTS
const char CharScanner::NO_CHAR;
const char CharScanner::EOF_CHAR;
#endif
