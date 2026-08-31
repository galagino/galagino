@echo off
echo --------- Convert Circus Charlie ---------

rem echo Circus Charlie Logo
rem python ./logoconv.py ../logos/circusc.png ../source/src/machines/circusc/circusc_logo.h
rem if errorlevel 1 goto :error

echo Converting Circus Charlie (tiles+sprites+palette+roms)
cd circusc
python circusc_rom_convert.py
cd ..

if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
