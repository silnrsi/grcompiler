!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

TARGET=GrCompiler
TARGET_PP=gdlpp
GRC_SRC=.\compiler
GRC_GRMR_SRC=.\compiler\Grammar
GRC_ANTLR_SRC=.\compiler\Grammar\Antlr
GRC_RES=.\compiler
GRC_GEN_SRC=.\compiler\Generic
GRC_LIB_SRC=.\compiler
TTF_LIB_SRC=.\compiler
PP_SRC=.\preprocessor
ICU_BIN=.\icu\bin

!IF "$(CFG)" == ""
CFG=RELEASE
!ENDIF 

!IF "$(CFG)" == "RELEASE"

OUTDIR=.\release
INTDIR=.\release_temp

all : "$(OUTDIR)\$(TARGET).exe" "$(OUTDIR)\$(TARGET_PP).exe"
	- copy $(ICU_BIN)\icuuc36.dll $(OUTDIR)\icuuc36.dll
	- copy $(ICU_BIN)\icudt36.dll $(OUTDIR)\icudt36.dll

clean :
    @- rd /s/q $(INTDIR)
    
realclean : clean
    @- rd /s/q $(OUTDIR)

CPP_PROJ=/Zc:wchar_t- /nologo /MT /W3 /GR /EHsc /O2 /I "./compiler" /I "./compiler/grammar" /I "./compiler/Grammar/Antlr" /I "./compiler/generic" /I "./icu/source/common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\$(TARGET).res" /d "NDEBUG"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib icuuc.lib /nologo /subsystem:console /incremental:no /machine:I386 /out:"$(OUTDIR)\$(TARGET).exe" /LIBPATH:".\icu\lib\"
BSC32_FLAGS=/nologo /o"$(OUTDIR)\$(TARGET).bsc" 

CPP_PP_PROJ=/Zc:wchar_t- /nologo /MT /W3 /GR /EHsc /O2 /I "./preprocessor" /D "GDLPP" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /c 
LINK32_PP_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:no /machine:I386 /out:"$(OUTDIR)\$(TARGET_PP).exe"


!ELSEIF "$(CFG)" == "DEBUG"

OUTDIR=.\debug
INTDIR=.\debug_temp

all : "$(OUTDIR)\$(TARGET).exe" "$(OUTDIR)\$(TARGET).bsc" "$(OUTDIR)\$(TARGET_PP).exe"
	- copy $(ICU_BIN)\icuuc36d.dll $(OUTDIR)\icuuc36d.dll
	- copy $(ICU_BIN)\icudt36.dll $(OUTDIR)\icudt36.dll

clean :
    @- rd /s/q $(INTDIR)
    
realclean : clean
    @- rd /s/q $(OUTDIR)

CPP_PROJ=/nologo /MTd /W3 /Gm /GR /EHsc /RTC1 /ZI /Od /I "./compiler" /I "./compiler/grammar" /I "./compiler/Grammar/Antlr" /I "./compiler/generic" /I "./icu/source/common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\$(TARGET).res" /d "_DEBUG"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib icuucd.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\$(TARGET).pdb" /debug /machine:I386 /out:"$(OUTDIR)\$(TARGET).exe" /pdbtype:sept /LIBPATH:".\icu\lib\"
BSC32_FLAGS=/nologo /o"$(OUTDIR)\$(TARGET).bsc" 

CPP_PP_PROJ=/nologo /MTd /W3 /Gm /GR /EHsc /RTC1 /ZI /Od /I "./preprocessor" /D "GDLPP" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /c 
LINK32_PP_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\$(TARGET_PP).pdb" /debug /machine:I386 /out:"$(OUTDIR)\$(TARGET_PP).exe" /pdbtype:sept


!ENDIF

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

{$(GRC_SRC)}.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

{$(GRC_SRC)}.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

{$(GRC_GRMR_SRC)}.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

{$(GRC_GRMR_SRC)}.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

{$(GRC_GEN_SRC)}.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

{$(GRC_GEN_SRC)}.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

{$(GRC_LIB_SRC)}.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

{$(GRC_LIB_SRC)}.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

{$(TTF_LIB_SRC)}.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

{$(TTF_LIB_SRC)}.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

{$(PP_SRC)}.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PP_PROJ) $< 
<<


CPP=cl.exe
RSC=rc.exe
BSC32=bscmake.exe
LINK32=link.exe

