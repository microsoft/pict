@echo off
echo ####################################################################
echo #
echo # Install Perl before running the tests
echo # This script works on Windows; create an equiv. for other platforms
echo #
echo ####################################################################

echo.
echo #
echo # Rebuilding binaries
echo #
call _build.cmd

echo.
echo #
echo # Testing release bits
echo #
pushd test
perl test.pl ..\release\cli\pict.exe rel.log
popd

echo.
echo #
echo # Testing debug bits
echo #
pushd test
perl test.pl ..\debug\cli\pict.exe   dbg.log
popd
