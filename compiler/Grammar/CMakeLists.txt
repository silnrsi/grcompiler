project(parser)

include_directories(${PROJECT_SOURCE_DIR})

add_library(parser STATIC
    AST.cpp 
	ANTLRException.cpp 
	ASTFactory.cpp 
	BitSet.cpp 
	CharBuffer.cpp 
	CharScanner.cpp 
	CommonASTNode.cpp 
	CommonToken.cpp 
	InputBuffer.cpp 
	LexerSharedInputState.cpp 
	LLkParser.cpp 
	MismatchedTokenException.cpp 
	NoViableAltException.cpp 
	Parser.cpp 
	ParserException.cpp 
	ParserSharedInputState.cpp 
	ScannerException.cpp 
	Token.cpp 
	TokenBuffer.cpp)