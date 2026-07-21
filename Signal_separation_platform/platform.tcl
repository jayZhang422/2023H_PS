# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct E:\7020_Project\2023H\2023H_PS\Signal_separation_platform\platform.tcl
# 
# OR launch xsct and run below command.
# source E:\7020_Project\2023H\2023H_PS\Signal_separation_platform\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {Signal_separation_platform}\
-hw {E:\7020_Project\2023H\2023H_PL\2023H_pl\top.xsa}\
-proc {ps7_cortexa9_0} -os {standalone} -fsbl-target {psu_cortexa53_0} -out {E:/7020_Project/2023H/2023H_PS}

platform write
platform generate -domains 
platform active {Signal_separation_platform}
bsp reload
catch {bsp regenerate}
platform config -updatehw {E:/7020_Project/2023H/2023H_PL/2023H_pl/top.xsa}
bsp reload
catch {bsp regenerate}
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform config -updatehw {E:/7020_Project/2023H/2023H_PL/2023H_pl/top.xsa}
bsp reload
catch {bsp regenerate}
platform clean
platform generate
platform clean
platform generate
platform config -updatehw {E:/7020_Project/2023H/2023H_PL/2023H_pl/top.xsa}
bsp reload
catch {bsp regenerate}
platform clean
platform generate
platform clean
platform generate
platform config -updatehw {E:/7020_Project/2023H/2023H_PL/2023H_pl/top.xsa}
bsp reload
catch {bsp regenerate}
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform active {Signal_separation_platform}
bsp reload
platform clean
platform generate
platform active {Signal_separation_platform}
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform active {Signal_separation_platform}
platform clean
platform generate
platform active {Signal_separation_platform}
platform config -updatehw {E:/7020_Project/2023H/2023H_PL/2023H_pl/top.xsa}
platform config -updatehw {E:/7020_Project/2023H/2023H_PL/2023H_pl/top.xsa}
platform clean
platform generate
platform clean
platform generate
platform active {Signal_separation_platform}
bsp reload
platform active {Signal_separation_platform}
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform active {Signal_separation_platform}
platform config -updatehw {E:/7020_Project/2023H/2023H_PL/2023H_pl/top.xsa}
bsp reload
catch {bsp regenerate}
platform clean
platform generate
platform clean
platform generate
platform clean
platform generate
platform active {Signal_separation_platform}
platform config -updatehw {E:/7020_Project/2023H/2023H_PL/2023H_pl/top.xsa}
bsp reload
catch {bsp regenerate}
platform clean
platform generate
platform clean
platform generate
platform active {Signal_separation_platform}
