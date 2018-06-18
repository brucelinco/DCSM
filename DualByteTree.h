//
//  DualByteTree.h
//  PFAC-Duo
//
//  Created by CbS Ghost on 2017/2/10.
//  Copyright (c) 2017 UrBX Creative Studio. All rights reserved.
//

#ifndef __DualByteTree__
#define __DualByteTree__

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include "dpfac-internal.h"
#include "HashTable.h"
#include "MetaTable.h"

#ifdef DPFAC_API
namespace DPFAC_Internal {
#endif
    class DualByteTree
    {
    public:
        typedef Tree::MetaTable::MetaMode MetaMode;
        
        DualByteTree(const MetaMode mode = MetaMode::MODE_BOTH);
        DualByteTree(std::istream &stream, const MetaMode mode = MetaMode::MODE_BOTH);
        DualByteTree(const std::vector<std::string> &pattern_list, const MetaMode mode = MetaMode::MODE_BOTH);
        DualByteTree(const std::vector<std::vector<uint8_t>> &pattern_list, const MetaMode mode = MetaMode::MODE_BOTH);
        virtual ~DualByteTree();
        
        int32_t addPatterns(std::istream &stream);
        int32_t addPatterns(const std::string &pattern);
        int32_t addPatterns(const std::vector<uint8_t> &pattern);
        int32_t addPatterns(const std::vector<std::string> &pattern_list);
        int32_t addPatterns(const std::vector<std::vector<uint8_t>> &pattern_list);
        
        std::vector<const std::vector<int32_t> *>  treeInfo();
        std::vector<const std::vector<uint32_t> *> treeData();
        std::vector<const std::vector<uint16_t> *> patternMask();
        const int32_t maxPatternSize() const;
        const int32_t maxPatternBlockSize() const;
        
    protected:
        void reset();
        int32_t buildTree();
        
    private:
        std::vector<std::vector<uint8_t>> db_pattern_list;
        std::vector<Tree::HashTable> db_hash_table_list;
        std::vector<Tree::MetaTable> db_meta_table_list;
        int32_t db_pattern_max_size;
        bool flag_seperate_table;
        bool flag_need_update;
        
        void setMode(const MetaMode mode = MetaMode::MODE_BOTH);
    };
#ifdef DPFAC_API
}
#endif

#endif /* defined(__DualByteTree__) */