LINK32_OBJS= \
		 "$(INTDIR)\Compiler.obj" \
		 "$(INTDIR)\ErrorCheckClasses.obj" \
		 "$(INTDIR)\ErrorCheckFeatures.obj" \
		 "$(INTDIR)\ErrorCheckRules.obj" \
#		 "$(INTDIR)\explicit_instantiations.obj" \
		 "$(INTDIR)\Fsm.obj" \
		 "$(INTDIR)\GdlExpression.obj" \
		 "$(INTDIR)\GdlFeatures.obj" \
		 "$(INTDIR)\GdlGlyphClassDefn.obj" \
		 "$(INTDIR)\GdlRenderer.obj" \
		 "$(INTDIR)\GdlRule.obj" \
		 "$(INTDIR)\GdlTablePass.obj" \
		 "$(INTDIR)\GrcErrorList.obj" \
		 "$(INTDIR)\GrcFont.obj" \
		 "$(INTDIR)\GrcGlyphAttrMatrix.obj" \
		 "$(INTDIR)\GrcManager.obj" \
		 "$(INTDIR)\GrcMasterTable.obj" \
		 "$(INTDIR)\GrcSymtable.obj" \
		 "$(INTDIR)\GrpExtensions.obj" \
		 "$(INTDIR)\GrpLexer.obj" \
		 "$(INTDIR)\GrpParser.obj" \
		 "$(INTDIR)\GrpParserDebug.obj" \
		 "$(INTDIR)\main.obj" \
		 "$(INTDIR)\OutputToFont.obj" \
		 "$(INTDIR)\ParserTreeWalker.obj" \
		 "$(INTDIR)\PostParser.obj" \
		 "$(INTDIR)\AST.obj" \
		 "$(INTDIR)\ANTLRException.obj" \
		 "$(INTDIR)\ASTFactory.obj" \
		 "$(INTDIR)\BitSet.obj" \
		 "$(INTDIR)\CharBuffer.obj" \
		 "$(INTDIR)\CharScanner.obj" \
		 "$(INTDIR)\CommonASTNode.obj" \
		 "$(INTDIR)\CommonToken.obj" \
		 "$(INTDIR)\InputBuffer.obj" \
		 "$(INTDIR)\LexerSharedInputState.obj" \
		 "$(INTDIR)\LLkParser.obj" \
		 "$(INTDIR)\MismatchedTokenException.obj" \
		 "$(INTDIR)\NoViableAltException.obj" \
		 "$(INTDIR)\Parser.obj" \
		 "$(INTDIR)\ParserException.obj" \
		 "$(INTDIR)\ParserSharedInputState.obj" \
		 "$(INTDIR)\ScannerException.obj" \
		 "$(INTDIR)\String.obj" \
		 "$(INTDIR)\Token.obj" \
		 "$(INTDIR)\TokenBuffer.obj" \
#		 "$(INTDIR)\Util.obj" \
#		 "$(INTDIR)\UtilString.obj" \
#		 "$(INTDIR)\HashMap.obj" \
		 "$(INTDIR)\Platform.obj" \
		 "$(INTDIR)\TtfUtil.obj" \
		 
#		 "$(INTDIR)\GrCompiler.res"

"$(OUTDIR)\$(TARGET).exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

LINK_PP_OBJS= \
		 "$(INTDIR)\cpp1.obj" \
		 "$(INTDIR)\cpp2.obj" \
		 "$(INTDIR)\cpp3.obj" \
		 "$(INTDIR)\cpp4.obj" \
		 "$(INTDIR)\cpp5.obj" \
		 "$(INTDIR)\cpp6.obj" \
		 "$(INTDIR)\memory.obj" \
		 "$(INTDIR)\usecpp.obj"

"$(OUTDIR)\$(TARGET_PP).exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK_PP_OBJS)
    $(LINK32) @<<
  $(LINK32_PP_FLAGS) $(LINK_PP_OBJS)
<<

BSC32_SBRS= \
		 "$(INTDIR)\Compiler.sbr" \
		 "$(INTDIR)\ErrorCheckClasses.sbr" \
		 "$(INTDIR)\ErrorCheckFeatures.sbr" \
		 "$(INTDIR)\ErrorCheckRules.sbr" \
