#Microsoft Nmake build file for msbackup programs
#OS = {MSDOS or WIN32} in environment
# in a single compiler environment just include the correct file

!if defined(OS)
!   if "$(OS)" == "MSDOS"
!include "macro.qcl"
!   else
!include "macro.vc"
!   endif
!endif


#dependancies below generic for Unix gcc and the Evil Empire's qcl/cl

ALL: msqic.$(EXE) ntbkup.$(EXE)

CLEAN:
   $(DEL) *.$(OBJ)
#  delete *.exe cause can conflict with CYGWIN
   $(DEL) ntbkup.$(EXE)
   $(DEL) msqic.$(EXE)


#*.QIC reader
msqic.$(EXE): msqic.$(OBJ) qicdcomp.$(OBJ) msqicrcv.$(OBJ) 
    $(LNK) msqic qicdcomp msqicrcv $(DELIM)

msqic.$(OBJ): msqic.c msqic.h
    $(CC) msqic.c

msqicrcv.$(OBJ): msqicrcv.c msqic.h
    $(CC) msqicrcv.c

qicdcomp.$(OBJ): qicdcomp.c msqic.h
    $(CC) qicdcomp.c

#*.BKF reader
ntbkup.$(EXE): ntbkup.$(OBJ) nttree.$(OBJ)
    $(LNK) ntbkup.$(OBJ) nttree.$(OBJ) $(DELIM)

ntbkup.$(OBJ): ntbkup.c  ntbkup.h
    $(CC) ntbkup.c

nttree.$(OBJ): nttree.c  ntbkup.h
    $(CC) nttree.c


