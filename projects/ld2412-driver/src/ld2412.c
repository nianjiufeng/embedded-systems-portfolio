#include "ld2412.h"                                  // 引入 LD2412 驱动接口定义

#define LD2412_CMD_HEAD_0             0xFDu          // 配置命令/ACK 帧头第 1 字节
#define LD2412_CMD_HEAD_1             0xFCu          // 配置命令/ACK 帧头第 2 字节
#define LD2412_CMD_HEAD_2             0xFBu          // 配置命令/ACK 帧头第 3 字节
#define LD2412_CMD_HEAD_3             0xFAu          // 配置命令/ACK 帧头第 4 字节
#define LD2412_CMD_TAIL_0             0x04u          // 配置命令/ACK 帧尾第 1 字节
#define LD2412_CMD_TAIL_1             0x03u          // 配置命令/ACK 帧尾第 2 字节
#define LD2412_CMD_TAIL_2             0x02u          // 配置命令/ACK 帧尾第 3 字节
#define LD2412_CMD_TAIL_3             0x01u          // 配置命令/ACK 帧尾第 4 字节

#define LD2412_REPORT_HEAD_0          0xF4u          // 主动上报帧头第 1 字节
#define LD2412_REPORT_HEAD_1          0xF3u          // 主动上报帧头第 2 字节
#define LD2412_REPORT_HEAD_2          0xF2u          // 主动上报帧头第 3 字节
#define LD2412_REPORT_HEAD_3          0xF1u          // 主动上报帧头第 4 字节
#define LD2412_REPORT_TAIL_0          0xF8u          // 主动上报帧尾第 1 字节
#define LD2412_REPORT_TAIL_1          0xF7u          // 主动上报帧尾第 2 字节
#define LD2412_REPORT_TAIL_2          0xF6u          // 主动上报帧尾第 3 字节
#define LD2412_REPORT_TAIL_3          0xF5u          // 主动上报帧尾第 4 字节

#define LD2412_FRAME_OVERHEAD         10u            // 帧头 4 字节 + 长度 2 字节 + 帧尾 4 字节

static uint16_t ld2412_le16(const uint8_t *data)      // 从串口小端字节中读取 16 位数据
{                                                     // 函数体开始
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8); // 低字节在前，高字节在后，符合协议小端格式
}                                                     // 函数体结束

static void ld2412_put_le16(uint8_t *data, uint16_t value) // 把 16 位数据写成串口小端格式
{                                                     // 函数体开始
    data[0] = (uint8_t)(value & 0xFFu);               // 写入低 8 位到串口低字节
    data[1] = (uint8_t)((value >> 8) & 0xFFu);        // 写入高 8 位到串口高字节
}                                                     // 函数体结束

static bool ld2412_has_cmd_head(const uint8_t *buf)   // 判断缓存开头是否为命令/ACK 帧头
{                                                     // 函数体开始
    return (buf[0] == LD2412_CMD_HEAD_0) &&           // 比较帧头第 1 字节
           (buf[1] == LD2412_CMD_HEAD_1) &&           // 比较帧头第 2 字节
           (buf[2] == LD2412_CMD_HEAD_2) &&           // 比较帧头第 3 字节
           (buf[3] == LD2412_CMD_HEAD_3);             // 比较帧头第 4 字节
}                                                     // 函数体结束

static bool ld2412_has_report_head(const uint8_t *buf) // 判断缓存开头是否为主动上报帧头
{                                                     // 函数体开始
    return (buf[0] == LD2412_REPORT_HEAD_0) &&        // 比较上报帧头第 1 字节
           (buf[1] == LD2412_REPORT_HEAD_1) &&        // 比较上报帧头第 2 字节
           (buf[2] == LD2412_REPORT_HEAD_2) &&        // 比较上报帧头第 3 字节
           (buf[3] == LD2412_REPORT_HEAD_3);          // 比较上报帧头第 4 字节
}                                                     // 函数体结束

static bool ld2412_has_cmd_tail(const uint8_t *tail)  // 判断命令/ACK 帧尾是否正确
{                                                     // 函数体开始
    return (tail[0] == LD2412_CMD_TAIL_0) &&          // 比较帧尾第 1 字节
           (tail[1] == LD2412_CMD_TAIL_1) &&          // 比较帧尾第 2 字节
           (tail[2] == LD2412_CMD_TAIL_2) &&          // 比较帧尾第 3 字节
           (tail[3] == LD2412_CMD_TAIL_3);            // 比较帧尾第 4 字节
}                                                     // 函数体结束

