/* LCD1602 八位接口：控制 PB5~PB7，数据 PB8~PB15。 */
#include "lcd1602.h"
void LCD_Write_byte(unsigned char value)
{
    LCD_RW(0);
    LCD_EN(0);
    GPIOB->BSRR = ((unsigned long)(~value & 0xFF) << 24) |
                  ((unsigned long)value << 8);
    HAL_Delay(1);
    LCD_EN(1);
    HAL_Delay(1);
    LCD_EN(0);
    HAL_Delay(1);
}
void LCD_write_cmd(unsigned char cmd)
{
    LCD_RS(0);
    LCD_Write_byte(cmd);
    if (cmd == 0x01 || cmd == 0x02) HAL_Delay(2);
}
void LCD_write_data(unsigned char value)
{
    LCD_RS(1);
    LCD_Write_byte(value);
}
void LCD_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOB, 0xFFE0, GPIO_PIN_RESET);
    gpio.Pin = 0xFFE0;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_Delay(40);
    LCD_write_cmd(0x38); HAL_Delay(5);
    LCD_write_cmd(0x38); HAL_Delay(1);
    LCD_write_cmd(0x38);
    LCD_write_cmd(0x08);
    LCD_write_cmd(0x01);
    LCD_write_cmd(0x06);
    LCD_write_cmd(0x0C);
}
void LCD_set_xy(unsigned char x, unsigned char y)
{
    LCD_write_cmd((y == 0 ? 0x80 : 0xC0) + x);
}
void LCD_write_string(unsigned char x, unsigned char y, char *s)
{
    LCD_set_xy(x, y);
    while (*s && x < 16)
    {
        LCD_write_data(*s++);
        x++;
    }
}
void LCD_wstring(unsigned char address, unsigned char *s)
{
    LCD_write_cmd(address);
    while (*s) LCD_write_data(*s++);
}
void lcd_delay_ms(unsigned int t) { HAL_Delay(t); }
void lcd_delay_us(unsigned int t)
{
    volatile unsigned int i;
    while (t--) for (i = 0; i < 8; i++) { }
}
