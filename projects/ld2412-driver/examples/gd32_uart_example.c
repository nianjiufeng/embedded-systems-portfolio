#include "gd32f10x.h"                                 // GD32 标准外设库头文件；其他系列请换成对应 gd32xxxx.h
#include "ld2412.h"                                   // LD2412 协议驱动头文件

#define LD2412_USART                  USART0          // 选择连接 LD2412 的 GD32 串口外设
#define LD2412_USART_IRQ              USART0_IRQn     // 选择该串口对应的中断号
#define LD2412_USART_IRQHandler       USART0_IRQHandler // 选择该串口对应的中断服务函数名
#define LD2412_USART_RCU              RCU_USART0      // 选择该串口对应的外设时钟
#define LD2412_GPIO_RCU               RCU_GPIOA       // 选择 TX/RX 引脚所在 GPIO 端口时钟
#define LD2412_GPIO_PORT              GPIOA           // 选择 TX/RX 引脚所在 GPIO 端口
#define LD2412_TX_PIN                 GPIO_PIN_9      // GD32 TX 引脚，连接 LD2412 RX
#define LD2412_RX_PIN                 GPIO_PIN_10     // GD32 RX 引脚，连接 LD2412 TX
#define LD2412_UART_BAUD              115200U         // LD2412 出厂默认串口波特率 115200

static ld2412_t g_ld2412;                             // 定义 LD2412 驱动对象，保存串口解析状态
static volatile ld2412_report_t g_last_report;        // 保存最近一帧雷达上报数据，主循环可读取
static volatile uint8_t g_report_ready = 0u;          // 雷达上报新数据标志，1 表示有新帧
static volatile uint16_t g_last_ack_cmd = 0u;         // 保存最近一次 ACK 对应的命令字
static volatile uint16_t g_last_ack_status = 0u;      // 保存最近一次 ACK 状态，0 成功，1 失败
static volatile uint8_t g_ack_ready = 0u;             // ACK 新数据标志，1 表示收到命令应答

static void ld2412_uart_send(const uint8_t *data, uint16_t len) // GD32 串口发送回调，供协议驱动调用
{                                                     // 函数体开始
    uint16_t i = 0u;                                  // 定义循环变量，用于逐字节发送
    for (i = 0u; i < len; i++)                        // 遍历待发送的命令帧字节
    {                                                 // 循环体开始
        while (usart_flag_get(LD2412_USART, USART_FLAG_TBE) == RESET) // 等待发送数据寄存器为空
        {                                             // 等待体开始
        }                                             // 等待体结束
        usart_data_transmit(LD2412_USART, data[i]);   // 通过 GD32 USART 发送 1 个命令帧字节
    }                                                 // 循环体结束
    while (usart_flag_get(LD2412_USART, USART_FLAG_TC) == RESET) // 等待最后 1 字节完全发送完成
    {                                                 // 等待体开始
    }                                                 // 等待体结束
}                                                     // 函数体结束

static void ld2412_report_handler(const ld2412_report_t *report) // 雷达主动上报回调函数
{                                                     // 函数体开始
    if (report == 0) { return; }                      // 防止空指针
    g_last_report = *report;                          // 保存解析后的目标状态、距离、能量等串口数据
    g_report_ready = 1u;                              // 置位新上报标志，通知主循环处理
}                                                     // 函数体结束

static void ld2412_ack_handler(const ld2412_ack_t *ack) // 命令 ACK 回调函数
{                                                     // 函数体开始
    if (ack == 0) { return; }                         // 防止空指针
    g_last_ack_cmd = ack->command;                    // 保存 ACK 对应的原始命令字
    g_last_ack_status = ack->status;                  // 保存 ACK 执行状态，0 成功
    g_ack_ready = 1u;                                 // 置位 ACK 标志，通知主循环命令有结果
}                                                     // 函数体结束

