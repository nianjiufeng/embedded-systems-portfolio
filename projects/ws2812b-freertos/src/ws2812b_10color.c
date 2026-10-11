#include "ws2812b_10color.h"

/* 这里集中定义硬件连接：本实验 DIN 接到 PF5。 */
#define WS2812B_GPIO_PORT             GPIOF
#define WS2812B_GPIO_CLK              RCU_GPIOF
#define WS2812B_GPIO_PIN              GPIO_PIN_5

/*
 * WS2812B 是单总线协议，一个 bit 大约 1.2us。
 * 0 码：高电平短一点；1 码：高电平长一点。
 */
#define WS2812B_T0H_NS                300U
#define WS2812B_T1H_NS                650U
#define WS2812B_BIT_TOTAL_NS          1200U
#define WS2812B_RESET_US              80U

/* 用结构体表示一个 RGB 颜色。 */
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} ws2812b_color_t;

/* 像素缓存：先把 10 颗灯的颜色存在数组里，再统一 refresh 发出去。 */
static ws2812b_color_t ws2812b_pixels[WS2812B_LED_COUNT];

/* 当前亮度，set_pixel 存原始 RGB，refresh 发送时再统一乘亮度。 */
static uint8_t ws2812b_brightness = WS2812B_DEFAULT_BRIGHTNESS;

/* 根据 SystemCoreClock 算出来的精确延时周期数。 */
static uint32_t ws2812b_cycles_t0h;
static uint32_t ws2812b_cycles_t1h;
static uint32_t ws2812b_cycles_total;

/* Watch 调试变量。 */
volatile uint32_t ws2812b_debug_stage;
volatile uint32_t ws2812b_debug_dwt_ready;
volatile uint32_t ws2812b_debug_refresh_count;
volatile uint32_t ws2812b_debug_cycles_t0h;
volatile uint32_t ws2812b_debug_cycles_t1h;
volatile uint32_t ws2812b_debug_cycles_total;
volatile uint32_t ws2812b_debug_gpio_octl;
volatile uint32_t ws2812b_debug_gpio_istat;
volatile uint32_t ws2812b_debug_pin_level;

/* 把纳秒换算成 CPU 周期数，用于精确延时。 */
static uint32_t ws2812b_ns_to_cycles(uint32_t ns)
{
    return (uint32_t)((((uint64_t)SystemCoreClock * ns) + 999999999ULL) / 1000000000ULL);
}

/* 把微秒换算成 CPU 周期数，用于 reset 低电平延时。 */
static uint32_t ws2812b_us_to_cycles(uint32_t us)
{
    return (uint32_t)((((uint64_t)SystemCoreClock * us) + 999999ULL) / 1000000ULL);
}

/* 打开 Cortex-M 的 DWT 计数器，用它做很短的精确延时。 */
static void ws2812b_dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/* 检查 DWT 计数器是否真的在走。 */
static uint8_t ws2812b_dwt_check(void)
{
    volatile uint32_t delay;
    uint32_t start = DWT->CYCCNT;

    for(delay = 0U; delay < 1000U; delay++) {
    }

    return (DWT->CYCCNT != start) ? 1U : 0U;
}

/* 延时指定 CPU 周期数。 */
static void ws2812b_delay_cycles(uint32_t cycles)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t guard = 0U;

    while((uint32_t)(DWT->CYCCNT - start) < cycles) {
        guard++;
        if(guard > (cycles + 10000U)) {
            break;
        }
    }
}

/* 根据全局亮度缩放单个颜色通道。 */
static uint8_t ws2812b_apply_brightness(uint8_t value)
{
    return (uint8_t)(((uint16_t)value * ws2812b_brightness) / 255U);
}

/* 发送 1 个 bit：先拉高，再按 0/1 不同宽度拉低。 */
__STATIC_FORCEINLINE void ws2812b_send_bit(uint8_t bit_value)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t guard = 0U;
    uint32_t high_cycles = bit_value ? ws2812b_cycles_t1h : ws2812b_cycles_t0h;

    GPIO_BOP(WS2812B_GPIO_PORT) = WS2812B_GPIO_PIN;
    while((uint32_t)(DWT->CYCCNT - start) < high_cycles) {
        guard++;
        if(guard > 10000U) {
            break;
        }
    }

    GPIO_BC(WS2812B_GPIO_PORT) = WS2812B_GPIO_PIN;
    while((uint32_t)(DWT->CYCCNT - start) < ws2812b_cycles_total) {
        guard++;
        if(guard > 20000U) {
            break;
        }
    }
}

