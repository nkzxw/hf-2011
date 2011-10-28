@echo off

cd 1.SecurityBlock\
call Make.bat PellesC NOPAUSE
cd..
cd 2.VirtualAddress\
call Make.bat PellesC NOPAUSE
cd..
cd 3.Options\
call Make.bat PellesC NOPAUSE
cd..
cd 4.Stack mouvements\
call Make.bat PellesC NOPAUSE
cd..
cd 5.Control flow\
call Make.bat PellesC NOPAUSE
cd..
cd 6.Dataflow(eax)\
call Make.bat PellesC NOPAUSE
cd..
cd 7.Dataflow(modified memory)\
call Make.bat PellesC NOPAUSE
cd..
cd 8.Dynamic Patterns\
call Make.bat PellesC NOPAUSE
cd..
cd 9.Driver\masm32\
call MakeDriver.bat NOPAUSE
cd..
cd..
cd 9.Driver\MinGW\
call MakeDriver.bat NOPAUSE
cd..
cd..
cd 11.Control flow (JMP)
call Make.bat PellesC NOPAUSE
cd..
pause





