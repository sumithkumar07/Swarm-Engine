@echo off
echo ============================================================
echo   swarm V10 — BUILDING TRAINER (train_v10.exe)
echo ============================================================
echo.

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

cd /d "c:\Users\sumit\Downloads\infra refine\engine"

echo [1/2] Compiling trainer (no -DBUILD_DLL = includes main())...
cl /EHsc /O2 /openmp swarm_v10_swarm.cpp /Fe:train_v10.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] train_v10.exe compiled.
    echo.
    echo Next: Run train_v10.exe to train the Master Brain.
    echo       This will read training_data.txt and vocab.txt
    echo       and output master_brain.bin
) else (
    echo.
    echo [ERROR] Compilation failed.
)
