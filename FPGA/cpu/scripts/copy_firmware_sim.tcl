# Pre-simulation: copy memory init images into xsim run directory for $readmemh(...).
set script_dir [file dirname [file normalize [info script]]]
set cpu_root   [file normalize [file join $script_dir ..]]
set hex_src    [file join $cpu_root mem firmware.hex]
set mem_dir    [file join $cpu_root mem]

if {![file exists $hex_src]} {
    puts "WARNING: firmware.hex not found at $hex_src (run make in firmware/)"
    return
}

set xsim_dir [file normalize [pwd]]
if {[file tail $xsim_dir] ne "xsim"} {
    set xsim_dir [file join [get_property DIRECTORY [current_project]] cpu.sim sim_1 behav xsim]
}

file mkdir $xsim_dir
file copy -force $hex_src [file join $xsim_dir firmware.hex]
puts "Copied firmware.hex -> $xsim_dir"

foreach pattern {*.hex *.regs *.sram} {
    foreach mem_file [glob -nocomplain -directory $mem_dir $pattern] {
        file copy -force $mem_file [file join $xsim_dir [file tail $mem_file]]
    }
}
puts "Copied CPU test memory files -> $xsim_dir"
