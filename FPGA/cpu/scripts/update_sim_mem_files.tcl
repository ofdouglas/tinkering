# Add generated CPU memory/test data files to an existing Vivado project.
# Usage from an open project:
#   source C:/Design/FPGA/cpu/scripts/update_sim_mem_files.tcl

set script_dir [file dirname [file normalize [info script]]]
set cpu_root   [file normalize [file join $script_dir ..]]
set mem_dir    [file join $cpu_root mem]

if {[current_project -quiet] eq ""} {
    error "open the Vivado project before sourcing this script"
}

set mem_files [list]
foreach pattern {*.hex *.regs *.sram} {
    foreach mem_file [glob -nocomplain -directory $mem_dir $pattern] {
        lappend mem_files $mem_file
    }
}

if {[llength $mem_files] == 0} {
    error "no memory init files found under $mem_dir; run make -C $cpu_root/test-fw build first"
}

foreach mem_file $mem_files {
    if {[llength [get_files -quiet $mem_file]] == 0} {
        add_files -norecurse $mem_file
    }
    set mem_obj [get_files -quiet $mem_file]
    set_property file_type {Memory Initialization Files} $mem_obj
    set_property USED_IN_SIMULATION 1 $mem_obj
    set_property USED_IN_SYNTHESIS 0 $mem_obj
}

set firmware_hex [file join $mem_dir firmware.hex]
if {[file exists $firmware_hex]} {
    set_property USED_IN_SYNTHESIS 1 [get_files -quiet $firmware_hex]
}

set sim_copy_tcl [file normalize [file join $script_dir copy_firmware_sim.tcl]]
if {[file exists $sim_copy_tcl]} {
    set_property xsim.simulate.custom_tcl $sim_copy_tcl [get_filesets sim_1]
}

update_compile_order -fileset sim_1
puts "Added [llength $mem_files] memory init file(s) from $mem_dir"
