@echo off

set TestEna=-c
set KPktNum=10

set PRG_PATH=..\..\bin\x64\release

taskkill /FI "WINDOWTITLE eq no_dll_test_rx" > nul
taskkill /FI "WINDOWTITLE eq no_dll_test_tx" > nul
taskkill /FI "WINDOWTITLE eq dll_test_rx"    > nul
taskkill /FI "WINDOWTITLE eq dll_test_tx"    > nul

cmd.exe /C "start "no_dll_test_rx" start_rx.bat %PRG_PATH%\no_dll_test.exe %TestEna% %KPktNum%"
cmd.exe /C "start "no_dll_test_tx" start_tx.bat %PRG_PATH%\no_dll_test.exe %TestEna% %KPktNum%"

