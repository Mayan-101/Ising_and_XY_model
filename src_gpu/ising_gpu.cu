#include <cuda_runtime.h>
#include <curand_kernel.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define CUDA_CHECK(call)                                         \
    do                                                           \
    {                                                            \
        cudaError_t _e = (call);                                 \
        if (_e != cudaSuccess)                                   \
        {                                                        \
            fprintf(stderr, "CUDA %s:%d — %s\n",                 \
                    __FILE__, __LINE__, cudaGetErrorString(_e)); \
            exit(1);                                             \
        }                                                        \
    } while (0)

struct IsingGpuState
{
    int height, width;
    int *d_grid;
    uint32_t *d_pixels;
    curandState *d_rng;
    double *d_sum_m;
    
    double *sweep_h;
    double *sweep_M;
    int *sweep_branch;
    int sweep_capacity;
    int sweep_count;
};

__global__ void k_init_rng(curandState *states, int height, int width,
                           unsigned long long seed)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height)
        return;
    curand_init(seed, (unsigned long long)(y * width + x), 0,
                &states[y * width + x]);
}

__global__ void k_update(int *grid, int height, int width,
                          double T, double h, double J, int parity, curandState *rng)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height)
        return;
    if (((x + y) & 1) != parity)
        return;

    int idx = y * width + x;
    curandState local = rng[idx];

    int current = grid[idx];
    int tL = grid[y * width + (x - 1 + width) % width];
    int tR = grid[y * width + (x + 1) % width];
    int tU = grid[((y - 1 + height) % height) * width + x];
    int tD = grid[((y + 1) % height) * width + x];

    int neighbor_sum = tL + tR + tU + tD;
    double delta_E = 2.0 * current * (J * neighbor_sum + h);

    if (delta_E <= 0.0 || (T > 0.0 && curand_uniform_double(&local) < exp(-delta_E / T)))
        grid[idx] = -current;

    rng[idx] = local;
}

__global__ void k_render(const int *grid, uint32_t *pixels,
                         int height, int width)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height)
        return;

    int current = grid[y * width + x];
    uint8_t r, g, b;
    if (current == 1) {
        r = 255; g = 45; b = 1;
    } else {
        r = 73; g = 16; b = 230;
    }
    pixels[y * width + x] = (255u << 24) | (b << 16) | (g << 8) | r;
}

static dim3 blocks(int w, int h, dim3 t)
{
    return dim3((w + t.x - 1) / t.x, (h + t.y - 1) / t.y);
}

__global__ void k_calc_M(const int *grid, int height, int width, double *d_sum_m)
{
    extern __shared__ double sdata[]; 
    double *s_m = sdata;

    int tid = threadIdx.y * blockDim.x + threadIdx.x;
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    double my_m = 0.0;

    if (x < width && y < height) {
        my_m = (double)grid[y * width + x];
    }

    s_m[tid] = my_m;
    __syncthreads();

    for (int s = (blockDim.x * blockDim.y) / 2; s > 0; s >>= 1) {
        if (tid < s) {
            s_m[tid] += s_m[tid + s];
        }
        __syncthreads();
    }

    if (tid == 0) {
        atomicAdd(d_sum_m, s_m[0]);
    }
}

