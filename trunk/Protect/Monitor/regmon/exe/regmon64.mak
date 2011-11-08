# Microsoft Developer Studio Generated NMAKE File, Based on Regmon.dsp
!IF "$(CFG)" == ""
CFG=Release
!MESSAGE No configuration specified. Defaulting to Release.
!ENDIF 

!IF "$(CFG)" != "Release" && "$(CFG)" != "Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Regmon.mak" CFG="Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Release"
!MESSAGE "Debug"
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "Release"

OUTDIR=.\Rel64
INTDIR=.\Rel64
# Begin Custom Macros
OutDir=.\Rel64
# End Custom Macros

ALL : "$(OUTDIR)\Regmon.exe"


CLEAN :
	-@erase "$(INTDIR)\INSTDRV.OBJ"
	-@erase "$(INTDIR)\REGMON.OBJ"
	-@erase "$(INTDIR)\REGMON.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\Regmon.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "Win64" /D "_WINDOWS" /D _Win64_WINNT=0x400 /D "_MBCS" /Fo"$(INTDIR)\\" /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\REGMON.res" /d "WIN64" /d "NDEBUG" 
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib comctl32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\Regmon.pdb" /out:"$(OUTDIR)\Regmon.exe" 
LINK32_OBJS= \
	"$(INTDIR)\INSTDRV.OBJ" \
	"$(INTDIR)\REGMON.OBJ" \
	"$(INTDIR)\REGMON.res"

"$(OUTDIR)\Regmon.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Debug"

OUTDIR=.\Deb64
INTDIR=.\Deb64
# Begin Custom Macros
OutDir=.\Deb64
# End Custom Macros

ALL : "$(OUTDIR)\Regmon.exe"


CLEAN :
	-@erase "$(INTDIR)\INSTDRV.OBJ"
	-@erase "$(INTDIR)\REGMON.OBJ"
	-@erase "$(INTDIR)\REGMON.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\Regmon.exe"
	-@erase "$(OUTDIR)\Regmon.ilk"
	-@erase "$(OUTDIR)\Regmon.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /Zi /W3 /WX /GX /Od /D "_DEBUG" /D "Win64" /D "_WINDOWS" /D _Win64_WINNT=0x400 /D "_MBCS" /Fo"$(INTDIR)\\" /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\REGMON.res" /d "WIN64" /d "_DEBUG" 
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /subsystem:windows,4.0 /incremental:yes /pdb:"$(OUTDIR)\Regmon.pdb" /debug /machine:IA64 /out:"$(OUTDIR)\Regmon.exe" 
LINK32_OBJS= \
	"$(INTDIR)\INSTDRV.OBJ" \
	"$(INTDIR)\REGMON.OBJ" \
	"$(INTDIR)\REGMON.res"

"$(OUTDIR)\Regmon.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("Regmon.dep")
!INCLUDE "Regmon.dep"
!ELSE 
!MESSAGE Warning: cannot find "Regmon.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Release" || "$(CFG)" == "Debug"
SOURCE=.\INSTDRV.C

"$(INTDIR)\INSTDRV.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\REGMON.C

"$(INTDIR)\REGMON.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\REGMON.RC

"$(INTDIR)\REGMON.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

