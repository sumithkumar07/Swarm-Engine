call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
nvcc -allow-unsupported-compiler -O3 swarm_v10_cuda.cu -o swarm_v10_cuda.exe
swarm_v10_cuda.exe
