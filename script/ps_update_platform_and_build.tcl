# Update the Vitis platform with the latest PL XSA, reset/regenerate BSP, then clean and build the system project.
# Run: xsct ps_update_platform_and_build.tcl
# Check names only: xsct ps_update_platform_and_build.tcl --check

proc require_file {path description} {
    if {![file isfile $path]} {
        error "$description not found: $path"
    }
}

set script_dir [file dirname [file normalize [info script]]]
set workspace [file normalize [file join $script_dir ..]]
set xsa_file [file normalize [file join $workspace .. 2023H_PL 2023H_pl top.xsa]]
set platform_name Signal_separation_platform
set system_name Signal_separation_app_system

require_file $xsa_file "Latest PL XSA"
setws $workspace
platform active $platform_name

if {[lsearch -exact $argv "--check"] >= 0} {
    puts "CHECK PASSED: workspace, platform $platform_name, system $system_name, and $xsa_file are available."
    return
}

platform config -updatehw $xsa_file
# XSCT 2020.2 uses 'bsp reload' for the GUI BSP reset operation.
bsp reload
bsp regenerate
platform clean
platform generate
sysproj clean -name $system_name
sysproj build -name $system_name
puts "DONE: updated $platform_name from $xsa_file and built $system_name."
