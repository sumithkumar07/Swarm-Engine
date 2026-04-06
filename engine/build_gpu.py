import os
import subprocess

# Precise Paths for VS 2026 (MSVC 14.50)
vcvars = r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
msvc_bin = r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64"
cuda_bin = r"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.1\bin"

def build():
    print("[DEBUG] Setting up swarm GPU build...")
    # Add CUDA to path just in case
    os.environ["PATH"] = cuda_bin + os.pathsep + os.environ["PATH"]
    
    cmd = [
        os.path.join(cuda_bin, "nvcc.exe"),
        "-O3", "-arch=sm_75", "--allow-unsupported-compiler",
        "-ccbin", msvc_bin,
        "swarm_v10_cuda.cu",
        "-o", "train_v10_cuda.exe"
    ]
    
    print(f"[DEBUG] Executing: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    if result.returncode == 0:
        print("[SUCCESS] train_v10_cuda.exe built!")
        return True
    else:
        print("[ERROR] CUDA Compilation failed!")
        print("STDOUT:", result.stdout)
        print("STDERR:", result.stderr)
        return False

if __name__ == "__main__":
    build()
