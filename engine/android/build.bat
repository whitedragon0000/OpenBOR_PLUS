@setlocal
@echo off

#set "JAVA_HOME=C:\Program Files\Java\jdk-21"
set "PATH=%JAVA_HOME%\bin;%PATH%"

cd ../
set TOOLS=../tools/bin;../tools/7-Zip;../tools/svn/bin
set PATH=%TOOLS%;%PATH%
bash.exe version.sh
cd ./android

set mypath=%~dp0

::set "ANDROID_HOME=C:\android\sdk"

::IF NOT EXIST "%ANDROID_HOME%" (
::	mkdir "%ANDROID_HOME%"
::)

::IF NOT EXIST %ANDROID_HOME%cmdline-tools\bin\sdkmanager.bat (
::	echo cmdline-tools missing please download and extract to %ANDROID_HOME%
::	echo.
::	echo example path: %ANDROID_HOME%cmdline-tools\bin\sdkmanager.bat
::	pause
::	exit
::)

::IF NOT EXIST %ANDROID_HOME%licenses\ (
::	cd %ANDROID_HOME%
::	%ANDROID_HOME%cmdline-tools\bin\sdkmanager.bat --sdk_root=%ANDROID_HOME% --licenses
::)

cmd /k "cd %mypath% & java -version & gradlew.bat --version & gradlew.bat clean & gradlew.bat assembleDebug"

@endlocal

@rem clean
@rem assembleRelease
@rem assembleDebug
