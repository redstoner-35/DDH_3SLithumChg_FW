#ifndef _Oled_
#define _Oled_

//类型声明
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#include <stdbool.h>

//OLED休眠时间

#define OLEDSleepTimeOut 90 //OLED屏幕休眠时间

//OLED屏幕大小
#define OLEDHoriSize 64
#define OLEDVertSize 32 //OLED横向和竖向尺寸

//屏幕引脚定义
#define OLED_SCL_IOB STRCAT2(GPIO_P,OLED_SCL_IOBank)
#define OLED_SCL_IOG STRCAT2(HT_GPIO,OLED_SCL_IOBank)
#define OLED_SCL_IOP STRCAT2(GPIO_PIN_,OLED_SCL_IOPinNum) //SCL自动Define

#define OLED_SDA_IOB STRCAT2(GPIO_P,OLED_SDA_IOBank)
#define OLED_SDA_IOG STRCAT2(HT_GPIO,OLED_SDA_IOBank)
#define OLED_SDA_IOP STRCAT2(GPIO_PIN_,OLED_SDA_IOPinNum) //SDA自动Define

#define OLED_RST_IOB STRCAT2(GPIO_P,OLED_RST_IOBank)
#define OLED_RST_IOG STRCAT2(HT_GPIO,OLED_RST_IOBank)
#define OLED_RST_IOP STRCAT2(GPIO_PIN_,OLED_RST_IOPinNum) //RST自动Define

//屏幕操作定义
#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据

#define OLED_SCL_Clr() GPIO_ClearOutBits(OLED_SCL_IOG,OLED_SCL_IOP)//SCL
#define OLED_SCL_Set() GPIO_SetOutBits(OLED_SCL_IOG,OLED_SCL_IOP)

#define OLED_SDA_Clr() GPIO_ClearOutBits(OLED_SDA_IOG,OLED_SDA_IOP)//DIN
#define OLED_SDA_Set() GPIO_SetOutBits(OLED_SDA_IOG,OLED_SDA_IOP)

#define OLED_RES_Clr() GPIO_ClearOutBits(OLED_RST_IOG,OLED_RST_IOP)//RES
#define OLED_RES_Set() GPIO_SetOutBits(OLED_RST_IOG,OLED_RST_IOP)

//外部声明
extern unsigned char OLED_GRAM[OLEDHoriSize][OLEDVertSize/8];//OLED的显存

//硬件相关函数
void OLED_Init(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_SetBrightness(char Brightness);

//四舍五入处理
int iroundf(float IN);

//显示控件相关函数
void OLED_DrawPoint(char x,char y,char t);//画点
void OLED_DrawLine(char x1,char y1,char x2,char y2,char mode);//画线
void OLED_DrawCircle(char x,char y,char r);//画圆
void OLED_ShowPicture(char x,char y,char sizex,char sizey,const char *BMP,char mode);//显示图片
void OLED_ShowSingleMINIASCII(char x,char y,char Code,char mode);//显示单个5x5字符
void OLED_ShowMiniNum(char x,char y,char Number,char mode);//显示单个5x5数字
void OLED_ShowStringViaMiniASCII(char Startx,char Starty,char MaxmiumLenPerLine,char *StringIN,int len,int mode);//用5x5字符显示字符串
void OLED_Show3DigitInt(char x,char y,int Num);//用5x5字符显示三位整数
void OLED_ShowProgressBar(char x,char y,char Presentage);//显示5高进度条
void OLED_ShowFloatNum(float Num,char x,char y);//渲染3位浮点数字(0-999)
void OLED_Fill(char x,char y,char sizex,char sizey,char mode);//填充指定区域
void OLED_Printf(char Startx,char Starty,char MaxmiumLenPerLine,char mode,char *Format,...);//OLED输出
void OLED_Printfn(char Startx,char Starty,char MaxmiumLenPerLine,int n,char mode,char *Format,...);//OLED输出指定数值
void OLED_Show4DigitInt(char x,char y,int Num);//显示四位整数
void OLED_DisplayButton(char x,char y,char *Text,bool IsSelect);//显示按钮
void OLED_DrawRectangle(char x1,char y1,char x2,char y2,char mode);//显示空心长方形
void OLED_ImageDisappear(void);//屏幕消失特效
void OLED_OldTVFade(void);//OLED老电视消失效果
bool OLED_CheckIsGRAMHasImage(void);//检查显存里面是否还包含数据

#endif
