@echo off
echo --------- Convert Galaxian ---------
echo Moon Cresta Unpack roms
python ./unpack.py mooncrst.zip
if errorlevel 1 goto :error

echo Converting Moon Cresta
cd mooncresta
python mooncresta_rom_convert.py
cd ..
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
