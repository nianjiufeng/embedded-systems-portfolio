# Embedded Systems Portfolio / 嵌入式系统作品集

这是一个面向嵌入式软件、嵌入式软硬件开发岗位的精简作品集。仓库只保留能够说明设计思路与编码能力的个人源码和脱敏案例，不包含公司原理图、PCB、内部文档、芯片资料、个人简历或联系方式。

## 项目一览

| 项目 | 技术栈 | 可验证内容 |
| --- | --- | --- |
| [LD2412 毫米波雷达驱动](projects/ld2412-driver/) | C99、UART、字节流解析、回调接口 | 命令组帧、ACK/主动上报解析、异常流重同步、主机单元测试 |
| [WS2812B + FreeRTOS 示例](projects/ws2812b-freertos/) | GD32H759、Cortex-M7 DWT、GPIO、FreeRTOS | 纳秒级软件时序、亮度缩放、GRB 数据发送、流水灯任务 |
| [机器人自动回充控制板案例](case-studies/robot-charging-dock/) | ESP32-C3、外设通信、中断/定时器、原理图与 PCB、板级调试 | 以脱敏方式说明需求拆解、联调与问题定位方法 |

## 能力重点

- 嵌入式 C：面向资源受限设备的固定缓冲区、回调接口和边界检查。
- 通信与协议：UART 字节流解析、帧同步、长度校验、错误恢复。
- RTOS：FreeRTOS 任务组织、周期调度与外设刷新任务。
- 板级开发：器件选型、原理图与 PCB、焊接调试、示波器和万用表排障。
- 工程质量：可重复构建、主机侧单元测试、清晰文档和敏感信息隔离。

## 快速验证

LD2412 协议层不依赖具体 MCU，可直接在电脑上构建并运行测试：

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

WS2812B 示例依赖 GD32H7 标准外设库、CMSIS 与 FreeRTOS，需要在对应 MCU 工程中编译。仓库没有附带体积较大的厂商 SDK；接入方式见项目内说明。

## 仓库结构

```text
.
├── case-studies/
│   └── robot-charging-dock/
├── projects/
│   ├── ld2412-driver/
│   └── ws2812b-freertos/
├── CMakeLists.txt
└── README.md
```

## 公开边界

该仓库不公开实习单位的内部设计文件、完整产品参数或可直接复刻的硬件资料。案例仅用于说明个人承担的工作、工程方法和问题定位能力。
