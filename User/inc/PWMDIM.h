#ifndef _PWMDIM_
#define _PWMDIM_

#include "Pindefs.h" //引脚定义
#include <stdbool.h>
#include "ht32.h"

//PWM引脚自动定义
#define PWMO_IOG STRCAT2(HT_GPIO,PWMO_IOBank)
#define PWMO_IOB STRCAT2(GPIO_P,PWMO_IOBank)
#define PWMO_IOP STRCAT2(GPIO_PIN_,PWMO_IOPinNum)

//定义
#define SYSHCLKFreq 48000000  //系统AHB频率48MHz
#define PWMFreq 20000 //PWM频率20Khz

//函数
void PWMTimerInit(void);//启动定时器
void SetPWMDuty(float Duty);//设置定时器输出值
void GenerateMainLEDStrobe(int count);//执照主LED闪烁

#endif
