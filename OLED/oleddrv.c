#include "ht32.h"
#include "oled.h"
#include "delay.h"
#include "Pindefs.h"
#include "LEDMgmt.h"
#include <stdio.h>
#include <string.h>

//全局变量
unsigned char OLED_GRAM[OLEDHoriSize][OLEDVertSize/8];//OLED的显存
static bool RetryWrite=false;
extern bool EnteredMainApp;

//延时
static void OLED_IIC_delay(void)
{
	u8 t=7;
	while(t--); //t=7 SMBUS约480KHz
}

//错误处理函数，如果屏幕初始化或者工作失败则进入此处
static void ScreenNACKErrorHandler(void)
{
EnteredMainApp=false; //标记退出主循环
CurrentLEDIndex=10;//屏幕启动失败，提示错误
while(1); //无限循环
}

//设置传输方向
static void I2C_SetTransDir(bool IsOutput)
{
GPIO_DirectionConfig(OLED_SDA_IOG,OLED_SDA_IOP,IsOutput?GPIO_DIR_OUT:GPIO_DIR_IN);//配置IO方向
GPIO_InputConfig(OLED_SDA_IOG,OLED_SDA_IOP,IsOutput?DISABLE:ENABLE);//启用或禁用IDR
}

//起始信号
static void I2C_Start(void)
{
	I2C_SetTransDir(true);
	OLED_SDA_Set();
	OLED_SCL_Set();
	OLED_IIC_delay();
	OLED_SDA_Clr();
	OLED_IIC_delay();
	OLED_SCL_Clr();
	OLED_IIC_delay();
}

//结束信号
static void I2C_Stop(void)
{
	I2C_SetTransDir(true);
	OLED_SDA_Clr();
	OLED_SCL_Set();
	OLED_IIC_delay();
	OLED_SDA_Set();
}

//等待信号响应
static void I2C_WaitAck(void) //测数据信号的电平
{
	int err=0;
	OLED_SDA_Set();
	OLED_IIC_delay();
	OLED_SCL_Set();
  OLED_IIC_delay();
  I2C_SetTransDir(false); //释放SDA为Input，读取屏幕ACK结果
	while(GPIO_ReadInBit(OLED_SDA_IOG,OLED_SDA_IOP))  //等待ACK
	  {
		err++;
		if(err==250) //等待超时
		  {
			I2C_Stop();
			RetryWrite=true; //标记本次传输失败，重试
			return;
			}
		OLED_IIC_delay();
		}
	I2C_SetTransDir(true);
	OLED_SCL_Clr();
	OLED_IIC_delay();
}

//写入一个字节
static void Send_Byte(u8 dat)
{
	u8 i;
  I2C_SetTransDir(true);
	for(i=0;i<8;i++)
	{
		if(dat&0x80)//将dat的8位从最高位依次写入
		{
			OLED_SDA_Set();
    }
		else
		{
			OLED_SDA_Clr();
    }
		OLED_IIC_delay();
		OLED_SCL_Set();
		OLED_IIC_delay();
		OLED_SCL_Clr();//将时钟信号设置为低电平
		dat<<=1;
  }
}

//发送一个字节
//mode:数据/命令标志 0,表示命令;1,表示数据;
static void OLED_WR_Byte(u8 dat,u8 mode)
{
	int RetryCount=0;
	RetryWrite=false;
	do
		{
		RetryCount++;
		if(RetryCount==30)ScreenNACKErrorHandler();//重试失败次数过多，进入错误处理
		I2C_Start();
		Send_Byte(0x78); //发出屏幕地址
		I2C_WaitAck();
	  if(RetryWrite)continue; //命令执行失败
		Send_Byte(mode?0x40:0x00); //发出数据类型标记位
		I2C_WaitAck();
		if(RetryWrite)continue; //命令执行失败
		Send_Byte(dat); //发送实际数据
		I2C_WaitAck();
		if(RetryWrite)continue; //命令执行失败
		I2C_Stop();	
	  }
  while(RetryWrite);
}

//更新显存到OLED	
void OLED_Refresh(void)
{
	u8 i=0,n;
	int ErrorCount=0;
	while(i<OLEDVertSize/8)
	{
		//错误计数过多，屏幕故障进入错误处理
		if(ErrorCount>=30)ScreenNACKErrorHandler(); //进行处理
		//设置地址参数
		OLED_WR_Byte(0xb0+i,OLED_CMD); //设置行起始地址
		OLED_WR_Byte(0x00,OLED_CMD);   //设置低列起始地址
		OLED_WR_Byte(0x12,OLED_CMD);   //设置高列起始地址
		//开始发送frame data
		I2C_Start();
		Send_Byte(0x78);
		I2C_WaitAck();
		if(RetryWrite){ErrorCount++;continue;} //命令执行失败
		Send_Byte(0x40);
		I2C_WaitAck();
		if(RetryWrite){ErrorCount++;continue;} //命令执行失败,增加错误计数
		for(n=0;n<OLEDHoriSize;n++)
		  {
			Send_Byte(OLED_GRAM[n][i]);
			I2C_WaitAck();
			if(RetryWrite)break; //遇到错误，跳出循环
		  }
		//发送结束处理
		if(n==OLEDHoriSize)
		  {
		  I2C_Stop();
			i++;  //本行的数据顺利完成发送，继续下一行	
			}	
		else ErrorCount++; //本行发送失败，重试
  }
}

