@echo off
setlocal

set MSBUILD_TOOLS_DIR=%ProgramFiles(x86)%\MSBuild\12.0\Bin

for %%i in (Debug, Release) DO (
  "%MSBUILD_TOOLS_DIR%\msbuild.exe" pict.sln /p:Configuration="%%i" /t:clean
  rmdir /s /q %%i
  for %%j in (api, api-usage, cli) DO (
    rmdir /s /q %%j\%%i
  )
)

rmdir /q /s bin

attrib -h *.suo
del /q *.suo
del /q *.sdf

del test\dbg.log
del test\rel.log
del /q test\.std*

endlocal



