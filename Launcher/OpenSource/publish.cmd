@echo off
setlocal
cd /d "%~dp0"
dotnet publish src\AmdnrLauncher.App\AmdnrLauncher.App.csproj -c Release -o publish
echo.
echo Output: %cd%\publish\AMDNR-Launcher.exe
dir /b publish\AMDNR-Launcher.exe
