# Microsoft Developer Studio Generated NMAKE File, Based on Filemon.dsp
!IF "$(CFG)" == ""
CFG=Release
!MESSAGE No configuration specified. Defaulting to Release
!ENDIF 

!IF "$(CFG)" != "Release" && "$(CFG)" != "Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Filemon.mak" CFG="Release"
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

ALL : "$(OUTDIR)\Filemon.exe"


CLEAN :
	-@erase "$(INTDIR)\FILEMON.OBJ"
	-@erase "$(INTDIR)\FILEMON.res"
	-@erase "$(INTDIR)\INSTDRV.OBJ"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\Filemon.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "S:\mstools\include" /I "S:\ddk\inc32" /I "S:\vtd95\include" /I "..\dd" /D "NDEBUG" /D "Win64" /D "_WINDOWS" /D _Win64_WINNT=0x400 /D "_MBCS" /Fo"$(INTDIR)\\" /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /Win64 
RSC=rc.exe
RSC_PROJ=/l 0x0 /fo"$(INTDIR)\FILEMON.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Filemon.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /incremental:no /pdb:"$(OUTDIR)\Filemon.pdb" /out:"$(OUTDIR)\Filemon.exe" /SUBSYSTEM:windows,4.0 
LINK32_OBJS= \
	"$(INTDIR)\FILEMON.OBJ" \
	"$(INTDIR)\INSTDRV.OBJ" \
	"$(INTDIR)\FILEMON.res"

"$(OUTDIR)\Filemon.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Debug"

OUTDIR=.\Deb64
INTDIR=.\Deb64
# Begin Custom Macros
OutDir=.\Deb64
# End Custom Macros

ALL : "$(OUTDIR)\Filemon.exe"


CLEAN :
	-@erase "$(INTDIR)\FILEMON.OBJ"
	-@erase "$(INTDIR)\FILEMON.res"
	-@erase "$(INTDIR)\INSTDRV.OBJ"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\Filemon.exe"
	-@erase "$(OUTDIR)\Filemon.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /Zi /W3 /WX /GX /Od /I "e:\mstools\include" /D "_DEBUG" /D "Win64" /D "_WINDOWS" /D _Win64_WINNT=0x400 /D "_MBCS" /Fo"$(INTDIR)\\" /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /Win64 
RSC=rc.exe
RSC_PROJ=/l 0x0 /fo"$(INTDIR)\FILEMON.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Filemon.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib  /nologo /pdb:$(INTDIR)\Filemon.pdb /map:"$(INTDIR)\Filemon.map" /debug /machine:IA64 /out:"$(OUTDIR)\Filemon.exe" /SUBSYSTEM:windows,4.0 
LINK32_OBJS= \
	"$(INTDIR)\FILEMON.OBJ" \
	"$(INTDIR)\INSTDRV.OBJ" \
	"$(INTDIR)\FILEMON.res"

"$(OUTDIR)\Filemon.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("Filemon.dep")
!INCLUDE "Filemon.dep"
!ELSE 
!MESSAGE Warning: cannot find "Filemon.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Release" || "$(CFG)" == "Debug"
SOURCE=.\FILEMON.C

"$(INTDIR)\FILEMON.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\FILEMON.RC

"$(INTDIR)\FILEMON.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\INSTDRV.C

"$(INTDIR)\INSTDRV.OBJ" : $(SOURCE) "$(INTDIR)"



!ENDIF 

