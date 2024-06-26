#ifndef LEDMgmt_
#define LEDMgmt_

#include "Pindefs.h"
#include <stdbool.h>

//处理LED GPIO的自动define
#define LED_Green_IOP STRCAT2(GPIO_PIN_,LED_Green_IOPinNum)
#define LED_Green_IOB STRCAT2(GPIO_P,LED_Green_IOBank)
#define LED_Green_IOG STRCAT2(HT_GPIO,LED_Green_IOBank)

#define LED_Red_IOB STRCAT2(GPIO_P,LED_Red_IOBank)
#define LED_Red_IOG STRCAT2(HT_GPIO,LED_Red_IOBank)
#define LED_Red_IOP STRCAT2(GPIO_PIN_,LED_Red_IOPinNum)

//函数
void LED_Init(void);
void LEDMgmt_CallBack(void);
void LED_Reset(void);
void LED_AddStrobe(int count,const char *ColorStr);//在自定义闪缓存加上strobe
void LED_ShowLoopOperationOnce(int index); //仅一次显示循环的LED内容 

//外部变量
extern volatile int CurrentLEDIndex;//给外部函数设置LED状态
extern char *ExtLEDIndex; //用于传入的外部序列
extern char LEDModeStr[64]; //LED模式的字符串

#endif
