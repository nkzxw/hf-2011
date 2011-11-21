# XPACKET.mak - makefile for VxD XPACKET

DEVICENAME = XPACKET
DYNAMIC = 1
NDIS = 1
FRAMEWORK = C
#DEBUG = 1
XFLAGS = -W3 /DUSE_NDIS

OBJECTS = XPACKET.OBJ NDISHOOK.OBJ \
			PROCESS.OBJ PACKET.OBJ PACKETBUFFER.OBJ \
			NETBIOS.OBJ COMMONFUNCTION.OBJ MEMORYACL.OBJ

!include $(VTOOLSD)\include\vtoolsd.mak
!include $(VTOOLSD)\include\vxdtarg.mak

SOUCES =	XPACKET.c	\
			NDISHOOK.c	\
			process.c	\
			packet.c	\
			NetBios.c	\
			CommonFunction.c \
			MemoryAcl.c	\
			PacketBuffer.c \
			xfilter.rc
