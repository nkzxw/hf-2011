cl /c /Zi main.cpp
link /subsystem:windows /DEBUG


/Zi 编译选项非常重要，否则windbg无法调试。