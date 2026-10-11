# LD2412 毫米波雷达 UART 驱动

面向 MCU 的轻量级 C 驱动。协议层不依赖芯片厂商库：应用层只需提供 UART 发送函数，并把接收到的字节逐个交给解析器。

## 设计特点

- 固定大小缓冲区，不使用动态内存。
- 同时解析配置 ACK 与主动上报帧。
- 支持基础信息和 14 个距离门的工程模式数据。
- 遇到噪声、错误帧头、错误帧尾或异常长度时重新同步。
- 命令层覆盖配置模式、距离门、灵敏度、波特率、蓝牙、光感和恢复出厂等操作。
- 对接收长度和发送负载执行加法前边界检查，避免 16 位长度回绕导致越界访问。

## 文件说明

```text
include/ld2412.h             公共数据结构和 API
src/ld2412.c                 平台无关协议实现
examples/gd32_uart_example.c GD32 标准外设库移植示例
tests/test_ld2412.c          主机侧协议与异常输入测试
```

## 最小接入示例

```c
static ld2412_t radar;

static void radar_uart_send(const uint8_t *data, uint16_t length)
{
    /* 使用目标 MCU 的 UART 发送 data[0..length-1]。 */
}

void radar_init(void)
{
    ld2412_init(&radar, radar_uart_send);
    ld2412_set_report_callback(&radar, on_radar_report);
    ld2412_set_ack_callback(&radar, on_radar_ack);
}

void uart_rx_isr(void)
{
    ld2412_input_byte(&radar, uart_read_byte());
}
```

配置命令应串行执行：进入配置模式后，每发送一条命令等待对应 ACK，再发送下一条，最后退出配置模式。驱动负责组帧和解析；超时、重试和命令队列由应用层根据产品需求实现。

## 测试覆盖

- 进入配置模式命令的完整字节序列。
- ACK 命令字、状态和负载解析。
- 基础目标上报中的状态、距离和能量解析。
- 串口噪声后的重新同步。
- 畸形超长接收帧和超大发送负载的拒绝处理。

在仓库根目录执行：

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## 硬件说明

默认串口参数为 115200-8-N-1。`examples/` 中的引脚和外设编号仅作移植参考，接线、电源和复用配置必须以实际开发板及模块资料为准。主机测试只能验证协议逻辑，最终产品仍需用真实模块完成波形、电源和长时间通信测试。
