//
//  dpfac_kernel.cl
//  PFAC-Duo
//
//  Created by Chung-Yu Liao on 2017/2/20.
//  Copyright (c) 2017 NTNU HCLab. All rights reserved.
//

typedef struct _HashCell {
    ushort state;
    ushort next_state;
} HashCell __attribute__ ((aligned (4)));

kernel void DpfacKernel_Normal(global write_only int *result, global short *input_buf, global read_only const int *hash_info, global read_only const HashCell *hash_data, const uint hash_data_size, const ushort pat_count, const ushort max_pat_block_len)
{
    uint   hash_data_size_reg = hash_data_size;
    ushort pat_count_reg = pat_count;
    
    size_t gid = get_global_id(0);
    
    int      pos[2];
    int      run_nextstate[2] = {0, 0};
    ushort   in_pat, pat_key[2];
    ushort   match_result[2];
    HashCell h_cell[2] = {{0xFFFE, 0}, {0xFFFE, 0}};
    
    // match the first ushort
    pat_key[0] = input_buf[gid];
    pat_key[1] = pat_key[0] & 0xFF00;
    pos[0] = hash_info[0] + pat_key[0];
    pos[1] = hash_info[0] + pat_key[1];
    h_cell[0] = (pos[0] >= 0 && pos[0] < (int)hash_data_size_reg && (pat_key[0] & 0x00FF)) ? hash_data[pos[0]] : h_cell[0];
    h_cell[1] = (pos[1] >= 0 && pos[1] < (int)hash_data_size_reg) ? hash_data[pos[1]] : h_cell[1];
    
    //#pragma unroll
    for (int j = 0; j < 2; j ++) {
        run_nextstate[j] = (run_nextstate[j] == h_cell[j].state) ? h_cell[j].next_state : -run_nextstate[j];
    }
    
    // loop of ushort matching
    for (int i = 1; run_nextstate[0] > pat_count_reg || run_nextstate[1] > pat_count_reg; i ++) {
        in_pat = input_buf[gid + i];
        //#pragma unroll
        for (int j = 0; j < 2; j ++) {
            if (run_nextstate[j] >= pat_count_reg) {
                pat_key[j] = in_pat;
                pos[j] = hash_info[run_nextstate[j]] + pat_key[j];
                h_cell[j] = (pos[j] >= 0 && pos[j] < (int)hash_data_size_reg) ? hash_data[pos[j]]: h_cell[j];
                run_nextstate[j] = (run_nextstate[j] == h_cell[j].state) ? h_cell[j].next_state : -run_nextstate[j];
            }
        }
    }
    
    // match the last ushort
    //#pragma unroll
    for (int j = 0; j < 2; j ++) {
        if (run_nextstate[j] <= 0) {
            pat_key[j] &= 0x00FF;
            run_nextstate[j] = -run_nextstate[j];
            pos[j] = hash_info[run_nextstate[j]] + pat_key[j];
            h_cell[j] = (pos[j] >= 0 && pos[j] < (int)hash_data_size_reg) ? hash_data[pos[j]] : h_cell[j];
            run_nextstate[j] = (run_nextstate[j] == h_cell[j].state) ? h_cell[j].next_state : 0;
        }
    }
    
    // write to output
    //#pragma unroll
    for (int i = 0; i < 2; i ++) {
        match_result[i] = (run_nextstate[i] <= pat_count_reg) ? run_nextstate[i] : 0;
    }

    if (*((int *)match_result)) {
        result[gid] = *((int *)match_result);
    }
}
