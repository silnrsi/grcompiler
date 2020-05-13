TARGET=gdlpp
GDLPP_SRC=.

!IF "$(CFG)" == ""
CFG=RELEASE
!ENDIF 

!IF  "$(CFG)" == "RELEASE"

OUTDIR=..\release
INTDIR=..\release_temp

all : "$(OUTDIR)\$(TARGET).exe"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "UNIX" /D "unix" /D "_CONSOLE" /D "_MBCS" /D "GDLPP" /D "_CRT_SECURE_NO_WARNINGS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\$(TARGET).pdb" /machine:I386 /out:"$(OUTDIR)\$(TARGET).exe" 
BSC32_FLAGS=/nologo /o"$(OUTDIR)\$(TARGET).bsc" 

!ELSEIF  "$(CFG)" == "DEBUG"

OUTDIR=..\debug
INTDIR=..\debug_temp

all : "$(OUTDIR)\$(TARGET).exe" "$(OUTDIR)\$(TARGET).bsc"

CPP_PROJ=/nologo /MLd /W3 /GX /Gm /ZI /Od /D "_DEBUG" /D "WIN32" /D "UNIX" /D "unix" /D "_CONSOLE" /D "_MBCS" /D "GDLPP" /D "_CRT_SECURE_NO_WARNINGS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FR"$(INTDIR)\\" /FD /GZ /c 
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\$(TARGET).pdb" /debug /pdbtype:sept /machine:I386 /out:"$(OUTDIR)\$(TARGET).exe" 
BSC32_FLAGS=/nologo /o"$(OUTDIR)\$(TARGET).bsc" 

!ENDIF 

clean :
    @- rd /s/q $(INTDIR)

realclean : clean
    @- del /q $(OUTDIR)\$(TARGET).exe

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

{$(GDLPP_SRC)}.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

{$(GDLPP_SRC)}.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

CPP=cl.exe
LINK32=link.exe
RSC=rc.exe
BSC32=bscmake.exe

LINK32_OBJS= \
	"$(INTDIR)\cpp1.obj" \
	"$(INTDIR)\cpp2.obj" \
	"$(INTDIR)\cpp3.obj" \
	"$(INTDIR)\cpp4.obj" \
	"$(INTDIR)\cpp5.obj" \
	"$(INTDIR)\cpp6.obj" \
	"$(INTDIR)\memory.obj" \
	"$(INTDIR)\usecpp.obj"

"$(OUTDIR)\$(TARGET).exe" : "$(OUTDIR)" $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

BSC32_SBRS= \
	"$(INTDIR)\cpp1.sbr" \
	"$(INTDIR)\cpp2.sbr" \
	"$(INTDIR)\cpp3.sbr" \
	"$(INTDIR)\cpp4.sbr" \
	"$(INTDIR)\cpp5.sbr" \
	"$(INTDIR)\cpp6.sbr" \
	"$(INTDIR)\memory.sbr" \
	"$(INTDIR)\usecpp.sbr"

"$(OUTDIR)\$(TARGET).bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

SOURCE=cpp1
#"$(INTDIR)\$(SOURCE).obj" "$(INTDIR)\$(SOURCE).sbr" : "$(GDLPP_SRC)\$(SOURCE).c" "$(INTDIR)"
"$(INTDIR)\cpp1.obj" "$(INTDIR)\cpp1.sbr" : "$(GDLPP_SRC)\cpp1.c" "$(INTDIR)"
SOURCE=cpp2
"$(INTDIR)\$(SOURCE).obj" "$(INTDIR)\$(SOURCE).sbr" : "$(GDLPP_SRC)\$(SOURCE).c" "$(INTDIR)"
SOURCE=cpp3
"$(INTDIR)\$(SOURCE).obj" "$(INTDIR)\$(SOURCE).sbr" : "$(GDLPP_SRC)\$(SOURCE).c" "$(INTDIR)"
SOURCE=cpp4
"$(INTDIR)\$(SOURCE).obj" "$(INTDIR)\$(SOURCE).sbr" : "$(GDLPP_SRC)\$(SOURCE).c" "$(INTDIR)"
SOURCE=cpp5
"$(INTDIR)\$(SOURCE).obj" "$(INTDIR)\$(SOURCE).sbr" : "$(GDLPP_SRC)\$(SOURCE).c" "$(INTDIR)"
SOURCE=cpp6
"$(INTDIR)\$(SOURCE).obj" "$(INTDIR)\$(SOURCE).sbr" : "$(GDLPP_SRC)\$(SOURCE).c" "$(INTDIR)"
SOURCE=memory
"$(INTDIR)\$(SOURCE).obj" "$(INTDIR)\$(SOURCE).sbr" : "$(GDLPP_SRC)\$(SOURCE).c" "$(INTDIR)"
SOURCE=usecpp
"$(INTDIR)\$(SOURCE).obj" "$(INTDIR)\$(SOURCE).sbr" : "$(GDLPP_SRC)\$(SOURCE).c" "$(INTDIR)"

