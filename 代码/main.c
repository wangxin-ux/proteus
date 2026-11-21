/*
 * 51单片机万年历 —— Keil uVision5 / C51 工程
 * 芯片：AT89C51，主晶振12MHz；DS1302晶振32.768kHz。
 *
 * 同学1负责：DS1302读写、日期合法性、星期、闰年/大小月、保存时间。
 * 对应硬件：U2 DS1302、X2 32.768kHz晶振、B1备用电池。
 * 接线：I/O=P1.0，SCLK=P1.1，RST/CE=P1.2。
 *
 * 同学2负责：LCD1602初始化、只刷新变化字符防闪烁、按键扫描/消抖。
 * 对应硬件：LCD1、RP1上拉排阻、K1~K4；U3采用轮询，不用外部中断。
 * 接线：LCD数据=P0；RS=P2.0，RW=P2.1，E=P2.2。
 *       K1=P3.4选择，K2=P3.5加，K3=P3.6减，K4=P3.7保存。
 *
 * 公共部分：U1 AT89C51、X1 12MHz、复位电路；main()连接两人功能。
 * K1依次选择年/月/日/时/分/秒，K4保存后从SET返回RUN。
 */
#include <REG51.H>
#include <INTRINS.H>
typedef unsigned char uint8_t;
typedef unsigned int uint16_t;

/* 函数声明：供两位同学的功能和公共主程序互相调用。 */
void delay_us(uint16_t us);
void delay_ms(uint16_t ms);
void read_time(void);
void save_time(void);
void adjust(uint8_t plus);
uint8_t valid_time(void);
uint8_t weekday(void);
uint8_t rtc_read(uint8_t addr);
void rtc_init_pins(void);
void lcd_init(void);
void display(void);
uint8_t key(void);
uint8_t editing=0;

/* ========== 同学1：时钟与日历功能开始 ========== */
/* 同学1：DS1302时钟驱动、日历算法、时间保存与日期边界处理。
 * 对应硬件：U2 DS1302、X2 32.768kHz晶振、B1备用电池。
 * I/O=P1.0, SCLK=P1.1, CE=P1.2。主控程序负责调用本模块。
 */
sbit RTC_IO = P1^0;
sbit RTC_CLK = P1^1;
sbit RTC_CE = P1^2;
uint8_t time[7];
uint8_t code fields[6] = {6,4,3,2,1,0};
void rtc_init_pins(void) {RTC_CE=0;RTC_CLK=0;RTC_IO=1;}
void rtc_send(uint8_t value) {
    uint8_t i;
    for(i=0;i<8;i++) {
        RTC_IO=value&1; RTC_CLK=1;
        _nop_();
        RTC_CLK=0; value>>=1;
    }
}
void rtc_write(uint8_t addr,uint8_t value) {
    RTC_CE=0; RTC_CLK=0; RTC_CE=1;
    rtc_send(addr); rtc_send(value); RTC_CE=0; RTC_IO=1;
}
uint8_t rtc_read(uint8_t addr) {
    uint8_t value=0,i;
    RTC_CE=0; RTC_CLK=0; RTC_CE=1;
    rtc_send(addr); RTC_IO=1;
    for(i=0;i<8;i++) {
        if(RTC_IO) value|=(1u<<i);
        RTC_CLK=1; _nop_(); RTC_CLK=0;
    }
    RTC_CE=0; return value;
}
uint8_t bcd(uint8_t n) { return ((n/10)<<4)|(n%10); }
uint8_t dec(uint8_t n) { return (n>>4)*10+(n&15); }
uint8_t month_days(void) {
    uint8_t m=time[4];
    if(m==2) return (time[6]%4==0)?29:28;
    if(m==4 || m==6 || m==9 || m==11) return 30;
    return 31;
}
uint8_t weekday(void) {
    uint16_t y=2000u+time[6];
    uint8_t m=time[4];
    static uint8_t code offset[12]={0,3,2,5,0,3,5,1,4,6,2,4};
    if(m<3) y--;
    return (y+y/4-y/100+y/400+offset[m-1]+time[3])%7+1;
}
void read_time(void) {
    uint8_t i, first;
    /* Retry when the seconds roll over during a register read. */
    do {
        first=rtc_read(0x81)&0x7f;
        for(i=0;i<7;i++) time[i]=dec(rtc_read(0x81+2*i));
    } while(first!=(rtc_read(0x81)&0x7f));
}
uint8_t valid_time(void) {
    if(time[6]>99 || time[4]<1 || time[4]>12 || time[3]<1) return 0;
    if(time[3]>month_days() || time[2]>23 || time[1]>59 || time[0]>59) return 0;
    return 1;
}
void save_time(void) {
    uint8_t i;
    time[5]=weekday();
    rtc_write(0x8e,0); rtc_write(0x80,bcd(time[0])|0x80);
    for(i=1;i<7;i++) rtc_write(0x80+2*i,bcd(time[i]));
    rtc_write(0x80,bcd(time[0])); rtc_write(0x8e,0x80);
}
void adjust(uint8_t plus) {
    uint8_t idx=fields[editing-1],lo=0,hi=59;
    if(idx==6) hi=99;
    else if(idx==4) {lo=1;hi=12;}
    else if(idx==3) {lo=1;hi=month_days();}
    else if(idx==2) hi=23;
    if(plus) time[idx]=(time[idx]>=hi)?lo:time[idx]+1;
    else time[idx]=(time[idx]<=lo)?hi:time[idx]-1;
    if(time[3]>month_days()) time[3]=month_days();
}

