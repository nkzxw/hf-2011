##################################################################
#                                                                #
#  Winpooch : Windows Watchdog                                   #
#  Copyright (C) 2004-2006  Benoit Blanchon                      #
#                                                                #
#  This program is free software; you can redistribute it        #
#  and/or modify it under the terms of the GNU General Public    #
#  License as published by the Free Software Foundation; either  #
#  version 2 of the License, or (at your option) any later       #
#  version.                                                      #
#                                                                #
#  This program is distributed in the hope that it will be       #
#  useful, but WITHOUT ANY WARRANTY; without even the implied    #
#  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       #
#  PURPOSE.  See the GNU General Public License for more         #
#  details.                                                      #
#                                                                #
#  You should have received a copy of the GNU General Public     #
#  License along with this program; if not, write to the Free    #
#  Software Foundation, Inc.,                                    #
#  675 Mass Ave, Cambridge, MA 02139, USA.                       #
#                                                                #
##################################################################

DEFINES = -DUNICODE -D_UNICODE
DEFINES += -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0500 -DWINVER=0x0500
INCLUDES = -I../common

CFLAGS = -Wall -Werror -gcoff $(DEFINES) $(INCLUDES)


#
# Programs needed
#
CC	= gcc
GREP	= grep -n
LD	= gcc
MKDEP	= gcc -MM $(CFLAGS)
MV	= mv -f
RC	= windres $(DEFINES)
RM	= rm -f
SED	= sed
STRIP	= strip


#
# Application specific stuffs
#
APPLICATION = Winpooch
APPLICATION_BIN = $(APPLICATION).exe
APPLICATION_INCLUDES =
APPLICATION_LDFLAGS = -mwindows
APPLICATION_LDLIBS = -lpsapi -lcomctl32 -lcomdlg32 -lmsimg32 -lfreeimage -lwinmm -lshlwapi -lwininet -lversion -lclamav
APPLICATION_SRCS = \
	../common/Assert.c \
	../common/Filter.c \
	../common/FiltCond.c \
	../common/FiltRule.c \
	../common/FiltParam.c \
	../common/FilterSet.c \
	../common/ImgInfo.c \
	../common/Strlcpy.c \
	../common/Trace.c \
	../common/Wildcards.c \
	AboutWnd.c \
	Application.c \
	AskDlg.c \
	BkgndScan.c \
	Config.c \
	ConfigWnd.c \
	EventLog.c \
	FilterFile.c \
	FilterWnd.c \
	FilterTools.c \
	FilterDefault.c \
	FreshClam.c \
	HistoryWnd.c \
	IncompReport.c \
	Language.c \
	Link.c \
	LogFile.c \
	MainWnd.c \
	PicBtnCtl.c \
	PicMenuCtl.c \
	ProcList.c \
	ProcListWnd.c \
	ProgPathDlg.c \
	RuleDlg.c \
	ScanCacheWnd.c \
	Scanner.c \
	Sounds.c \
	SplashWnd.c \
	SpyServer.c \
	StringList.c \
	TrayIcon.c \
	UpdWatcher.c \
	VirusDlg.c
APPLICATION_OBJS = $(patsubst %.c,%.o,$(APPLICATION_SRCS)) Resources.o


all : $(APPLICATION_BIN)

clean :
	$(RM) ../common/*.o *.o *.a *.d *.c~ *.h~ *.rc~ *.pl~ Makefile~ README~ CHANGELOG~ FAQ~

strip : $(APPLICATION_BIN)
	$(STRIP) $(APPLICATION_BIN)

BuildCount.h : *.c *.h *.rc ../common/*.c ../common/*.h
	perl ../common/BuildCounter.pl $@ APPLICATION_BUILD

$(APPLICATION_BIN) : $(APPLICATION_OBJS)
	$(LD) $(APPLICATION_LDFLAGS) $(APPLICATION_OBJS) $(APPLICATION_LDLIBS) -o $@

%.o : %.rc
	$(RC) $(INCLUDES) -o $@ $<

%.d : %.c
	$(MKDEP) $< > $@.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g'  $@.$$$$ > $@; \
	$(RM) $@.$$$$

#
# Static dependencies
#
Resources.o : Resources.h BuildCount.h ../rsrc/*

#
# Automatic dependencies
#
-include $(APPLICATION_SRCS:.c=.d)
