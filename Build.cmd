@echo off
cd /d "%~dp0"
set "FIXER_CSC=%SystemRoot%\Microsoft.NET\Framework\v4.0.30319\csc.exe"
if not exist "%FIXER_CSC%" (
 echo .NET Framework 4.0 or later is required.
 pause
 exit /b 1
)
"%FIXER_CSC%" /nologo /target:winexe /platform:anycpu /out:LegacySteamFixer.exe /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll src\Core.cs src\DeltaPatch.cs src\Program.cs src\Analysis.cs src\AutoBridge.cs src\LaunchProfiles.cs src\ZstdSupport.cs src\UiTheme.cs src\ThemedForm.cs src\UiDialogs.cs src\AppInfo.cs src\UpdateService.cs
if errorlevel 1 goto fail
"%FIXER_CSC%" /nologo /target:exe /platform:x86 /out:probes\SteamProbe-x86.exe src\SteamProbe.cs
if errorlevel 1 goto fail
"%FIXER_CSC%" /nologo /target:exe /platform:x64 /out:probes\SteamProbe-x64.exe src\SteamProbe.cs
if errorlevel 1 goto fail
exit /b 0
:fail
pause
exit /b 1
