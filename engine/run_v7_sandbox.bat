call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /EHsc /O2 swarm_v7_sandbox.cpp /Fe:swarm_v7_sandbox.exe
swarm_v7_sandbox.exe
