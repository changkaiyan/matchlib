# Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
# 
# Licensed under the Apache License, Version 2.0 (the "License")
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#global setup file that runs to right before the go analyze step

solution new -state initial
solution options defaults
flow package require /SCVerify
solution options set /Output/PackageOutput false
#
## Use fsdb file for power flow - make sure your environment var $NOVAS_INST_DIR has been set before your launch Catapult.
solution options set /Flows/LowPower/SWITCHING_ACTIVITY_TYPE fsdb
## SCVerify settings
solution options set /Flows/SCVerify/USE_MSIM false
solution options set /Flows/SCVerify/USE_OSCI false
solution options set /Flows/SCVerify/USE_VCS true
solution options set /Flows/VCS/VCS_HOME /home/tools/vcs/mx-2015.09-SP2
solution options set /Flows/VCS/VG_ENV64_SCRIPT source_me.csh

options set Input/CompilerFlags {-D_SYNTHESIS_ -DHLS_CATAPULT=1}
options set Input/SearchPath ". ../../cmod ../../cmod/include"

# Verilog/VHDL
solution options set Output OutputVerilog true
solution options set Output/OutputVHDL false
# Reset FFs
solution options set Architectural/DefaultResetClearsAllRegs true

# General constrains. Please refer to tool ref manual for detailed descriptions.
directive set -DESIGN_GOAL area
directive set -OLD_SCHED false
directive set -SPECULATE true
directive set -MERGEABLE true
directive set -REGISTER_THRESHOLD 256
directive set -MEM_MAP_THRESHOLD 32
directive set -FSM_ENCODING none
directive set -REG_MAX_FANOUT 0
directive set -NO_X_ASSIGNMENTS true
directive set -SAFE_FSM false
directive set -REGISTER_SHARING_LIMIT 0
directive set -ASSIGN_OVERHEAD 0
directive set -TIMING_CHECKS true
directive set -MUXPATH true
directive set -REALLOC true
directive set -UNROLL no
directive set -IO_MODE super
directive set -REGISTER_IDLE_SIGNAL false
directive set -IDLE_SIGNAL {}
directive set -TRANSACTION_DONE_SIGNAL true
directive set -DONE_FLAG {}
directive set -START_FLAG {}
directive set -BLOCK_SYNC none
directive set -TRANSACTION_SYNC ready
directive set -DATA_SYNC none
directive set -RESET_CLEARS_ALL_REGS true
directive set -CLOCK_OVERHEAD 20.000000
directive set -OPT_CONST_MULTS -1
directive set -CSA 0
directive set -CHARACTERIZE_ROM false
directive set -PROTOTYPE_ROM true
directive set -ROM_THRESHOLD 64
directive set -CLUSTER_INPUT_WIDTH_THRESHOLD 0
directive set -CLUSTER_OPT_CONSTANT_INPUTS true
directive set -CLUSTER_RTL_SYN false
directive set -CLUSTER_FAST_MODE false
directive set -CLUSTER_TYPE combinational
directive set -COMPGRADE fast
directive set -PIPELINE_RAMP_UP true

# End
