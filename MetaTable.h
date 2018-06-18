//
//  DPFAC_MetaTable.h
//  PFAC-Duo
//
//  Created by CbS Ghost on 2017/2/10.
//  Copyright (c) 2017 UrBX Creative Studio. All rights reserved.
//

#ifndef __MetaTable__
#define __MetaTable__

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>
#include "dpfac-internal.h"

#ifdef DPFAC_API
namespace DPFAC_Internal {
#endif
    namespace Tree {
        class MetaTable {
        public:
            enum MetaMode : uint8_t {
                MODE_FIRST  = 0x01,
                MODE_SECOND = 0x02,
                MODE_BOTH   = 0x03,
                
                MODE_EXTEND = 0x40,
            };
            
            typedef union _MetaKey {
                uint16_t key;
                uint8_t  byte[2];
            } MetaKey;
            
            typedef struct _MetaCell {
                MetaKey  key;
                uint32_t next_state;
            } MetaCell;
            
            typedef struct _MetaRow {
                uint32_t state;
                std::vector<MetaCell> cell;
            } MetaRow;
            
            MetaTable(const MetaMode mode = MetaMode::MODE_BOTH);
            MetaTable(const std::vector<std::vector<uint8_t>> &pattern_list, const MetaMode mode = MetaMode::MODE_BOTH);
            virtual ~MetaTable();
            
            int64_t fillTable(const std::vector<std::vector<uint8_t>> &pattern_list);
            int32_t patternLookUp(const std::vector<uint8_t> &pattern, const int32_t offset = 0) const;
            
            std::vector<MetaRow *> table() const;
            const std::vector<uint16_t> &patternMask() const;
            
            static void sortRowsByCellCounts(std::vector<MetaRow *> &table);
            static void sortCellsByKey(std::vector<MetaCell> &cell);
            
        protected:
            void reset();
            int64_t addPattern(const std::vector<uint8_t> &pattern, const uint16_t pattern_id, const int32_t offset = 0);
            int64_t findKeyInState(const MetaKey &key, const uint32_t state) const;
            
        private:
            typedef struct _MetaSortingCell {
                uint32_t id;
                const std::vector<uint8_t> *str;
            } MetaSortingCell;
            
            std::vector<MetaRow> meta_table;
            std::vector<uint16_t> pattern_mask_list;
            MetaMode meta_mode;
            uint32_t pattern_list_size;
            uint32_t state_count;
            
            static bool cmpMetaHasMoreCells(const MetaRow *row_a, const MetaRow *row_b);
            static bool cmpMetaHasSmallerKey(const MetaCell &cell_a, const MetaCell &cell_b);
            static bool cmpMetaHasShorterLength(const MetaSortingCell &scell_a, const MetaSortingCell &scell_b);
        };
    }
#ifdef DPFAC_API
}
#endif

#endif /* defined(__MetaTable__) */
