@echo off
color 07
taskkill /f /im mchshell.exe
taskkill /f /im mchshell32.exe
del mchshell.exe
del mchshell32.exe
del mchshell.zip
g++ terminal.cpp -o mchshell
g++ terminal.cpp -o mchshell32 -m32
"C:\Program Files\7-Zip\7z.exe" a mchshell.zip mchshell.exe mchshell32.exe vcruntime140.dll