static bool ld2412_has_report_tail(const uint8_t *tail) // 判断主动上报帧尾是否正确
{                                                     // 函数体开始
    return (tail[0] == LD2412_REPORT_TAIL_0) &&       // 比较上报帧尾第 1 字节
           (tail[1] == LD2412_REPORT_TAIL_1) &&       // 比较上报帧尾第 2 字节
           (tail[2] == LD2412_REPORT_TAIL_2) &&       // 比较上报帧尾第 3 字节
           (tail[3] == LD2412_REPORT_TAIL_3);         // 比较上报帧尾第 4 字节
}                                                     // 函数体结束

static void ld2412_shift_left(uint8_t *buf, uint16_t *len) // 丢弃 1 个错误字节并左移缓存
{                                                     // 函数体开始
    uint16_t i = 0u;                                  // 定义循环变量，用于移动串口缓存
    if (*len == 0u) { return; }                       // 没有数据时不需要移动
    for (i = 1u; i < *len; i++)                       // 从第 2 个字节开始向前覆盖
    {                                                 // 循环体开始
        buf[i - 1u] = buf[i];                         // 把后续串口字节向前移动一位
    }                                                 // 循环体结束
    (*len)--;                                         // 缓存长度减少 1 字节
}                                                     // 函数体结束

static void ld2412_parse_ack(ld2412_t *dev, const uint8_t *data, uint16_t len) // 解析配置命令 ACK 帧内数据
{                                                     // 函数体开始
    ld2412_ack_t ack;                                 // 定义 ACK 结果结构体
    uint16_t i = 0u;                                  // 定义循环变量，用于复制 ACK 返回值
    if (len < 4u) { return; }                         // ACK 至少包含 2 字节命令字和 2 字节状态
    ack.command = (uint16_t)(ld2412_le16(&data[0]) & 0xFEFFu); // ACK 命令字清除 0x0100 应答标志后得到原命令
    ack.status = ld2412_le16(&data[2]);               // 读取 ACK 状态，0 成功，1 失败
    ack.payload_len = (uint16_t)(len - 4u);           // 返回值长度等于帧内数据减去命令字和状态
    if (ack.payload_len > LD2412_ACK_PAYLOAD_MAX)     // 判断返回值是否超过本地缓存
    {                                                 // 判断体开始
        ack.payload_len = LD2412_ACK_PAYLOAD_MAX;     // 超长时截断，防止串口数据覆盖内存
    }                                                 // 判断体结束
    for (i = 0u; i < ack.payload_len; i++)            // 遍历返回值字节
    {                                                 // 循环体开始
        ack.payload[i] = data[4u + i];                // 复制 ACK 返回值，例如版本号/MAC/参数
    }                                                 // 循环体结束
    if (dev->ack_cb != 0)                             // 判断用户是否注册 ACK 回调
    {                                                 // 判断体开始
        dev->ack_cb(&ack);                            // 通知应用层本次串口命令已有 ACK
    }                                                 // 判断体结束
}                                                     // 函数体结束

