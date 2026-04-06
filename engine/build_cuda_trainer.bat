@echo off
set "VC_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
set "MSVC_BIN=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64"

if not exist "%VC_PATH%" (
    echo [ERROR] Could not find vcvarsall.bat at %VC_PATH%
    exit /b 1
)

echo [DEBUG] Initializing MSVC Environment...
call "%VC_PATH%" x64

echo [DEBUG] Building swarm swarm V10 (CUDA)...
nvcc -O3 -arch=sm_75 --allow-unsupported-compiler ^
    -ccbin "%MSVC_BIN%" ^
    swarm_v10_cuda.cu tokenizer.cpp ^
    -o train_v10_cuda.exe

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CUDA Compilation failed.
    exit /b 1
)

echo [SUCCESS] train_v10_cuda.exe built.
echo Run with: .\train_v10_cuda.exe
