/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef __AXI_T_MASTER_FROM_FILE__
#define __AXI_T_MASTER_FROM_FILE__

#include <systemc.h>
#include <ac_reset_signal_is.h>

#include <axi/axi4.h>
#include <axi/testbench/CSVFileReader.h>
#include <nvhls_connections.h>
#include <hls_globals.h>

#include <queue>
#include <string>
#include <sstream>
#include <vector>
#include <math.h>
#include <boost/assert.hpp>

/**
 * \brief An AXI master that generates traffic according to a file for use in testbenches.
 * \ingroup AXI
 *
 * \tparam axiCfg                   A valid AXI config.
 *
 * \par Overview
 * AxiMasterFromFile reads write and read requests from a CSV and issues them as an AXI master.  Read responses are checked agains the expected values provided in the file.  The format of the CSV is as follows:
 * - Writes: delay_from_previous_request,W,address_in_hex,data_in_hex
 * - Reads: delay_from_previous_request,R,address_in_hex,expected_response_data_in_hex
 * 
 *  For reads, it's best to specify the full DATA_WIDTH of expected response data.
 *
 */
template <typename axiCfg> class MasterFromFile : public sc_module {
 public:
  static const int kDebugLevel = 0;
  typedef axi::axi4<axiCfg> axi4_;

  typename axi4_::read::template master<> if_rd;
  typename axi4_::write::template master<> if_wr;

  sc_in<bool> reset_bar;
  sc_in<bool> clk;

  static const int bytesPerBeat = axi4_::DATA_WIDTH >> 3;
  static const int bytesPerWord = axi4_::DATA_WIDTH >> 3;
  static const int axiAddrBitsPerWord = nvhls::log2_ceil<bytesPerWord>::val;

  std::queue< int > delay_q;
  std::queue< bool > isWrite_q;
  std::queue< typename axi4_::AddrPayload > raddr_q;
  std::queue< typename axi4_::AddrPayload > waddr_q;
  std::queue< typename axi4_::WritePayload > wdata_q;
  /* std::queue< sc_uint<axi4_::DATA_WIDTH> > rresp_q; */
  std::queue<typename axi4_::Data> rresp_q;
    
  typename axi4_::AddrPayload addr_pld;
  typename axi4_::WritePayload wr_data_pld;
  typename axi4_::ReadPayload data_pld;
  typename axi4_::AddrPayload wr_addr_pld;
  typename axi4_::WRespPayload wr_resp_pld;

  sc_out<bool> done;

  SC_HAS_PROCESS(MasterFromFile);

  MasterFromFile(sc_module_name name_, std::string filename="requests.csv")
      : sc_module(name_), if_rd("if_rd"), if_wr("if_wr"), reset_bar("reset_bar"), clk("clk") {

    CDCOUT("Reading file: " << filename << endl, kDebugLevel);
    CSVFileReader reader(filename);
    std::vector< std::vector<std::string> > dataList = reader.readCSV();
    for (unsigned int i=0; i < dataList.size(); i++) {
      std::vector<std::string> vec = dataList[i];
      NVHLS_ASSERT_MSG(vec.size() == 4, "Each_request_must_have_four_elements");
      delay_q.push(atoi(vec[0].c_str()));
      if (vec[1] == "R") {
        isWrite_q.push(0);
        std::stringstream ss;
        sc_uint<axi4_::ADDR_WIDTH> addr;
        ss << hex << vec[2];
        ss >> addr;
        addr_pld.addr = static_cast<typename axi4_::Addr>(addr);
        addr_pld.len = 0;
        raddr_q.push(addr_pld);
        //CDCOUT(sc_time_stamp() << " " << name() << " Stored read request:"
        //              << " addr=" << hex << addr_pld.addr.to_uint64()
        //              << endl, kDebugLevel);
        std::stringstream ss_data;
        sc_uint<axi4_::DATA_WIDTH> data;
        ss_data << hex << vec[3];
        ss_data >> data;
        rresp_q.push(static_cast<typename axi4_::Data>(data));
      } else if (vec[1] == "W") {
        isWrite_q.push(1);
        std::stringstream ss;
        sc_uint<axi4_::ADDR_WIDTH> addr;
        ss << hex << vec[2];
        ss >> addr;
        addr_pld.addr = static_cast<typename axi4_::Addr>(addr);
        addr_pld.len = 0;
        waddr_q.push(addr_pld);
        std::stringstream ss_data;
        sc_uint<axi4_::DATA_WIDTH> data;
        ss_data << hex << vec[3];
        ss_data >> data;
        wr_data_pld.data = static_cast<typename axi4_::Data>(data);
        wr_data_pld.wstrb = 0xFF;
        wr_data_pld.last = 1;
        wdata_q.push(wr_data_pld);
        //CDCOUT(sc_time_stamp() << " " << name() << " Stored write request:"
        //              << " addr=" << hex << addr_pld.addr.to_uint64()
        //              << " data=" << hex << wr_data_pld.data.to_uint64()
        //              << endl, kDebugLevel);
      } else {
        NVHLS_ASSERT_MSG(1,"Requests_must_be_R_or_W");
      }
    }

    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(reset_bar, false);
  }

 protected:
  void run() {

    done = 0;

    if_rd.reset();
    if_wr.reset();

    wait(20);

    while (!delay_q.empty()) {
      int delay = delay_q.front();
      if (delay > 0) wait(delay);
      delay_q.pop();
      if (isWrite_q.front()) {
        addr_pld = waddr_q.front();
        if_wr.aw.Push(addr_pld);
        waddr_q.pop();
        wr_data_pld = wdata_q.front();
        if_wr.w.Push(wr_data_pld);
        wdata_q.pop();
        if_wr.b.Pop();
        CDCOUT(sc_time_stamp() << " " << name() << " Sent write request:"
                      << " addr=" << hex << addr_pld.addr.to_uint64()
                      << " data=" << hex << wr_data_pld.data.to_uint64()
                      << endl, kDebugLevel);
      } else {
        addr_pld = raddr_q.front();
        if_rd.ar.Push(addr_pld);
        raddr_q.pop();
        CDCOUT(sc_time_stamp() << " " << name() << " Sent read request:"
                      << " addr=" << hex << addr_pld.addr.to_uint64()
                      << endl, kDebugLevel);
        data_pld = if_rd.r.Pop();
        CDCOUT(sc_time_stamp() << " " << name() << " Received read response:"
                      << " data=" << hex << data_pld.data.to_uint64()
                      << endl, kDebugLevel);
        NVHLS_ASSERT_MSG(data_pld.data == rresp_q.front(),"Read_response_did_not_match_expected_value");
        rresp_q.pop();
      }
      isWrite_q.pop();
    }
    done = 1;
  }
};

#endif
