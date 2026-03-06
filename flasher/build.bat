@echo off
setlocal enabledelayedexpansion

echo.
echo  ==========================================
echo    ESP32 Opfine Flasher  -  Build Script
echo  ==========================================
echo.
echo  Este script:
echo    1. Instala las dependencias Python
echo    2. Copia los binarios compilados del firmware
echo    3. Empaqueta todo en un .exe standalone
echo.
pause

REM ── Paso 1: Dependencias ───────────────────────────────────────────────────
echo [1/4] Instalando dependencias Python...
pip install -r requirements.txt
if %errorlevel% neq 0 (
    echo ERROR: Fallo al instalar dependencias.
    pause & exit /b 1
)

REM ── Paso 2: Crear carpeta bins ─────────────────────────────────────────────
echo.
echo [2/4] Copiando binarios del firmware...
if not exist "bins" mkdir bins

REM Firmware compilado por PlatformIO (pio run)
set FW=%~dp0..\{.pio}\build\esp32dev
REM Corregir la ruta (sin llaves, solo para claridad en el comentario)
set FW=%~dp0..\.pio\build\esp32dev

if exist "%FW%\firmware.bin" (
    copy "%FW%\firmware.bin"   "bins\" /Y >nul
    copy "%FW%\bootloader.bin" "bins\" /Y >nul
    copy "%FW%\partitions.bin" "bins\" /Y >nul
    echo    OK: firmware.bin  bootloader.bin  partitions.bin
) else (
    echo    ADVERTENCIA: No se encontraron los .bin compilados.
    echo    Ejecuta primero:  pio run
    echo    en la carpeta raiz del proyecto, luego vuelve a ejecutar este script.
    pause & exit /b 1
)

REM mkspiffs  (descargado por pio la primera vez que se ejecuta uploadfs)
set FOUND_MKSPIFFS=0
for /d %%d in ("%USERPROFILE%\.platformio\packages\tool-mkspiffs*") do (
    for %%f in ("%%d\mkspiffs*.exe") do (
        copy "%%f" "bins\" /Y >nul
        echo    OK: %%~nxf
        set FOUND_MKSPIFFS=1
    )
)
if "%FOUND_MKSPIFFS%"=="0" (
    echo    ADVERTENCIA: mkspiffs.exe no encontrado.
    echo    Ejecuta:  pio run --target uploadfs
    echo    para que PlatformIO lo descargue, luego repite este script.
    pause & exit /b 1
)

REM boot_app0.bin  (incluido con el framework de Arduino para ESP32)
set FOUND_APP0=0
for /d %%d in ("%USERPROFILE%\.platformio\packages\framework-arduinoespressif32*") do (
    if exist "%%d\tools\partitions\boot_app0.bin" (
        copy "%%d\tools\partitions\boot_app0.bin" "bins\" /Y >nul
        echo    OK: boot_app0.bin
        set FOUND_APP0=1
    )
)
if "%FOUND_APP0%"=="0" (
    echo    ADVERTENCIA: boot_app0.bin no encontrado.
    echo    Asegurate de haber compilado con: pio run
    pause & exit /b 1
)

REM ── Paso 3: Construir el .exe ──────────────────────────────────────────────
echo.
echo [3/4] Construyendo el ejecutable (puede tardar 1-2 minutos)...
echo.

pyinstaller ^
    --onefile ^
    --windowed ^
    --name "ESP32-Opfine-Flasher" ^
    --add-data "bins;bins" ^
    --hidden-import "customtkinter" ^
    --hidden-import "serial.tools.list_ports" ^
    --hidden-import "esptool" ^
    flasher.py

if %errorlevel% neq 0 (
    echo ERROR: Fallo al crear el ejecutable.
    pause & exit /b 1
)

REM ── Paso 4: Limpiar archivos temporales de PyInstaller ─────────────────────
echo.
echo [4/4] Limpiando archivos temporales...
if exist "ESP32-Opfine-Flasher.spec" del /f /q "ESP32-Opfine-Flasher.spec"
if exist "__pycache__" rd /s /q "__pycache__"
if exist "build" rd /s /q "build"

echo.
echo  ==========================================
echo   LISTO!
echo  ==========================================
echo.
echo   El instalador esta en:
echo   flasher\dist\ESP32-Opfine-Flasher.exe
echo.
echo   Puedes subir ese .exe a GitHub Releases
echo   para que cualquier persona lo descargue
echo   y flashee su ESP32 sin instalar nada.
echo.
pause
