/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License")
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MEM_ARRAY_H
#define MEM_ARRAY_H

#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_array.h>
#include <nvhls_marshaller.h>
#include <TypeToBits.h>

// T: data type
// N: number of lines
template <typename T, int N>
class mem_array {
 public:
  T data[N];
  mem_array() {
    T value;
    for (unsigned i = 0; i < N; i++) {
      data[i] = value;
    }
  }
};

// T: data type
// N: number of lines
// A: associativity
template <typename T, int N, int A>
class mem_array_2d {
 public:
  T data[N / A][A];
};

template <typename T, int N, int A>
class mem_array_2d_transp {
 public:
  T data[A][N / A];
};

/**
 * \brief Abstract Memory Class 
 * \ingroup MemArray
 *
 * \tparam T                Datatype of an entry to be stored in memory 
 * \tparam NumEntries       Number of entries in memory 
 * \tparam NumBanks         Number of banks in memory
 *
 *
 * \par A Simple Example
 * \code
 *      #include <mem_array.h>
 *        ...
 *        mem_array_sep <MemWord_t, NUM_ENTRIES, NBANKS> banks;
 *      
 *        read_data = banks.read(bank_addr, bank_sel);
 *
 *        banks.write(bank_addr, bank_sel, write_data);
 *        ...
 *      
 *
 * \endcode
 * \par
 *
 */

template <typename T, int NumEntries, int NumBanks, int NumByteEnables=1>
class mem_array_sep {
 public:
  typedef Wrapped<T> WData_t;
  static const unsigned int NumEntriesPerBank = NumEntries/NumBanks;
  static const unsigned int WordWidth = WData_t::width;
  static const unsigned int SliceWidth = WordWidth/NumByteEnables;
  typedef NVUINTW(nvhls::index_width<NumEntriesPerBank>::val) LocalIndex;
  typedef NVUINTW(nvhls::index_width<NumBanks>::val) BankIndex;
  typedef sc_lv<WordWidth> Data_t;
  typedef sc_lv<SliceWidth> Slice_t;
  typedef NVUINTW(NumByteEnables) WriteMask;

  typedef Slice_t BankType[NumEntriesPerBank*NumByteEnables];
  nvhls::nv_array<BankType, NumBanks> bank;

  mem_array_sep() {
    Slice_t value;
    for (unsigned i = 0; i < NumBanks; i++) {
      for (unsigned j = 0; j < NumByteEnables* NumEntriesPerBank; j++) {
        bank[i][j] = value;
      }
    }
  }
 
  void clear() {
    Slice_t value = 0;
    for (unsigned i = 0; i < NumBanks; i++) {
      for (unsigned j = 0; j < NumByteEnables * NumEntriesPerBank; j++) {
        bank[i][j] = value;
      }
    }
  }

  T read(LocalIndex idx, BankIndex bank_sel=0) {
    Data_t read_data = 0;
    #pragma hls_unroll yes
    for (int i = 0; i < NumByteEnables; i++) {
      read_data.range((i+1)*SliceWidth-1, i*SliceWidth) = bank[bank_sel][idx * NumByteEnables + i];
    } 
    return BitsToType<T>(read_data);
  }

  void write(LocalIndex idx, BankIndex bank_sel, T val, WriteMask write_mask=~0, bool wce=1) {
    Slice_t tmp[NumByteEnables];
    Data_t write_data = TypeToBits<T>(val);
    #pragma hls_unroll yes
    for (int i = 0; i < NumByteEnables; i++) {
      tmp[i] = write_data.range((i+1)*SliceWidth-1, i*SliceWidth);
    }
    if (wce) {
      #pragma hls_unroll yes
      for (int i = 0; i < NumByteEnables; i++) {
        if (write_mask[i] == 1) {
          bank[bank_sel][idx* NumByteEnables +i] = tmp[i];
        }
      }
    }
  }
};


#endif