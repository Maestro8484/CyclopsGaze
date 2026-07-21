@echo off
REM Flash CyclopsGaze to the Teensy 4.1 over USB and open the serial monitor.
REM Default port: COM6. Override: flash_com6.bat COM7
REM Bump FIRMWARE_VERSION in src\config.h before running this if firmware code changed
REM (RULES.md "Firmware discipline") -- this script does not do that for you.

setlocal
set "PORT=%~1"
if "%PORT%"=="" set "PORT=COM6"

set "REPO_DIR=%~dp0.."
set "PIO=%USERPROFILE%\.platformio\penv\Scripts\pio.exe"
if not exist "%PIO%" set "PIO=pio"

echo.
echo === CyclopsGaze flash: %PORT% ===
echo.

pushd "%REPO_DIR%"

"%PIO%" run -e cyclopsgaze -t upload --upload-port %PORT%
if errorlevel 1 (
    echo.
    echo === FLASH FAILED ^(exit %errorlevel%^) -- check the port and that the Teensy is connected ===
    popd
    endlocal
    exit /b 1
)

echo.
echo === Flash OK -- opening serial monitor on %PORT% at 115200 baud ^(Ctrl+C to exit^) ===
echo.

"%PIO%" device monitor -b 115200 --port %PORT%

popd
endlocal
