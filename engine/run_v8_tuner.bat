call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /EHsc /O2 swarm_v8_schema_tuner.cpp /Fe:swarm_v8_schema_tuner.exe
swarm_v8_schema_tuner.exe
