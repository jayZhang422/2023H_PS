# Clean and build the existing Vitis system project after PS source changes.
# Run: xsct ps_rebuild_system.tcl
# Check names only: xsct ps_rebuild_system.tcl --check

set script_dir [file dirname [file normalize [info script]]]
set workspace [file normalize [file join $script_dir ..]]
set system_name Signal_separation_app_system

setws $workspace
if {[lsearch -exact $argv "--check"] >= 0} {
    puts "CHECK PASSED: workspace and system project $system_name are available."
    return
}

sysproj clean -name $system_name
sysproj build -name $system_name
puts "DONE: cleaned and built $system_name."
