@echo off
setlocal

set MSBUILD_TOOLS_DIR=%ProgramFiles(x86)%\MSBuild\14.0\Bin

for %%i in (Debug, Release) DO (
  "%MSBUILD_TOOLS_DIR%\msbuild.exe" /m pict.sln /p:Configuration="%%i"
)

endlocal
