@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d "c:\Users\sumit\Downloads\infra refine\engine"
cl /LD /EHsc /O2 swarm_v9_swarm.cpp /Fe:swarm.dll
if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] swarm.dll compiled.
) else (
    echo [ERROR] Compilation failed.
)
