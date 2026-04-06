@echo off
set "VCS_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "%VCS_PATH%" (
    echo [ERROR] vcvarsall.bat not found at %VCS_PATH%
    exit /b 1
)

call "%VCS_PATH%" x64 > nul 2>&1
echo [1/2] Compiling Swarm Benchmark...
nvcc -O3 -w swarm_v12_swarm_swarm.cu -o swarm_benchmark.exe 2> compile_errors.txt

if %ERRORLEVEL% EQU 0 (
    echo [2/2] Running Benchmark...
    swarm_benchmark.exe > benchmark_output.txt
    echo [SUCCESS] Benchmark results saved to benchmark_output.txt
) else (
    echo [ERROR] Compilation failed. See compile_errors.txt
    type compile_errors.txt
)