void ld2412_gd32_usart_init(void)                     // 初始化连接 LD2412 的 GD32 串口
{                                                     // 函数体开始
    rcu_periph_clock_enable(LD2412_GPIO_RCU);         // 打开 TX/RX 所在 GPIO 时钟
    rcu_periph_clock_enable(LD2412_USART_RCU);        // 打开 USART 外设时钟
    gpio_init(LD2412_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, LD2412_TX_PIN); // 配置 GD32 TX 为复用推挽输出
    gpio_init(LD2412_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, LD2412_RX_PIN); // 配置 GD32 RX 为浮空输入
    usart_deinit(LD2412_USART);                       // 复位 USART 配置，避免旧配置影响 LD2412 通信
    usart_baudrate_set(LD2412_USART, LD2412_UART_BAUD); // 设置串口波特率为 115200
    usart_word_length_set(LD2412_USART, USART_WL_8BIT); // 设置 8 位数据位
    usart_stop_bit_set(LD2412_USART, USART_STB_1BIT); // 设置 1 位停止位
    usart_parity_config(LD2412_USART, USART_PM_NONE); // 设置无校验位
    usart_hardware_flow_rts_config(LD2412_USART, USART_RTS_DISABLE); // 关闭 RTS 硬件流控
    usart_hardware_flow_cts_config(LD2412_USART, USART_CTS_DISABLE); // 关闭 CTS 硬件流控
    usart_receive_config(LD2412_USART, USART_RECEIVE_ENABLE); // 使能串口接收，接收 LD2412 上报和 ACK
    usart_transmit_config(LD2412_USART, USART_TRANSMIT_ENABLE); // 使能串口发送，向 LD2412 下发命令
    usart_interrupt_enable(LD2412_USART, USART_INT_RBNE); // 使能接收非空中断，每来 1 字节进入 ISR
    nvic_irq_enable(LD2412_USART_IRQ, 1U, 0U);        // 使能 USART 中断，抢占优先级 1，子优先级 0
    usart_enable(LD2412_USART);                       // 使能 USART 外设开始通信
}                                                     // 函数体结束

void ld2412_app_init(void)                            // 初始化 LD2412 应用层和协议驱动
{                                                     // 函数体开始
    ld2412_gd32_usart_init();                         // 先初始化 GD32 串口硬件
    ld2412_init(&g_ld2412, ld2412_uart_send);         // 初始化协议驱动并绑定串口发送函数
    ld2412_set_report_callback(&g_ld2412, ld2412_report_handler); // 注册雷达上报数据回调
    ld2412_set_ack_callback(&g_ld2412, ld2412_ack_handler); // 注册命令 ACK 回调
}                                                     // 函数体结束

void LD2412_USART_IRQHandler(void)                    // GD32 USART 中断服务函数
{                                                     // 函数体开始
    uint8_t rx_byte = 0u;                             // 定义变量保存本次收到的串口字节
    if (usart_interrupt_flag_get(LD2412_USART, USART_INT_FLAG_RBNE) != RESET) // 判断是否为接收非空中断
    {                                                 // 判断体开始
        rx_byte = (uint8_t)usart_data_receive(LD2412_USART); // 读取 USART 数据寄存器，得到 LD2412 的 1 字节
        ld2412_input_byte(&g_ld2412, rx_byte);        // 把该字节交给协议驱动拼帧和解析
    }                                                 // 判断体结束
}                                                     // 函数体结束

void ld2412_example_config_basic(void)                // 示例：配置基础参数
{                                                     // 函数体开始
    (void)ld2412_enter_config(&g_ld2412);             // 第 1 步，进入配置模式，后续配置命令才有效
    (void)ld2412_set_basic(&g_ld2412, 1u, 14u, 5u, false); // 第 2 步，最小门 1，最大门 14，无人持续 5 秒，有人 OUT 高
    (void)ld2412_engineering_on(&g_ld2412);           // 第 3 步，打开工程模式，上报各距离门能量和光感值
    (void)ld2412_exit_config(&g_ld2412);              // 第 4 步，结束配置，雷达恢复正常工作上报
}                                                     // 函数体结束

void ld2412_example_loop(void)                        // 示例：在 main 的 while(1) 中周期调用
{                                                     // 函数体开始
    if (g_report_ready != 0u)                         // 判断是否收到新的雷达主动上报帧
    {                                                 // 判断体开始
        g_report_ready = 0u;                          // 清除新上报标志
        if (g_last_report.state == LD2412_TARGET_NONE) // 判断上报状态是否为无人
        {                                             // 判断体开始
            /* 用户在这里关闭灯或清除有人标志 */      // 串口解析结果：无目标
        }                                             // 判断体结束
        else                                          // 其他状态表示有人或校正状态
        {                                             // 判断体开始
            /* 用户在这里打开灯或设置有人标志 */      // 串口解析结果：检测到目标
        }                                             // 判断体结束
    }                                                 // 判断体结束
    if (g_ack_ready != 0u)                            // 判断是否收到新的命令 ACK
    {                                                 // 判断体开始
        g_ack_ready = 0u;                             // 清除 ACK 标志
        /* 可在这里检查 g_last_ack_cmd 和 g_last_ack_status */ // 串口命令执行结果
    }                                                 // 判断体结束
}                                                     // 函数体结束
