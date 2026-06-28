# Batch synth after create_project.tcl.
launch_runs synth_1 -jobs 8
wait_on_run synth_1
set prog [get_property PROGRESS [get_runs synth_1]]
if {$prog != "100%"} {
    error "synth_1 failed (progress=$prog)"
}
puts "synth_1 completed successfully"
