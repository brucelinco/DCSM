//
//  main.cpp
//  PFAC-Duo
//
//  Created by CbS Ghost on 2017/2/10.
//  Copyright (c) 2017 UrBX Creative Studio. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <vector>

#include "dpfac-internal.h"
#include "DualByteTree.h"
#include "MetaTable.h"
#include "Dispatcher.h"

using namespace std;

int main(int argc, const char * argv[]) {
    // insert code here...
    //std::cout << "Hello, World!\n";
    //DPFAC_Internal::Tree::MetaTable::MetaMode mode;
    //DPFAC_Internal::Tree::MetaTable tree(static_cast<DPFAC_Internal::Tree::MetaTable::MetaMode>(DPFAC_Internal::Tree::MetaTable::MetaMode::MODE_BOTH));
    //DPFAC_Internal::Tree::MetaTable tree(static_cast<DPFAC_Internal::Tree::MetaTable::MetaMode>(DPFAC_Internal::Tree::MetaTable::MetaMode::MODE_BOTH | DPFAC_Internal::Tree::MetaTable::MetaMode::MODE_NOID));
    if (argc < 3) {
        cout << "usage: " << argv[0] << "[pat_file] [ref_file]\a" << endl;
        return 0;
    }
    //std::ifstream patfile("cve_rule.txt");
    std::ifstream patfile(argv[1]);
    //if (patfile.is_open()) {
    //    std::cout << "pat opened\n";
    //}
    //std::ifstream inputfile("CVE-file.pcap");
    std::ifstream inputfile(argv[2]);
    //if (patfile.is_open()) {
    //    std::cout << "input opened\n";
    //}
    
    string line;
    int stream_lines = 0;
    
    // get lines of stream
    //patfile.clear();
    //patfile.seekg(0, ios::beg);
    //while (getline(patfile, line)) {
    //    if (!line.empty()) {
    //        stream_lines ++;
    //    }
    //}
    
    // read the pattern list
//    vector<vector<uint8_t>> pattern_list(stream_lines);
//    patfile.clear();
//    patfile.seekg(0, ios::beg);
//    for (int i = 0; i < stream_lines; i ++) {
//        getline(patfile, line);
//        if (!line.empty()) {
//            pattern_list[i] = vector<uint8_t>(line.begin(), line.end());
//        }
//    }
//    
//    patfile.clear();
//    patfile.seekg(0, ios::beg);
    
    //tree.fillTable(pattern_list);
    //DPFAC_Internal::DualByteTree tree1(patfile, static_cast<DPFAC_Internal::DualByteTree::MetaMode>(DPFAC_Internal::DualByteTree::MetaMode::MODE_BOTH));
    //vector<const std::vector<uint32_t> *> result = tree1.treeData();
    //for (int i = 0; i < result[0]->size(); i ++) {
    //    cout << result[0]->at(i) << " ";
    //}
    //cout << endl << "hhhhhhh" << endl;
    //while (1);
    //cl_int err;
    DPFAC_Internal::Dispatcher dispatcher(0);
    //DPFAC_Internal::Dispatcher dispatcher(CL_DEVICE_TYPE_GPU, 0, NULL);
    //if (err != CL_SUCCESS) {
    //    cout << "gg~~" << endl;
    //    return 0;
    //}
    dispatcher.compareToPFACFormat(patfile, inputfile);
    
    return 0;
}