static void ld2412_parse_report(ld2412_t *dev, const uint8_t *data, uint16_t len) // 解析雷达主动上报帧内数据
{                                                     // 函数体开始
    ld2412_report_t report;                           // 定义上报结果结构体
    uint16_t i = 0u;                                  // 定义循环变量，用于复制距离门能量
    if (len < 11u) { return; }                        // 基础上报帧内数据长度应至少为 0x0B
    if (data[1] != 0xAAu) { return; }                 // 帧内数据第 2 字节必须为头部 0xAA
    if (data[len - 2u] != 0x55u) { return; }          // 帧内数据倒数第 2 字节必须为尾部 0x55
    report.data_type = data[0];                       // 读取数据类型，0x02 基础模式，0x01 工程模式
    report.state = (ld2412_target_state_t)data[2];    // 读取目标状态字节
    report.moving_distance_cm = ld2412_le16(&data[3]); // 读取运动目标距离，小端 2 字节，单位厘米
    report.moving_energy = data[5];                   // 读取运动目标能量值
    report.static_distance_cm = ld2412_le16(&data[6]); // 读取静止目标距离，小端 2 字节，单位厘米
    report.static_energy = data[8];                   // 读取静止目标能量值
    report.engineering_mode = false;                  // 默认按基础模式处理
    report.max_moving_gate = 0u;                      // 清空工程模式最大运动门
    report.max_static_gate = 0u;                      // 清空工程模式最大静止门
    report.light_value = 0u;                          // 清空工程模式光感值
    for (i = 0u; i < LD2412_GATE_COUNT; i++)          // 清空距离门能量数组
    {                                                 // 循环体开始
        report.moving_gate_energy[i] = 0u;            // 清空运动距离门能量
        report.static_gate_energy[i] = 0u;            // 清空静止距离门能量
    }                                                 // 循环体结束
    if ((report.data_type == 0x01u) && (len >= 43u))  // 工程模式帧示例长度为 0x2B，包含距离门能量和光感
    {                                                 // 判断体开始
        report.engineering_mode = true;               // 标记当前帧是工程模式串口数据
        report.max_moving_gate = data[9];             // 读取最大运动距离门
        report.max_static_gate = data[10];            // 读取最大静止距离门
        for (i = 0u; i < LD2412_GATE_COUNT; i++)      // 复制 14 个运动距离门能量
        {                                             // 循环体开始
            report.moving_gate_energy[i] = data[11u + i]; // 读取运动距离门 i 的能量
            report.static_gate_energy[i] = data[25u + i]; // 读取静止距离门 i 的能量
        }                                             // 循环体结束
        report.light_value = data[39];                // 读取光感测量值，0 到 255
    }                                                 // 判断体结束
    if (dev->report_cb != 0)                          // 判断用户是否注册上报回调
    {                                                 // 判断体开始
        dev->report_cb(&report);                      // 通知应用层收到一帧雷达目标数据
    }                                                 // 判断体结束
}                                                     // 函数体结束

static void ld2412_try_parse(ld2412_t *dev)           // 尝试从串口接收缓存中解析完整帧
{                                                     // 函数体开始
    bool is_cmd = false;                              // 记录当前缓存是否为 ACK 帧
    bool is_report = false;                           // 记录当前缓存是否为主动上报帧
    uint16_t data_len = 0u;                           // 保存帧内数据长度字段
    uint16_t total_len = 0u;                          // 保存整帧长度
    while (dev->rx_len >= 4u)                         // 至少收到 4 字节才可能判断帧头
    {                                                 // 循环体开始
        is_cmd = ld2412_has_cmd_head(dev->rx_buf);    // 检查是否匹配命令/ACK 帧头
        is_report = ld2412_has_report_head(dev->rx_buf); // 检查是否匹配主动上报帧头
        if (!is_cmd && !is_report)                    // 如果不是任何合法帧头
        {                                             // 判断体开始
            ld2412_shift_left(dev->rx_buf, &dev->rx_len); // 丢弃 1 字节，继续寻找串口帧头
            continue;                                 // 回到循环继续判断
        }                                             // 判断体结束
        if (dev->rx_len < 6u) { return; }             // 还没收到长度字段时等待更多串口字节
        data_len = ld2412_le16(&dev->rx_buf[4]);      // 读取帧内数据长度，小端 2 字节
        if (data_len > (uint16_t)(sizeof(dev->rx_buf) - LD2412_FRAME_OVERHEAD)) // 先校验再做加法，避免 16 位整数回绕
        {                                             // 判断体开始
            ld2412_shift_left(dev->rx_buf, &dev->rx_len); // 丢弃错误帧头，并继续寻找下一帧
            continue;                                 // 防止损坏长度触发越界读取
        }                                             // 判断体结束
        total_len = (uint16_t)(data_len + LD2412_FRAME_OVERHEAD); // 已确认加法不会溢出
        if (dev->rx_len < total_len) { return; }      // 未收满完整帧时继续等待串口数据
        if (is_cmd && ld2412_has_cmd_tail(&dev->rx_buf[6u + data_len])) // ACK 帧尾校验
        {                                             // 判断体开始
            ld2412_parse_ack(dev, &dev->rx_buf[6], data_len); // 解析 ACK 帧内数据
        }                                             // 判断体结束
        else if (is_report && ld2412_has_report_tail(&dev->rx_buf[6u + data_len])) // 上报帧尾校验
        {                                             // 判断体开始
            ld2412_parse_report(dev, &dev->rx_buf[6], data_len); // 解析雷达主动上报数据
        }                                             // 判断体结束
        else                                          // 帧尾不匹配说明串口数据错位或丢字节
        {                                             // 判断体开始
            ld2412_shift_left(dev->rx_buf, &dev->rx_len); // 丢弃 1 字节重新找帧头
            continue;                                 // 继续解析剩余缓存
        }                                             // 判断体结束
        dev->rx_len = 0u;                             // 当前完整帧已处理，清空缓存等待下一帧
    }                                                 // 循环体结束
}                                                     // 函数体结束

