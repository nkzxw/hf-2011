@echo off
copy bin\ndis_hk.sys %SystemRoot%\system32\drivers > nul
regedit /s install_2k.reg
