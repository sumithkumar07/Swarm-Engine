call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /EHsc /O2 sovereign_v8_schema_tuner.cpp /Fe:sovereign_v8_schema_tuner.exe
sovereign_v8_schema_tuner.exe
