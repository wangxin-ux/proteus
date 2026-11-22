/* 图中 C1~C4 接 PA8、PA7、PA6、PA5；R1~R4 接 PA9~PA12。 */
#include "key4x4.h"

static const unsigned short col_pin[4] = {
    GPIO_PIN_8, GPIO_PIN_7, GPIO_PIN_6, GPIO_PIN_5
};
static const unsigned short row_pin[4] = {
    GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12
};
static const unsigned char key_table[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

void Key_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5 | GPIO_PIN_6 |
                     GPIO_PIN_7 | GPIO_PIN_8, GPIO_PIN_SET);
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio);
}

/* 逐列扫描，一次只按一个键；多键同时按下不作输入。 */
static unsigned char Read_Key(void)
{
    unsigned char row, col, count = 0, key = 0;
    volatile unsigned char delay;
    for (col = 0; col < 4; col++)
    {
        HAL_GPIO_WritePin(GPIOA, col_pin[col], GPIO_PIN_RESET);
        for (delay = 0; delay < 20; delay++) { }
        for (row = 0; row < 4; row++)
        {
            if (HAL_GPIO_ReadPin(GPIOA, row_pin[row]) == GPIO_PIN_RESET)
            {
                key = key_table[row][col];
                count++;
            }
        }
        HAL_GPIO_WritePin(GPIOA, col_pin[col], GPIO_PIN_SET);
    }
    if (count == 1) return key;
    return count ? 0xFF : 0;
}

/* Active-low, open-drain columns avoid opposing push-pull outputs when
 * multiple switches are pressed. 0xFF means ambiguous/multiple keys.
 * Require a stable full release before accepting another press. */
unsigned char Key_Scan(void)
{
    static unsigned char last = 0, latched = 0;
    static uint32_t change_time = 0;
    unsigned char key = Read_Key();
    uint32_t now = HAL_GetTick();
    if (key != last) {
        last = key;
        change_time = now;
    }
    if (key == 0xFF) latched = 1;
    if ((uint32_t)(now - change_time) >= 20U) {
        if (key == 0) latched = 0;
        else if (!latched) {
            latched = 1;
            return key;
        }
    }
    return 0;
}
