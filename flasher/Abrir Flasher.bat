@echo off
cd /d "%~dp0"

REM Buscar Python en ubicaciones comunes
set PYTHON=
for %%p in (
    "c:\Python314\python.exe"
    "c:\Python313\python.exe"
    "c:\Python312\python.exe"
    "c:\Python311\python.exe"
    "c:\Python310\python.exe"
) do (
    if exist %%p if "%PYTHON%"=="" set PYTHON=%%p
)

REM Si no encontró, intentar con el python del PATH
if "%PYTHON%"=="" (
    where python >nul 2>&1
    if %errorlevel%==0 set PYTHON=python
)

if "%PYTHON%"=="" (
    echo.
    echo  ERROR: No se encontro Python instalado.
    echo  Descargalo desde https://python.org
    pause
    exit /b 1
)

REM Instalar dependencias si faltan (silencioso)
%PYTHON% -m pip install customtkinter pyserial --quiet --disable-pip-version-check 2>nul

REM Abrir la app
start "" %PYTHON% "%~dp0flasher.py"
