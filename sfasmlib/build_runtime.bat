@echo off
REM Builds 32-bit sfasmlib.dll from sfasmlib_runtime.c (8 exports: console + Winsock).
REM Optional argument: destination directory — DLL written as DESTDIR\sfasmlib.dll (CMake passes CMAKE_BINARY_DIR).
REM Without argument: writes ..\sfasmlib.dll next to the sfasmlib folder.
REM Toolchain: tries MinGW i686 gcc first, then MSVC cl.exe if gcc is missing or fails.

setlocal EnableDelayedExpansion

set "SRC=%~dp0sfasmlib_runtime.c"

if "%~1"=="" (
  set "OUT=%~dp0..\sfasmlib.dll"
) else (
  set "OUT=%~f1\sfasmlib.dll"
)

set "GCC32="
set "PF86=%ProgramFiles(x86)%"

if defined SFASM_GCC32 if exist "!SFASM_GCC32!" set "GCC32=!SFASM_GCC32!"

if not defined GCC32 where i686-w64-mingw32-gcc >nul 2>&1 && set "GCC32=i686-w64-mingw32-gcc"

if not defined GCC32 if defined MINGW32 if exist "!MINGW32!\bin\gcc.exe" set "GCC32=!MINGW32!\bin\gcc.exe"

if not defined GCC32 for %%P in (
  "%ProgramFiles%\msys64\mingw32\bin\gcc.exe"
  "!PF86!\msys64\mingw32\bin\gcc.exe"
  "%LOCALAPPDATA%\Programs\msys64\mingw32\bin\gcc.exe"
  "C:\msys64\mingw32\bin\gcc.exe"
  "D:\msys64\mingw32\bin\gcc.exe"
) do if not defined GCC32 if exist %%~P set "GCC32=%%~P"

if defined GCC32 (
  set "XFLAGS=-lws2_32"
  for %%I in ("!GCC32!") do set "GCCEXE=%%~nxI"
  if /i "!GCCEXE!"=="i686-w64-mingw32-gcc.exe" set "XFLAGS=-m32 !XFLAGS!"

  echo [sfasmlib] Using gcc: !GCC32!
  echo [sfasmlib] Output: !OUT!

  REM gcc.exe only finds cc1/collect2/ld reliably if this toolchain's bin is on PATH (CMake/cmd often omit it).
  for %%I in ("!GCC32!") do set "GCCBINDIR=%%~dpI"
  set "PATH=!GCCBINDIR!;%PATH%"

  set "OUTU=!OUT:\=/!"
  set "SRCU=!SRC:\=/!"
  for %%O in ("!OUT!") do set "LOG=%%~dpOsfasm_gcc_last.log"

  (
    echo [sfasmlib] gcc invoke
    echo GCC32=!GCC32!
    echo GCCBINDIR=!GCCBINDIR!
    echo SRC=!SRC!
    echo SRCU=!SRCU!
    echo OUT=!OUT!
    echo OUTU=!OUTU!
    echo XFLAGS=!XFLAGS!
    echo ----- gcc output -----
  ) > "!LOG!"
  "!GCC32!" -shared -O2 -Wall -o "!OUTU!" "!SRCU!" !XFLAGS! >> "!LOG!" 2>&1
  set "GCC_EC=!ERRORLEVEL!"
  if not "!GCC_EC!"=="0" (
    echo [sfasmlib] gcc failed — exit !GCC_EC!
    echo [sfasmlib] Full log: !LOG!
    echo ----- log -----
    type "!LOG!"
    echo ----- end log -----
    goto try_msvc
  )
  del "!LOG!" 2>nul
  echo [sfasmlib] OK: !OUT!
  exit /b 0
)

:try_msvc
where cl >nul 2>&1
if errorlevel 1 (
  echo [sfasmlib] ERROR: no usable MinGW i686 gcc and no cl.exe in PATH.
  echo Install MSYS2 mingw-w64-i686-gcc ^(...\mingw32\bin^) or use "x86 Native Tools Command Prompt for VS".
  exit /b 1
)

pushd "%~dp0"
echo [sfasmlib] Using MSVC cl — Output: !OUT!
cl /nologo /O2 /W3 /LD sfasmlib_runtime.c ws2_32.lib /Fe"!OUT!" /link /MACHINE:X86 /DLL
if errorlevel 1 (
  del /q sfasmlib_runtime.obj 2>nul
  del /q sfasmlib_runtime.lib 2>nul
  del /q sfasmlib_runtime.exp 2>nul
  popd
  echo [sfasmlib] ERROR: MSVC build failed.
  exit /b 1
)

del /q sfasmlib_runtime.obj 2>nul
del /q sfasmlib_runtime.lib 2>nul
del /q sfasmlib_runtime.exp 2>nul
popd

echo [sfasmlib] OK: !OUT!
exit /b 0
