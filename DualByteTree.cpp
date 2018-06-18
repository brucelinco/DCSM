//
//  DualByteTree.cpp
//  PFAC-Duo
//
//  Created by CbS Ghost on 2017/2/10.
//  Copyright (c) 2017 UrBX Creative Studio. All rights reserved.
//

#include "DualByteTree.h"
using namespace std;
#ifdef DPFAC_API
using namespace DPFAC_Internal;
#endif

#pragma mark - DualByteTree Public Methods
DualByteTree::DualByteTree(const MetaMode mode)
{
    reset();
    setMode(mode);
}

DualByteTree::DualByteTree(istream &stream, const MetaMode mode)
{
    reset();
    setMode(mode);
    
    addPatterns(stream);
}

DualByteTree::DualByteTree(const vector<string> &pattern_list, const MetaMode mode)
{
    reset();
    setMode(mode);
    
    addPatterns(pattern_list);
}

DualByteTree::DualByteTree(const vector<vector<uint8_t>> &pattern_list, const MetaMode mode)
{
    reset();
    setMode(mode);
    
    addPatterns(pattern_list);
}

DualByteTree::~DualByteTree()
{
    // nothing ...
}

#pragma mark -
int32_t DualByteTree::addPatterns(istream &stream)
{
    string  line;
    int32_t pat_count = 0;
    
    while (getline(stream, line)) {
        if (!line.empty()) {
            db_pattern_max_size = max(db_pattern_max_size, (int32_t)line.size());
            db_pattern_list.push_back(vector<uint8_t>(line.begin(), line.end()));
            pat_count += 1;
        }
    }
    
    flag_need_update = pat_count ? true : false;
    return pat_count;
}

int32_t DualByteTree::addPatterns(const string &pattern)
{
    vector<uint8_t> flat_pat = vector<uint8_t>(pattern.begin(), pattern.end());
    db_pattern_max_size = max(db_pattern_max_size, (int32_t)pattern.size());
    db_pattern_list.push_back(flat_pat);
    
    flag_need_update = true;
    return 1;
}

int32_t DualByteTree::addPatterns(const vector<uint8_t> &pattern)
{
    db_pattern_max_size = max(db_pattern_max_size, (int32_t)pattern.size());
    db_pattern_list.push_back(pattern);
    
    flag_need_update = true;
    return 1;
}

int32_t DualByteTree::addPatterns(const vector<string> &pattern_list)
{
    vector<uint8_t> flat_pat;
    int32_t cur_pat_count = (int32_t)db_pattern_list.size();
    db_pattern_list.resize(cur_pat_count + pattern_list.size());
    
    for (int32_t i = 0; i < (int32_t)pattern_list.size(); i ++) {
        flat_pat = vector<uint8_t>(pattern_list[i].begin(), pattern_list[i].end());
        db_pattern_max_size = max(db_pattern_max_size, (int32_t)flat_pat.size());
        db_pattern_list[cur_pat_count + i] = flat_pat;
    }
    
    flag_need_update = pattern_list.size() ? true : false;
    return (int32_t)pattern_list.size();
}

int32_t DualByteTree::addPatterns(const vector<vector<uint8_t>> &pattern_list)
{
    for (int32_t i = 0; i < (int32_t)pattern_list.size(); i ++) {
        db_pattern_max_size = max(db_pattern_max_size, (int32_t)pattern_list[i].size());
    }
    db_pattern_list.insert(db_pattern_list.end(), pattern_list.begin(), pattern_list.end());
    
    flag_need_update = pattern_list.size() ? true : false;
    return (int32_t)pattern_list.size();
}

#pragma mark -
vector<const vector<int32_t> *> DualByteTree::treeInfo()
{
    if (flag_need_update) {
        buildTree();
        flag_need_update = false;
    }
    
    vector<const vector<int32_t> *> info;
    for (int32_t i = 0; i < (int32_t)db_hash_table_list.size(); i ++) {
        info.push_back(&db_hash_table_list[i].info());
    }
    
    return info;
}

