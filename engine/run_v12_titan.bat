call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
nvcc -allow-unsupported-compiler -O3 swarm_v12_swarm_swarm.cu -o swarm_swarm.exe
swarm_swarm.exe
