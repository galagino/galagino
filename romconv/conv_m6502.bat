@echo off
echo --------- Convert M6502 ---------
echo M6502
python3 ./m6502patch.py
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
