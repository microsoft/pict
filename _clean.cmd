@echo off
setlocal

for %%i in (Debug, Release) DO (
  msbuild pict.sln /p:Configuration="%%i" /t:clean
  rmdir /s /q %%i
  for %%j in (api, api-usage, cli, clidll, clidll-usage) DO (
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



