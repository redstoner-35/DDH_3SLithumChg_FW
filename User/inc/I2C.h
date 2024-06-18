#ifndef _I2C_
#define _I2C_

//内部包含
#include "Pindefs.h"

//I2C总线操作自动define，禁止修改！
#define IIC_SCL_IOB STRCAT2(GPIO_P,IIC_SCL_IOBank)
#define IIC_SCL_IOG STRCAT2(HT_GPIO,IIC_SCL_IOBank)
#define IIC_SCL_IOP STRCAT2(GPIO_PIN_,IIC_SCL_IOPinNum) //SCL自动Define

#define IIC_SDA_IOB STRCAT2(GPIO_P,IIC_SDA_IOBank)
#define IIC_SDA_IOG STRCAT2(HT_GPIO,IIC_SDA_IOBank)
#define IIC_SDA_IOP STRCAT2(GPIO_PIN_,IIC_SDA_IOPinNum) //SDA自动Define

#define IIC_DIR_IOB STRCAT2(GPIO_P,IIC_DIR_IOBank)
#define IIC_DIR_IOG STRCAT2(HT_GPIO,IIC_DIR_IOBank)
#define IIC_DIR_IOP STRCAT2(GPIO_PIN_,IIC_DIR_IOPinNum) //DIR自动Define

#define IIC_SDA_Set() SetSDAOD(true)
#define IIC_SDA_Clr() SetSDAOD(false)
#define IIC_SCL_Set() GPIO_ClearOutBits(IIC_SCL_IOG,IIC_SCL_IOP)
#define IIC_SCL_Clr() GPIO_SetOutBits(IIC_SCL_IOG,IIC_SCL_IOP)   //使用MOS管开漏输出故电平相反
#define READ_SDA GPIO_ReadInBit(IIC_SDA_IOG,IIC_SDA_IOP)

//函数
void SMBUS_Init(void);
void IIC_Start(void);
void IIC_Stop(void);
char IIC_Wait_Ack(void);
void IIC_Ack(void);
void IIC_NAck(void);
void IIC_Send_Byte(unsigned char txd);
unsigned char IIC_Read_Byte(unsigned char ack);

#endif
