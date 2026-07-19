# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: E:\7020_Project\2023H\2023H_PS\Signal_separation_app_system\_ide\scripts\debugger_signal_separation_app-default.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source E:\7020_Project\2023H\2023H_PS\Signal_separation_app_system\_ide\scripts\debugger_signal_separation_app-default.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Digilent JTAG-HS1 210512180081" && level==0 && jtag_device_ctx=="jsn-JTAG-HS1-210512180081-23727093-0"}
fpga -file E:/7020_Project/2023H/2023H_PS/Signal_separation_app/_ide/bitstream/top.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw E:/7020_Project/2023H/2023H_PS/Signal_separation_platform/export/Signal_separation_platform/hw/top.xsa -mem-ranges [list {0x40000000 0xbfffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
source E:/7020_Project/2023H/2023H_PS/Signal_separation_app/_ide/psinit/ps7_init.tcl
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "*A9*#0"}
dow E:/7020_Project/2023H/2023H_PS/Signal_separation_app/Debug/Signal_separation_app.elf
configparams force-mem-access 0
bpadd -addr &main
