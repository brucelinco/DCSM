//
//  dpfac_kernel.cuh
//  PFAC-Duo
//
//  Created by CbS Ghost on 2017/2/10.
//  Copyright (c) 2017 UrBX Creative Studio. All rights reserved.
//

#ifndef __DispatcherCU__
#define __DispatcherCU__

#include <stdint.h>
#include <stdio.h>
#include <cuda_runtime.h>

#ifdef  __cplusplus
extern "C" {
#endif

int32_t DPFAC_InitDevice_CUDA(const int count);

int32_t DPFAC_CompareToPFACFormat_CUDA(const uint16_t *input_buf,     const uint32_t input_buf_size,
                                       const  int32_t *hash_info_buf, const uint32_t hash_info_buf_size,
                                       const  int32_t *hash_data_buf, const uint32_t hash_data_buf_size,
                                       const  int16_t *pat_mask_buf,  const uint32_t pat_mask_buf_size,
                                             uint16_t *output_buf,
                                       const uint16_t  pat_count,     const uint16_t max_pat_block_len);

#ifdef  __cplusplus
}
#endif

#endif /* defined(__dpfac_kernel_cuh__) */
