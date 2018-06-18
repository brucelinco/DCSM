	//
//  MetaTable.cpp
//  PFAC-Duo
//
//  Created by CbS Ghost on 2017/2/10.
//  Copyright (c) 2017 UrBX Creative Studio. All rights reserved.
//

#include "MetaTable.h"
using namespace std;
#ifdef DPFAC_API
using namespace DPFAC_Internal::Tree;
#endif

#pragma mark - MetaTable Public Methods
MetaTable::MetaTable(const MetaMode mode)
{
    meta_mode = mode;
}

MetaTable::MetaTable(const vector<vector<uint8_t>> &pattern_list, const MetaMode mode)
{
    meta_mode = mode;
    
    fillTable(pattern_list);
}

MetaTable::~MetaTable()
{
    // nothing...
}

#pragma mark -
int64_t MetaTable::fillTable(const vector<vector<uint8_t>> &pattern_list)
{
    if (pattern_list.size() <= 0 || pattern_list.size() > numeric_limits<int16_t>::max()) {
        return DPFAC::FAILED_OUT_OF_RANGE;
    }
    
    // generate pattern mask list and fill the id of pattern list
    reset();
    pattern_list_size = (uint32_t)pattern_list.size() + 1;
    state_count = pattern_list_size;
    pattern_mask_list.resize(pattern_list_size);
    pattern_mask_list[0] = 0x0000;
    vector<MetaSortingCell> pat_sorted_list(pattern_list.size());
    for (int64_t i = 0; i < (int64_t)pattern_list.size(); i ++) {
        pattern_mask_list[i + 1] = pattern_list[i].size() % 2 ? 0xFF00 : 0xFFFF;
        if (meta_mode & MetaMode::MODE_SECOND) {
            pattern_mask_list[i + 1] ^= 0x00FF;
        }
        pat_sorted_list[i].id = (uint32_t)i + 1;
        pat_sorted_list[i].str = &pattern_list[i];
    }
    
    // sort ids of pattern list
    sort(pat_sorted_list.begin(), pat_sorted_list.end(), cmpMetaHasShorterLength);
    
    // build the semi DFA table
    int32_t ret_add_pat;
    for (int64_t i = 0; i < (int64_t)pat_sorted_list.size(); i ++) {
        if (meta_mode & MetaMode::MODE_FIRST) {
            ret_add_pat = addPattern(*pat_sorted_list[i].str, pat_sorted_list[i].id, 0);
            if (ret_add_pat < 0) {
                reset();
                return ret_add_pat;
            }
        }
        if (meta_mode & MetaMode::MODE_SECOND) {
            ret_add_pat = addPattern(*pat_sorted_list[i].str, pat_sorted_list[i].id, 1);
            if (ret_add_pat < 0) {
                reset();
                return ret_add_pat;
            }
        }
    }

    ////debug///////
    //for (int i = 0; i < (uint16_t)pattern_list.size(); i ++) {
    //    cout << string(pattern_list[i].begin(), pattern_list[i].end()) << ":";
    //    cout << patternLookUp(pattern_list[i], 0) << ",";
    //    cout << patternLookUp(pattern_list[i], 1) << endl;
    //}
    int summ = 0;
    for (int i = 0; i < meta_table.size(); i ++) {
        summ += meta_table[i].cell.size();
    }
    
    return state_count;
}

int32_t MetaTable::patternLookUp(const vector<uint8_t> &pattern, const int32_t offset) const
{
    int32_t  pattern_size = (int32_t)pattern.size();
    MetaKey  m_key;
    uint32_t m_state = 0;
    int64_t  m_next_state;
    
    // set up pattern
    vector<uint8_t> m_pat(pattern.begin(), pattern.end());
    m_pat.insert(m_pat.end(), {0, 0});
    if (offset != 0) {
        m_pat.insert(m_pat.begin(), {0});
        pattern_size += 1;
    }
    
    for (int32_t i = 0; i < pattern_size; i += 2) {
        m_key.byte[0] = m_pat[i];
        m_key.byte[1] = m_pat[i + 1];
        m_next_state = findKeyInState(m_key, m_state);
        if (m_next_state < pattern_list_size) {
            return m_next_state;
        }
        m_state = (uint32_t)m_next_state;
    }
    
    return DPFAC::FAILED_UNKNOWN;
}

