
		费尔个人防火墙 2.1 源程序
	       版权所有 (C) 费尔安全实验室
		  http://www.xfilt.com
		   xstudio@xfilt.com
		   2002 年 12 月 22 日
===============================================================

在使用费尔个人防火墙源程序之前，请首先阅读用户许可协议(本目录
的 License.txt 文件)。

费尔个人防火墙 2.1 源程序包括 XFILTER.EXE, XFILTER.DLL, 
XPACKET.VXD ( 限于95/98/ME版 ) , XAPCKET.SYS ( 限于 2000/XP 版 )
的源程序。

此代码包中没有包含编译后的程序，要获得这些程序可以通过下载安装
包得到。安装包是免费的，使用 ZIP 兼容的压缩方式，用 ZIP 工具解
压。


重要文件/目录说明

Xfilter.dsw 	- VC 工作区文件，包括以下工程
		- TcpIpDog\TcpIpDog.dsp
		- xfilter\xfilter.dsp
		- xpacket2k.dsp ( 限于 2000/XP 版 )
		- xpacket9x.dsp ( 限于 95/98/ME 版 )

Common 	  - 存放公用代码文件
RELEASE   - 存放编译后的程序
TcpIpDog  - XFILTER.DLL 的代码
XFILTER   - XFILTER.EXE 的代码
xpacket2k - XPACKET.SYS 的代码 (2000/XP 版才会有此目录)
xpacket9x - XPACKET.VXD 的代码 (95/98/ME 版才会有此目录)


编程工具

Microsoft Visual C++ 6.0
DDK For Windows 2000 (仅 2000/XP 版需要)
VToolsD For Windows 98 (仅 95/98/ME 版需要)


XFILTER.EXE 编译说明 (在Windows 98/2000 下均可)

用 VC 打开工作区文件 XFilter.dsw，选择当前工程为 "XFilter"，按
"Build" 按钮，编译后的 XFILTER.EXE 放在 Release 目录。


XFILTER.DLL 编译说明 (在Windows 98/2000 下均可)

用 VC 打开工作区文件 XFilter.dsw，选择当前工程为 "TcpIpDog"，按
"Build" 按钮，编译后的 XFILTER.DLL 放在 Release 目录。


XPACKET.SYS 编译说明 (仅限于 2000/XP 版) (只能在 Windows 2000 下)

首先保证安装好 DDK For Windows 2000。

用 VC 打开工作区文件 XFilter.dsw，编译使用 XPacket2k\Build.bat
完成，并且需要传递参数，所以需要首先设置工作区中的 "Xpacket2k"
工程的命令行编译参数。设置方法为: VC 主界面 -> Project 菜单 -> 
Settings 菜单 -> 在弹出对话框上选择 Xpacket2k 工程 -> Settings For
选择 Debug 或 Release -> General -> Build Command Line 为

build.bat %DDKROOT% free F: F:\xfilter2\xpacket2k %SystemRoot%

第一个参数 %DDKROOT% 表示 DDK 安装目录的环境变量，可以不用更改。
第二个参数 free 表示编译为 release 版本，checked 表示编译为 debug 
	   版本。
第三个参数 F: 源程序所在盘符，需要修改为自己的正确盘符。
第四个参数 F:\xfilter2\xpacket2k 源程序所在的路径，需要修改为自己
	   的正确路径。
第五个参数 %SystemRoot% 表示系统目录的环境变量，批处理会将编译后的 
	   Xpacket.sys 自动复制到系统目录，一般为Winnt\system32。

修改后确认，然后选择当前工程为 "XPACKET2K"，按 "Build" 按钮，编译
后的 XPACKET.SYS 自动复制到 Release 目录和系统目录。


xpacket.sys For XP 的编译说明：

2.0.5增加了对XP的支持，由于 Win 2000 与 Win XP 的驱动程序基本兼容，所以没有重新编写驱动程序，但是为了提高兼容性，最好使用DDK For XP对xpacket2k进行重新编译，编译方法如下：

1. 安装XP操作系统
2. 安装DDK For XP
3. 在"Start"->"All Program"->"Development Kits"->"Windows DDK 2600"->"Build Environments"->"Win XP Free Build Environment"（编译Release版本）或者"Win XP Checked Build Environment"（编译Debug版本）
4. 进入命令行后切换到源程序所在的xpacket2k目录
5. 执行命令“Build.exe -cZ”
6. 完成编译。


XPACKET.VXD 编译说明 (仅限于 95/98/ME 版) 

在 Windows 98/2000 下均可编译，但必须保证 98/2000 同时安装在同一台
计算机，并且必须保证在 Windows 98 下安装好 VToolsD For Windows 98。

用 VC 打开工作区文件 XFilter.dsw，然后选择当前工程为 "XPACKET9X"，
按 "Build" 按钮，编译后的 XPACKET.VXD 自动复制到 Release 目录。
可以根据需求编辑 Xpacket9x\Build.bat。Debug 和 Release 版本区分
需要编辑 Xpacket9x\xpacket.mak 文件。在 xpacket.mak 中

DEBUG = 1 时表示为 Debug 版本
#DEBUG = 1 注释掉DEBUG，则编译为 Release 版本


===============================================================
在编译过程中可能出现警告错误，但这不影响程序的正常运行，如果发
生其它不能解决的错误，请与我们联系。



安装

要想运行编译好的程序需按照一下步骤进行安装：

1. Win95/98/ME 下需将 Xpacket.vxd 复制到 Windows\System 目录, 
   并将 Release\Xpacket9x.reg 导入注册表。
   Windows2000 下需将 Xpacket.sys 复制到 Winnt\System32 目录，
   并将 Release\Xpacket2k.reg 导入注册表。

2. 在命令行模式执行 xfilter.exe -install。

3. 重新启动，完成安装。


卸载

1. 在命令行模式执行 xfilter.exe -remove。

2. Win95/98/ME 删除注册表分支 
   HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\VxD\XPACKET
   Windows2000 删除注册表分支 
   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\XPacket

3. 重新启动，完成卸载。






