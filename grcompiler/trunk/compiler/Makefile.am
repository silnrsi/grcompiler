AM_CPPFLAGS = -I@srcdir@/Generic -I@srcdir@/Grammar -I@srcdir@/LZ4

SUBDIRS = Generic Grammar LZ4
LIBS = @LIBS@ @LIBICONV@ -LGeneric -LGrammar -LLZ4 -lgeneric -lparser -llz4
EXTRA_DIST = resource.h GrCompiler.rc GrpParser.g GrpParser_readme.txt GrpParserTokenTypes.txt stddef.gdh

bin_PROGRAMS = grcompiler
pkgdata_DATA = stddef.gdh

grcompiler_SOURCES = \
        Compiler.cpp \
        constants.h \
        ErrorCheckClasses.cpp \
        ErrorCheckFeatures.cpp \
        ErrorCheckRules.cpp \
        Fsm.cpp \
        Fsm.h \
        GlyphBoundaries.cpp \
        GlyphBoundaries.h \
        GdlExpression.cpp \
        GdlExpression.h \
        GdlFeatures.cpp \
        GdlFeatures.h \
        GdlGlyphClassDefn.cpp \
        GdlGlyphClassDefn.h \
        GdlGlyphDefn.h \
        GdlNameDefn.h \
        GdlObject.h \
        GdlRenderer.cpp \
        GdlRenderer.h \
        GdlRule.cpp \
        GdlRule.h \
        GdlTablePass.cpp \
        GdlTablePass.h \
        GrcBinaryStream.h \
        GrcEnv.h \
        GrcErrorList.cpp \
        GrcErrorList.h \
        GrcFont.cpp \
        GrcFont.h \
        GrcGlyphAttrMatrix.cpp \
        GrcGlyphAttrMatrix.h \
        GrcManager.cpp \
        GrcManager.h \
        GrcMasterTable.cpp \
        GrcMasterTable.h \
        GrcSymTable.cpp \
        GrcSymTable.h \
        GrpASTNode.hpp \
        GrpExtensions.cpp \
        Grp.h \
        GrpLexer.cpp \
        GrpLexer.hpp \
        GrpLineAndFile.hpp \
        GrpParser.cpp \
        GrpParserDebug.cpp \
        GrpParser.hpp \
        GrpParserTokenTypes.hpp \
        GrpToken.hpp \
        GrpTokenStreamFilter.hpp \
        LgCharPropsStub.h \
        main.cpp \
        main.h \
        OutputToFont.cpp \
        ParserTreeWalker.cpp \
        PostParser.cpp \
        StrAnsiHash.h \
        TtfUtil.cpp \
        TtfUtil.h \
        TtfTypes.h