#pragma mark -
vector<MetaTable::MetaRow *> MetaTable::table() const
{
    vector<MetaRow *> m_table(meta_table.size());
    for (int32_t i = 0; i < (int32_t)meta_table.size(); i ++) {
        m_table[i] = (MetaRow *)&meta_table[i];
    }
    
    return m_table;
}

const vector<uint16_t> &MetaTable::patternMask() const
{
    return (const vector<uint16_t> &)pattern_mask_list;
}

#pragma mark -
void MetaTable::sortRowsByCellCounts(vector<MetaRow *> &table)
{
    sort(table.begin(), table.end(), cmpMetaHasMoreCells);
}

void MetaTable::sortCellsByKey(vector<MetaCell> &cell)
{
    sort(cell.begin(), cell.end(), cmpMetaHasSmallerKey);
}

#pragma mark - MetaTable Protected Methods
void MetaTable::reset()
{
    meta_table.clear();
    pattern_mask_list.clear();
}

int64_t MetaTable::addPattern(const vector<uint8_t> &pattern, const uint16_t pattern_id, const int32_t offset)
{
    MetaCell m_cell;
    int64_t  m_state = 0;
    int64_t  m_next_state;
    int64_t  pattern_size = (int64_t)pattern.size();
    
    // set up pattern
    vector<uint8_t> m_pat(pattern.begin(), pattern.end());
    m_pat.insert(m_pat.end(), {0, 0});
    if (offset != 0) {
        m_pat.insert(m_pat.begin(), {0});
        pattern_size += 1;
    }
    
    for (int64_t i = 0; i < pattern_size; i += 2) {
        // check null end
        m_cell.key.byte[0] = m_pat[i];
        m_cell.key.byte[1] = 0;
        m_next_state = findKeyInState(m_cell.key, m_state);
        if (m_next_state >= 0) {
            return m_next_state;
        }
        
        m_cell.key.byte[1] = m_pat[i + 1];
        m_next_state = findKeyInState(m_cell.key, m_state);
        if (m_next_state < 0) {
            // assign the cell to table
            if ((i + 2) >= pattern_size) {
                m_cell.next_state = pattern_id;
            } else {
                m_cell.next_state = state_count;
            }
            
            // init new rows in table and push back the cell
            for (int64_t j = (int64_t)meta_table.size(); j <= m_state; j ++) {
                meta_table.push_back(MetaRow{(uint32_t)j, vector<MetaCell>()});
            }
            meta_table[m_state].cell.push_back(m_cell);
            m_state = state_count;
            state_count += 1;
            if (state_count > (int64_t)numeric_limits<uint32_t>::max() - 2) {
                reset();
                return DPFAC::FAILED_OUT_OF_RANGE;
            }
            
        } else if (m_next_state < pattern_list_size) {
            return m_next_state;
        } else {
            m_state = m_next_state;
        }
    }
    
    return m_state;
}

int64_t MetaTable::findKeyInState(const MetaKey &key, const uint32_t state) const
{
    if (state >= meta_table.size()) {
        return DPFAC::FAILED_NOT_FOUND;
    }
    
    const vector<MetaCell> &m_row = meta_table[state].cell;
    for (int64_t i = 0; i < (int64_t)m_row.size(); i ++) {
        if (m_row[i].key.key == key.key) {
            return (int64_t)m_row[i].next_state;
        }
    }
    
    return DPFAC::FAILED_NOT_FOUND;
}

#pragma mark - MetaTable Private Methods
bool MetaTable::cmpMetaHasMoreCells(const MetaRow *row_a, const MetaRow *row_b)
{
    return row_a->cell.size() > row_b->cell.size();
}

bool MetaTable::cmpMetaHasSmallerKey(const MetaCell &cell_a, const MetaCell &cell_b)
{
    return cell_a.key.key < cell_b.key.key;
}

bool MetaTable::cmpMetaHasShorterLength(const MetaSortingCell &scell_a, const MetaSortingCell &scell_b)
{
    return scell_a.str->size() < scell_b.str->size();
}
