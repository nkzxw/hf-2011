#include "stdafx.h"
#include "PortInfo.h"
CPortInfo::CPortInfo()
{
	
}
CPortInfo::~CPortInfo()
{

}
void CPortInfo::init()
{
	mapPortInfo.SetAt(_T("1"),_T("TCPMUX (TCP port service multiplexer)"));
	mapPortInfo.SetAt(_T("5"),_T("RJE (Remote Job Entry)"));
	mapPortInfo.SetAt(_T("7"),_T("ECHO protocol"));
	mapPortInfo.SetAt(_T("9"),_T("DISCARD protocol"));
	mapPortInfo.SetAt(_T("13"),_T("DAYTIME protocol"));
	mapPortInfo.SetAt(_T("17"),_T("QOTD (Quote of the Day) protocol"));
	mapPortInfo.SetAt(_T("18"),_T("Message Send Protocol"));
	mapPortInfo.SetAt(_T("19"),_T("CHARGEN (Character Generator) protocol"));
	mapPortInfo.SetAt(_T("20"),_T("FTP - data port"));
	mapPortInfo.SetAt(_T("21"),_T("FTP - control (command) port"));
	mapPortInfo.SetAt(_T("22"),_T("SSH (Secure Shell),(scp, sftp) "));
	mapPortInfo.SetAt(_T("23"),_T("Telnet protocol - unencrypted text communications"));
	mapPortInfo.SetAt(_T("25"),_T("SMTP - used for sending E-mails"));
	mapPortInfo.SetAt(_T("26"),_T("RSFTP - A simple FTP-like protocol"));
	mapPortInfo.SetAt(_T("37"),_T("TIME protocol"));
	mapPortInfo.SetAt(_T("38"),_T("Route Access Protocol"));
	mapPortInfo.SetAt(_T("39"),_T("Resource Location Protocol"));
	mapPortInfo.SetAt(_T("41"),_T("Graphics"));
	mapPortInfo.SetAt(_T("42"),_T("Host Name Server"));
	mapPortInfo.SetAt(_T("49"),_T("TACACS Login Host protocol"));
	mapPortInfo.SetAt(_T("53"),_T("DNS (Domain Name Server)"));
	mapPortInfo.SetAt(_T("57"),_T("MTP, Mail Transfer Protocol"));
	mapPortInfo.SetAt(_T("67"),_T("BOOTP (BootStrap Protocol) server; also used by DHCP "));
	mapPortInfo.SetAt(_T("68"),_T("BOOTP client; also used by DHCP"));
	mapPortInfo.SetAt(_T("69"),_T("TFTP (Trivial File Transfer Protocol)"));
	mapPortInfo.SetAt(_T("70"),_T("Gopher protocol"));
	mapPortInfo.SetAt(_T("79"),_T("Finger protocol"));
	mapPortInfo.SetAt(_T("80"),_T("HTTP (HyperText Transfer Protocol) - used for transferring web pages"));
	mapPortInfo.SetAt(_T("88"),_T("Kerberos - authenticating agent"));
	mapPortInfo.SetAt(_T("101"),_T("HOSTNAME"));
	mapPortInfo.SetAt(_T("107"),_T("Remote Telnet Service"));
	mapPortInfo.SetAt(_T("109"),_T("POP, Post Office Protocol, version 2"));
	mapPortInfo.SetAt(_T("110"),_T("POP3 (Post Office Protocol version 3) - used for retrieving E-mails"));
	mapPortInfo.SetAt(_T("113"),_T("ident - old server identification system"));
	mapPortInfo.SetAt(_T("115"),_T("SFTP, Simple File Transfer Protocol"));
	mapPortInfo.SetAt(_T("118"),_T("SQL Services"));
	mapPortInfo.SetAt(_T("119"),_T("NNTP (Network News Transfer Protocol) - used for retrieving newsgroups messages"));
	mapPortInfo.SetAt(_T("123"),_T("NTP (Network Time Protocol) - used for time synchronization"));
	mapPortInfo.SetAt(_T("137"),_T("NetBIOS NetBIOS Name Service"));
	mapPortInfo.SetAt(_T("138"),_T("NetBIOS NetBIOS Datagram Service"));
	mapPortInfo.SetAt(_T("139"),_T("NetBIOS NetBIOS Session Service"));
	mapPortInfo.SetAt(_T("143"),_T("IMAP4 (Internet Message Access Protocol 4) - used for retrieving E-mails"));
	mapPortInfo.SetAt(_T("152"),_T("BFTP, Background File Transfer Program"));
	mapPortInfo.SetAt(_T("153"),_T("SGMP, Simple Gateway Monitoring Protocol"));
	mapPortInfo.SetAt(_T("156"),_T("SQL Service"));
	mapPortInfo.SetAt(_T("158"),_T("DMSP, Distributed Mail Service Protocol"));
	mapPortInfo.SetAt(_T("161"),_T("SNMP (Simple Network Management Protocol)"));
	mapPortInfo.SetAt(_T("162"),_T("SNMPTRAP"));
	mapPortInfo.SetAt(_T("179"),_T("BGP (Border Gateway Protocol)"));
	mapPortInfo.SetAt(_T("194"),_T("IRC (Internet Relay Chat)"));
	mapPortInfo.SetAt(_T("201"),_T("AppleTalk Routing Maintenance"));
	mapPortInfo.SetAt(_T("209"),_T("The Quick Mail Transfer Protocol"));
	mapPortInfo.SetAt(_T("213"),_T("IPX"));
	mapPortInfo.SetAt(_T("218"),_T("MPP, Message Posting Protocol"));
	mapPortInfo.SetAt(_T("220"),_T("IMAP, Interactive Mail Access Protocol, version 3"));
	mapPortInfo.SetAt(_T("259"),_T("ESRO, Efficient Short Remote Operations"));
	mapPortInfo.SetAt(_T("264"),_T("BGMP, Border Gateway Multicast Protocol"));
	mapPortInfo.SetAt(_T("318"),_T("TSP, Time Stamp Protocol"));
	mapPortInfo.SetAt(_T("323"),_T("IMMP, Internet Message Mapping Protocol"));
	mapPortInfo.SetAt(_T("366"),_T("SMTP, Simple Mail Transfer Protocol. ODMR, On-Demand Mail Relay"));
	mapPortInfo.SetAt(_T("369"),_T("Rpc2portmap"));
	mapPortInfo.SetAt(_T("384"),_T("A Remote Network Server System"));
	mapPortInfo.SetAt(_T("387"),_T("AURP, AppleTalk Update-based Routing Protocol"));
	mapPortInfo.SetAt(_T("389"),_T("LDAP (Lightweight Directory Access Protocol)"));
	mapPortInfo.SetAt(_T("401"),_T("UPS Uninterruptible Power Supply"));
	mapPortInfo.SetAt(_T("427"),_T("SLP (Serivce Location Protocol)"));
	mapPortInfo.SetAt(_T("443"),_T("HTTPS - HTTP Protocol over TLS/SSL (encrypted transmission)"));
	mapPortInfo.SetAt(_T("444"),_T("SNPP, Simple Network Paging Protocol"));
	mapPortInfo.SetAt(_T("445"),_T("Microsoft-DS (Active Directory, Windows shares, Sasser-worm, Agobot, Zobotworm)"));
	mapPortInfo.SetAt(_T("464"),_T("Kerberos Change/Set password"));
	mapPortInfo.SetAt(_T("500"),_T("Isakmp, IKE-Internet Key Exchange"));
	mapPortInfo.SetAt(_T("514"),_T("rsh protocol - used to execute non-interactive commandline"));
	mapPortInfo.SetAt(_T("515"),_T("LPD Printer protocol - used in LPD printer servers"));
	mapPortInfo.SetAt(_T("524"),_T("NCP (NetWare Core Protocol) "));
	mapPortInfo.SetAt(_T("530"),_T("Rpc"));
	mapPortInfo.SetAt(_T("531"),_T("AOL Instant Messenger, IRC"));
	mapPortInfo.SetAt(_T("540"),_T("UUCP (Unix-to-Unix Copy Protocol)"));
	mapPortInfo.SetAt(_T("542"),_T("commerce (Commerce Applications) "));
	mapPortInfo.SetAt(_T("546"),_T("DHCPv6 client"));
	mapPortInfo.SetAt(_T("547"),_T("DHCPv6 server"));
	mapPortInfo.SetAt(_T("554"),_T("RTSP (Real Time Streaming Protocol)"));
	mapPortInfo.SetAt(_T("563"),_T("NNTP protocol over TLS/SSL (NNTPS)"));
	mapPortInfo.SetAt(_T("587"),_T("email message submission (SMTP) (RFC 2476)"));
	mapPortInfo.SetAt(_T("591"),_T("FileMaker 6.0 Web Sharing (HTTP Alternate, see port 80)"));
	mapPortInfo.SetAt(_T("593"),_T("HTTP RPC Ep Map"));
	mapPortInfo.SetAt(_T("604"),_T("TUNNEL"));
	mapPortInfo.SetAt(_T("631"),_T("IPP, Internet Printing Protocol"));
	mapPortInfo.SetAt(_T("636"),_T("LDAP over SSL (encrypted transmission)"));
	mapPortInfo.SetAt(_T("639"),_T("MSDP, Multicast Source Discovery Protocol"));
	mapPortInfo.SetAt(_T("646"),_T("LDP, Label Distribution Protocol"));
	mapPortInfo.SetAt(_T("647"),_T("DHCP Failover Protocol"));
	mapPortInfo.SetAt(_T("648"),_T("RRP, Registry Registrar Protocol"));
	mapPortInfo.SetAt(_T("652"),_T("DTCP, Dynamic Tunnel Configuration Protocol"));
	mapPortInfo.SetAt(_T("666"),_T("id Software's Doom multiplayer game played over TCP "));
	mapPortInfo.SetAt(_T("674"),_T("ACAP, Application Configuration Access Protocol"));
	mapPortInfo.SetAt(_T("691"),_T("MS Exchange Routing"));
	mapPortInfo.SetAt(_T("692"),_T("Hyperwave-ISP"));
	mapPortInfo.SetAt(_T("695"),_T("IEEE-MMS-SSL"));
	mapPortInfo.SetAt(_T("698"),_T("OLSR, Optimized Link State Routing"));
	mapPortInfo.SetAt(_T("699"),_T("Access Network"));
	mapPortInfo.SetAt(_T("700"),_T("EPP, Extensible Provisioning Protocol"));
	mapPortInfo.SetAt(_T("701"),_T("LMP, Link Management Protocol."));
	mapPortInfo.SetAt(_T("702"),_T("IRIS over BEEP"));
	mapPortInfo.SetAt(_T("706"),_T("SILC, Secure Internet Live Conferencing"));
	mapPortInfo.SetAt(_T("720"),_T("SMQP, Simple Message Queue Protocol"));
	mapPortInfo.SetAt(_T("829"),_T("CMP (Certificate Managemaent Protocol)"));
	mapPortInfo.SetAt(_T("873"),_T("rsync File synchronisation protocol"));
	mapPortInfo.SetAt(_T("901"),_T("Samba Web Administration Tool (SWAT) - Unofficial"));
	mapPortInfo.SetAt(_T("989"),_T("FTP Protocol ( data) over TLS/SSL"));
	mapPortInfo.SetAt(_T("990"),_T("FTP Protocol (control) over TLS/SSL"));
	mapPortInfo.SetAt(_T("991"),_T("NAS (Netnews Admin System)"));
	mapPortInfo.SetAt(_T("992"),_T("Telnet protocol over TLS/SSL"));
	mapPortInfo.SetAt(_T("993"),_T("IMAP4 over SSL (encrypted transmission)"));
	mapPortInfo.SetAt(_T("995"),_T("POP3 over SSL (encrypted transmission)"));
	mapPortInfo.SetAt(_T("1080"),_T("SOCKS proxy"));
	mapPortInfo.SetAt(_T("1099"),_T("RMI Registry"));
	mapPortInfo.SetAt(_T("1194"),_T("OpenVPN"));
	mapPortInfo.SetAt(_T("1198"),_T("The cajo project Free dynamic transparent distributed computing in Java"));
	mapPortInfo.SetAt(_T("1214"),_T("Kazaa"));
	mapPortInfo.SetAt(_T("1223"),_T("TGP: \"TrulyGlobal Protocol\" aka \"The Gur Protocol\""));
	mapPortInfo.SetAt(_T("1352"),_T("IBM Lotus Notes/Domino RPC"));
	mapPortInfo.SetAt(_T("1414"),_T("IBM MQSeries"));
	mapPortInfo.SetAt(_T("1433"),_T("Microsoft SQL database system"));
	mapPortInfo.SetAt(_T("1434"),_T("Microsoft SQL Monitor"));
	mapPortInfo.SetAt(_T("1723"),_T("Microsoft PPTP VPN"));
	mapPortInfo.SetAt(_T("1863"),_T("MSN Messenger"));
	mapPortInfo.SetAt(_T("1900"),_T("Microsoft SSDP Enables discovery of UPnP devices"));
	mapPortInfo.SetAt(_T("1935"),_T("Macromedia Flash Communications Server MX"));
	mapPortInfo.SetAt(_T("2000"),_T("Cisco SCCP (Skinny)"));
	mapPortInfo.SetAt(_T("2181"),_T("EForward-document transport system"));
	mapPortInfo.SetAt(_T("2427"),_T("Cisco MGCP"));
	mapPortInfo.SetAt(_T("2447"),_T("ovwdb - OpenView Network Node Manager (NNM) daemon"));
	mapPortInfo.SetAt(_T("3306"),_T("MySQL Database system"));
	mapPortInfo.SetAt(_T("3389"),_T("Microsoft Terminal Server (RDP) officially registered as Windows Based Terminal (WBT)"));
	mapPortInfo.SetAt(_T("3396"),_T("Novell NDPS Printer Agent"));
	mapPortInfo.SetAt(_T("3689"),_T("DAAP Digital Audio Access Protocol used by Apple's ITunes"));
	mapPortInfo.SetAt(_T("3690"),_T("Subversion version control system"));
	mapPortInfo.SetAt(_T("3784"),_T("Ventrilo VoIP program used by Ventrilo"));
	mapPortInfo.SetAt(_T("4894"),_T("LysKOM Protocol A"));
	mapPortInfo.SetAt(_T("4899"),_T("RAdmin remote administration tool (often Trojan horse)"));
	mapPortInfo.SetAt(_T("5003"),_T("Filemaker Filemaker Pro"));
	mapPortInfo.SetAt(_T("5190"),_T("AOL and AOL Instant Messenger"));
	mapPortInfo.SetAt(_T("5222"),_T("XMPP/Jabber - client connection"));
	mapPortInfo.SetAt(_T("5269"),_T("XMPP/Jabber - server connection"));
	mapPortInfo.SetAt(_T("5434"),_T("PostgreSQL database system"));
	mapPortInfo.SetAt(_T("5800"),_T("VNC remote desktop protocol - for use over HTTP"));
	mapPortInfo.SetAt(_T("6000"),_T("X11 - used between an X client and server over the network"));
	mapPortInfo.SetAt(_T("6346"),_T("Gnutella Filesharing (Bearshare, Limewire etc.)"));
	mapPortInfo.SetAt(_T("6347"),_T("Gnutella"));
	mapPortInfo.SetAt(_T("6901"),_T("MSN Messenger (Voice)"));
	mapPortInfo.SetAt(_T("7000"),_T("Default Port for Azureus's built in HTTPS Bittorrent Tracker"));
	mapPortInfo.SetAt(_T("8000"),_T("iRDMI - often mistakenly used instead of port 8080"));
	mapPortInfo.SetAt(_T("8080"),_T("HTTP Alternate (http-alt) - used when running a second web server on the same machine"));
	mapPortInfo.SetAt(_T("8118"),_T("Privoxy web proxy - advertisements-filtering web proxy"));
	mapPortInfo.SetAt(_T("10000"),_T("Webmin - web based linux admin tool"));
	mapPortInfo.SetAt(_T("11371"),_T("OpenPGP HTTP Keyserver"));
	mapPortInfo.SetAt(_T("20000"),_T("Usermin - web based user tool"));

}
BOOL CPortInfo::ReturnPortInfo(CString strPort,CString &strReturn)
{
	if(!mapPortInfo.Lookup(strPort ,strReturn))
	{
		return FALSE;
	}
	return TRUE;
}