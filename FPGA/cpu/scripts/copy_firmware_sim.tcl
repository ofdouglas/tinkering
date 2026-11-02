# Pre-simulation: copy memory init images into xsim run directory for $readmemh(...).
set script_dir [file dirname [file normalize [info script]]]
set cpu_root   [file normalize [file join $script_dir ..]]
set hex_src    [file join $cpu_root mem bootloader.hex]
set app_src    [file join $cpu_root mem led_toggle.bin]
set mem_dir    [file join $cpu_root mem]

if {![file exists $hex_src]} {
    puts "WARNING: bootloader.hex not found at $hex_src (run make in firmware/)"
    return
}

set xsim_dir [file normalize [pwd]]
if {[file tail $xsim_dir] ne "xsim"} {
    set xsim_dir [file join [get_property DIRECTORY [current_project]] cpu.sim sim_1 behav xsim]
}

file mkdir $xsim_dir
file copy -force $hex_src [file join $xsim_dir bootloader.hex]
puts "Copied bootloader.hex -> $xsim_dir"
if {[file exists $app_src]} {
    file copy -force $app_src [file join $xsim_dir led_toggle.bin]
    puts "Copied led_toggle.bin -> $xsim_dir"
}

foreach pattern {*.hex *.regs *.sram} {
    foreach mem_file [glob -nocomplain -directory $mem_dir $pattern] {
        file copy -force $mem_file [file join $xsim_dir [file tail $mem_file]]
    }
}
puts "Copied CPU test memory files -> $xsim_dir"
