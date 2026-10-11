# WS2812B 流水灯与 FreeRTOS 任务示例

该示例面向 GD32H759（Cortex-M7），使用 GPIO 位操作和 DWT 周期计数器生成 WS2812B 单总线时序，并在 FreeRTOS 任务中驱动 10 颗灯珠循环显示 10 种颜色。

## 实现要点

- 使用 DWT `CYCCNT` 按当前内核频率换算纳秒级高电平和位周期。
- 按 WS2812B 要求以 GRB 顺序、最高位优先发送数据。
- 刷新期间保存 `PRIMASK` 并临时关闭中断，避免位时序被抢占破坏。
- 提供全局亮度缩放、单灯设置、整条灯带设置、清屏和刷新接口。
- 提供 GPIO 翻转和固定波形两种调试模式，便于使用示波器确认引脚与时序。
- FreeRTOS 示例使用 `vTaskDelay(pdMS_TO_TICKS(...))` 明确任务周期。

## 文件说明

```text
include/ws2812b_10color.h  驱动配置和公共 API
src/ws2812b_10color.c      GD32H7 GPIO/DWT 实现
examples/freertos_task.c   10 色循环任务
```

## 移植步骤

1. 将 `include/` 与 `src/` 加入 GD32H7 工程。
2. 确认 `SystemCoreClock` 已正确更新，DWT 可用。
3. 按实际硬件修改 GPIO 端口、引脚、灯珠数量和时序宏。
4. 在调度器启动前调用 `ws2812b_init()`。
5. 参考 `examples/freertos_task.c` 创建刷新任务。
6. 先使用固定波形模式连接示波器，确认 T0H、T1H、位周期与复位时间，再连接灯带。

## 工程取舍

软件位时序简单直观，但刷新期间会短暂关闭中断。10 颗灯珠的单次数据段约为 300 微秒，通常可以接受；灯珠数量增加或系统有严格实时中断要求时，应改用 SPI 编码、定时器 PWM + DMA 等方案。

该代码依赖 GD32H7 标准外设库与 CMSIS，因此未纳入主机侧构建。厂商 SDK 和芯片手册未复制到本仓库。
