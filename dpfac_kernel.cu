//
//  dpfac_kernel.cu
//  PFAC-Duo
//
//  Created by CbS Ghost on 2017/2/10.
//  Copyright (c) 2017 UrBX Creative Studio. All rights reserved.
//

#include "dpfac_kernel.cuh"
static void HandleError( cudaError_t err, const char *file, int line ) {
    if (err != cudaSuccess) {
        printf( "%s in %s at line %d\n", cudaGetErrorString( err ),
        file, line );
        exit( EXIT_FAILURE );
    }
}
#define HANDLE_ERROR( err ) (HandleError( err, __FILE__, __LINE__ ))

typedef struct _HashCell {
    uint16_t state;
    uint16_t next_state;
} HashCell __attribute__ ((aligned (4)));

__global__ void DpfacKernelCU_Normal(int32_t *result, uint16_t *input_buf, const uint32_t input_buf_size, const __restrict__ int32_t *hash_info, const __restrict__ HashCell *hash_data, const uint32_t hash_data_size, const uint16_t pat_count, const uint16_t max_pat_block_len)
{
    uint32_t gid = threadIdx.x + blockIdx.x * blockDim.x;
    if (gid >= input_buf_size) {
        return;
    }
    
    uint32_t hash_data_size_reg = hash_data_size;
    uint16_t pat_count_reg = pat_count;
    
    int32_t  pos[2];
    int32_t  run_nextstate[2] = {0, 0};
    uint16_t in_pat, pat_key[2];
    uint16_t match_result[2];
    HashCell h_cell[2] = {{0xFFFE, 0}, {0xFFFE, 0}};
    
    // match the first ushort
    pat_key[0] = input_buf[gid];
    pat_key[1] = pat_key[0] & 0xFF00;
    pos[0] = hash_info[0] + pat_key[0];
    pos[1] = hash_info[0] + pat_key[1];
    h_cell[0] = (pos[0] >= 0 && pos[0] < (int32_t)hash_data_size_reg && (pat_key[0] & 0x00FF)) ? hash_data[pos[0]] : h_cell[0];
    h_cell[1] = (pos[1] >= 0 && pos[1] < (int32_t)hash_data_size_reg) ? hash_data[pos[1]] : h_cell[1];
    #pragma unroll
    for (int32_t j = 0; j < 2; j ++) {
        run_nextstate[j] = (run_nextstate[j] == h_cell[j].state) ? h_cell[j].next_state : -run_nextstate[j];
    }
    
    // loop of ushort matching
    for (int32_t i = 1; run_nextstate[0] > pat_count_reg || run_nextstate[1] > pat_count_reg; i ++) {
        in_pat = input_buf[gid + i];
        #pragma unroll
        for (int32_t j = 0; j < 2; j ++) {
            if (run_nextstate[j] >= pat_count_reg) {
                pat_key[j] = in_pat;
                pos[j] = hash_info[run_nextstate[j]] + pat_key[j];
                h_cell[j] = (pos[j] >= 0 && pos[j] < (int32_t)hash_data_size_reg) ? hash_data[pos[j]]: h_cell[j];
                run_nextstate[j] = (run_nextstate[j] == h_cell[j].state) ? h_cell[j].next_state : -run_nextstate[j];
            }
        }
    }

    // match the last ushort
    #pragma unroll
    for (int32_t j = 0; j < 2; j ++) {
        if (run_nextstate[j] <= 0) {
            pat_key[j] &= 0x00FF;
            run_nextstate[j] = -run_nextstate[j];
            pos[j] = hash_info[run_nextstate[j]] + pat_key[j];
            h_cell[j] = (pos[j] >= 0 && pos[j] < (int32_t)hash_data_size_reg) ? hash_data[pos[j]] : h_cell[j];
            run_nextstate[j] = (run_nextstate[j] == h_cell[j].state) ? h_cell[j].next_state : 0;
        }
    }
    
    // write to output
    #pragma unroll
    for (int32_t i = 0; i < 2; i ++) {
        match_result[i] = (run_nextstate[i] <= pat_count_reg) ? run_nextstate[i] : 0;
    }
    if (*((int32_t *)match_result)) {
        result[gid] = *((int32_t *)match_result);
    }
}

int32_t DPFAC_InitDevice_CUDA(const int count)
{
    int32_t dev_count;
    HANDLE_ERROR( cudaGetDeviceCount(&dev_count) );
    if(dev_count == 0) {
        fprintf(stderr, "There is no device.\n");
        return -1;
    }
    if(dev_count < count) {
        fprintf(stderr, "Device out of range.\n");
        return -1;
    }
    
    HANDLE_ERROR( cudaSetDevice(count) );
    cudaDeviceSetCacheConfig(cudaFuncCachePreferL1);
    
    return 0;
}

