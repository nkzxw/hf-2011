; allow ApiOverrideExceptionHandler to be concidered as a safe security exception handler
; REQUIERED FOR XP SP2 and newer OS
; must be compiled with "ml.exe /safeseh /c /Cx /coff safeseh.asm" 
;Follow these steps to build the project: 
;1) Go to Project properties, select All Configurations in the Settings For list. 
;2) In "Build Event" / "Pre-Build Event"
;     put "Command Line" to MakeSafeSeh.bat
;3)In "Linker" / "Input" add safeseh.obj to "Additional Dependencies" 

.386
.MODEL flat,c

ApiOverrideExceptionHandler PROTO
.SAFESEH ApiOverrideExceptionHandler

END