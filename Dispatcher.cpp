//
//  Dispatcher.cpp
//  PFAC-Duo
//
//  Created by CbS Ghost on 2017/2/10.
//  Copyright (c) 2017 UrBX Creative Studio. All rights reserved.
//

#include "Dispatcher.h"
using namespace std;
using namespace cl;
#ifdef DPFAC_API
using namespace DPFAC_Internal;
#endif

Dispatcher::Dispatcher(const cl_device_type type, const int count, cl_int *err)
{
    cl_int ret = CL_SUCCESS;
    ret = initCLDevive(type, count);
    if (err != nullptr) {
        *err = ret;
    }
    flag_cuda_backend = false;
    flag_cl_use_external_ctx = false;
}

Dispatcher::Dispatcher(const int count)
{
    DPFAC_InitDevice_CUDA(count);
    flag_cuda_backend = true;
}

cl_int Dispatcher::initCLDevive(const cl_device_type type, const int count)
{
    cl_int err = CL_SUCCESS;
    flag_cl_use_external_ctx = false;
    try {
        vector<Platform> platforms;
        Platform::get(&platforms);
        if (platforms.size() == 0) {
            cerr << "ERROR: Unable to find OpenCL platforms(" << CL_INVALID_PLATFORM << ")" << endl;
            return CL_INVALID_PLATFORM;
        }
        cl_context_properties prop[3] = { CL_CONTEXT_PLATFORM,
            (cl_context_properties)(platforms[0])(), 0};
        context = new Context(type, prop);
        
        vector<Device> devices = context->getInfo<CL_CONTEXT_DEVICES>();
        
        string devname = devices[count].getInfo<CL_DEVICE_NAME>();
        cout << "Device: " << devname.c_str() << endl;
        
        string version = devices[count].getInfo<CL_DEVICE_VERSION>();
        cout << "OpenCL Version: " << version.c_str() << endl;
        
        workgroup_size = (int)devices[count].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
        //maxstring_size = (int)devices[count].getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() / 4;
        
        queue = new CommandQueue(*context, devices[0], CL_QUEUE_PROFILING_ENABLE);
        
        // Temp Solution: Load text file externally.
        //ifstream kernelfile("dpfac_kernel.cl");
        //string kernel_str;
        //getline(kernelfile, kernel_str, (char)kernelfile.eof());
        string kernelfile(DPFAC_KERNEL_DATA, DPFAC_KERNEL_DATA + DPFAC_KERNEL_SIZE);
        //cout << kernelfile << endl;
        program = Program(*context, kernelfile);
        program.build(devices);
        
        NormalKernel = Kernel(program, "DpfacKernel_Normal");
        
        //HashInfoBuffer.push_back(Buffer(*context, CL_MEM_READ_ONLY, 65536 * sizeof(cl_int), NULL, &err));
        //HashInfoBuffer.push_back(Buffer(*context, CL_MEM_READ_ONLY, maxstring_size * sizeof(cl_char), NULL, &err));
        //HashDataBuffer.push_back(Buffer(*context, CL_MEM_READ_ONLY, 65536 * sizeof(cl_int), NULL, &err));
        //HashDataBuffer.push_back(Buffer(*context, CL_MEM_READ_ONLY, 65536 * sizeof(cl_int), NULL, &err));
        
        //PatMaskBuffer.push_back(Buffer(*context, CL_MEM_READ_ONLY, 2048 * sizeof(cl_short), NULL, &err));
        
        //HashInfoBuffer.push_back(Buffer(*context, CL_MEM_READ_ONLY, (257 * 1024 * 1024) * sizeof(cl_char), NULL, &err));
        //OutputBuffer.push_back(Buffer(*context, CL_MEM_WRITE_ONLY, 512 * 1024 * 1024 * sizeof(cl_char), NULL, &err));
    } catch (Error err) {
        cerr << "ERROR: " << err.what() << "(" << err.err()
        << ")" << endl;
    }
    swap_index = 0;
    ///RefString.resize(maxstring_size + 1);
    
    return CL_SUCCESS;
}

int32_t Dispatcher::compareToPFACFormat(std::istream &patterns, std::istream &str)
{
    if (flag_cuda_backend) {
        return compareToPFACFormat_CUDA(patterns, str);
    }
    return compareToPFACFormat_CL(patterns, str);
}

