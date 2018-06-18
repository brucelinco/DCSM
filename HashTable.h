//
//  HashTable.h
//  PFAC-Duo
//
//  Created by CbS Ghost on 2017/2/10.
//  Copyright (c) 2017 UrBX Creative Studio. All rights reserved.
//

#ifndef __HashTable__
#define __HashTable__

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include "dpfac-internal.h"
#include "MetaTable.h"

#ifdef DPFAC_API
namespace DPFAC_Internal {
#endif
    namespace Tree {
        class HashTable
        {
        public:
            typedef struct _HashCell {
                uint16_t state;
                uint16_t next_state;
            } HashCell __attribute__ ((aligned (sizeof(int32_t))));
            
            HashTable(const int32_t reserve = 0);
            HashTable(std::vector<MetaTable::MetaRow *> &table, const int32_t reserve = 0);
            HashTable(const std::vector<uint32_t> &info, const std::vector<HashCell> &data);
            virtual ~HashTable();
            
            int32_t build(std::vector<MetaTable::MetaRow *> &table, const bool sort_table = true);
            int32_t setTable(const std::vector<uint32_t> &info, const std::vector<HashCell> &data);
            int32_t cellLookUp(const MetaTable::MetaKey &key, const uint16_t state) const;
            int32_t cellLookUp(const uint16_t key, const uint16_t state) const;
            int32_t patternLookUp(const std::vector<uint8_t> &pattern, const int32_t pattern_count, const int32_t offset = 0) const;
            int32_t patternLookUp(const std::string &pattern, const int32_t pattern_count, const int32_t offset = 0) const;
            
            const std::vector<int32_t>  &info() const;
            const std::vector<HashCell> &data() const;
            
        protected:
            void reset();
            int32_t findFillableCellPos(const std::vector<MetaTable::MetaCell> &row) const;
            
            friend bool operator==(const HashCell &cell_a, const HashCell &cell_b);
            friend bool operator!=(const HashCell &cell_a, const HashCell &cell_b);
            
        private:
            std::vector<int32_t>  hash_info;
            std::vector<HashCell> hash_data;
        };
    }
#ifdef DPFAC_API
}
#endif

#endif /* defined(__HashTable__) */
