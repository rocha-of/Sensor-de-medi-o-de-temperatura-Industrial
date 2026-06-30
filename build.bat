@echo off
REM Compila o sistema de monitoramento de sensores (Windows)
REM Requer o g++ (MinGW-w64) instalado e no PATH.
REM Uso: clique duas vezes ou execute build.bat no prompt.
g++ -std=c++17 -O2 -Wall -o sensores.exe main.cpp
if %errorlevel%==0 (
    echo.
    echo Compilado com sucesso: sensores.exe
    echo Execute com: sensores.exe
) else (
    echo.
    echo ERRO na compilacao. Verifique se o g++ ^(MinGW-w64^) esta instalado e no PATH.
)
pause