int32_t DPFAC_CompareToPFACFormat_CUDA(const uint16_t *input_buf,     const uint32_t input_buf_size,
                                  const  int32_t *hash_info_buf, const uint32_t hash_info_buf_size,
                                  const  int32_t *hash_data_buf, const uint32_t hash_data_buf_size,
                                  const  int16_t *pat_mask_buf,  const uint32_t pat_mask_buf_size,
                                        uint16_t *output_buf,
                                  const uint16_t  pat_count,     const uint16_t max_pat_block_len)
{
    uint16_t *dev_input_buf;
    int32_t  *dev_hash_info_buf;
    int32_t  *dev_hash_data_buf;
    int16_t  *dev_pat_mask_buf;
    int32_t *dev_output_buf;
    HANDLE_ERROR( cudaMalloc(&dev_input_buf, sizeof(uint16_t) * input_buf_size) );
    HANDLE_ERROR( cudaMalloc(&dev_hash_info_buf, sizeof(int32_t) * 65536) );
    HANDLE_ERROR( cudaMalloc(&dev_hash_data_buf, sizeof(int32_t) * hash_data_buf_size) );
    //HandleError( cudaMalloc(&dev_pat_mask_buf, sizeof(int16_t) * pat_mask_buf_size) );
    HANDLE_ERROR( cudaMalloc(&dev_output_buf, sizeof(uint16_t) * input_buf_size * 2) );
    
    HANDLE_ERROR( cudaMemcpy(dev_input_buf, input_buf, sizeof(uint16_t) * input_buf_size, cudaMemcpyHostToDevice) );
    HANDLE_ERROR( cudaMemcpy(dev_output_buf, output_buf, sizeof(uint16_t) * 256 * 1024 * 1024, cudaMemcpyHostToDevice) );
    HANDLE_ERROR( cudaMemcpy(dev_hash_info_buf, hash_info_buf, sizeof(int32_t) * hash_info_buf_size, cudaMemcpyHostToDevice) );
    HANDLE_ERROR( cudaMemcpy(dev_hash_data_buf, hash_data_buf, sizeof(int32_t) * hash_data_buf_size, cudaMemcpyHostToDevice) );
    //HandleError( cudaMemcpy(dev_pat_mask_buf, pat_mask_buf, sizeof(int16_t) * pat_mask_buf_size, cudaMemcpyHostToDevice) );
    
    static float elapsedTime1 = 0.0;
    float elapsedTime;
    static int the_time = 0;
    for (int i =0;i<256;i++) {
        //printf("GPU i=%d\n",i);
    cudaEvent_t ev_begin, ev_end;
    cudaEventCreate(&ev_begin);
    cudaEventCreate(&ev_end);
    cudaEventRecord(ev_begin, 0);
    
    // kkkkkkk
   // TestCU<<<65536, 1024>>>(dev_output_buf);
   //dim3 numBlocks(128*1024);
   
    DpfacKernelCU_Normal<<<128 * 1024 * 8, 128>>>(dev_output_buf, dev_input_buf, 256 * 1024 * 1024, dev_hash_info_buf, (HashCell *)dev_hash_data_buf, hash_data_buf_size, pat_count, max_pat_block_len);
    cudaError_t err = cudaGetLastError();
    //if (err != cudaSuccess)
    //    printf("Error: %s\n", cudaGetErrorString(err));

    cudaEventRecord(ev_end, 0);
    cudaEventSynchronize(ev_end);
    
    the_time += 1;
    HANDLE_ERROR( cudaEventElapsedTime(&elapsedTime, ev_begin, ev_end) );
    elapsedTime1 += elapsedTime;
    
    cudaEventDestroy(ev_begin);
    cudaEventDestroy(ev_end);
    
    HANDLE_ERROR( cudaDeviceSynchronize() );
    }
    fprintf(stdout, "GPU average time: %lf ms\n", elapsedTime1 / the_time);
    HANDLE_ERROR( cudaMemcpy(output_buf, dev_output_buf, sizeof(uint16_t) * 256 * 1024 * 1024, cudaMemcpyDeviceToHost) );
    
    HANDLE_ERROR( cudaFree(dev_input_buf) );
    HANDLE_ERROR( cudaFree(dev_hash_info_buf) );
    HANDLE_ERROR( cudaFree(dev_hash_data_buf) );
    //HandleError( cudaFree(dev_pat_mask_buf) );
    HANDLE_ERROR( cudaFree(dev_output_buf) );
    
    

    return 0;
}
