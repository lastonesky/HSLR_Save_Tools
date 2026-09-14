@echo off
REM ============================================================
REM build.bat — Build HSLR Save Editor with TCC
REM ============================================================
REM Usage: build.bat
REM Requires: tcc.exe in PATH or TCC_DIR environment variable
REM ============================================================

setlocal

REM Find TCC
if defined TCC_DIR (
    set "TCC=%TCC_DIR%\tcc.exe"
) else (
    set "TCC=tcc.exe"
)

REM Check TCC exists
%TCC% --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: TCC not found. Please set TCC_DIR or add tcc to PATH.
    echo   Example: set TCC_DIR=E:\Dev\tcc
    exit /b 1
)

REM Generate import libraries from system DLLs if missing
if not exist comctl32.def (
    echo Generating import libraries from system DLLs...
    %TCC% -impdef "%SystemRoot%\System32\comctl32.dll" -o comctl32.def
    %TCC% -impdef "%SystemRoot%\System32\comdlg32.dll" -o comdlg32.def
    %TCC% -impdef "%SystemRoot%\System32\shell32.dll" -o shell32.def
)

echo Compiling HSLR Save Editor (TCC)...
echo.

REM Compile: all .c files + .def import libs -> hslr_editor.exe
REM TCC compiles multiple source files in one command
%TCC% -o hslr_editor.exe ^
    -I. ^
    comctl32.def ^
    comdlg32.def ^
    shell32.def ^
    cJSON.c ^
    hslr_save.c ^
    hslr_data.c ^
    hslr_gui.c

if errorlevel 1 (
    echo.
    echo BUILD FAILED!
    exit /b 1
)

echo.
echo BUILD SUCCESS: hslr_editor.exe
echo.
echo To run:
echo   hslr_editor.exe
echo.
echo Make sure data\items.csv exists next to the exe for item names.
echo.

endlocal