#		 "$(INTDIR)\explicit_instantiations.sbr" \
		 "$(INTDIR)\Fsm.sbr" \
		 "$(INTDIR)\GdlExpression.sbr" \
		 "$(INTDIR)\GdlFeatures.sbr" \
		 "$(INTDIR)\GdlGlyphClassDefn.sbr" \
		 "$(INTDIR)\GdlRenderer.sbr" \
		 "$(INTDIR)\GdlRule.sbr" \
		 "$(INTDIR)\GdlTablePass.sbr" \
		 "$(INTDIR)\GrcErrorList.sbr" \
		 "$(INTDIR)\GrcFont.sbr" \
		 "$(INTDIR)\GrcGlyphAttrMatrix.sbr" \
		 "$(INTDIR)\GrcManager.sbr" \
		 "$(INTDIR)\GrcMasterTable.sbr" \
		 "$(INTDIR)\GrcSymtable.sbr" \
		 "$(INTDIR)\GrpExtensions.sbr" \
		 "$(INTDIR)\GrpLexer.sbr" \
		 "$(INTDIR)\GrpParser.sbr" \
		 "$(INTDIR)\GrpParserDebug.sbr" \
		 "$(INTDIR)\main.sbr" \
		 "$(INTDIR)\OutputToFont.sbr" \
		 "$(INTDIR)\ParserTreeWalker.sbr" \
		 "$(INTDIR)\PostParser.sbr" \
		 "$(INTDIR)\AST.sbr" \
		 "$(INTDIR)\ANTLRException.sbr" \
		 "$(INTDIR)\ASTFactory.sbr" \
		 "$(INTDIR)\BitSet.sbr" \
		 "$(INTDIR)\CharBuffer.sbr" \
		 "$(INTDIR)\CharScanner.sbr" \
		 "$(INTDIR)\CommonASTNode.sbr" \
		 "$(INTDIR)\CommonToken.sbr" \
		 "$(INTDIR)\InputBuffer.sbr" \
		 "$(INTDIR)\LexerSharedInputState.sbr" \
		 "$(INTDIR)\LLkParser.sbr" \
		 "$(INTDIR)\MismatchedTokenException.sbr" \
		 "$(INTDIR)\NoViableAltException.sbr" \
		 "$(INTDIR)\Parser.sbr" \
		 "$(INTDIR)\ParserException.sbr" \
		 "$(INTDIR)\ParserSharedInputState.sbr" \
		 "$(INTDIR)\ScannerException.sbr" \
		 "$(INTDIR)\String.sbr" \
		 "$(INTDIR)\Token.sbr" \
		 "$(INTDIR)\TokenBuffer.sbr" \
#		 "$(INTDIR)\Util.sbr" \
#		 "$(INTDIR)\UtilString.sbr" \
#		 "$(INTDIR)\HashMap.sbr" \
		 "$(INTDIR)\Platform.sbr"\
		 "$(INTDIR)\TtfUtil.sbr" \
#

"$(OUTDIR)\$(TARGET).bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

