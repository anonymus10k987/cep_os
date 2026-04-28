@echo off
echo ========================================
echo  Mini OS Kernel Simulator - Build Script
echo ========================================
echo.

if not exist "build" mkdir build
if not exist "output\logs" mkdir output\logs
if not exist "output\logs\fcfs" mkdir output\logs\fcfs
if not exist "output\logs\rr" mkdir output\logs\rr
if not exist "output\logs\priority" mkdir output\logs\priority

echo [1/2] Compiling...
g++ -std=c++20 -O2 -I include src/main.cpp -o build/os_simulator.exe -lpthread

if errorlevel 1 (
    echo.
    echo [ERROR] Compilation failed!
    exit /b 1
)

echo [2/2] Build successful!
echo.
echo Run with:
echo   build\os_simulator.exe --mode full
echo   build\os_simulator.exe --mode compare --quiet
echo   build\os_simulator.exe --mode sync
echo   build\os_simulator.exe --mode memory
echo   build\os_simulator.exe --config config\default.cfg --mode scheduling
echo.
