# FILEMON.mak - makefile for VxD FILEMON

PATH=d:\driverstudio\vtoolsd\bin;$(PATH)
DEVICENAME = FILEVXD
DYNAMIC = 1
FRAMEWORK = C
DEBUG = 0
OBJECTS = filemon.OBJ

all: 	$(DEVICENAME).vxd
         copy    *.vxd ..\exe\release\.
         copy    *.vxd ..\exe\debug\.

!include $(VTOOLSD)\include\vtoolsd.mak
!include $(VTOOLSD)\include\vxdtarg.mak

filemon.OBJ:	filemon.c filemon.h
