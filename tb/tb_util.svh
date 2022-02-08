// Copyright 2022 OpenHW Group
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

`ifndef SYNTHESIS
// Task for loading 'mem' with SystemVerilog system task $readmemh()
export "DPI-C" task tb_util_ReadMemh;
export "DPI-C" task tb_util_WriteToSram0;
export "DPI-C" task tb_util_WriteToSram1;
export "DPI-C" task tb_get_MemSize;

import core_v_mini_mcu_pkg::*;


task tb_get_MemSize;
  output int mem_size;
  mem_size = core_v_mini_mcu_pkg::MEM_SIZE;
endtask

task tb_util_ReadMemh;
  input  string file;
  output logic [7:0] stimuli[core_v_mini_mcu_pkg::MEM_SIZE];
  $readmemh(file,stimuli);
endtask


task tb_util_WriteToSram0;
  input integer addr;
  input [7:0]   val3;
  input [7:0]   val2;
  input [7:0]   val1;
  input [7:0]   val0;
  memory_subsystem_i.ram0_i.tc_ram_i.sram[addr] = {val3, val2, val1, val0};
endtask

task tb_util_WriteToSram1;
  input integer addr;
  input [7:0]   val3;
  input [7:0]   val2;
  input [7:0]   val1;
  input [7:0]   val0;
  memory_subsystem_i.ram1_i.tc_ram_i.sram[addr] = {val3, val2, val1, val0};
endtask
`endif
