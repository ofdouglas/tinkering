# Run system_tb long enough for bootloader advertise + UART download + LED toggle.
set script_dir [file dirname [file normalize [info script]]]
set cpu_root   [file normalize [file join $script_dir ..]]
set hex_src    [file join $cpu_root mem bootloader.hex]
set app_src    [file join $cpu_root mem led_toggle.bin]
set proj_file  [file normalize [file join $script_dir .. .. vivado cpu cpu.xpr]]

if {![file exists $hex_src]} {
    error "bootloader.hex not found at $hex_src — run: make -C $cpu_root/firmware"
}

if {[current_project -quiet] eq ""} {
    open_project $proj_file
}

set xsim_dir [file join [get_property DIRECTORY [current_project]] cpu.sim sim_1 behav xsim]
file mkdir $xsim_dir
file copy -force $hex_src [file join $xsim_dir bootloader.hex]
puts "Copied bootloader.hex -> $xsim_dir"
if {[file exists $app_src]} {
    file copy -force $app_src [file join $xsim_dir led_toggle.bin]
    puts "Copied led_toggle.bin -> $xsim_dir"
}

set_property -name {xsim.simulate.runtime} -value {50ms} -objects [get_filesets sim_1]
launch_simulation
close_sim -force
puts "simulation completed successfully"