cl_int Dispatcher::compareToPFACFormat_CL(std::istream &patterns, std::istream &str)
{
    cl_int err;
    std::vector<uint8_t> fileContents;
    fileContents.reserve(256*1024*1024);
    fileContents.assign(std::istreambuf_iterator<char>(str), std::istreambuf_iterator<char>());
    
    DualByteTree tree(patterns);
    vector<const vector<int32_t> *> h_info = tree.treeInfo();
    vector<const vector<uint32_t> *> h_data = tree.treeData();
    vector<const vector<uint16_t> *> pat_mask = tree.patternMask();
    uint32_t h_info_size = (uint32_t)h_info[0]->size();
    uint32_t h_data_size = (uint32_t)h_data[0]->size();
    uint16_t pat_count = (uint16_t)pat_mask[0]->size();
    uint16_t max_pat_block_len = (uint16_t)tree.maxPatternBlockSize() - 1;
    
    int32_t  *HostHashInfoBuffer = new int32_t[h_info[0]->size()];
    int32_t  *HostHashDataBuffer = new int32_t[h_data[0]->size()];
    uint16_t *HostInputBuffer = new uint16_t[129*1024*1024];
    uint16_t *HostOutputBuffer = new uint16_t[256*1024*1024];
    
    HashInfoBuffer.clear();
    HashDataBuffer.clear();
    InputBuffer.clear();
    OutputBuffer.clear();
    //Buffer fff(Buffer(*context, CL_MEM_READ_ONLY, 65536 * sizeof(cl_int), NULL, &err));
    HashInfoBuffer.push_back(Buffer(*context, CL_MEM_READ_ONLY, 65536 * sizeof(cl_int), NULL, &err));
    HashDataBuffer.push_back(Buffer(*context, CL_MEM_READ_ONLY, h_data[0]->size() * sizeof(cl_int), NULL, &err));
    InputBuffer.push_back(Buffer(*context, CL_MEM_READ_ONLY, 129*1024*1024 * sizeof(cl_ushort), NULL, &err));
    OutputBuffer.push_back(Buffer(*context, CL_MEM_WRITE_ONLY, 256*1024*1024 * sizeof(cl_ushort), NULL, &err));

    //Event UploadEvent, DownloadEvent;
    //vector<vector<Event>> DownloadEvent;
    //DownloadEvent.clear();
    //DownloadEvent.resize(2);
    //DownloadEvent[0].resize(1);
    //DownloadEvent[1].resize(1);
    //vector<Event> ExeEvent, DownloadEvent1;
    //ExeEvent.clear();
    //ExeEvent.resize(1);
    Event TestEvent;
    uint16_t *rrr = new uint16_t [256 * 1024 * 1024 * sizeof(cl_short)];
    
    std::size_t read_size;
    try {
        queue->enqueueWriteBuffer(HashInfoBuffer[0], CL_TRUE, 0, h_info[0]->size() * sizeof(cl_int), h_info[0]->data());
        queue->enqueueWriteBuffer(HashDataBuffer[0], CL_TRUE, 0, h_data[0]->size() * sizeof(cl_int), h_data[0]->data());
        queue->enqueueWriteBuffer(InputBuffer[0], CL_TRUE, 0, fileContents.size() > 256*1024*1024 ? 256*1024*1024 : fileContents.size(), fileContents.data());
        queue->enqueueWriteBuffer(OutputBuffer[0], CL_TRUE, 0, 256 * 1024 * 1024 * sizeof(cl_short), rrr);
        NormalKernel.setArg(0, OutputBuffer[0]);
        NormalKernel.setArg(1, InputBuffer[0]);
        NormalKernel.setArg(2, HashInfoBuffer[0]);
        NormalKernel.setArg(3, HashDataBuffer[0]);
        NormalKernel.setArg(4, sizeof(cl_uint), &h_data_size);
        NormalKernel.setArg(5, sizeof(cl_ushort), &pat_count);
        NormalKernel.setArg(6, sizeof(cl_ushort), &max_pat_block_len);
        queue->enqueueNDRangeKernel(NormalKernel, NullRange, NDRange(128*1024*1024), NullRange, NULL, &TestEvent);
        //queue->finish();
        
        queue->enqueueReadBuffer(OutputBuffer[0], CL_TRUE, 0, 256 *1024*1024* sizeof(cl_short), rrr, 0, NULL);
        //for (int64_t i = 0; i < 256 * 1024 * 1024; i ++) {
         //   cout << rrr[i] << " ";
        //}
        cout << endl;
        queue->finish();
        double start_time, end_time;
        start_time = TestEvent.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        end_time = TestEvent.getProfilingInfo<CL_PROFILING_COMMAND_END>();
                //printf("32 bit: %lf ms\n", ((double)end_time - (double)start_time) / 1000000.0);
        cout << "OpenCL time: " <<(((double)end_time - (double)start_time) / 1000000.0) << " ms" << endl;
        return 0; //***********
        //cout << "time: " << end_time << ","<< start_time << endl;
        //            FastKernel_D16.setArg(1, StrBuffer[swap_index]);
        //            FastKernel_D16.setArg(2, Local(workgroup_size + pattern_maxlen));
        //            FastKernel_D16.setArg(3, OutBuffer[swap_index]);
        //            FastKernel_D16.setArg(4, sizeof(cl_int), &pattern_maxlen);
        //            FastKernel_D16.setArg(5, sizeof(cl_int), &pat_size);

//        for (std::size_t i = 0; i < str_size; i += maxstring_size) {
//            read_size = str_size - i;
//            read_size = (read_size > maxstring_size) ? maxstring_size : read_size;
//            str.read(RefString.data(), read_size);
//            queue.enqueueWriteBuffer(StrBuffer[swap_index], CL_TRUE, 0, read_size, RefString.data());
//            
//            //DownloadEvent[0].wait();
//            
//            FastKernel_D16.setArg(0, PatBuffer);
//            FastKernel_D16.setArg(1, StrBuffer[swap_index]);
//            FastKernel_D16.setArg(2, Local(workgroup_size + pattern_maxlen));
//            FastKernel_D16.setArg(3, OutBuffer[swap_index]);
//            FastKernel_D16.setArg(4, sizeof(cl_int), &pattern_maxlen);
//            FastKernel_D16.setArg(5, sizeof(cl_int), &pat_size);
//            //cout << "dd\n";
//            
//            queue.enqueueNDRangeKernel(FastKernel_D16, NullRange, NDRange(maxstring_size), NDRange(workgroup_size), &DownloadEvent1, &ExeEvent[0]);
//            
//            //queue.finish();
//            
//            //cout << "dd\n";
//            //result.clear();
//            //result.resize(1);
//            //result[0].resize(maxstring_size);
//            //cout << "dd\n";
//            //vector<vector<int>> sss;
//            //sss.clear();
//            //sss.resize(1);
//            //sss[0].resize(ref_str_size);
//            cout << "eee: " << maxstring_size<<endl;
//            queue.enqueueReadBuffer(OutBuffer[0], CL_FALSE, 0, maxstring_size * sizeof(cl_int), result[0].data() + i, &ExeEvent, &DownloadEvent[0][0]);
//            
//            DownloadEvent[0].swap(DownloadEvent[1]);
//            //queue.enqueueReadBuffer(OutBuffer, CL_TRUE, 0, ref_str_size * sizeof(int), result.at(0).data());
//            //break;
//        }
        
        
    } catch (Error err) {
        cerr << "ERROR: " << err.what() << "(" << err.err()
        << ")" << endl;
    }
    
    
    
    
    string  lline;
    vector<string> pattern_listt;
    
    patterns.clear();
    patterns.seekg(0, std::ios::beg);
    while (getline(patterns, lline)) {
        if (!lline.empty()) {
            pattern_listt.push_back(lline);
        }
    }
    int64_t match_count = 0;
    vector<uint8_t> uii;
    
    int32_t *ttt;
    ttt = (int32_t *)&rrr[0];
    cout << "ttt: " << *ttt << endl;
    uint8_t *tt;
    tt = (uint8_t *)&rrr[0];
    cout << "[" << tt[0] << tt[1] << tt[2] << tt[3] << "]" << endl;
    int error = 0;
    for(int32_t i = 0; i < 256*1024*1024; i ++) {
        error = 0;
        //cout << "i="<<i<<endl;
        if (rrr[i] >= pat_mask[0]->size()) {
            cout << "i="<<i<<endl;
            cout << rrr[i]<<endl;
            cout << "unknown error" << endl;
        } else if (rrr[i] != 0){
            //cout << "i="<<i<<endl;
            match_count += 1;
            //uii.clear();
            //uii.insert(uii.begin(), HostInputBuffer[i], HostInputBuffer[i + pattern_listt.at(i).size()]);
            //        tt = (uint8_t *)&HostOutputBuffer[i];
            //cout << "(" << HostOutputBuffer[i] << ") ";
            //                     cout << "[" << tt[0] << tt[1] << "]" << endl;
            //        if (HostOutputBuffer[i] <= 0 || HostOutputBuffer[i] > 2000) {cout << "GG" << endl;continue;}
            //cout << "(" << i<<":"<< HostOutputBuffer[i] << ") " << pattern_listt.at(HostOutputBuffer[i] - 1) << " ~ ";
            for (int j = 0; j < pattern_listt.at(rrr[i] - 1).size(); j ++) {
                //cout << fileContents[i + j];
                if (fileContents[i + j] != pattern_listt.at(rrr[i] - 1).at(j)) {
                    error = 1;
                    cout << "error";
                    cout << "(" << i<<":"<< rrr[i] << ") " << pattern_listt.at(rrr[i] - 1) << " ~ ";
                    break;
                }
            }
            if (error) {
                for (int j = 0; j < pattern_listt.at(rrr[i] - 1).size(); j ++) {
                    cout << fileContents[i + j];
                }
                cout << endl;
            } else {
                //cout << "(" << i<<":"<< rrr[i] << ") " << pattern_listt.at(rrr[i] - 1) << " ~ ";
                //for (int j = 0; j < pattern_listt.at(rrr[i] - 1).size(); j ++) {
                //    cout << fileContents[i + j];
                //}
                //cout << endl;
            }
            //cout << endl;
            
            //        tt = (uint8_t *)&HostOutputBuffer[i];
            //        cout << hex << (int)tt[0] <<" " << (int)tt[1]<<" " << (int)tt[2] <<" "<< (int)tt[3] <<" "<< endl;
            //        tt = (uint8_t *)&HostInputBuffer[i];
            //        cout << (int)tt[0]<<" " << (int)tt[1]<<" " << (int)tt[2]<<" " << (int)tt[3]<<" " << dec << endl;
        }
    }
    cout << "matched: " << match_count << endl;
    
    return err;
}

