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

#include "Antlr/LLkParser.hpp"
#include <iostream>

/**An LL(k) parser.
 *
 * @see antlr.Token
 * @see antlr.TokenBuffer
 * @see antlr.LL1Parser
 */

//	LLkParser(int k_);

LLkParser::LLkParser(const ParserSharedInputState& state, int k_)
: Parser(state)
{ k=k_; }

LLkParser::LLkParser(TokenBuffer& tokenBuf, int k_)
: Parser(tokenBuf)
{ k=k_; }

LLkParser::LLkParser(TokenStream& lexer, int k_)
: Parser(new TokenBuffer(lexer))
{ k=k_; }

/**Consume another token from the input stream.  Can only write sequentially!
 * If you need 3 tokens ahead, you must consume() 3 times.
 * <p>
 * Note that it is possible to overwrite tokens that have not been matched.
 * For example, calling consume() 3 times when k=2, means that the first token
 * consumed will be overwritten with the 3rd.
 */
void LLkParser::consume()
{ inputState->getInput().consume(); }

int LLkParser::LA(int i)
{ return inputState->getInput().LA(i); }

RefToken LLkParser::LT(int i)
{ return inputState->getInput().LT(i); }

void LLkParser::trace(const std::string& ee, const std::string& rname)
{
	std::cout << ee << rname << ((inputState->guessing>0)?"; [guessing]":"; ");
	for (int i = 1; i <= k; i++)
	{
		if (i != 1) {
			std::cout << ", ";
		}
		std::cout << "LA(" << i << ")==" << LT(i)->getText();
	}
	std::cout << std::endl;
}

void LLkParser::traceIn(const std::string& rname)
{ trace("enter ",rname); }

void LLkParser::traceOut(const std::string& rname)
{ trace("exit ",rname); }
