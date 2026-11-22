# Copy ROM init image into the synthesis run directory for $readmemh("bootloader.hex", ...).
set script_dir [file dirname [file normalize [info script]]]
set hex_src [file normalize [file join $script_dir .. mem bootloader.hex]]

# This script is the synth_1 pre-hook, so refresh the ID for builds started
# from either the GUI or Tcl.
source [file join $script_dir set_rtl_build_id.tcl]
set_rtl_build_id

if {[info exists synth_dir]} {
    set run_dir $synth_dir
} elseif {[llength [get_runs -quiet synth_1]]} {
    set run_dir [get_property DIRECTORY [get_runs synth_1]]
} else {
    set run_dir [pwd]
}
set hex_dst [file normalize [file join $run_dir bootloader.hex]]

if {![file exists $hex_src]} {
    send_msg_id {COPY-FIRMWARE} {WARNING} "bootloader.hex not found at $hex_src (run make in firmware/)"
} else {
    file mkdir [file dirname $hex_dst]
    file copy -force $hex_src $hex_dst
}
