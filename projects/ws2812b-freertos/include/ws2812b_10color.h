#ifndef __WS2812B_10COLOR_H
#define __WS2812B_10COLOR_H

#include "gd32h7xx.h"

/* 灯带上一共有 10 颗 WS2812B 灯珠。 */
#define WS2812B_LED_COUNT             10U

/* 全局亮度，范围 0-255；数值越大越亮。 */
#define WS2812B_DEFAULT_BRIGHTNESS    64U

/* 调试开关：1 = 只翻转引脚，用万用表/示波器确认 PF5 是否有输出。 */
#define WS2812B_PIN_TEST_MODE         0U

/* 调试开关：1 = 输出固定波形，用示波器看 WS2812B 时序。 */
#define WS2812B_WAVEFORM_TEST_MODE    0U

/* 这些变量放到 Watch 窗口里看，方便判断程序有没有跑到对应阶段。 */
extern volatile uint32_t ws2812b_debug_stage;
extern volatile uint32_t ws2812b_debug_dwt_ready;
extern volatile uint32_t ws2812b_debug_refresh_count;
extern volatile uint32_t ws2812b_debug_cycles_t0h;
extern volatile uint32_t ws2812b_debug_cycles_t1h;
extern volatile uint32_t ws2812b_debug_cycles_total;
extern volatile uint32_t ws2812b_debug_gpio_octl;
extern volatile uint32_t ws2812b_debug_gpio_istat;
extern volatile uint32_t ws2812b_debug_pin_level;

/*
 * .h 文件只放“别人需要调用的函数声明”。
 * main.c 只要 include 这个头文件，就能调用下面这些函数。
 */
void ws2812b_init(void);
void ws2812b_set_brightness(uint8_t brightness);
void ws2812b_set_pixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
void ws2812b_set_all(uint8_t red, uint8_t green, uint8_t blue);
void ws2812b_clear(void);
void ws2812b_refresh(void);
void ws2812b_pin_test_toggle(void);

#endif
