call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
nvcc -allow-unsupported-compiler -O3 sovereign_v12_titan_swarm.cu -o titan_swarm.exe
titan_swarm.exe
