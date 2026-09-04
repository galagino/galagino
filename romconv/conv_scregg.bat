@echo off
setlocal
pushd "%~dp0"

echo --------- Convert Scrambled Egg ---------
REM echo Scrambled Egg Unpack roms
REM python -c "import pathlib, zipfile; src=pathlib.Path(r'..\romszip\scregg.zip'); dst=pathlib.Path(r'..\romszip\scregg_unpack'); dst.mkdir(parents=True, exist_ok=True); zipfile.ZipFile(src).extractall(dst)"
REM if errorlevel 1 goto :error

rem echo Scrambled Egg Logo
rem python ./logoconv.py ../logos/scregg.png ../source/src/machines/scregg/scregg_logo.h
rem if errorlevel 1 goto :error

echo Converting Scrambled Egg
cd scregg
python scregg_rom_convert.py
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