extern "C"
{

    __declspec(dllexport)
    IsingGpuState *
    gpu_init(int height, int width)
    {
        IsingGpuState *s = (IsingGpuState *)malloc(sizeof(IsingGpuState));
        s->height = height;
        s->width = width;
        CUDA_CHECK(cudaMalloc(&s->d_grid, height * width * sizeof(int)));
        CUDA_CHECK(cudaMalloc(&s->d_pixels, height * width * sizeof(uint32_t)));
        CUDA_CHECK(cudaMalloc(&s->d_rng, height * width * sizeof(curandState)));
        CUDA_CHECK(cudaMalloc(&s->d_sum_m, sizeof(double)));

        dim3 t(16, 16);
        k_init_rng<<<blocks(width, height, t), t>>>(s->d_rng, height, width, 42ULL);
        CUDA_CHECK(cudaDeviceSynchronize());
        
        s->sweep_capacity = 2000;
        s->sweep_h = (double *)malloc(s->sweep_capacity * sizeof(double));
        s->sweep_M = (double *)malloc(s->sweep_capacity * sizeof(double));
        s->sweep_branch = (int *)malloc(s->sweep_capacity * sizeof(int));
        s->sweep_count = 0;
        
        return s;
    }

    __declspec(dllexport) void gpu_destroy(IsingGpuState *s)
    {
        cudaFree(s->d_grid);
        cudaFree(s->d_pixels);
        cudaFree(s->d_rng);
        cudaFree(s->d_sum_m);
        free(s->sweep_h);
        free(s->sweep_M);
        free(s->sweep_branch);
        free(s);
    }

    __declspec(dllexport) void gpu_upload_grid(IsingGpuState *s, const int *host_grid)
    {
        CUDA_CHECK(cudaMemcpy(s->d_grid, host_grid,
                              s->height * s->width * sizeof(int),
                              cudaMemcpyHostToDevice));
    }

    __declspec(dllexport) void gpu_update_grid(IsingGpuState *s, double T, double h, double J)
    {
        dim3 t(16, 16);
        dim3 b = blocks(s->width, s->height, t);
        for (int parity = 0; parity <= 1; parity++)
        {
            k_update<<<b, t>>>(s->d_grid, s->height, s->width, T, h, J, parity, s->d_rng);
            CUDA_CHECK(cudaDeviceSynchronize());
        }
    }

    __declspec(dllexport) void gpu_render_pixels(IsingGpuState *s, uint32_t *host_pixels)
    {
        dim3 t(16, 16);
        k_render<<<blocks(s->width, s->height, t), t>>>(
            s->d_grid, s->d_pixels, s->height, s->width);
        CUDA_CHECK(cudaDeviceSynchronize());
        CUDA_CHECK(cudaMemcpy(host_pixels, s->d_pixels,
                              s->height * s->width * sizeof(uint32_t),
                              cudaMemcpyDeviceToHost));
    }

    __declspec(dllexport) double gpu_measure_M(IsingGpuState *s)
    {
        double zero = 0.0;
        CUDA_CHECK(cudaMemcpy(s->d_sum_m, &zero, sizeof(double), cudaMemcpyHostToDevice));

        dim3 t(16, 16);
        int shared_mem_size = t.x * t.y * sizeof(double);
        k_calc_M<<<blocks(s->width, s->height, t), t, shared_mem_size>>>(s->d_grid, s->height, s->width, s->d_sum_m);
        CUDA_CHECK(cudaDeviceSynchronize());

        double sum_m = 0.0;
        CUDA_CHECK(cudaMemcpy(&sum_m, s->d_sum_m, sizeof(double), cudaMemcpyDeviceToHost));

        double M = sum_m / (double)(s->height * s->width);
        return M;
    }

    __declspec(dllexport) void gpu_run_hysteresis(IsingGpuState *s, double T, double h_max, int h_steps, int equil_sweeps, int meas_sweeps, double J)
    {
        int total_steps = (h_steps + 1) * 2;
        if (s->sweep_capacity < total_steps) {
            s->sweep_capacity = total_steps;
            s->sweep_h = (double *)realloc(s->sweep_h, s->sweep_capacity * sizeof(double));
            s->sweep_M = (double *)realloc(s->sweep_M, s->sweep_capacity * sizeof(double));
            s->sweep_branch = (int *)realloc(s->sweep_branch, s->sweep_capacity * sizeof(int));
        }
        s->sweep_count = 0;

        dim3 t(16, 16);
        dim3 b = blocks(s->width, s->height, t);
        int shared_mem_size = t.x * t.y * sizeof(double);

        // DRY loop implementation consolidating branch 0 and branch 1
        for (int branch = 0; branch <= 1; branch++)
        {
            for (int step = 0; step <= h_steps; step++)
            {
                double h = (branch == 0)
                         ? (h_max - (2.0 * h_max * step) / h_steps)
                         : (-h_max + (2.0 * h_max * step) / h_steps);

                // Equilibrate
                for (int eq = 0; eq < equil_sweeps; eq++) {
                    k_update<<<b, t>>>(s->d_grid, s->height, s->width, T, h, J, 0, s->d_rng);
                    k_update<<<b, t>>>(s->d_grid, s->height, s->width, T, h, J, 1, s->d_rng);
                }

                double zero = 0.0;
                CUDA_CHECK(cudaMemcpyAsync(s->d_sum_m, &zero, sizeof(double), cudaMemcpyHostToDevice));

                // Measure sweeps
                for (int me = 0; me < meas_sweeps; me++) {
                    k_update<<<b, t>>>(s->d_grid, s->height, s->width, T, h, J, 0, s->d_rng);
                    k_update<<<b, t>>>(s->d_grid, s->height, s->width, T, h, J, 1, s->d_rng);
                    k_calc_M<<<b, t, shared_mem_size>>>(s->d_grid, s->height, s->width, s->d_sum_m);
                }

                double total_sum_m = 0.0;
                CUDA_CHECK(cudaMemcpy(&total_sum_m, s->d_sum_m, sizeof(double), cudaMemcpyDeviceToHost));
                
                s->sweep_h[s->sweep_count] = h;
                s->sweep_M[s->sweep_count] = (total_sum_m / (double)(s->height * s->width)) / meas_sweeps;
                s->sweep_branch[s->sweep_count] = branch;
                s->sweep_count++;
            }
        }
    }

    __declspec(dllexport) void gpu_save_csv(IsingGpuState *s, const char *filename)
    {
        FILE *fp = fopen(filename, "w");
        if (!fp) {
            fprintf(stderr, "gpu_save_csv: Cannot open %s for writing\n", filename);
            return;
        }
        fprintf(fp, "h,M,branch\n");
        for (int i = 0; i < s->sweep_count; i++) {
            fprintf(fp, "%.8f,%.8f,%d\n", s->sweep_h[i], s->sweep_M[i], s->sweep_branch[i]);
        }
        fclose(fp);
    }
}
