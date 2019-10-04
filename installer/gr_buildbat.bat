@echo off
setlocal
set _I=%0
set _R=ret1
set _D=\
goto getpath
:ret1
rem      GPATH = directory of the compiler
set GPATH=%_I%
set _I=%1
set _R=ret2
set _D=.
goto getpath
:ret2
rem      FPATH = directory of the font
set FPATH=%_I%

rem      generate output
echo @echo off > "%FPATH%bat"
echo "%GPATH%grcompiler" "%1" "%2" >> "%FPATH%bat"
echo pause >> "%FPATH%bat"

goto done

rem      :getpath -- figure out directory from a file with path
:getpath
set _I=%_I:"=%
:getpath_l
set _T=%_I:~-1,1%
if %_T%X==%_D%X goto getpath_d
if X%_I%==X goto getpath_d
set _I=%_I:~0,-1%
goto getpath_l
:getpath_d
goto %_R%

:done
