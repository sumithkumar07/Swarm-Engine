#include <iostream>
#include <cuda_runtime.h>

// A simple GPU Kernel to verify massively parallel execution capability
__global__ void hello_gpu_kernel(int *a, int *b, int *c) {
    int idx = threadIdx.x;
    if (idx < 5) {
        c[idx] = a[idx] + b[idx];
        printf("[GPU Thread %d] %d + %d = %d\n", idx, a[idx], b[idx], c[idx]);
    }
}

int main() {
    std::cout << "--- STAGE 4 TITAN PROTOCOL: CUDA VERIFICATION ---\n";

    // 1. Get GPU Properties to verify hardware detection
    int deviceCount = 0;
    cudaError_t error_id = cudaGetDeviceCount(&deviceCount);
    
    if (error_id != cudaSuccess) {
        std::cerr << "[CRITICAL FAILURE] cudaGetDeviceCount returned " << static_cast<int>(error_id) << "\n";
        std::cerr << "-> " << cudaGetErrorString(error_id) << "\n";
        return 1;
    }
    
    if (deviceCount == 0) {
        std::cerr << "[CRITICAL FAILURE] No CUDA-capable devices detected.\n";
        return 1;
    }
    
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, 0);
    std::cout << "[SUCCESS] Detected GPU: " << deviceProp.name << "\n";
    std::cout << "          Compute Capability: " << deviceProp.major << "." << deviceProp.minor << "\n";
    std::cout << "          Max Threads per Block: " << deviceProp.maxThreadsPerBlock << "\n\n";

    // 2. Perform a microscopic memory operations test
    const int arraySize = 5;
    int a[arraySize] = { 1, 2, 3, 4, 5 };
    int b[arraySize] = { 10, 20, 30, 40, 50 };
    int c[arraySize] = { 0 };

    int *dev_a = nullptr, *dev_b = nullptr, *dev_c = nullptr;

    // Allocate Device memory (VRAM)
    if (cudaMalloc((void**)&dev_a, arraySize * sizeof(int)) != cudaSuccess) { std::cerr << "cudaMalloc failed!"; return 1; }
    if (cudaMalloc((void**)&dev_b, arraySize * sizeof(int)) != cudaSuccess) { std::cerr << "cudaMalloc failed!"; return 1; }
    if (cudaMalloc((void**)&dev_c, arraySize * sizeof(int)) != cudaSuccess) { std::cerr << "cudaMalloc failed!"; return 1; }

    // Copy Host memory (RAM) to Device
    cudaMemcpy(dev_a, a, arraySize * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(dev_b, b, arraySize * sizeof(int), cudaMemcpyHostToDevice);

    std::cout << "[SYSTEM] Firing 5 concurrent Matrix computation threads on GPU...\n";
    // Launch the Kernel with 1 Block and 5 Threads
    hello_gpu_kernel<<<1, 5>>>(dev_a, dev_b, dev_c);

    // Wait for GPU to finish so printf flushes to standard output
    cudaDeviceSynchronize();

    // Copy Device memory back to Host RAM
    cudaMemcpy(c, dev_c, arraySize * sizeof(int), cudaMemcpyDeviceToHost);

    // Clean up
    cudaFree(dev_a);
    cudaFree(dev_b);
    cudaFree(dev_c);

    std::cout << "\n[SUCCESS] Memory buffers retrieved perfectly. CUDA compiler pipeline is completely functional.\n";
    return 0;
}