void ld2412_init(ld2412_t *dev, ld2412_uart_tx_fn uart_tx) // 初始化驱动上下文
{                                                     // 函数体开始
    uint16_t i = 0u;                                  // 定义循环变量，用于清空串口缓存
    if (dev == 0) { return; }                         // 防止空指针调用
    dev->uart_tx = uart_tx;                           // 保存 GD32 串口发送函数
    dev->report_cb = 0;                               // 默认没有上报回调
    dev->ack_cb = 0;                                  // 默认没有 ACK 回调
    dev->rx_len = 0u;                                 // 清空接收长度
    for (i = 0u; i < sizeof(dev->rx_buf); i++)        // 遍历接收缓存
    {                                                 // 循环体开始
        dev->rx_buf[i] = 0u;                          // 清空串口接收缓存字节
    }                                                 // 循环体结束
}                                                     // 函数体结束

void ld2412_set_report_callback(ld2412_t *dev, ld2412_report_cb cb) // 设置上报帧回调函数
{                                                     // 函数体开始
    if (dev == 0) { return; }                         // 防止空指针调用
    dev->report_cb = cb;                              // 保存应用层雷达数据处理函数
}                                                     // 函数体结束

void ld2412_set_ack_callback(ld2412_t *dev, ld2412_ack_cb cb) // 设置 ACK 回调函数
{                                                     // 函数体开始
    if (dev == 0) { return; }                         // 防止空指针调用
    dev->ack_cb = cb;                                 // 保存应用层命令应答处理函数
}                                                     // 函数体结束

void ld2412_input_byte(ld2412_t *dev, uint8_t byte)   // 输入 1 个 USART 接收字节
{                                                     // 函数体开始
    if (dev == 0) { return; }                         // 防止空指针调用
    if (dev->rx_len >= sizeof(dev->rx_buf))           // 判断接收缓存是否已满
    {                                                 // 判断体开始
        dev->rx_len = 0u;                             // 缓存满时清空，重新同步帧头
    }                                                 // 判断体结束
    dev->rx_buf[dev->rx_len] = byte;                  // 保存本次串口接收到的 1 字节
    dev->rx_len++;                                    // 接收长度加 1
    ld2412_try_parse(dev);                            // 每收到 1 字节都尝试解析完整帧
}                                                     // 函数体结束

bool ld2412_send_command(ld2412_t *dev, uint16_t command, const uint8_t *payload, uint16_t payload_len) // 发送通用命令帧
{                                                     // 函数体开始
    uint8_t frame[64];                                // 命令发送缓存，最大可覆盖 14 字节灵敏度命令
    uint16_t data_len = 0u;                           // 帧内数据长度，等于命令字 2 字节加命令值长度
    uint16_t total_len = 0u;                          // 整个串口发送帧长度
    uint16_t i = 0u;                                  // 循环变量，用于复制命令值
    if ((dev == 0) || (dev->uart_tx == 0)) { return false; } // 没有设备或发送函数时不能发串口
    if ((payload_len > 0u) && (payload == 0)) { return false; } // 有命令值长度却没有命令值指针则返回失败
    if (payload_len > (uint16_t)(sizeof(frame) - LD2412_FRAME_OVERHEAD - 2u)) { return false; } // 加法前限制负载，避免长度回绕和写越界
    data_len = (uint16_t)(2u + payload_len);          // 帧内数据 = 命令字 2 字节 + 命令值 N 字节
    total_len = (uint16_t)(data_len + LD2412_FRAME_OVERHEAD); // 总长度 = 帧头 + 长度 + 帧内数据 + 帧尾
    if (total_len > sizeof(frame)) { return false; }  // 防止发送缓存溢出
    frame[0] = LD2412_CMD_HEAD_0;                     // 写入命令帧头第 1 字节 FD
    frame[1] = LD2412_CMD_HEAD_1;                     // 写入命令帧头第 2 字节 FC
    frame[2] = LD2412_CMD_HEAD_2;                     // 写入命令帧头第 3 字节 FB
    frame[3] = LD2412_CMD_HEAD_3;                     // 写入命令帧头第 4 字节 FA
    ld2412_put_le16(&frame[4], data_len);             // 写入帧内数据长度，小端 2 字节
    ld2412_put_le16(&frame[6], command);              // 写入命令字，小端 2 字节
    for (i = 0u; i < payload_len; i++)                // 复制命令值字节
    {                                                 // 循环体开始
        frame[8u + i] = payload[i];                   // 写入命令值第 i 字节
    }                                                 // 循环体结束
    frame[8u + payload_len] = LD2412_CMD_TAIL_0;      // 写入命令帧尾第 1 字节 04
    frame[9u + payload_len] = LD2412_CMD_TAIL_1;      // 写入命令帧尾第 2 字节 03
    frame[10u + payload_len] = LD2412_CMD_TAIL_2;     // 写入命令帧尾第 3 字节 02
    frame[11u + payload_len] = LD2412_CMD_TAIL_3;     // 写入命令帧尾第 4 字节 01
    dev->uart_tx(frame, total_len);                   // 通过 GD32 USART 发送完整命令帧
    return true;                                      // 返回发送成功
}                                                     // 函数体结束

