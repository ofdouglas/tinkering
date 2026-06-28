# Run system_tb for 500us and check for completion.
open_project [file normalize [file join [file dirname [info script]] .. .. vivado cpu cpu.xpr]]
launch_simulation
run 500us
close_sim -force
puts "simulation completed successfully"
