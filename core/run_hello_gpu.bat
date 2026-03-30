call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
nvcc -allow-unsupported-compiler -O3 hello_gpu.cu -o hello_gpu.exe
hello_gpu.exe
