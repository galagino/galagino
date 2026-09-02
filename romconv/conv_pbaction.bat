@echo off
echo --------- Convert Pinball Action ---------

rem echo Pinball Action Logo
rem python ./logoconv.py ../logos/pbaction.png ../source/src/machines/pbaction/pbaction_logo.h
rem if errorlevel 1 goto :error

echo Converting Pinball Action
cd pbaction
python pbaction_rom_convert.py
cd ..

if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
