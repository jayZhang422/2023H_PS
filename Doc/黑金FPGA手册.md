# ALINX AX7020 开发板关键信息提取

## 1. 核心硬件参数
*   **开发板型号**: ALINX AX7020 
*   **核心 ZYNQ 芯片**: XC7Z020-2CLG400I (FBGA封装，400引脚，包含双核 ARM Cortex-A9 和 FPGA 可编程逻辑) 
*   **DDR3 型号**: 2片 SK hynix `H5TQ4G63AFR-PBC` (兼容 MT41J256M16RE-125) 
    *   **容量**: 每片 4Gbit，总容量 8Gbit (1GB) 
    *   **总线宽度**: 32 bit 
    *   **最高速率**: 533MHz (数据速率 1066Mbps) 
*   **QSPI FLASH**: `W25Q256` (容量 32M Byte / 256Mbit，Winbond，用于存储启动镜像和文件) 
*   **以太网 PHY**: Realtek `RTL8211E-VL` (支持 10/100/1000 Mbps，通过 RGMII 接口通信) 
*   **USB 2.0 芯片**: `USB3320C-EZK` (1.8V 高速 ULPI 接口，支持 Host / Slave 模式) 
*   **USB 转串口芯片**: Silicon Labs `CP2102GM` (通过 Micro USB 与 PC 通信) 
*   **EEPROM**: `24LC04` (容量 4Kbit，IIC 接口通信) 
*   **实时时钟 (RTC)**: `DS1302` (配有 CR1220 纽扣电池座，外接 32.768KHz 晶振) 

---

## 2. 时钟配置引脚

### 2.1 PS 端时钟 (包含 ZYNQ 引脚名与引脚号)
| 时钟源 | 频率 | 信号名称 | ZYNQ 引脚名 | ZYNQ 引脚号 | 备注 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| PS 系统时钟  | 33.333MHz  | PS_CLK_500  | PS_CLK_500  | E7  | 为 ARM 系统提供稳定时钟  |

### 2.2 PL 端时钟 (仅包含 ZYNQ 引脚号)
| 时钟源 | 频率 | 信号名称 | ZYNQ 引脚号 | 备注 |
| :--- | :--- | :--- | :--- | :--- |
| PL 系统时钟  | 50MHz  | PL_GCLK  | U18  | 驱动 FPGA 全局时钟 (MRCC)  |

---

## 3. PS 端外设引脚分配 (包含 ZYNQ 引脚名与引脚号)

### 3.1 基础交互外设 (PS LED & 按键)
| 信号名称 | ZYNQ 引脚名 | ZYNQ 引脚号 | 备注 |
| :--- | :--- | :--- | :--- |
| MIO0_LED  | PS_MIO0_500  | E6  | PS 控制的 LED1  |
| MIO13_LED  | PS_MIO13_500  | E8  | PS 控制的 LED2  |
| MIO_KEY1  | PS_MIO50_501  | B13  | PS 用户按键1  |
| MIO_KEY2  | PS_MIO51_501  | B9  | PS 用户按键2  |

### 3.2 串行通信与存储 (UART / SD / SPI)
| 接口类型 | 信号名称 | ZYNQ 引脚名 | ZYNQ 引脚号 | 备注 |
| :--- | :--- | :--- | :--- | :--- |
| **UART**  | UART_TX  | PS_MIO48_501  | B12  | Uart 数据输出  |
| **UART**  | UART_RX  | PS_MIO49_501  | C12  | Uart 数据输入  |
| **SD 卡**  | SD_CLK  | PS_MIO40  | D14  | SD时钟信号  |
| **SD 卡**  | SD_CMD  | PS_MIO41  | C17  | SD命令信号  |
| **SD 卡**  | SD_D0  | PS_MIO42  | E12  | SD数据0  |
| **SD 卡**  | SD_CD  | PS_MIO47  | B14  | SD卡插入信号  |
| **QSPI Flash** | QSPI_CLK  | PS_MIO6_500  | A5  | QSPI 时钟  |
| **QSPI Flash** | QSPI_CS  | PS_MIO1_500  | A7  | QSPI 片选  |
| **QSPI Flash** | QSPI_D0  | PS_MIO2_500  | B8  | QSPI 数据0  |

*(注：DDR3、以太网、USB等其余复杂外设由于引脚数量较多，完整清单请参阅手册原文。)* 

---

## 4. PL 端外设引脚分配 (仅包含 ZYNQ 引脚号)

### 4.1 基础交互外设 (PL LED & 按键)
| 信号名称 | ZYNQ 引脚号 | 备注 |
| :--- | :--- | :--- |
| LED1  | M14  | PL 用户 LED1  |
| LED2  | M15  | PL 用户 LED2  |
| LED3  | K16  | PL 用户 LED3  |
| LED4  | J16  | PL 用户 LED4  |
| KEY1  | N15  | PL 用户按键1  |
| KEY2  | N16  | PL 用户按键2  |
| KEY3  | T17  | PL 用户按键3  |
| KEY4  | R17  | PL 用户按键4  |

### 4.2 视频与通信接口 (HDMI / EEPROM / RTC)
| 接口类型 | 信号名称 | ZYNQ 引脚号 | 备注 |
| :--- | :--- | :--- | :--- |
| **HDMI**  | HDMI_CLK_P  | N18  | HDMI 时钟正  |
| **HDMI**  | HDMI_CLK_N  | P19  | HDMI 时钟负  |
| **HDMI**  | HDMI_D0_P  | V20  | HDMI 数据0正  |
| **HDMI**  | HDMI_D0_N  | W20  | HDMI 数据0负  |
| **HDMI**  | HDMI_SCL  | R18  | HDMI IIC 时钟  |
| **HDMI**  | HDMI_SDA  | R16  | HDMI IIC 数据  |
| **EEPROM**  | EEPROM_I2C_SCL  | T19  | IIC 时钟信号  |
| **EEPROM**  | EEPROM_I2C_SDA  | U19  | IIC 数据信号  |
| **RTC**  | RTC_SCLK  | R19  | RTC 时钟信号  |
| **RTC**  | RTC_DATA  | L14  | RTC 数据信号  |
| **RTC**  | RTC_RESET  | L15  | RTC 复位信号  |