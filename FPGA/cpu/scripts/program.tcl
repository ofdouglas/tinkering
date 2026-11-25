# Program the Nexys Video through Vivado Hardware Manager.
# Usage:
#   vivado -mode batch -source FPGA/cpu/scripts/program.tcl
#   vivado -mode batch -source FPGA/cpu/scripts/program.tcl -tclargs /path/to/system.bit

set script_dir [file dirname [file normalize [info script]]]
set default_bit [file normalize \
    [file join $script_dir .. .. vivado cpu cpu.runs impl_1 system.bit]]
set bit_file [expr {[llength $argv] > 0 ? [file normalize [lindex $argv 0]] : $default_bit}]

if {![file exists $bit_file]} {
    error "bitstream not found: $bit_file"
}

open_hw_manager
connect_hw_server -allow_non_jtag
open_hw_target

set devices [get_hw_devices -quiet *xc7a200t*]
if {[llength $devices] == 0} {
    error "Nexys Video FPGA not found on the hardware target"
}

set device [lindex $devices 0]
current_hw_device $device
refresh_hw_device -update_hw_probes false $device
set_property PROGRAM.FILE $bit_file $device
program_hw_devices $device
refresh_hw_device $device

puts "Programmed $device with $bit_file"
close_hw_target
disconnect_hw_server
close_hw_manager
