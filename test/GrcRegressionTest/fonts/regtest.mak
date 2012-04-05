# Rebuild the test fonts and then run the regression test
# Run: nmake -f regtest.mak

RTEXE = ..\Debug
GRCEXE = ..\..\..\Release
FONTS = .

TEST_FONTS =\
	$(FONTS)\SchTest.ttf\
	$(FONTS)\CharisTest.ttf\
	$(FONTS)\PigLatinTest_v2.ttf\
	$(FONTS)\PigLatinTest_v3.ttf\
	$(FONTS)\PadaukTest.ttf
    

all : deleteTestFonts $(GRCEXE)\grcompiler.exe $(TEST_FONTS) grcregtest.log

check : deleteTestFonts $(GRCEXE)\grcompiler.exe $(TEST_FONTS) regtest.log
	
deleteTestFonts :
	- del $(FONTS)\SchTest.ttf
	- del $(FONTS)\CharisTest.ttf
	- del $(FONTS)\PigLatinTest_v2.ttf
	- del $(FONTS)\PigLatinTest_v3.ttf
	- del $(FONTS)\PadaukTest.ttf
	- del .\grcregtest.log
	- del .\regtest.log
	
$(FONTS)\SchTest.ttf :
	$(GRCEXE)\grcompiler -v2 $(FONTS)\SchMain.gdl $(FONTS)\SchInput.ttf $(FONTS)\SchTest.ttf
    
$(FONTS)\CharisTest.ttf :
	$(GRCEXE)\grcompiler -v2 $(FONTS)\CharisMain.gdl $(FONTS)\CharisInput.ttf $(FONTS)\CharisTest.ttf
    
$(FONTS)\PigLatinTest_v2.ttf :
	$(GRCEXE)\grcompiler -v2 $(FONTS)\PigLatinMain.gdl $(FONTS)\PigLatinInput.ttf $(FONTS)\PigLatinTest_v2.ttf
    
$(FONTS)\PigLatinTest_v3.ttf :
	$(GRCEXE)\grcompiler -v3 $(FONTS)\PigLatinMain.gdl $(FONTS)\PigLatinInput.ttf $(FONTS)\PigLatinTest_v3.ttf
    
$(FONTS)\PadaukTest.ttf :
	$(GRCEXE)\grcompiler -v3 $(FONTS)\PadaukMain.gdl $(FONTS)\PadaukInput.ttf $(FONTS)\PadaukTest.ttf

grcregtest.log :
	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\SchBenchmark.ttf $(FONTS)\SchTest.ttf
	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\CharisBenchmark.ttf $(FONTS)\CharisTest.ttf
 	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\PigLatinBenchmark_v2.ttf $(FONTS)\PigLatinTest_v2.ttf
 	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\PigLatinBenchmark_v3.ttf $(FONTS)\PigLatinTest_v3.ttf
 	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\PadaukBenchmark_v3.ttf $(FONTS)\PadaukTest.ttf

regtest.log :
	$(RTEXE)\GrcRegressionTest.exe -l regtest.log $(FONTS)\SchBenchmark.ttf $(FONTS)\SchTest.ttf
	$(RTEXE)\GrcRegressionTest.exe -l regtest.log  $(FONTS)\CharisBenchmark.ttf $(FONTS)\CharisTest.ttf
 	$(RTEXE)\GrcRegressionTest.exe -l regtest.log  $(FONTS)\PigLatinBenchmark_v2.ttf $(FONTS)\PigLatinTest_v2.ttf
 	$(RTEXE)\GrcRegressionTest.exe -l regtest.log  $(FONTS)\PigLatinBenchmark_v3.ttf $(FONTS)\PigLatinTest_v3.ttf
 	$(RTEXE)\GrcRegressionTest.exe -l regtest.log  $(FONTS)\PadaukBenchmark_v3.ttf $(FONTS)\PadaukTest.ttf

 	
