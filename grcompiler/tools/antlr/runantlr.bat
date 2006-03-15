# This file can be used to auto-generate the following compiler source-code files
# based on the grammar defined in GrpParser.g:
#	GrpParser.cpp
#	GrpParser.hpp
#	GrpLexer.cpp
#	GrpLexer.hpp
#	GrpParserTokenTypes.hpp
#	GrpParserTokenTypes.txt (documentation only, not needed to build)
#
# Note: it is normal to get "nondeterminism" warnings when running Antlr.
#
# You will need to have some useful version of Java installed. Change the line
# below to indicate the location of your Java executable.

c:\progra~1\java\j2re1.4.1_02\bin\java -cp .\antlr-2.6.0 antlr.Tool ..\compiler\GrpParser.g
@echo  
@echo Done!