vector<const vector<uint32_t> *> DualByteTree::treeData()
{
    if (flag_need_update) {
        buildTree();
        flag_need_update = false;
    }
    
    vector<const vector<uint32_t> *> data;
    for (int32_t i = 0; i < (int32_t)db_hash_table_list.size(); i ++) {
        data.push_back((const vector<uint32_t> *)(&db_hash_table_list[i].data()));
    }
    
    return data;
}

vector<const vector<uint16_t> *> DualByteTree::patternMask()
{
    if (flag_need_update) {
        buildTree();
        flag_need_update = false;
    }
    
    vector<const vector<uint16_t> *> mask;
    for (int32_t i = 0; i < (int32_t)db_meta_table_list.size(); i ++) {
        mask.push_back(&db_meta_table_list[i].patternMask());
    }
    
    return mask;
}

const int32_t DualByteTree::maxPatternSize() const
{
    return db_pattern_max_size;
}

const int32_t DualByteTree::maxPatternBlockSize() const
{
    return (db_pattern_max_size / 2) + (db_pattern_max_size % 2);
}

#pragma mark - DualByteTree Protected Methods
void DualByteTree::reset()
{
    db_hash_table_list.clear();
    db_meta_table_list.clear();
    db_pattern_list.clear();
    
    db_pattern_max_size = 0;
    
    flag_seperate_table = false;
    flag_need_update = false;
}

int32_t DualByteTree::buildTree()
{
    int32_t ret;
    db_hash_table_list.clear();
    db_meta_table_list.clear();
    vector<Tree::MetaTable::MetaRow *> m_table;
    MetaMode m_mode = static_cast<MetaMode>(0);
    
    if (flag_seperate_table) {
        db_meta_table_list.push_back(static_cast<MetaMode>(m_mode | MetaMode::MODE_FIRST));
        db_meta_table_list.push_back(static_cast<MetaMode>(m_mode | MetaMode::MODE_SECOND));
        db_hash_table_list.resize(2);
        ret = db_meta_table_list[0].fillTable(db_pattern_list);
        if (ret < 0) {
            return ret;
        }
        ret = db_meta_table_list[1].fillTable(db_pattern_list);
        if (ret < 0) {
            return ret;
        }
        m_table = db_meta_table_list[0].table();
        ret = db_hash_table_list[0].build(m_table);
        if (ret < 0) {
            return ret;
        }
        m_table = db_meta_table_list[1].table();
        ret = db_hash_table_list[1].build(m_table);
        if (ret < 0) {
            return ret;
        }
    } else {
        db_meta_table_list.push_back(static_cast<MetaMode>(m_mode | MetaMode::MODE_BOTH));
        db_hash_table_list.resize(1);
        ret = db_meta_table_list[0].fillTable(db_pattern_list);
        if (ret < 0) {
            return ret;
        }
        m_table = db_meta_table_list[0].table();
        ret = db_hash_table_list[0].build(m_table);
        if (ret < 0) {
            return ret;
        }
    }

    //////// debug /////////
//    int rett;
//    for (int i = 0; i < (int)db_pattern_list.size(); i ++) {
//        rett = db_hash_table_list[0].patternLookUp(db_pattern_list[i], (int32_t)db_pattern_list.size(), 0);
//        if (rett <= (int)db_pattern_list.size()) {
//            if (rett < 1) {
//                cout << "gg:" << rett << db_pattern_list[i].data() << endl;
//            } else {
//                cout << "found(" << rett << ")"<< db_pattern_list[i].data() << endl;
//            }
//        }
//        rett = db_hash_table_list[0].patternLookUp(db_pattern_list[i], (int32_t)db_pattern_list.size(), 1);
//        if (rett <= (int)db_pattern_list.size()) {
//            if (rett < 1) {
//                cout << "gg:" <<  rett << db_pattern_list[i].data() << endl;
//            } else {
//                cout << "found(" << rett << ")"<< db_pattern_list[i].data() << endl;
//            }
//        }
//    }
    
    return (int32_t)db_pattern_list.size();
}

#pragma mark - DualByteTree Private Methods
void DualByteTree::setMode(const MetaMode mode)
{
    flag_seperate_table = (mode & MetaMode::MODE_EXTEND) ? true : false;
    
    flag_need_update = false;
}
