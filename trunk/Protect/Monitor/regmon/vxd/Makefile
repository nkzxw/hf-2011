# REGMON.mak - makefile for VxD REGMON

PATH=c:\vtd31\bin;$(PATH)
DEVICENAME = REGVXD
DYNAMIC = 1
FRAMEWORK = C
DEBUG = 0
OBJECTS = regmon.OBJ

all: 	$(DEVICENAME).vxd
    copy    *.vxd ..\exe\release\.
	copy 	*.vxd ..\exe\debug\.

!include $(VTOOLSD)\include\vtoolsd.mak
!include $(VTOOLSD)\include\vxdtarg.mak

regmon.OBJ:	regmon.c regmon.h
