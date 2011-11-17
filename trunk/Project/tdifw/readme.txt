Welcome to TDI-based Open Source Personal Firewall (TdiFw)


INSTALLATION

1. Run install.bat or install_nt4.bat for NT4

2. Edit %SystemRoot%\system32\drivers\etc\tdifw.conf for your taste

3. Restart Windows


RULES

Edit file %SystemRoot%\system32\drivers\etc\tdifw.conf
Description of file format is in it.

To reload rules you have to restart tdifw service:
C:\>net stop tdifw
C:\>net start tdifw

Errors are written in log (see below).


LOGS

There are two types of logs and three types of records to write.

The first type of logs is text log files. They're in
%SystemRoot%\system32\LogFiles\tdifw directory.
File name consists of year, month and date: YYYYMMDD.log

The second type of logs is Windows' "Event Log". Records are writing to 
Application log with "tdifw" event source. You can use "Event Viewer" 
to view events.

By default all events are written to text log files. If you want specify what
kind of events are to be written in "Event Log" see [_config_] section of
tdi_fw.conf file.

See also log_format.txt


STATE INFO

You can get list of listening ports with processes owning them by:

 tdifw listen

You can get list of opening connections with processes owning them and
even direction of connections by:

 tdifw conn


BUGS

Check the latest version at http://sf.net/projects/tdifw

Mail to iptables@mail.ru


DEBUGGING

First you have to disable autostarting of driver and helper service.

1. Remove autostart of driver by executing: bin\install remove drv
or by starting uninstall_nt4.reg for NT4
2. Uninstall service by executing: tdifw remove
3. Restart Windows

And now see debug.txt file