/* ========== 同学1功能结束 ========== */

/* ========== 同学2：显示与按键功能开始 ========== */
/* 同学2：LCD1602显示、仅更新改变字符、防闪烁、四按键消抖。
 * 对应硬件：LCD1、RP1上拉排阻、K1~K4；与门U3无需中断驱动。
 * LCD D0~D7=P0, RS=P2.0, RW=P2.1, E=P2.2。
 * K1=P3.4, K2=P3.5, K3=P3.6, K4=P3.7。
 */
sbit LCD_RS = P2^0;
sbit LCD_RW = P2^1;
sbit LCD_E = P2^2;
sbit KEY_SELECT = P3^4;
sbit KEY_PLUS = P3^5;
sbit KEY_MINUS = P3^6;
sbit KEY_SAVE = P3^7;
static uint8_t cache[32];
static char code layout[]="20  -  -          :  :          ";
static char code names[]="SUNMONTUEWEDTHUFRISAT";
static char code status[]="RUN ";
void lcd_write(uint8_t value,uint8_t data_mode) {
    LCD_E=0;LCD_RW=0;LCD_RS=data_mode;P0=value;
    _nop_();
    LCD_E=1;_nop_();_nop_();
    LCD_E=0;
    if(!data_mode && (value==1 || value==2)) delay_ms(2);
    else delay_us(50);
}
void lcd_init(void) {
    uint8_t i;
    LCD_E=0;LCD_RW=0;delay_ms(40);
    lcd_write(0x30,0);delay_ms(5);
    lcd_write(0x30,0);delay_ms(1);lcd_write(0x30,0);
    lcd_write(0x38,0);lcd_write(0x0c,0);lcd_write(0x06,0);lcd_write(1,0);
    for(i=0;i<32;i++) cache[i]=0xff;
}
uint8_t display_char(uint8_t p,uint8_t w) {
    uint8_t n;
    if(p==2 || p==3) n=time[6];
    else if(p==5 || p==6) n=time[4];
    else if(p==8 || p==9) n=time[3];
    else if(p==16 || p==17) n=time[2];
    else if(p==19 || p==20) n=time[1];
    else if(p==22 || p==23) n=time[0];
    else if(p>=12 && p<=14) return names[(w-1)*3+p-12];
    else if(p>=27 && p<=30) {
        if(!editing) return status[p-27];
        if(p==27) return 'S';if(p==28) return 'E';
        if(p==29) return 'T';return '0'+editing;
    } else return layout[p];
    if(p==2 || p==5 || p==8 || p==16 || p==19 || p==22) return '0'+n/10;
    return '0'+n%10;
}
void display(void) {
    uint8_t p,c,w=weekday();
    for(p=0;p<32;p++) {
        c=display_char(p,w);
        if(c!=cache[p]) {
            lcd_write(p<16?0x80+p:0xc0+p-16,0);
            lcd_write(c,1);cache[p]=c;
        }
    }
}
uint8_t key(void) {
    uint8_t k=0;
    if(!KEY_SELECT) k=1;
    else if(!KEY_PLUS) k=2;
    else if(!KEY_MINUS) k=3;
    else if(!KEY_SAVE) k=4;
    if(!k) return 0;
    delay_ms(20);
    if((k==1 && KEY_SELECT)||(k==2 && KEY_PLUS)||
       (k==3 && KEY_MINUS)||(k==4 && KEY_SAVE)) return 0;
    while(!KEY_SELECT || !KEY_PLUS || !KEY_MINUS || !KEY_SAVE) delay_ms(1);
    delay_ms(20); return k;
}

/* ========== 同学2功能结束 ========== */

/* 公共延时和主循环：Timer0采用12分频，每个计数1us。 */
void delay_us(uint16_t us) {
    uint16_t reload=0u-us;
    TR0=0;TF0=0;TH0=reload>>8;TL0=reload&0xff;
    TR0=1;while(!TF0);TR0=0;TF0=0;
}
void delay_ms(uint16_t ms) {while(ms--) delay_us(1000);}
void main(void) {
    uint8_t k;
    EA=0;TMOD=(TMOD&0xf0)|1;P0=0xff;P1=0xff;P3=0xff;
    rtc_init_pins();lcd_init();read_time();
    if((rtc_read(0x81)&0x80) || !valid_time()) {
        time[0]=0;time[1]=0;time[2]=12;time[3]=21;time[4]=11;time[6]=26;
        save_time();
    }
    while(1) {
        if(!editing) read_time();
        k=key();
        if(k==1) {editing++;if(editing>6) editing=1;}
        else if(k==4 && editing) {save_time();editing=0;}
        else if(editing && (k==2 || k==3)) adjust(k==2);
        display();delay_ms(10);
    }
}