bool ld2412_enter_config(ld2412_t *dev)               // 发送使能配置命令
{                                                     // 函数体开始
    const uint8_t payload[2] = {0x01u, 0x00u};        // 命令值为 0x0001，小端发送 01 00
    return ld2412_send_command(dev, LD2412_CMD_ENTER_CONFIG, payload, 2u); // 发送 FD FC FB FA 04 00 FF 00 01 00 ...
}                                                     // 函数体结束

bool ld2412_exit_config(ld2412_t *dev)                // 发送结束配置命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_EXIT_CONFIG, 0, 0u); // 结束配置命令没有命令值
}                                                     // 函数体结束

bool ld2412_set_resolution(ld2412_t *dev, ld2412_resolution_t resolution) // 发送设置分辨率命令
{                                                     // 函数体开始
    uint8_t payload[6] = {0u, 0u, 0u, 0u, 0u, 0u};    // 分辨率命令值 6 字节，后 5 字节固定为 0
    payload[0] = (uint8_t)resolution;                 // 第 1 字节写入分辨率值 0/1/3
    return ld2412_send_command(dev, LD2412_CMD_SET_RESOLUTION, payload, 6u); // 发送设置分辨率命令帧
}                                                     // 函数体结束

bool ld2412_get_resolution(ld2412_t *dev)             // 发送读取分辨率命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_GET_RESOLUTION, 0, 0u); // 读取命令没有命令值
}                                                     // 函数体结束

bool ld2412_set_basic(ld2412_t *dev, uint8_t min_gate, uint8_t max_gate, uint16_t hold_s, bool out_low_when_present) // 设置基础参数
{                                                     // 函数体开始
    uint8_t payload[5];                               // 基础参数命令值共 5 字节
    payload[0] = min_gate;                            // 第 1 字节为最小距离门
    payload[1] = max_gate;                            // 第 2 字节为最大距离门
    ld2412_put_le16(&payload[2], hold_s);             // 第 3/4 字节为无人持续时间，单位秒，小端
    payload[4] = out_low_when_present ? 1u : 0u;      // 第 5 字节为 OUT 极性，0 有人高电平，1 有人低电平
    return ld2412_send_command(dev, LD2412_CMD_SET_BASIC, payload, 5u); // 发送基础参数配置帧
}                                                     // 函数体结束

bool ld2412_get_basic(ld2412_t *dev)                  // 发送读取基础参数命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_GET_BASIC, 0, 0u); // 读取命令没有命令值
}                                                     // 函数体结束

bool ld2412_engineering_on(ld2412_t *dev)             // 发送打开工程模式命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_ENGINEERING_ON, 0, 0u); // 工程模式命令没有命令值
}                                                     // 函数体结束

bool ld2412_engineering_off(ld2412_t *dev)            // 发送关闭工程模式命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_ENGINEERING_OFF, 0, 0u); // 关闭工程模式命令没有命令值
}                                                     // 函数体结束

bool ld2412_set_moving_sensitivity(ld2412_t *dev, const uint8_t sensitivity[LD2412_GATE_COUNT]) // 设置运动灵敏度
{                                                     // 函数体开始
    if (sensitivity == 0) { return false; }           // 灵敏度数组不能为空
    return ld2412_send_command(dev, LD2412_CMD_SET_MOVE_SENS, sensitivity, LD2412_GATE_COUNT); // 发送 14 字节运动灵敏度
}                                                     // 函数体结束

