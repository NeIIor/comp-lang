@echo off
REM Thin wrapper: build_runtime.bat tries MinGW gcc then MSVC cl.
call "%~dp0build_runtime.bat" %*
