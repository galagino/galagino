@echo off
echo --------- Convert Tutankham ---------
echo Tutankham Unpack roms
python ./unpack.py tutankhm.zip
if errorlevel 1 goto :error

rem echo Tutankham Logos
rem python ./logoconv.py ../logos/tutankhm.png ../source/src/machines/tutankhm/tutankhm_logo.h
rem if errorlevel 1 goto :error

echo Converting Tutankham
cd tutankhm
python tutankhm_rom_convert.py
cd ..
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
