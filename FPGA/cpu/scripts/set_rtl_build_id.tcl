set rtl_build_id_script_dir [file dirname [file normalize [info script]]]
set rtl_build_id_repo_root  [file normalize [file join $rtl_build_id_script_dir .. .. ..]]

proc set_rtl_build_id {} {
    global rtl_build_id_repo_root
    set sha32 [string trim [exec git -C $rtl_build_id_repo_root rev-parse --short=8 HEAD]]
    set build_id_define "RTL_BUILD_ID=32'h${sha32}"

    # Design sources are compiled in both fileset contexts, depending on
    # whether Vivado is synthesizing or simulating the project.
    foreach fileset_name {sources_1 sim_1} {
        set fileset [get_filesets -quiet $fileset_name]
        if {$fileset eq ""} {
            continue
        }

        set defines {}
        foreach define [get_property verilog_define $fileset] {
            if {![string match "RTL_BUILD_ID=*" $define]} {
                lappend defines $define
            }
        }
        lappend defines $build_id_define
        set_property verilog_define $defines $fileset
    }

    puts "RTL_BUILD_ID set to 32'h${sha32}"
}