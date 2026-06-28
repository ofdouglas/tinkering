# Regenerate FPGA/vivado/cpu from tracked sources under FPGA/cpu/.
# Usage:
#   vivado -mode gui  FPGA/cpu/scripts/create_project.tcl
#   vivado -mode batch FPGA/cpu/scripts/create_project.tcl

set script_dir [file dirname [file normalize [info script]]]
set cpu_root   [file normalize [file join $script_dir ..]]
set fpga_root  [file normalize [file join $cpu_root ..]]
set vivado_dir [file join $fpga_root vivado cpu]
set rtl_dir    [file join $cpu_root rtl]
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

# Compile order: package -> interface -> modules (Vivado resolves deps via update_compile_order)
set rtl_files [list \
    [file join $rtl_dir cpu_config_pkg.sv] \
    [file join $rtl_dir bus_interface.sv] \
    [file join $rtl_dir block_rom.sv] \
    [file join $rtl_dir system.sv] \
    [file join $rtl_dir picorv32.v] \
    $rtl_shared \
]
add_files -norecurse $rtl_files

# Packages/interfaces require SystemVerilog; default Verilog mode rejects them.
foreach f [get_files -of_objects [get_filesets sources_1] *.sv] {
    set_property file_type SystemVerilog $f
}

# Compile order: package -> interface -> modules (must precede block_rom)
set fs [get_filesets sources_1]
reorder_files -of_objects $fs -front [get_files -quiet [file join $rtl_dir cpu_config_pkg.sv]]
reorder_files -of_objects $fs -after [get_files -quiet [file join $rtl_dir bus_interface.sv]] \
    [get_files -quiet [file join $rtl_dir cpu_config_pkg.sv]]

update_compile_order -fileset sources_1

set hex_file [file join $cpu_root mem firmware.hex]
if {[file exists $hex_file]} {
    add_files -norecurse $hex_file
    set hex_obj [get_files -quiet [file tail $hex_file]]
    set_property file_type {Memory Initialization Files} $hex_obj
    set_property USED_IN_SIMULATION 1 $hex_obj
    set_property USED_IN_SYNTHESIS 1 $hex_obj
} else {
    puts "WARNING: $hex_file not found — run make -C [file join $cpu_root firmware] first"
}

set sim_copy_tcl [file normalize [file join $script_dir copy_firmware_sim.tcl]]
if {[file exists $sim_copy_tcl]} {
    set_property -name {xsim.simulate.custom_tcl} -value $sim_copy_tcl [get_filesets sim_1]
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