"$(INTDIR)\Compiler.obj" "$(INTDIR)\Compiler.sbr" : "$(GRC_SRC)\Compiler.cpp" "$(INTDIR)"
"$(INTDIR)\ErrorCheckClasses.obj" "$(INTDIR)\ErrorCheckClasses.sbr" : "$(GRC_SRC)\ErrorCheckClasses.cpp" "$(INTDIR)"
"$(INTDIR)\ErrorCheckFeatures.obj" "$(INTDIR)\ErrorCheckFeatures.sbr" : "$(GRC_SRC)\ErrorCheckFeatures.cpp" "$(INTDIR)"
"$(INTDIR)\ErrorCheckRules.obj" "$(INTDIR)\ErrorCheckRules.sbr" : "$(GRC_SRC)\ErrorCheckRules.cpp" "$(INTDIR)"
#"$(INTDIR)\explicit_instantiations.obj" "$(INTDIR)\Compiler.explicit_instantiations" : "$(GRC_SRC)\explicit_instantiations.cpp" "$(INTDIR)"
"$(INTDIR)\Fsm.obj" "$(INTDIR)\Fsm.sbr" : "$(GRC_SRC)\Fsm.cpp" "$(INTDIR)"
"$(INTDIR)\GdlExpression.obj" "$(INTDIR)\GdlExpression.sbr" : "$(GRC_SRC)\GdlExpression.cpp" "$(INTDIR)"
"$(INTDIR)\GdlFeatures.obj" "$(INTDIR)\GdlFeatures.sbr" : "$(GRC_SRC)\GdlFeatures.cpp" "$(INTDIR)"
"$(INTDIR)\GdlGlyphClassDefn.obj" "$(INTDIR)\GdlGlyphClassDefn.sbr" : "$(GRC_SRC)\GdlGlyphClassDefn.cpp" "$(INTDIR)"
"$(INTDIR)\GdlRenderer.obj" "$(INTDIR)\GdlRenderer.sbr" : "$(GRC_SRC)\GdlRenderer.cpp" "$(INTDIR)"
"$(INTDIR)\GdlRule.obj" "$(INTDIR)\GdlRule.sbr" : "$(GRC_SRC)\GdlRule.cpp" "$(INTDIR)"
"$(INTDIR)\GdlTablePass.obj" "$(INTDIR)\GdlTablePass.sbr" : "$(GRC_SRC)\GdlTablePass.cpp" "$(INTDIR)"
"$(INTDIR)\GrcErrorList.obj" "$(INTDIR)\GrcErrorList.sbr" : "$(GRC_SRC)\GrcErrorList.cpp" "$(INTDIR)"
"$(INTDIR)\GrcFont.obj" "$(INTDIR)\GrcFont.sbr" : "$(GRC_SRC)\GrcFont.cpp" "$(INTDIR)"
"$(INTDIR)\GrcGlyphAttrMatrix.obj" "$(INTDIR)\GrcGlyphAttrMatrix.sbr" : "$(GRC_SRC)\GrcGlyphAttrMatrix.cpp" "$(INTDIR)"
"$(INTDIR)\GrcManager.obj" "$(INTDIR)\GrcManager.sbr" : "$(GRC_SRC)\GrcManager.cpp" "$(INTDIR)"
"$(INTDIR)\GrcMasterTable.obj" "$(INTDIR)\GrcMasterTable.sbr" : "$(GRC_SRC)\GrcMasterTable.cpp" "$(INTDIR)"
"$(INTDIR)\GrcSymtable.obj" "$(INTDIR)\GrcSymtable.sbr" : "$(GRC_SRC)\GrcSymtable.cpp" "$(INTDIR)"
"$(INTDIR)\GrpExtensions.obj" "$(INTDIR)\GrpExtensions.sbr" : "$(GRC_SRC)\GrpExtensions.cpp" "$(INTDIR)"
"$(INTDIR)\GrpLexer.obj" "$(INTDIR)\GrpLexer.sbr" : "$(GRC_SRC)\GrpLexer.cpp" "$(INTDIR)"
"$(INTDIR)\GrpParser.obj" "$(INTDIR)\GrpParser.sbr" : "$(GRC_SRC)\GrpParser.cpp" "$(INTDIR)"
"$(INTDIR)\GrpParserDebug.obj" "$(INTDIR)\GrpParserDebug.sbr" : "$(GRC_SRC)\GrpParserDebug.cpp" "$(INTDIR)"
"$(INTDIR)\main.obj" "$(INTDIR)\main.sbr" : "$(GRC_SRC)\main.cpp" "$(INTDIR)"
"$(INTDIR)\OutputToFont.obj" "$(INTDIR)\OutputToFont.sbr" : "$(GRC_SRC)\OutputToFont.cpp" "$(INTDIR)"
"$(INTDIR)\ParserTreeWalker.obj" "$(INTDIR)\ParserTreeWalker.sbr" : "$(GRC_SRC)\ParserTreeWalker.cpp" "$(INTDIR)"
"$(INTDIR)\PostParser.obj" "$(INTDIR)\PostParser.sbr" : "$(GRC_SRC)\PostParser.cpp" "$(INTDIR)"