/* 发送 1 个字节，高位先发。 */
static void ws2812b_send_byte(uint8_t data)
{
    uint8_t mask;

    for(mask = 0x80U; mask != 0U; mask >>= 1U) {
        ws2812b_send_bit((data & mask) ? 1U : 0U);
    }
}

/* 初始化 GPIO、DWT、时序周期，并清空灯带。 */
void ws2812b_init(void)
{
    ws2812b_debug_stage = 1U;
    rcu_periph_clock_enable(WS2812B_GPIO_CLK);

    gpio_mode_set(WS2812B_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, WS2812B_GPIO_PIN);
    gpio_output_options_set(WS2812B_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, WS2812B_GPIO_PIN);
    GPIO_BC(WS2812B_GPIO_PORT) = WS2812B_GPIO_PIN;
    ws2812b_debug_stage = 2U;

    ws2812b_dwt_init();
    ws2812b_debug_dwt_ready = ws2812b_dwt_check();
    ws2812b_debug_stage = 3U;

    ws2812b_cycles_t0h = ws2812b_ns_to_cycles(WS2812B_T0H_NS);
    ws2812b_cycles_t1h = ws2812b_ns_to_cycles(WS2812B_T1H_NS);
    ws2812b_cycles_total = ws2812b_ns_to_cycles(WS2812B_BIT_TOTAL_NS);
    ws2812b_debug_cycles_t0h = ws2812b_cycles_t0h;
    ws2812b_debug_cycles_t1h = ws2812b_cycles_t1h;
    ws2812b_debug_cycles_total = ws2812b_cycles_total;
    ws2812b_debug_stage = 4U;

    ws2812b_clear();
    ws2812b_refresh();
    ws2812b_debug_stage = 5U;
}

/* 设置全局亮度，0 最暗，255 最亮。 */
void ws2812b_set_brightness(uint8_t brightness)
{
    ws2812b_brightness = brightness;
}

/* 设置某一颗灯的 RGB 颜色。 */
void ws2812b_set_pixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue)
{
    if(index >= WS2812B_LED_COUNT) {
        return;
    }

    ws2812b_pixels[index].red = red;
    ws2812b_pixels[index].green = green;
    ws2812b_pixels[index].blue = blue;
}

/* 设置全部灯珠为同一种颜色。 */
void ws2812b_set_all(uint8_t red, uint8_t green, uint8_t blue)
{
    uint16_t index;

    for(index = 0U; index < WS2812B_LED_COUNT; index++) {
        ws2812b_set_pixel(index, red, green, blue);
    }
}

/* 全部灯熄灭。 */
void ws2812b_clear(void)
{
    ws2812b_set_all(0U, 0U, 0U);
}

/* 慢速翻转 PF5，用于确认硬件引脚有没有接对。 */
void ws2812b_pin_test_toggle(void)
{
    gpio_bit_toggle(WS2812B_GPIO_PORT, WS2812B_GPIO_PIN);
    ws2812b_debug_gpio_octl = GPIO_OCTL(WS2812B_GPIO_PORT);
    ws2812b_debug_gpio_istat = GPIO_ISTAT(WS2812B_GPIO_PORT);
    ws2812b_debug_pin_level = (ws2812b_debug_gpio_istat & WS2812B_GPIO_PIN) ? 1U : 0U;
}

/*
 * 把缓存里的 10 颗灯颜色真正发到灯带。
 * 注意 WS2812B 的顺序是 GRB，不是 RGB。
 */
void ws2812b_refresh(void)
{
    uint16_t index;
    uint32_t primask;

    if(ws2812b_debug_dwt_ready == 0U) {
        return;
    }

    ws2812b_debug_stage = 10U;
    primask = __get_PRIMASK();
    __disable_irq();

    for(index = 0U; index < WS2812B_LED_COUNT; index++) {
        ws2812b_send_byte(ws2812b_apply_brightness(ws2812b_pixels[index].green));
        ws2812b_send_byte(ws2812b_apply_brightness(ws2812b_pixels[index].red));
        ws2812b_send_byte(ws2812b_apply_brightness(ws2812b_pixels[index].blue));
    }

    GPIO_BC(WS2812B_GPIO_PORT) = WS2812B_GPIO_PIN;
    __set_PRIMASK(primask);

    ws2812b_delay_cycles(ws2812b_us_to_cycles(WS2812B_RESET_US));
    ws2812b_debug_refresh_count++;
    ws2812b_debug_stage = 11U;
}