bool ld2412_get_moving_sensitivity(ld2412_t *dev)     // 发送读取运动灵敏度命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_GET_MOVE_SENS, 0, 0u); // 读取命令没有命令值
}                                                     // 函数体结束

bool ld2412_set_static_sensitivity(ld2412_t *dev, const uint8_t sensitivity[LD2412_GATE_COUNT]) // 设置静止灵敏度
{                                                     // 函数体开始
    if (sensitivity == 0) { return false; }           // 灵敏度数组不能为空
    return ld2412_send_command(dev, LD2412_CMD_SET_STATIC_SENS, sensitivity, LD2412_GATE_COUNT); // 发送 14 字节静止灵敏度
}                                                     // 函数体结束

bool ld2412_get_static_sensitivity(ld2412_t *dev)     // 发送读取静止灵敏度命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_GET_STATIC_SENS, 0, 0u); // 读取命令没有命令值
}                                                     // 函数体结束

bool ld2412_start_background_calibration(ld2412_t *dev) // 发送动态背景校正命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_BG_CALIBRATION, 0, 0u); // 校正命令没有命令值
}                                                     // 函数体结束

bool ld2412_get_background_state(ld2412_t *dev)       // 发送查询背景校正状态命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_GET_BG_STATE, 0, 0u); // 查询命令没有命令值
}                                                     // 函数体结束

bool ld2412_get_firmware_version(ld2412_t *dev)       // 发送读取固件版本命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_GET_VERSION, 0, 0u); // 读取版本命令没有命令值
}                                                     // 函数体结束

bool ld2412_set_baud(ld2412_t *dev, ld2412_baud_index_t baud) // 发送设置串口波特率命令
{                                                     // 函数体开始
    uint8_t payload[2];                               // 波特率索引命令值为 2 字节
    ld2412_put_le16(payload, (uint16_t)baud);         // 写入波特率索引，小端格式
    return ld2412_send_command(dev, LD2412_CMD_SET_BAUD, payload, 2u); // 发送波特率配置帧
}                                                     // 函数体结束

bool ld2412_factory_reset(ld2412_t *dev)              // 发送恢复出厂设置命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_FACTORY_RESET, 0, 0u); // 恢复出厂命令没有命令值
}                                                     // 函数体结束

bool ld2412_reboot(ld2412_t *dev)                     // 发送重启模块命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_REBOOT, 0, 0u); // 重启命令没有命令值
}                                                     // 函数体结束

bool ld2412_set_bluetooth(ld2412_t *dev, bool enable) // 发送蓝牙开关设置命令
{                                                     // 函数体开始
    uint8_t payload[2];                               // 蓝牙开关命令值为 2 字节
    ld2412_put_le16(payload, enable ? 0x0001u : 0x0000u); // 打开蓝牙按 PDF 示例发送 01 00，关闭蓝牙发送 00 00
    return ld2412_send_command(dev, LD2412_CMD_SET_BLUETOOTH, payload, 2u); // 发送蓝牙配置帧
}                                                     // 函数体结束

bool ld2412_get_mac(ld2412_t *dev)                    // 发送读取 MAC 地址命令
{                                                     // 函数体开始
    const uint8_t payload[2] = {0x01u, 0x00u};        // 查询 MAC 命令值为 0x0001，小端 01 00
    return ld2412_send_command(dev, LD2412_CMD_GET_MAC, payload, 2u); // 发送 MAC 查询命令帧
}                                                     // 函数体结束

bool ld2412_set_light_control(ld2412_t *dev, uint8_t mode, uint8_t threshold) // 设置光感辅助控制
{                                                     // 函数体开始
    uint8_t payload[2];                               // 光感控制命令值为 2 字节
    payload[0] = mode;                                // 第 1 字节为模式，0 关闭，1 小于阈值满足，2 大于阈值满足
    payload[1] = threshold;                           // 第 2 字节为光感阈值，范围 0 到 255
    return ld2412_send_command(dev, LD2412_CMD_SET_LIGHT_CTRL, payload, 2u); // 发送光感控制配置帧
}                                                     // 函数体结束

bool ld2412_get_light_control(ld2412_t *dev)          // 发送读取光感辅助控制命令
{                                                     // 函数体开始
    return ld2412_send_command(dev, LD2412_CMD_GET_LIGHT_CTRL, 0, 0u); // 查询命令没有命令值
}                                                     // 函数体结束
