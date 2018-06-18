//
//  Dispatcher.h
//  PFAC-Duo
//
//  Created by CbS Ghost on 2017/2/10.
//  Copyright (c) 2017 UrBX Creative Studio. All rights reserved.
//

#ifndef __Dispatcher__
#define __Dispatcher__

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include "dpfac-internal.h"
#include "DualByteTree.h"
#include "dpfac_kernel.cl.h"

//#if defined(__APPLE__) || defined(__MACOSX)
//#include <OpenCL/cl.hpp>
//#else
//#include <CL/cl.hpp>
//#endif
#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"

#include "dpfac_kernel.cuh"

//debug
#include <fstream>

#ifdef DPFAC_API
namespace DPFAC_Internal {
#endif
    class Dispatcher
    {
    public:
        Dispatcher(const cl_device_type type, const int count, cl_int *err);
        Dispatcher(const int count);
        void preparePatterns(std::istream &stream);
        int32_t compareToPFACFormat(std::istream &patterns, std::istream &str);
        //int32_t setPatterns(std::istream &patterns, std::istream &str);
        
    private:
        bool flag_cuda_backend;
        bool flag_cl_use_external_ctx;
        cl::Context *context;
        cl::CommandQueue *queue;
        cl::Program program;
        cl::Kernel NormalKernel;
        //cl::Buffer PatBuffer;
        std::vector<cl::Image1D> HashInfoImage;
        std::vector<cl::Buffer> HashInfoBuffer;
        std::vector<cl::Buffer> HashDataBuffer;
        std::vector<cl::Buffer> PatMaskBuffer;
        std::vector<cl::Buffer> InputBuffer;
        std::vector<cl::Buffer> OutputBuffer;
        
        cl_int initCLDevive(const cl_device_type type, const int count);
        cl_int compareToPFACFormat_CL(std::istream &patterns, std::istream &str);
        int32_t compareToPFACFormat_CUDA(std::istream &patterns, std::istream &str);
        //std::vector<Pat8_D16> Patterns;
        std::vector<char> RefString;
        std::vector<std::vector<cl_int>> result;
        
        int swap_index;
        int maxstring_size;
        int workgroup_size;
        int pattern_maxlen;
        
        
    };
#ifdef DPFAC_API
}
#endif

#endif /* defined(__Dispatcher__) */
