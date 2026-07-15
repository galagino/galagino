@echo off
echo --------- Convert Phoenix ---------
rem echo Phoenix Logos
rem python ./logoconv.py ../logos/phoenix.png ../source/src/machines/phoenix/phoenix_logo.h

echo Converting Phoenix
cd phoenix
python ./phoenix_rom_convert.py
cd ..
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
