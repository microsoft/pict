@echo off
setlocal

for %%i in (Debug, Release) DO (
  msbuild /m pict.sln /p:Configuration="%%i"
)

endlocal
 