//
//  HashTable.cpp
//  PFAC-Duo
//
//  Created by CbS Ghost on 2017/2/10.
//  Copyright (c) 2017 UrBX Creative Studio. All rights reserved.
//

#include "HashTable.h"
using namespace std;
#ifdef DPFAC_API
using namespace DPFAC_Internal::Tree;
#endif

#pragma mark - HashTable Public Methods
HashTable::HashTable(const int32_t reserve)
{
    if (reserve) {
        hash_data.reserve(reserve);
    }
}

HashTable::HashTable(vector<MetaTable::MetaRow *> &table, const int32_t reserve)
{
    if (reserve) {
        hash_data.reserve(reserve);
    }
    build(table);
}

HashTable::HashTable(const std::vector<uint32_t> &info, const vector<HashCell> &data)
{
    setTable(info, data);
}

HashTable::~HashTable()
{
    // nothing ...
}

#pragma mark -
int32_t HashTable::build(vector<MetaTable::MetaRow *> &table, const bool sort_table)
{
    reset();
    hash_info.resize(table.size());
    
    // sort the table
    if (sort_table) {
        MetaTable::sortRowsByCellCounts(table);
    }
    
    // sort the key1 values in each element
    for (int32_t i = 0; i < table.size(); i ++) {
        MetaTable::sortCellsByKey(table[i]->cell);
    }
    
    // start building the hash table
    int32_t m_pos;
    int32_t m_pos_base;
    int32_t m_state;
    int32_t m_next_state;
    vector<MetaTable::MetaCell> *m_cell_list;
    for (int32_t i = 0; i < (int32_t)table.size(); i ++) {
        m_cell_list = &table[i]->cell;
        m_pos_base = findFillableCellPos(*m_cell_list);
        m_state = table[i]->state;
        try {
            hash_info.at(m_state) = m_pos_base;
        } catch (const out_of_range&) {
            reset();
            return DPFAC::FAILED_OUT_OF_RANGE;
        }
        
        for (int32_t j = 0; j < (int32_t)m_cell_list->size(); j ++) {
            // check the range of next state
            m_next_state = m_cell_list->at(j).next_state;
            if (m_next_state > (int32_t)numeric_limits<uint16_t>::max()) {
                reset();
                return DPFAC::FAILED_OUT_OF_RANGE;
            }
            
            // fill the cell
            m_pos = (int32_t)m_cell_list->at(j).key.key + m_pos_base;
            if (m_pos >= hash_data.size()) {
                vector<HashCell> tmp_null_data(m_pos - hash_data.size() + 1, HashCell{0xFFFF, 0xFFFF});
                hash_data.insert(hash_data.end(), tmp_null_data.begin(), tmp_null_data.end());
            }
            if (hash_data[m_pos] != HashCell{0xFFFF, 0xFFFF}) {
                reset();
                return DPFAC::FAILED_GENERAL;
            }
            hash_data[m_pos].state = (uint16_t)m_state;
            hash_data[m_pos].next_state = (uint16_t)m_next_state;
        }
    }
    
    return (int32_t)hash_data.size();
}

int32_t HashTable::setTable(const std::vector<uint32_t> &info, const std::vector<HashCell> &data)
{
    reset();
    hash_info.insert(hash_info.end(), info.begin(), info.end());
    hash_data.insert(hash_data.end(), data.begin(), data.end());
    
    return (int32_t)hash_data.size();
}

int32_t HashTable::cellLookUp(const MetaTable::MetaKey &key, const uint16_t state) const
{
    return cellLookUp(key.key, state);
}

int32_t HashTable::cellLookUp(const uint16_t key, const uint16_t state) const
{
    HashCell cell;
    try {
        cell = hash_data.at(hash_info.at(state) + key);
    } catch (const out_of_range&) {
        return DPFAC::FAILED_NOT_FOUND;
    }

    if (cell == HashCell{0xFFFF, 0xFFFF} || cell.state != state) {
        return DPFAC::FAILED_NOT_FOUND;
    }
    return (int32_t)cell.next_state;
}

int32_t HashTable::patternLookUp(const vector<uint8_t> &pattern, const int32_t pattern_count, const int32_t offset) const
{
    if (pattern_count <= 0) {
        return DPFAC::FAILED_GENERAL;
    }
    
    int32_t pattern_size = (int32_t)pattern.size();
    vector<uint8_t> h_pat = pattern;
    h_pat.insert(h_pat.end(), {0, 0});
    if (offset != 0) {
        h_pat.insert(h_pat.begin(), {0});
        pattern_size += 1;
    }

    MetaTable::MetaKey h_key;
    int32_t h_state = 0;
    int32_t h_next_state = 0;
    for (int32_t i = 0; i < pattern_size; i += 2) {
        h_key.byte[0] = h_pat[i];
        h_key.byte[1] = h_pat[i + 1];
        h_next_state = cellLookUp(h_key.key, h_state);
        if (h_next_state <= pattern_count) {
            return h_next_state;
        }
        h_state = h_next_state;
    }
    
    return DPFAC::FAILED_UNKNOWN;
}

int32_t HashTable::patternLookUp(const string &pattern, const int32_t pattern_count, const int32_t offset) const
{
    return patternLookUp(vector<uint8_t>(pattern.begin(), pattern.end()), pattern_count, offset);
}

#pragma mark -
const std::vector<int32_t> &HashTable::info() const
{
    return hash_info;
}

const std::vector<HashTable::HashCell> &HashTable::data() const
{
    return hash_data;
}

#pragma mark - HashTable Protected Methods
void HashTable::reset()
{
    hash_info.clear();
    hash_data.clear();
}

int32_t HashTable::findFillableCellPos(const vector<MetaTable::MetaCell> &row) const
{
    if (row.size() == 0) {
        return 0;
    }
    
    int32_t pos = 0;
    bool invalid_flag = false;
    int32_t cell_pos;
    int32_t m_offset = (int32_t)row[0].key.key;
    for (int32_t &i = pos; i < (int32_t)numeric_limits<int32_t>::max(); i ++) {
        invalid_flag = false;
        for (int32_t j = 0; j < row.size(); j ++) {
            cell_pos = row[j].key.key + i - m_offset;
            if (cell_pos < hash_data.size() && hash_data[cell_pos] != HashCell{0xFFFF, 0xFFFF}) {
                invalid_flag = true;
                break;
            }
        }
        if (!invalid_flag) {
            break;
        }
    }
    
    return pos - m_offset;
}

#pragma mark -
bool DPFAC_Internal::Tree::operator==(const HashTable::HashCell &cell_a, const HashTable::HashCell &cell_b) {
    return ((cell_a.state == cell_b.state) && (cell_a.next_state == cell_b.next_state));
}

bool DPFAC_Internal::Tree::operator!=(const HashTable::HashCell &cell_a, const HashTable::HashCell &cell_b) {
    return ((cell_a.state != cell_b.state) || (cell_a.next_state != cell_b.next_state));
}
