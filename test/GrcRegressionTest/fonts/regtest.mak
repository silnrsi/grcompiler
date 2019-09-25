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

# Note: something in grcompiler chokes when we use $(FONTS)\SchTest.ttf

$(FONTS)\SchTest.ttf :
	$(GRCEXE)\grcompiler -D -v4 $(FONTS)\SchMain.gdl $(FONTS)\SchInput.ttf SchTest.ttf
    
$(FONTS)\CharisTest.ttf :
	$(GRCEXE)\grcompiler -D -v2 $(FONTS)\CharisMain.gdl $(FONTS)\CharisInput.ttf CharisTest.ttf
    
$(FONTS)\PigLatinTest_v2.ttf :
	$(GRCEXE)\grcompiler -D -v2 -p $(FONTS)\PigLatinMain.gdl $(FONTS)\PigLatinInput.ttf PigLatinTest_v2.ttf "PigLatin GrRegTest V2"
    
$(FONTS)\PigLatinTest_v3.ttf :
	$(GRCEXE)\grcompiler -D -v3 $(FONTS)\PigLatinMain.gdl $(FONTS)\PigLatinInput.ttf PigLatinTest_v3.ttf
    
$(FONTS)\PadaukTest.ttf :
	$(GRCEXE)\grcompiler -D -v3 -offsets -p $(FONTS)\PadaukMain.gdl $(FONTS)\PadaukInput.ttf PadaukTest.ttf

grcregtest.log :
  - @echo -----  
  - @echo -----
  - @echo ----- RUNNING TESTS -----
  - @echo -----
	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\SchBenchmark.ttf $(FONTS)\SchTest.ttf
	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\CharisBenchmark.ttf $(FONTS)\CharisTest.ttf
	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\PigLatinBenchmark_v2.ttf $(FONTS)\PigLatinTest_v2.ttf
	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\PigLatinBenchmark_v3.ttf $(FONTS)\PigLatinTest_v3.ttf
 	- $(RTEXE)\GrcRegressionTest.exe $(FONTS)\PadaukBenchmark_v3.ttf $(FONTS)\PadaukTest.ttf
 	-
	- echo N|comp $(FONTS)\SchBenchmark.ttf $(FONTS)\SchTest.ttf
	- echo N|comp $(FONTS)\CharisBenchmark.ttf $(FONTS)\CharisTest.ttf
	- echo N|comp $(FONTS)\PigLatinBenchmark_v2.ttf $(FONTS)\PigLatinTest_v2.ttf
	- echo N|comp $(FONTS)\PigLatinBenchmark_v3.ttf $(FONTS)\PigLatinTest_v3.ttf
	- echo N|comp $(FONTS)\PadaukBenchmark_v3.ttf $(FONTS)\PadaukTest.ttf

regtest.log :
	$(RTEXE)\GrcRegressionTest.exe -l regtest.log  $(FONTS)\SchBenchmark.ttf $(FONTS)\SchTest.ttf
	$(RTEXE)\GrcRegressionTest.exe -l regtest.log  $(FONTS)\CharisBenchmark.ttf $(FONTS)\CharisTest.ttf
	$(RTEXE)\GrcRegressionTest.exe -l regtest.log  $(FONTS)\PigLatinBenchmark_v2.ttf $(FONTS)\PigLatinTest_v2.ttf
	$(RTEXE)\GrcRegressionTest.exe -l regtest.log  $(FONTS)\PigLatinBenchmark_v3.ttf $(FONTS)\PigLatinTest_v3.ttf
	$(RTEXE)\GrcRegressionTest.exe -l regtest.log  $(FONTS)\PadaukBenchmark_v3.ttf $(FONTS)\PadaukTest.ttf

