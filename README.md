# mchShell

mchShell is a tiny wrapper for CMD made with the Windows API

![mchShell](https://img.shields.io/badge/mchShell-CMD%20Wrapper-blue)
![Windows](https://img.shields.io/badge/Platform-Windows%20XP%2B-0078d6)
![C++](https://img.shields.io/badge/C++-11%2B-orange)

## How to use
1. Compile using C++11 or newer
2. Run the program
3. Have fun using mchShell!

## Features
- **Full CMD Compatibility**: All Windows commands work seamlessly (some commands may launch in separate windows - please report any issues)
- **Environment Variable Support**: `cd %USERPROFILE%`, `cd %TEMP%`, etc.
- **Lightweight**: Minimal overhead over native CMD
- **Commands**: Custom `ff` command to find files/folders, `pwd` that displays the current directory, `find` which takes a file and a string and outputs anytime the string was spotted
, a `testnet` command which tests the internet connectivity using ping, a `size` command which gets the size of a file/folder and a `mchhelp` command
- **AutoExec**: Custom `autoexec.mch` file with instructions (like .bat)
- **MCH format**: 'Custom' executable format launchable with smch (.bat undercover)

## Compilation

### 64-bit version:
```bash
g++ terminal.cpp -o mchshell
```

### 32-bit version:
```bash
g++ terminal.cpp -o mchshell32 -m32
```