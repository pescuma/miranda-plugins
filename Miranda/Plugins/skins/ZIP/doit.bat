rem @echo off

rem Batch file to build and upload files
rem 
rem TODO: Integration with FL

set name=skins

rem To upload, this var must be set here or in other batch
rem set ftp=ftp://<user>:<password>@<ftp>/<path>

echo Building %name% ...

rem msdev ..\%name%.dsp /MAKE "%name% - Win32 Release" /REBUILD
rem msdev ..\%name%.dsp /MAKE "%name% - Win32 Unicode Release" /REBUILD

echo Generating files for %name% ...

del *.zip
del *.dll
del *.pdb
rd /S /Q Plugins
rd /S /Q Docs
rd /S /Q Skins
rd /S /Q src

copy ..\Docs\%name%_changelog.txt
copy ..\Docs\%name%_version.txt
copy ..\Docs\%name%_readme.txt
mkdir Skins
cd Skins
mkdir Default
cd Default
copy ..\..\..\..\mydetails\data\Skins\Default\*.msk
cd..
cd..
mkdir Docs
cd Docs
del /Q *.*
copy ..\..\Docs\%name%_readme.txt
copy ..\..\Docs\langpack_%name%.txt
copy ..\..\m_%name%.h
copy ..\..\m_%name%_cpp.h
cd ..
copy ..\Release\%name%.pdb
copy "..\Unicode_Release\%name%W.pdb"

mkdir Plugins
cd Plugins
copy "..\..\..\..\bin\release unicode\Plugins\%name%W.dll"
copy "..\..\..\..\bin\release\Plugins\mydetails.dll"
cd ..

"C:\Program Files\Filzip\Filzip.exe" -a -rp %name%W.zip Plugins Docs Skins


cd Plugins
del /Q %name%W.dll
copy "..\..\..\..\bin\release\Plugins\%name%.dll"
cd ..

"C:\Program Files\Filzip\Filzip.exe" -a -rp %name%.zip Plugins Docs Skins

"C:\Program Files\Filzip\Filzip.exe" -a -rp %name%_src.zip src\*.*
"C:\Program Files\Filzip\Filzip.exe" -a -rp %name%.pdb.zip %name%.pdb
"C:\Program Files\Filzip\Filzip.exe" -a -rp %name%W.pdb.zip %name%W.pdb

del *.pdb
rd /S /Q Plugins
rd /S /Q Docs
rd /S /Q Skins
rd /S /Q src

if "%ftp%"=="" GOTO END

echo Going to upload files...
pause

"C:\Program Files\FileZilla\FileZilla.exe" -u .\%name%.zip %ftp% -overwrite -close 
"C:\Program Files\FileZilla\FileZilla.exe" -u .\%name%W.zip %ftp% -overwrite -close 
"C:\Program Files\FileZilla\FileZilla.exe" -u .\%name%.pdb.zip %ftp% -overwrite -close 
"C:\Program Files\FileZilla\FileZilla.exe" -u .\%name%W.pdb.zip %ftp% -overwrite -close 
"C:\Program Files\FileZilla\FileZilla.exe" -u .\%name%_changelog.txt %ftp% -overwrite -close 
"C:\Program Files\FileZilla\FileZilla.exe" -u .\%name%_version.txt %ftp% -overwrite -close 
"C:\Program Files\FileZilla\FileZilla.exe" -u .\%name%_readme.txt %ftp% -overwrite -close 


:END

echo Done.
