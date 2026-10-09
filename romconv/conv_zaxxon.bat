@echo off
setlocal
pushd "%~dp0"

echo --------- Convert Zaxxon ---------
rem echo Zaxxon Logo
rem python ./logoconv.py ../logos/zaxxon.png ../source/src/machines/zaxxon/zaxxon_logo.h
rem if errorlevel 1 goto :error

echo Converting Zaxxon
cd zaxxon
python zaxxon_rom_convert.py
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
