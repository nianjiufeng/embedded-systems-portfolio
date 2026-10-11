#ifndef LD2412_H                                      // 防止头文件被重复包含
#define LD2412_H                                      // 定义头文件保护宏

#include <stdbool.h>                                  // 定义 bool 类型，表示串口解析结果是否有效
#include <stdint.h>                                   // 定义 uint8_t/uint16_t 等串口字节类型

#ifdef __cplusplus                                    // 判断是否被 C++ 工程包含
extern "C" {                                          // 使用 C 链接方式，方便 GD32 C/C++ 工程调用
#endif                                                // 结束 C++ 判断

#define LD2412_GATE_COUNT             14u             // LD2412 距离门数量，灵敏度和工程能量数组均为 14 字节
#define LD2412_REPORT_DATA_MAX        48u             // 雷达主动上报帧的最大帧内数据长度，工程模式示例为 0x2B
#define LD2412_ACK_PAYLOAD_MAX        32u             // ACK 返回值最大缓存长度，用于存放固件版本/MAC/参数等串口返回数据

#define LD2412_CMD_ENTER_CONFIG       0x00FFu         // 使能配置命令，发其他配置命令前必须先发
#define LD2412_CMD_EXIT_CONFIG        0x00FEu         // 结束配置命令，配置完成后让雷达回到工作模式
#define LD2412_CMD_SET_RESOLUTION     0x0001u         // 设置距离门分辨率命令，重启后生效
#define LD2412_CMD_GET_RESOLUTION     0x0011u         // 读取距离门分辨率命令
#define LD2412_CMD_SET_BASIC          0x0002u         // 设置基础参数命令，包含最小门/最大门/无人持续时间/OUT 极性
#define LD2412_CMD_GET_BASIC          0x0012u         // 读取基础参数命令
#define LD2412_CMD_ENGINEERING_ON     0x0062u         // 打开工程模式命令，上报帧追加各距离门能量
#define LD2412_CMD_ENGINEERING_OFF    0x0063u         // 关闭工程模式命令，上报帧只输出基础目标信息
#define LD2412_CMD_SET_MOVE_SENS      0x0003u         // 设置运动灵敏度命令，14 个距离门各 1 字节
#define LD2412_CMD_GET_MOVE_SENS      0x0013u         // 读取运动灵敏度命令
#define LD2412_CMD_SET_STATIC_SENS    0x0004u         // 设置静止灵敏度命令，14 个距离门各 1 字节
#define LD2412_CMD_GET_STATIC_SENS    0x0014u         // 读取静止灵敏度命令
#define LD2412_CMD_BG_CALIBRATION     0x000Bu         // 进入动态背景校正命令
#define LD2412_CMD_GET_BG_STATE       0x001Bu         // 查询动态背景校正状态命令
#define LD2412_CMD_GET_VERSION        0x00A0u         // 读取固件版本命令
#define LD2412_CMD_SET_BAUD           0x00A1u         // 设置串口波特率命令，重启后生效
#define LD2412_CMD_FACTORY_RESET      0x00A2u         // 恢复出厂设置命令，重启后生效
#define LD2412_CMD_REBOOT             0x00A3u         // 重启模块命令
#define LD2412_CMD_SET_BLUETOOTH      0x00A4u         // 蓝牙开关设置命令，重启后生效
#define LD2412_CMD_GET_MAC            0x00A5u         // 查询蓝牙 MAC 地址命令
#define LD2412_CMD_SET_LIGHT_CTRL     0x000Cu         // 设置光感辅助控制命令
#define LD2412_CMD_GET_LIGHT_CTRL     0x001Cu         // 查询光感辅助控制命令

typedef enum                                          // 定义目标状态枚举，来自上报帧目标状态字节
{                                                     // 枚举开始
    LD2412_TARGET_NONE = 0x00,                        // 无目标，串口上报状态值为 0x00
    LD2412_TARGET_MOVING = 0x01,                      // 运动目标，串口上报状态值为 0x01
    LD2412_TARGET_STATIC = 0x02,                      // 静止目标，串口上报状态值为 0x02
    LD2412_TARGET_MOVING_STATIC = 0x03,               // 运动和静止目标同时存在，串口上报状态值为 0x03
    LD2412_TARGET_BG_RUNNING = 0x04,                  // 正在底噪/背景检测，仅校正时有效
    LD2412_TARGET_BG_SUCCESS = 0x05,                  // 底噪/背景检测成功，仅校正时有效
    LD2412_TARGET_BG_FAILED = 0x06                    // 底噪/背景检测失败，仅校正时有效
} ld2412_target_state_t;                              // 枚举类型名称

typedef enum                                          // 定义距离门分辨率枚举，写入设置分辨率命令第 1 字节
{                                                     // 枚举开始
    LD2412_RES_75CM = 0u,                             // 0 表示 75cm/距离门，出厂常用配置
    LD2412_RES_50CM = 1u,                             // 1 表示 50cm/距离门
    LD2412_RES_20CM = 3u                              // 3 表示 20cm/距离门
} ld2412_resolution_t;                                // 枚举类型名称

typedef enum                                          // 定义串口波特率索引，写入设置波特率命令
{                                                     // 枚举开始
    LD2412_BAUD_9600 = 0x0001u,                       // 波特率索引 1，对应 9600
    LD2412_BAUD_19200 = 0x0002u,                      // 波特率索引 2，对应 19200
    LD2412_BAUD_38400 = 0x0003u,                      // 波特率索引 3，对应 38400
    LD2412_BAUD_57600 = 0x0004u,                      // 波特率索引 4，对应 57600
    LD2412_BAUD_115200 = 0x0005u,                     // 波特率索引 5，对应 115200，出厂默认
    LD2412_BAUD_230400 = 0x0006u,                     // 波特率索引 6，对应 230400
    LD2412_BAUD_256000 = 0x0007u,                     // 波特率索引 7，对应 256000
    LD2412_BAUD_460800 = 0x0008u                      // 波特率索引 8，对应 460800
} ld2412_baud_index_t;                                // 枚举类型名称

typedef struct                                        // 定义雷达主动上报数据结构
{                                                     // 结构体开始
    uint8_t data_type;                                // 上报数据类型，0x02 为基础信息，0x01 为工程模式信息
    ld2412_target_state_t state;                      // 目标状态，解析自目标数据第 1 字节
    uint16_t moving_distance_cm;                      // 运动目标距离，单位厘米，小端 2 字节
    uint8_t moving_energy;                            // 运动目标能量值，范围 0 到 100
    uint16_t static_distance_cm;                      // 静止目标距离，单位厘米，小端 2 字节
    uint8_t static_energy;                            // 静止目标能量值，范围 0 到 100
    bool engineering_mode;                            // 是否为工程模式上报帧
    uint8_t max_moving_gate;                          // 工程模式字段，最大运动距离门
    uint8_t max_static_gate;                          // 工程模式字段，最大静止距离门
    uint8_t moving_gate_energy[LD2412_GATE_COUNT];    // 工程模式字段，运动距离门 0 到 13 的能量值
    uint8_t static_gate_energy[LD2412_GATE_COUNT];    // 工程模式字段，静止距离门 0 到 13 的能量值
    uint8_t light_value;                              // 工程模式字段，光感测量值，0 到 255
} ld2412_report_t;                                    // 雷达上报数据结构名称

typedef struct                                        // 定义 ACK 应答数据结构
{                                                     // 结构体开始
    uint16_t command;                                 // 原始命令字，例如读取版本为 0x00A0
    uint16_t status;                                  // ACK 状态，0 表示成功，1 表示失败
    uint8_t payload[LD2412_ACK_PAYLOAD_MAX];          // ACK 返回值，去掉命令字和状态后的数据
    uint16_t payload_len;                             // ACK 返回值长度，单位字节
} ld2412_ack_t;                                       // ACK 应答数据结构名称

typedef void (*ld2412_uart_tx_fn)(const uint8_t *data, uint16_t len); // 串口发送回调，由 GD32 USART 实现
typedef void (*ld2412_report_cb)(const ld2412_report_t *report);      // 上报帧回调，解析到目标数据后调用
typedef void (*ld2412_ack_cb)(const ld2412_ack_t *ack);               // ACK 帧回调，收到命令应答后调用

typedef struct                                        // 定义驱动运行上下文
{                                                     // 结构体开始
    ld2412_uart_tx_fn uart_tx;                        // 串口发送函数指针，驱动通过它发命令帧
    ld2412_report_cb report_cb;                       // 雷达上报回调函数指针
    ld2412_ack_cb ack_cb;                             // 命令 ACK 回调函数指针
    uint8_t rx_buf[96];                               // 串口接收缓存，足够存放 ACK 帧和工程模式上报帧
    uint16_t rx_len;                                  // 当前接收缓存长度
} ld2412_t;                                           // 驱动上下文类型名称

void ld2412_init(ld2412_t *dev, ld2412_uart_tx_fn uart_tx);                             // 初始化驱动并绑定 GD32 串口发送函数
void ld2412_set_report_callback(ld2412_t *dev, ld2412_report_cb cb);                    // 设置雷达上报数据回调
void ld2412_set_ack_callback(ld2412_t *dev, ld2412_ack_cb cb);                          // 设置命令 ACK 回调
void ld2412_input_byte(ld2412_t *dev, uint8_t byte);                                    // 在 GD32 串口接收中断中喂入 1 个字节
bool ld2412_send_command(ld2412_t *dev, uint16_t command, const uint8_t *payload, uint16_t payload_len); // 发送任意命令帧
bool ld2412_enter_config(ld2412_t *dev);                                                // 发送使能配置命令
bool ld2412_exit_config(ld2412_t *dev);                                                 // 发送结束配置命令
bool ld2412_set_resolution(ld2412_t *dev, ld2412_resolution_t resolution);              // 设置距离门分辨率
bool ld2412_get_resolution(ld2412_t *dev);                                              // 读取距离门分辨率
bool ld2412_set_basic(ld2412_t *dev, uint8_t min_gate, uint8_t max_gate, uint16_t hold_s, bool out_low_when_present); // 设置基础参数
bool ld2412_get_basic(ld2412_t *dev);                                                   // 读取基础参数
bool ld2412_engineering_on(ld2412_t *dev);                                              // 打开工程模式
bool ld2412_engineering_off(ld2412_t *dev);                                             // 关闭工程模式
bool ld2412_set_moving_sensitivity(ld2412_t *dev, const uint8_t sensitivity[LD2412_GATE_COUNT]); // 设置运动灵敏度
bool ld2412_get_moving_sensitivity(ld2412_t *dev);                                      // 读取运动灵敏度
bool ld2412_set_static_sensitivity(ld2412_t *dev, const uint8_t sensitivity[LD2412_GATE_COUNT]); // 设置静止灵敏度
bool ld2412_get_static_sensitivity(ld2412_t *dev);                                      // 读取静止灵敏度
bool ld2412_start_background_calibration(ld2412_t *dev);                                // 启动动态背景校正
bool ld2412_get_background_state(ld2412_t *dev);                                        // 查询动态背景校正状态
bool ld2412_get_firmware_version(ld2412_t *dev);                                        // 读取固件版本
bool ld2412_set_baud(ld2412_t *dev, ld2412_baud_index_t baud);                          // 设置雷达串口波特率索引
bool ld2412_factory_reset(ld2412_t *dev);                                               // 恢复出厂设置
bool ld2412_reboot(ld2412_t *dev);                                                      // 重启雷达模块
bool ld2412_set_bluetooth(ld2412_t *dev, bool enable);                                  // 设置蓝牙开关
bool ld2412_get_mac(ld2412_t *dev);                                                     // 读取蓝牙 MAC 地址
bool ld2412_set_light_control(ld2412_t *dev, uint8_t mode, uint8_t threshold);           // 设置光感辅助控制
bool ld2412_get_light_control(ld2412_t *dev);                                           // 读取光感辅助控制

#ifdef __cplusplus                                    // 判断 C++ 包装是否需要结束
}                                                     // 结束 C 链接包装
#endif                                                // 结束 C++ 判断

#endif                                                // 结束头文件保护
