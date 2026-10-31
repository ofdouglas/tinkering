# Run system_tb long enough for the firmware UART message to complete.
set script_dir [file dirname [file normalize [info script]]]
set cpu_root   [file normalize [file join $script_dir ..]]
set hex_src    [file join $cpu_root mem firmware.hex]
set proj_file  [file normalize [file join $script_dir .. .. vivado cpu cpu.xpr]]

if {![file exists $hex_src]} {
    error "firmware.hex not found at $hex_src — run: make -C $cpu_root/firmware"
}

if {[current_project -quiet] eq ""} {
    open_project $proj_file
}

set xsim_dir [file join [get_property DIRECTORY [current_project]] cpu.sim sim_1 behav xsim]
file mkdir $xsim_dir
file copy -force $hex_src [file join $xsim_dir firmware.hex]
puts "Copied firmware.hex -> $xsim_dir"

set_property -name {xsim.simulate.runtime} -value {2ms} -objects [get_filesets sim_1]
launch_simulation
close_sim -force
puts "simulation completed successfully"
