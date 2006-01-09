#ifndef INC_MismatchedTokenException_hpp__
#define INC_MismatchedTokenException_hpp__

/**
 * <b>SOFTWARE RIGHTS</b>
 * <p>
 * ANTLR 2.6.0 MageLang Insitute, 1999
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
 * @version ANTLR 2.6.0 MageLang Insitute, 1999
 * @author Terence Parr, <a href=http://www.MageLang.com>MageLang Institute</a>
 * @author <br>John Lilley, <a href=http://www.Empathy.com>Empathy Software</a>
 * @author <br><a href="mailto:pete@yamuna.demon.co.uk">Pete Wells</a>
 */
/* Header modified with backport of throw() destructor from ANTLR 2.7.2 */
 
#include "Antlr/config.hpp"
#include "Antlr/ParserException.hpp"
#include "Antlr/BitSet.hpp"
#include "Antlr/Token.hpp"
#include "Antlr/AST.hpp"
#include <vector>

class MismatchedTokenException : public ParserException {
private:
	// Token names array for formatting
	std::vector<std::string> tokenNames;

public:
	// The token that was encountered
	const RefToken token;
	// The offending AST node if tree walking
	const RefAST node;

	std::string tokenText; // taken from node or token object

	// Types of tokens
#ifndef NO_STATIC_CONSTS
	static const int TOKEN = 1;
	static const int NOT_TOKEN = 2;
	static const int RANGE = 3;
	static const int NOT_RANGE = 4;
	static const int SET = 5;
	static const int NOT_SET = 6;
#else
	enum {
		TOKEN = 1,
		NOT_TOKEN = 2,
		RANGE = 3,
		NOT_RANGE = 4,
		SET = 5,
		NOT_SET = 6
	};
#endif

protected:
	// One of the above
	int mismatchType;

	// For TOKEN/NOT_TOKEN and RANGE/NOT_RANGE
	int expecting;

	// For RANGE/NOT_RANGE (expecting is lower bound of range)
	int upper;

	// For SET/NOT_SET
	BitSet set;

public:
	MismatchedTokenException();

	// Expected range / not range
	MismatchedTokenException(
		const std::vector<std::string>& tokenNames_,
		RefAST node_,
		int lower,
		int upper_,
		bool matchNot
	);

	// Expected token / not token
	MismatchedTokenException(
		const std::vector<std::string>& tokenNames_,
		RefAST node_,
		int expecting_,
		bool matchNot
	);

	// Expected BitSet / not BitSet
	MismatchedTokenException(
		const std::vector<std::string>& tokenNames_,
		RefAST node_,
		BitSet set_,
		bool matchNot
	);

	// Expected range / not range
	MismatchedTokenException(
		const std::vector<std::string>& tokenNames_,
		RefToken token_,
		int lower,
		int upper_,
		bool matchNot
	);

	// Expected token / not token
	MismatchedTokenException(
		const std::vector<std::string>& tokenNames_,
		RefToken token_,
		int expecting_,
		bool matchNot
	);

	// Expected BitSet / not BitSet
	MismatchedTokenException(
		const std::vector<std::string>& tokenNames_,
		RefToken token_,
		BitSet set_,
		bool matchNot
	);
	
	~MismatchedTokenException() throw() {};

	/**
	 * @return the column number that this exception happened on.
	 */
	int getColumn() const;

	/**
	 * Returns the error message that happened on the line/col given.
	 * Copied from toString().
	 */
	std::string getErrorMessage() const;

	/**
	 * @return the line number that this exception happened on.
	 */
	int getLine() const;

private:
	std::string tokenName(int tokenType) const;

public:
	std::string toString() const;

};

#endif //INC_MismatchedTokenException_hpp__

