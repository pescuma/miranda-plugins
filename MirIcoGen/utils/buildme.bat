@echo off

echo Icon Library Build batch file. Part of Icon Builder distribution.
echo:

set f=%1
if "%f%"=="" goto noparams

echo windres -o "%f%.o" "%f%.rc"
windres -o "%f%.o" "%f%.rc"
if errorlevel 1 goto end

rem --nmagic     Turns off page alignment
rem --strip-all  Omit all symbol information from the output file
rem --entry=0    Hard set entry so we don't have warning
rem --dll        Let linker know we are building DLL

ldw --nmagic --strip-all --entry=0 --dll -o "%f%.dll" "%f%.o"
if exist "%f%.dll" del "%f%.o"

goto end

:noparams
echo Error! No params.
echo:
echo Run buildme.bat [resource_file]
echo Where resource_file.rc is resource you want to compile
echo It will produce resource_file.dll lib to be used with Miranda
echo:
echo Example:
echo   buildme.bat autobuild [ENTER]
echo will read autobuild.rc and produce autbuild.dll
exit

:end
