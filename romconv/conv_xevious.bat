@echo off
echo --------- Convert Xevious ---------

rem echo Xevious Logo
rem python ./logoconv.py ../logos/xevious.png ../source/src/machines/xevious/xevious_logo.h
rem if errorlevel 1 goto :error

echo Converting Xevious (tiles+sprites+palette+roms+planetmap+wavetable)
cd xevious
python xevious_rom_convert.py
cd ..

if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