"$(INTDIR)\AST.obj" "$(INTDIR)\AST.sbr" : "$(GRC_GRMR_SRC)\AST.cpp" "$(INTDIR)"
"$(INTDIR)\ANTLRException.obj" "$(INTDIR)\ANTLRException.sbr" : "$(GRC_GRMR_SRC)\ANTLRException.cpp" "$(INTDIR)"
"$(INTDIR)\ASTFactory.obj" "$(INTDIR)\ASTFactory.sbr" : "$(GRC_GRMR_SRC)\ASTFactory.cpp" "$(INTDIR)"
"$(INTDIR)\BitSet.obj" "$(INTDIR)\BitSet.sbr" : "$(GRC_GRMR_SRC)\BitSet.cpp" "$(INTDIR)"
"$(INTDIR)\CharBuffer.obj" "$(INTDIR)\CharBuffer.sbr" : "$(GRC_GRMR_SRC)\CharBuffer.cpp" "$(INTDIR)"
"$(INTDIR)\CharScanner.obj" "$(INTDIR)\CharScanner.sbr" : "$(GRC_GRMR_SRC)\CharScanner.cpp" "$(INTDIR)"
"$(INTDIR)\CommonASTNode.obj" "$(INTDIR)\CommonASTNode.sbr" : "$(GRC_GRMR_SRC)\CommonASTNode.cpp" "$(INTDIR)"
"$(INTDIR)\CommonToken.obj" "$(INTDIR)\CommonToken.sbr" : "$(GRC_GRMR_SRC)\CommonToken.cpp" "$(INTDIR)"
"$(INTDIR)\InputBuffer.obj" "$(INTDIR)\InputBuffer.sbr" : "$(GRC_GRMR_SRC)\InputBuffer.cpp" "$(INTDIR)"
"$(INTDIR)\LexerSharedInputState.obj" "$(INTDIR)\LexerSharedInputState.sbr" : "$(GRC_GRMR_SRC)\LexerSharedInputState.cpp" "$(INTDIR)"
"$(INTDIR)\LLkParser.obj" "$(INTDIR)\LLkParser.sbr" : "$(GRC_GRMR_SRC)\LLkParser.cpp" "$(INTDIR)"
"$(INTDIR)\MismatchedTokenException.obj" "$(INTDIR)\MismatchedTokenException.sbr" : "$(GRC_GRMR_SRC)\MismatchedTokenException.cpp" "$(INTDIR)"
"$(INTDIR)\NoViableAltException.obj" "$(INTDIR)\NoViableAltException.sbr" : "$(GRC_GRMR_SRC)\NoViableAltException.cpp" "$(INTDIR)"
"$(INTDIR)\Parser.obj" "$(INTDIR)\Parser.sbr" : "$(GRC_GRMR_SRC)\Parser.cpp" "$(INTDIR)"
"$(INTDIR)\ParserException.obj" "$(INTDIR)\ParserException.sbr" : "$(GRC_GRMR_SRC)\ParserException.cpp" "$(INTDIR)"
"$(INTDIR)\ParserSharedInputState.obj" "$(INTDIR)\ParserSharedInputState.sbr" : "$(GRC_GRMR_SRC)\ParserSharedInputState.cpp" "$(INTDIR)"
"$(INTDIR)\ScannerException.obj" "$(INTDIR)\ScannerException.sbr" : "$(GRC_GRMR_SRC)\ScannerException.cpp" "$(INTDIR)"
"$(INTDIR)\String.obj" "$(INTDIR)\String.sbr" : "$(GRC_GRMR_SRC)\String.cpp" "$(INTDIR)"
"$(INTDIR)\Token.obj" "$(INTDIR)\Token.sbr" : "$(GRC_GRMR_SRC)\Token.cpp" "$(INTDIR)"
"$(INTDIR)\TokenBuffer.obj" "$(INTDIR)\TokenBuffer.sbr" : "$(GRC_GRMR_SRC)\TokenBuffer.cpp" "$(INTDIR)"

# "$(INTDIR)\Util.obj" "$(INTDIR)\Util.sbr" : "$(GRC_GEN_SRC)\Util.cpp" "$(INTDIR)"
# "$(INTDIR)\UtilString.obj" "$(INTDIR)\UtilString.sbr" : "$(GRC_GEN_SRC)\UtilString.cpp" "$(INTDIR)"
# "$(INTDIR)\HashMap.obj" "$(INTDIR)\HashMap.sbr" : "$(GRC_GEN_SRC)\HashMap.cpp" "$(INTDIR)"

"$(INTDIR)\Platform.obj" "$(INTDIR)\Platform.sbr" : "$(GRC_GEN_SRC)\Platform.cpp" "$(INTDIR)"
"$(INTDIR)\TtfUtil.obj" "$(INTDIR)\TtfUtil.sbr" : "$(TTF_LIB_SRC)\TtfUtil.cpp" "$(INTDIR)"


"$(INTDIR)\$(TARGET).res" : "$(GRC_RES)\$(TARGET).rc" "$(INTDIR)"
		 $(RSC) $(RSC_PROJ) "$(GRC_RES)\$(TARGET).rc"