int32_t Dispatcher::compareToPFACFormat_CUDA(std::istream &patterns, std::istream &str)
{
    //tmp solution
    //std::ifstream testFile("s256MB.dat", std::ios::binary);
    std::vector<uint8_t> fileContents;
    fileContents.reserve(256*1024*1024);
    fileContents.assign(std::istreambuf_iterator<char>(str), std::istreambuf_iterator<char>());
    
    DualByteTree tree(patterns);
    vector<const vector<int32_t> *> h_info = tree.treeInfo();
    vector<const vector<uint32_t> *> h_data = tree.treeData();
    vector<const vector<uint16_t> *> pat_mask = tree.patternMask();
    uint32_t h_info_size = (uint32_t)h_info[0]->size();
    uint32_t h_data_size = (uint32_t)h_data[0]->size();
    uint16_t pat_count = (uint16_t)pat_mask[0]->size();
    uint16_t max_pat_block_len = (uint16_t)tree.maxPatternBlockSize() - 1;
    
    int32_t  *HostHashInfoBuffer = new int32_t[h_info[0]->size()];
    int32_t  *HostHashDataBuffer = new int32_t[h_data[0]->size()];
    int16_t  *HostPatMaskBuffer = new int16_t[pat_mask[0]->size()];
    uint16_t *HostInputBuffer = new uint16_t[129*1024*1024];
    uint16_t *HostOutputBuffer = new uint16_t[256*1024*1024];
    
    copy_n(fileContents.data(), fileContents.size() > 256*1024*1024 ? 256*1024*1024 : fileContents.size(), (uint8_t *)HostInputBuffer);
    copy_n(h_info[0]->data(), h_info[0]->size(), HostHashInfoBuffer);
    copy_n(h_data[0]->data(), h_data[0]->size(), HostHashDataBuffer);
    copy_n(pat_mask[0]->data(), pat_mask[0]->size(), HostPatMaskBuffer);
    
    string  lline;
    vector<string> pattern_listt;
    
    patterns.clear();
    patterns.seekg(0, std::ios::beg);
    while (getline(patterns, lline)) {
        if (!lline.empty()) {
            pattern_listt.push_back(lline);
        }
    }
    uint8_t *tt;
    //for(int32_t i = 0; i < 18000; i ++) {
        //if (HostOutputBuffer[i] != 0) {
        //tt = (uint8_t *)&HostInputBuffer[i];
        //cout << tt[0] << tt[1];
        //}
        //        tt = (uint8_t *)&HostOutputBuffer[i];
        //        cout << hex << (int)tt[0] <<" " << (int)tt[1]<<" " << (int)tt[2] <<" "<< (int)tt[3] <<" "<< endl;
        //        tt = (uint8_t *)&HostInputBuffer[i];
        //        cout << (int)tt[0]<<" " << (int)tt[1]<<" " << (int)tt[2]<<" " << (int)tt[3]<<" " << dec << endl;
    //}
    cout <<endl;
    DPFAC_CompareToPFACFormat_CUDA(HostInputBuffer, 129*1024*1024, HostHashInfoBuffer, h_info_size, HostHashDataBuffer, h_data_size, HostPatMaskBuffer, pat_mask[0]->size(), HostOutputBuffer, pat_count, max_pat_block_len);
    //DPFAC_CompareToPFACFormat_CUDA(HostInputBuffer, 129*1024*1024, HostHashInfoBuffer, HostHashDataBuffer, h_data_size, HostPatMaskBuffer, pat_mask[0]->size(), HostOutputBuffer, pat_count, max_pat_block_len);
    //DPFAC_CompareToPFACFormat_CUDA(HostInputBuffer, 129*1024*1024, HostHashInfoBuffer, HostHashDataBuffer, h_data_size, HostPatMaskBuffer, pat_mask[0]->size(), HostOutputBuffer, pat_count, max_pat_block_len);
    //DPFAC_CompareToPFACFormat_CUDA(HostInputBuffer, 129*1024*1024, HostHashInfoBuffer, HostHashDataBuffer, h_data_size, HostPatMaskBuffer, pat_mask[0]->size(), HostOutputBuffer, pat_count, max_pat_block_len);
    //return 0; // ********
    int64_t match_count = 0;
    vector<uint8_t> uii;
    
    int32_t *ttt;
    ttt = (int32_t *)&HostOutputBuffer[0];
    cout << "ttt: " << *ttt << endl;
    tt = (uint8_t *)&HostOutputBuffer[0];
    cout << "[" << tt[0] << tt[1] << "]" << endl;
    int error = 0;
    for(int32_t i = 0; i < 256*1024*1024; i ++) {
        error = 0;
        //cout << "i="<<i<<endl;
        if (HostOutputBuffer[i] >= pat_mask[0]->size()) {
            cout << "i="<<i<<endl;
            cout << HostOutputBuffer[i]<<endl;
            cout << "unknown error" << endl;
        } else if (HostOutputBuffer[i] != 0){
            //cout << "i="<<i<<endl;
            match_count += 1;
            //uii.clear();
            //uii.insert(uii.begin(), HostInputBuffer[i], HostInputBuffer[i + pattern_listt.at(i).size()]);
//        tt = (uint8_t *)&HostOutputBuffer[i];
        //cout << "(" << HostOutputBuffer[i] << ") ";
//                     cout << "[" << tt[0] << tt[1] << "]" << endl;
//        if (HostOutputBuffer[i] <= 0 || HostOutputBuffer[i] > 2000) {cout << "GG" << endl;continue;}
            //cout << "(" << i<<":"<< HostOutputBuffer[i] << ") " << pattern_listt.at(HostOutputBuffer[i] - 1) << " ~ ";
        for (int j = 0; j < pattern_listt.at(HostOutputBuffer[i] - 1).size(); j ++) {
            //cout << fileContents[i + j];
            if (fileContents[i + j] != pattern_listt.at(HostOutputBuffer[i] - 1).at(j)) {
                error = 1;
                cout << "error";
                cout << "(" << i<<":"<< HostOutputBuffer[i] << ") " << pattern_listt.at(HostOutputBuffer[i] - 1) << " ~ ";
                break;
            }
        }
            if (error) {
                for (int j = 0; j < pattern_listt.at(HostOutputBuffer[i] - 1).size(); j ++) {
                    cout << fileContents[i + j];
                }
                cout << endl;
            } else {
                //cout << "(" << i<<":"<< HostOutputBuffer[i] << ") " << pattern_listt.at(HostOutputBuffer[i] - 1) << " ~ ";
                //for (int j = 0; j < pattern_listt.at(HostOutputBuffer[i] - 1).size(); j ++) {
                //    cout << fileContents[i + j];
                //}
                //cout << endl;
            }
        //cout << endl;

//        tt = (uint8_t *)&HostOutputBuffer[i];
//        cout << hex << (int)tt[0] <<" " << (int)tt[1]<<" " << (int)tt[2] <<" "<< (int)tt[3] <<" "<< endl;
//        tt = (uint8_t *)&HostInputBuffer[i];
//        cout << (int)tt[0]<<" " << (int)tt[1]<<" " << (int)tt[2]<<" " << (int)tt[3]<<" " << dec << endl;
        }
    }
    cout << "matched: " << match_count << endl;
    
    delete [] HostHashInfoBuffer;
    delete [] HostHashDataBuffer;
    delete [] HostPatMaskBuffer;
    delete [] HostInputBuffer;
    delete [] HostOutputBuffer;
    
    return 0;
}
