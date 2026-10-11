/*
 * 这个文件不是完整 main.c，只展示 WS2812B 任务怎么写。
 * 放到你的工程里时，需要：
 * 1. main.c include "ws2812b_10color.h"
 * 2. main() 里调用 ws2812b_init()
 * 3. FreeRTOS 创建 ws2812b_task()
 */

#include "ws2812b_10color.h"
#include "FreeRTOS.h"
#include "task.h"

volatile uint32_t ws2812b_loop_count;

void ws2812b_task(void *pvParameters)
{
    uint8_t offset = 0U;

    /*
     * 10 个颜色，和 10 颗灯一一对应。
     * 每次循环 offset 加 1，就会让颜色整体向前移动一格。
     */
    static const uint8_t colors[WS2812B_LED_COUNT][3] = {
        {255U, 0U,   0U},   /* 红 */
        {255U, 80U,  0U},   /* 橙红 */
        {255U, 180U, 0U},   /* 橙黄 */
        {255U, 255U, 0U},   /* 黄 */
        {0U,   255U, 0U},   /* 绿 */
        {0U,   255U, 120U}, /* 青绿 */
        {0U,   255U, 255U}, /* 青 */
        {0U,   80U,  255U}, /* 蓝 */
        {80U,  0U,   255U}, /* 紫 */
        {255U, 0U,   255U}  /* 洋红 */
    };

    (void)pvParameters;

    ws2812b_set_brightness(96U);

    while(1)
    {
        ws2812b_loop_count++;

#if (WS2812B_PIN_TEST_MODE != 0U)
        ws2812b_pin_test_toggle();
#elif (WS2812B_WAVEFORM_TEST_MODE != 0U)
        ws2812b_set_all(0xAAU, 0x55U, 0xAAU);
        ws2812b_refresh();
#else
        uint8_t index;

        for(index = 0U; index < WS2812B_LED_COUNT; index++)
        {
            uint8_t color_index = (uint8_t)((index + offset) % WS2812B_LED_COUNT);

            ws2812b_set_pixel(index,
                              colors[color_index][0],
                              colors[color_index][1],
                              colors[color_index][2]);
        }

        ws2812b_refresh();

        offset++;
        if(offset >= WS2812B_LED_COUNT)
        {
            offset = 0U;
        }
#endif

        vTaskDelay(pdMS_TO_TICKS(180));
    }
}

/*
 * main() 里大概这样调用：
 *
 * ws2812b_init();
 * ws2812b_set_brightness(WS2812B_DEFAULT_BRIGHTNESS);
 * xTaskCreate(ws2812b_task, "ws2812b", 512, NULL, 1, &Ws2812bTaskHandle);
 */
