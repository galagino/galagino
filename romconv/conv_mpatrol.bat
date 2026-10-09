@echo off
setlocal
pushd "%~dp0"

echo --------- Convert Moon Patrol ---------
rem echo Moon Patrol Logo
rem python ./logoconv.py ../logos/mpatrol.png ../source/src/machines/mpatrol/mpatrol_logo.h
rem if errorlevel 1 goto :error

echo Converting Moon Patrol
cd mpatrol
python mpatrol_rom_convert.py
if errorlevel 1 goto :error
cd ..

echo --- Success ---
popd
endlocal
exit /b 0

:error
echo --- Error #%errorlevel%.
popd
endlocal
exit /b 1
