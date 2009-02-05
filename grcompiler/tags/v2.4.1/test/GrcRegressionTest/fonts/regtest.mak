# Rebuild the test fonts and then run the regression test
# Run: nmake -f regtest.mak

RTEXE = ..\Debug
GRCEXE = ..\..\Release
FONTS = .

TEST_FONTS =\
	$(FONTS)\SchTest.ttf\
	$(FONTS)\CharisTest.ttf\
	$(FONTS)\PigLatinTest_v2.ttf\
	$(FONTS)\PigLatinTest_v3.ttf\
	$(FONTS)\PadaukTest.ttf
    

all : deleteTestFonts $(GRCEXE)\grcompiler.exe $(TEST_FONTS) grcregtest.log
	
deleteTestFonts :
	- delete $(FONTS)\SchTest.ttf
	- delete $(FONTS)\CharisTest.ttf
	- delete $(FONTS)\PigLatinTest_v2.ttf
	- delete $(FONTS)\PigLatinTest_v3.ttf
	- delete $(FONTS)\PadaukTest.ttf
	- delete .\grcregtest.log
	
$(FONTS)\SchTest.ttf :
	grcompiler -v2 $(FONTS)\SchMain.gdl $(FONTS)\SchInput.ttf $(FONTS)\SchTest.ttf
    
$(FONTS)\CharisTest.ttf :
	grcompiler -v2 $(FONTS)\CharisMain.gdl $(FONTS)\CharisInput.ttf $(FONTS)\CharisTest.ttf
    
$(FONTS)\PigLatinTest_v2.ttf :
	grcompiler -v2 $(FONTS)\PigLatinMain.gdl $(FONTS)\PigLatinInput.ttf $(FONTS)\PigLatinTest_v2.ttf
    
$(FONTS)\PigLatinTest_v3.ttf :
	grcompiler -v3 $(FONTS)\PigLatinMain.gdl $(FONTS)\PigLatinInput.ttf $(FONTS)\PigLatinTest_v3.ttf
    
$(FONTS)\PadaukTest.ttf :
	grcompiler -v3 $(FONTS)\PadaukMain.gdl $(FONTS)\PadaukInput.ttf $(FONTS)\PadaukTest.ttf

grcregtest.log :
	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\SchBenchmark.ttf $(FONTS)\SchTest.ttf
	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\CharisBenchmark.ttf $(FONTS)\CharisTest.ttf
 	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\PigLatinBenchmark_v2.ttf $(FONTS)\PigLatinTest_v2.ttf
 	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\PigLatinBenchmark_v3.ttf $(FONTS)\PigLatinTest_v3.ttf
 	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\PadaukBenchmark_v3.ttf $(FONTS)\PadaukTest.ttf

 	
