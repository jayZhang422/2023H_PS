# Program the PL and start the Debug ELF on Cortex-A9 core 0.
# Run: xsct ps_program_and_run.tcl
# Check files only: xsct ps_program_and_run.tcl --check

proc require_file {path description} {
    if {![file isfile $path]} {
        error "$description not found: $path"
    }
}

set script_dir [file dirname [file normalize [info script]]]
set workspace [file normalize [file join $script_dir ..]]
set bit_file [file join $workspace Signal_separation_app _ide bitstream top.bit]
set xsa_file [file join $workspace Signal_separation_platform export Signal_separation_platform hw top.xsa]
set ps7_init_file [file join $workspace Signal_separation_app _ide psinit ps7_init.tcl]
set elf_file [file join $workspace Signal_separation_app Debug Signal_separation_app.elf]

foreach {path description} [list \
    $bit_file "Application bitstream" \
    $xsa_file "Platform XSA" \
    $ps7_init_file "PS7 initialization script" \
    $elf_file "Debug ELF"] {
    require_file $path $description
}

if {[lsearch -exact $argv "--check"] >= 0} {
    puts "CHECK PASSED: top.bit, top.xsa, ps7_init.tcl, and Signal_separation_app.elf are available."
    return
}

connect
targets -set -nocase -filter {name =~ "APU*"}
rst -system
after 3000
targets -set -nocase -filter {name =~ "APU*"}
fpga -file $bit_file
loadhw -hw $xsa_file -mem-ranges [list {0x40000000 0xbfffffff}] -regs
configparams force-mem-access 1
source $ps7_init_file
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "*A9*#0"}
dow $elf_file
configparams force-mem-access 0
con
