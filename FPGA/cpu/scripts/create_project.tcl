# Regenerate FPGA/vivado/cpu from tracked sources under FPGA/cpu/.
# Usage:
#   vivado -mode gui  FPGA/cpu/scripts/create_project.tcl
#   vivado -mode batch FPGA/cpu/scripts/create_project.tcl

set script_dir [file dirname [file normalize [info script]]]
set cpu_root   [file normalize [file join $script_dir ..]]
set fpga_root  [file normalize [file join $cpu_root ..]]
set vivado_dir [file join $fpga_root vivado cpu]
set rtl_shared [file join $fpga_root rtl uart.sv]

set proj_name cpu
set part      xc7a200tsbg484-1
set board     digilentinc.com:nexys_video:part0:1.2

file mkdir $vivado_dir
create_project -force $proj_name $vivado_dir -part $part

set board_repo [file normalize [file join $env(APPDATA) Xilinx Vivado 2025.2 xhub board_store xilinx_board_store]]
if {[file isdirectory $board_repo]} {
    set_property board_part_repo_paths $board_repo [current_project]
    set_property board_part $board [current_project]
} else {
    puts "WARNING: board repo not found at $board_repo — continuing without board_part"
}

set rtl_files [list \
    [file join $cpu_root rtl system.sv] \
    [file join $cpu_root rtl picorv32.v] \
    $rtl_shared \
]
add_files -norecurse $rtl_files

set hex_file [file join $cpu_root mem firmware.hex]
if {[file exists $hex_file]} {
    add_files -norecurse $hex_file
} else {
    puts "WARNING: $hex_file not found — run make -C [file join $cpu_root firmware] first"
}

add_files -fileset constrs_1 -norecurse [file join $cpu_root constr nexys_video.xdc]

add_files -fileset sim_1 -norecurse [file join $cpu_root sim system_tb.sv]
set_property top system [current_fileset]
set_property top system_tb [get_filesets sim_1]
set_property top_lib xil_defaultlib [get_filesets sim_1]

set copy_tcl [file normalize [file join $script_dir copy_firmware.tcl]]
add_files -fileset utils_1 -norecurse $copy_tcl
set_property STEPS.SYNTH_DESIGN.TCL.PRE $copy_tcl [get_runs synth_1]

update_compile_order -fileset sources_1
update_compile_order -fileset sim_1

puts "Project created at $vivado_dir/$proj_name.xpr"
