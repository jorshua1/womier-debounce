@echo off
echo ==============================================
echo Compiling Womier Debounce for Windows
echo ==============================================

where gcc >nul 2>nul
if %ERRORLEVEL% equ 0 (
    echo Compiling with GCC / MinGW...
    gcc -O2 -Wall womier-debounce.c -o womier-debounce.exe -mwindows
    goto done
)

where cl >nul 2>nul
if %ERRORLEVEL% equ 0 (
    echo Compiling with MSVC cl.exe...
    cl /O2 womier-debounce.c /link /SUBSYSTEM:WINDOWS user32.lib
    goto done
)

echo [!] Error: No GCC (MinGW) or MSVC (cl.exe) compiler found in PATH.
echo [*] Install MinGW-w64 or Visual Studio Build Tools.
pause
exit /b 1

:done
echo [OK] Successfully built womier-debounce.exe
echo To run at Windows startup, place a shortcut to womier-debounce.exe in:
echo   %%APPDATA%%\Microsoft\Windows\Start Menu\Programs\Startup
pause
