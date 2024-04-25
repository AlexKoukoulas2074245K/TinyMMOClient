@echo off
mkdir build
cd build

:: Check visual studios 2022,2019, 2017, 2015 in order and generate with the first one found
reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.17.0" >> nul 2>&1
if %ERRORLEVEL% NEQ 0 ( echo Visual Studio 2022 not installed ) else (
	echo Generating project for Visual Studio 2022 x86
	cmake -G "Visual Studio 17 2022" ../ -A Win32
	cd ..
	exit /b
)

reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.16.0" >> nul 2>&1
if %ERRORLEVEL% NEQ 0 ( echo Visual Studio 2019 not installed ) else (
	echo Generating project for Visual Studio 2019 x86
	cmake -G "Visual Studio 16 2019" ../ -A Win32
	cd ..
	exit /b
)

reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.15.0" >> nul 2>&1
if %ERRORLEVEL% NEQ 0 ( echo Visual Studio 2017 not installed ) else (
	echo Generating project for Visual Studio 2017
	cmake -G "Visual Studio 15 2017" ../ -A Win32
	cd ..
	exit /b
)

reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.14.0" >> nul 2>&1
if %ERRORLEVEL% NEQ 0 ( echo Visual Studio 2015 not installed ) else (
	echo Generating project for Visual Studio 2015
	cmake -G "Visual Studio 14 2015" ../ -A Win32 
	cd ..
	exit /b
)