@echo off
setlocal
set _I=%0
set _R=ret1
goto getpath
:ret1
set GPATH=%_I%
set _I=%1
set _R=ret2
goto getpath
:ret2
set FPATH=%_I%

echo "%GPATH%grcompiler" "%1" "%2" > "%FPATH%grcompile.bat"

goto done

:getpath
set _I=%_I:"=%
:getpath_l
set _T=%_I:~-1,1%
if %_T%X==\X goto getpath_d
if X%_I%==X goto getpath_d
set _I=%_I:~0,-1%
goto getpath_l
:getpath_d
goto %_R%

:done
