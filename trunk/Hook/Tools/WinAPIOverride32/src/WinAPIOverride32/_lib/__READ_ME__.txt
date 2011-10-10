This file explain which .dll or .sys must be placed in the same directory of your created application

- InjLib.dll : always required
- APIOverride.dll : always required
- HookCom.dll : required only if you want to monitor / override COM interface or ActiveX
- HookNet.dll : required only if you want to monitor / override .NET methods
- SetEnvVarProc.dll : required only if you want to monitor / override .NET methods (needed for .Net services)
- ProcMon.sys : required only if you want to hook all new created processes