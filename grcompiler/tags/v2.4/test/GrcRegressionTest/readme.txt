HOW TO ADD AN ITEM TO THE REGRESSION TEST SUITE:

* Add the GDL code and the benchmark TTF file iknto the fonts directory.
* Create a "dbg_..." directory under fonts and put the benchmark debugger files there.
* Extend regtest.mak (in the font directory) to include the new test, so that the Test font will be generated.
* Add the new items to the GrcRegressionTest.cpp file, in the main() function.
