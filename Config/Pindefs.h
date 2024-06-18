#ifndef _PinDefs_
#define _PinDefs_

/***************************************************
整个工程所有的外设的引脚定义都在这里了，你可以按照
需要去修改。但是要注意，有些引脚是不可以修改的因为是
外设的输出引脚。
******* 目前已使用引脚  *******
PA：0 1 2 9 12 13 14
PB：0 1 2 3 4 8

******* 不可修改的引脚 *******
PA9：按键所在引脚，同时触发boot下载模式方便固件更新
PA12-13：预留给SWDIO debug port
PA14：预留给MCTM的PWM output channel#0
PB8：预留给MCTM的PWM output channel#3
***************************************************/

//侧按按键LED
#define LED_Green_IOBank C
#define LED_Green_IOPinNum 7 //绿色LED引脚定义（PC7）

#define LED_Red_IOBank B
#define LED_Red_IOPinNum 2 //红色LED引脚定义（PB2）

//按键
#define ExtKey_IOBank B
#define ExtKey_IOPN 12  //外部侧按按钮（PB12）

//二极管的数据控制脚
#define VDiode_IOBank B
#define VDiode_IOPinNum 13 //InputVD Pin=PB13

//电源管理部分控制MCU LDO自锁的引脚
#define LDO_EN_IOBank B
#define LDO_EN_IOPinNum 3 //LDOEN Pin=PB3

//IP2368的数据控制脚
#define IIC_SCL_IOBank B
#define IIC_SCL_IOPinNum 1 //SCL Pin=PB1

#define IIC_SDA_IOBank B
#define IIC_SDA_IOPinNum 0 //SDA Pin=PB0

#define IIC_DIR_IOBank A
#define IIC_DIR_IOPinNum 15 //DIR Pin=PA15

#define IP2368_INT_IOBank B
#define IP2368_INT_IOPinNum 14 //INT Pin=PB14
//屏幕I2C定义
#define OLED_SCL_IOBank A
#define OLED_SCL_IOPinNum 4 //OLED SCL Pin=PA4

#define OLED_SDA_IOBank A
#define OLED_SDA_IOPinNum 5 //OLED SDA Pin=PA5

#define OLED_RST_IOBank A
#define OLED_RST_IOPinNum 2 //OLED RST Pin=PA2

#endif
