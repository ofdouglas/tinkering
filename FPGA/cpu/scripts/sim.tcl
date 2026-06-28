# Run system_tb for 500us and check for completion.
set script_dir [file dirname [file normalize [info script]]]
set cpu_root   [file normalize [file join $script_dir ..]]
set hex_src    [file join $cpu_root mem firmware.hex]
set proj_file  [file normalize [file join $script_dir .. .. vivado cpu cpu.xpr]]

if {![file exists $hex_src]} {
    error "firmware.hex not found at $hex_src — run: make -C $cpu_root/firmware"
}

open_project $proj_file
launch_simulation

set xsim_dir [file join [get_property DIRECTORY [current_project]] cpu.sim sim_1 behav xsim]
if {![file isdirectory $xsim_dir]} {
    error "xsim dir not found: $xsim_dir"
}
file copy -force $hex_src [file join $xsim_dir firmware.hex]
puts "Copied firmware.hex -> $xsim_dir"

run 500us
close_sim -force
puts "simulation completed successfully"
