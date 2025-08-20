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

- **Unix-style Interface**: Beautiful ASCII art and prompt styling
- **Full CMD Compatibility**: All Windows commands work seamlessly (some commands may launch in separate windows - please report any issues)
- **Environment Variable Support**: `cd %USERPROFILE%`, `cd %TEMP%`, etc.
- **Lightweight**: Minimal overhead over native CMD

## Compilation

### 64-bit version:
```bash
g++ terminal.cpp -o mchshell
```

### 32-bit version:
```bash
g++ terminal.cpp -o mchshell32 -m32
```