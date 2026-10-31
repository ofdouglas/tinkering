# Batch synth after create_project.tcl.
# Usage:
#   vivado -mode batch -source FPGA/cpu/scripts/create_project.tcl
#   vivado -mode batch -source FPGA/cpu/scripts/build.tcl

set script_dir [file dirname [file normalize [info script]]]
set proj_file  [file normalize [file join $script_dir .. .. vivado cpu cpu.xpr]]

if {[current_project -quiet] eq ""} {
    if {![file exists $proj_file]} {
        error "project not found: $proj_file — run create_project.tcl first"
    }
    open_project $proj_file
}

launch_runs synth_1 -jobs 8
wait_on_run synth_1
set prog [get_property PROGRESS [get_runs synth_1]]
if {$prog != "100%"} {
    error "synth_1 failed (progress=$prog)"
}
puts "synth_1 completed successfully"

launch_runs impl_1 -to_step write_bitstream -jobs 8
wait_on_run impl_1
set impl_prog [get_property PROGRESS [get_runs impl_1]]
if {$impl_prog != "100%"} {
    error "impl_1 failed (progress=$impl_prog)"
}
puts "impl_1 completed successfully"