//设置屏幕亮度
void OLED_SetBrightness(char Brightness)
 {
 if(Brightness&0x80||Brightness==0x00)return; //非法数值
 OLED_WR_Byte(0x81,OLED_CMD); /*contract control*/ 
 OLED_WR_Byte(Brightness,OLED_CMD); /*128*/ 
 }

//初始化屏幕
void OLED_Init(void)
 {
 //配置GPIO(SCL)
 AFIO_GPxConfig(OLED_SCL_IOB,OLED_SCL_IOP, AFIO_FUN_GPIO);//I2C SCL(用来做时钟)
 GPIO_DirectionConfig(OLED_SCL_IOG,OLED_SCL_IOP,GPIO_DIR_OUT);//配置为输出
 GPIO_SetOutBits(OLED_SCL_IOG,OLED_SCL_IOP);//输出设置为1
 GPIO_PullResistorConfig(OLED_SCL_IOG,OLED_SCL_IOP,GPIO_PR_UP);//启用上拉
 //配置GPIO(SDA)
 AFIO_GPxConfig(OLED_SDA_IOB,OLED_SDA_IOP, AFIO_FUN_GPIO);//I2C SDA(用来做数据)
 GPIO_DirectionConfig(OLED_SDA_IOG,OLED_SDA_IOP,GPIO_DIR_OUT);//配置为输出
 GPIO_SetOutBits(OLED_SDA_IOG,OLED_SDA_IOP);//输出设置为1	 
 GPIO_PullResistorConfig(OLED_SDA_IOG,OLED_SDA_IOP,GPIO_PR_UP);//启用上拉
 //配置GPIO(RST)
 AFIO_GPxConfig(OLED_RST_IOB,OLED_RST_IOP, AFIO_FUN_GPIO);//屏幕复位引脚
 GPIO_DirectionConfig(OLED_RST_IOG,OLED_RST_IOP,GPIO_DIR_OUT);//配置为输出
 GPIO_SetOutBits(OLED_RST_IOG,OLED_RST_IOP);//输出设置为1 
 //生成复位脉冲
 delay_ms(10);	 
 OLED_RES_Clr();
 delay_ms(200);
 OLED_RES_Set();
 delay_ms(10);  //发送复位脉冲之后延迟10mS再送数据开始初始化
 //发送初始化指令
 OLED_WR_Byte(0xAE,OLED_CMD); /*display off*/
 OLED_WR_Byte(0x00,OLED_CMD); /*set lower column address*/ 
 OLED_WR_Byte(0x12,OLED_CMD); /*set higher column address*/
 OLED_WR_Byte(0x00,OLED_CMD); /*set display start line*/ 
 OLED_WR_Byte(0xB0,OLED_CMD); /*set page address*/ 
 OLED_WR_Byte(0x81,OLED_CMD); /*contract control*/ 
 OLED_WR_Byte(0x7F,OLED_CMD); /*128*/ 
 OLED_WR_Byte(0xA1,OLED_CMD); /*set segment remap*/ 
 OLED_WR_Byte(0xA6,OLED_CMD); /*normal / reverse*/ 
 OLED_WR_Byte(0xA8,OLED_CMD); /*multiplex ratio*/ 
 OLED_WR_Byte(0x1F,OLED_CMD); /*duty = 1/32*/ 
 OLED_WR_Byte(0xC8,OLED_CMD); /*Com scan direction*/ 
 OLED_WR_Byte(0xD3,OLED_CMD); /*set display offset*/ 
 OLED_WR_Byte(0x00,OLED_CMD); 
 OLED_WR_Byte(0xD5,OLED_CMD); /*set osc division*/ 
 OLED_WR_Byte(0x80,OLED_CMD); 
 OLED_WR_Byte(0xD9,OLED_CMD); /*set pre-charge period*/ 
 OLED_WR_Byte(0x1f,OLED_CMD); 
 OLED_WR_Byte(0xDA,OLED_CMD); /*set COM pins*/ 
 OLED_WR_Byte(0x12,OLED_CMD); 
 OLED_WR_Byte(0xdb,OLED_CMD); /*set vcomh*/ 
 OLED_WR_Byte(0x40,OLED_CMD); 
 OLED_WR_Byte(0x8d,OLED_CMD); /*set charge pump enable*/ 
 OLED_WR_Byte(0x14,OLED_CMD);
 OLED_Clear();
 OLED_WR_Byte(0xAF,OLED_CMD); /*display ON*/ 
 }

//清屏函数
void OLED_Clear(void)
{
unsigned char i;
for(i=0;i<OLEDHoriSize;i++)memset(&OLED_GRAM[i][0],0,OLEDVertSize/8);
}

