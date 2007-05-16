# Rebuild the test fonts and then run the regression test
# Run: nmake -f regtest.mak

RTEXE = ..\Debug
GRCEXE = ..\..\Release
FONTS = .

TEST_FONTS =\
	$(FONTS)\SchTest.ttf\
	$(FONTS)\CharisTest.ttf\
	$(FONTS)\PigLatinTest_v2.ttf\
	$(FONTS)\PigLatinTest_v3.ttf
    

all : deleteTestFonts $(GRCEXE)\grcompiler.exe $(TEST_FONTS) runRegressionTest
	
deleteTestFonts :
	- delete $(FONTS)\SchTest.ttf
	- delete $(FONTS)\CharisTest.ttf
	- delete $(FONTS)\PigLatinTest_v2.ttf
	- delete $(FONTS)\PigLatinTest_v3.ttf
	
$(FONTS)\SchTest.ttf :
	grcompiler -v2 $(FONTS)\SchMain.gdl $(FONTS)\SchInput.ttf $(FONTS)\SchTest.ttf
    
$(FONTS)\CharisTest.ttf :
	grcompiler -v2 $(FONTS)\CharisMain.gdl $(FONTS)\CharisInput.ttf $(FONTS)\CharisTest.ttf
    
$(FONTS)\PigLatinTest_v2.ttf :
	grcompiler -v2 $(FONTS)\PigLatin.gdl $(FONTS)\PigLatinInput.ttf $(FONTS)\PigLatinTest_v2.ttf
    
$(FONTS)\PigLatinTest_v3.ttf :
	grcompiler -v3 $(FONTS)\PigLatin.gdl $(FONTS)\PigLatinInput.ttf $(FONTS)\PigLatinTest_v3.ttf
    
runRegressionTest :
	$(RTEXE)\GrcRegressionTest.exe
    
