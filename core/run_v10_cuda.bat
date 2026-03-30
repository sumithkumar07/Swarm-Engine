call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
nvcc -allow-unsupported-compiler -O3 sovereign_v10_cuda.cu -o sovereign_v10_cuda.exe
sovereign_v10_cuda.exe
