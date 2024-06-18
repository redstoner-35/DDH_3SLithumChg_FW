#include "ht32.h"
#include "I2C.h"
#include "delay.h"

//I2C延时
#define IIC_delay() delay_us(20)

/*
static void IIC_delay(void)
  {
	int i=7;
	while(--i);
	}
*/

//SMBUS初始化
void SMBUS_Init(void)
  {
	 //配置GPIO(SCL)
   AFIO_GPxConfig(IIC_SCL_IOB,IIC_SCL_IOP, AFIO_FUN_GPIO);//I2C SCL(用来做时钟)
   GPIO_DirectionConfig(IIC_SCL_IOG,IIC_SCL_IOP,GPIO_DIR_OUT);//配置为输出
	 GPIO_ClearOutBits(IIC_SCL_IOG,IIC_SCL_IOP);//输出设置为0（因为是MOS开漏输出所以输出0等于bus idle）
	 GPIO_PullResistorConfig(IIC_SCL_IOG,IIC_SCL_IOP,GPIO_PR_DOWN);//启用下拉
	 //配置GPIO(SDA)
   AFIO_GPxConfig(IIC_SDA_IOB,IIC_SDA_IOP, AFIO_FUN_GPIO);//I2C SDA(用来做数据)
   GPIO_DirectionConfig(IIC_SDA_IOG,IIC_SDA_IOP,GPIO_DIR_IN);//配置为输入
   GPIO_ClearOutBits(IIC_SDA_IOG,IIC_SDA_IOP);//ODR设置为0	 
	 //配置GPIO(DIR)	
   AFIO_GPxConfig(IIC_DIR_IOB,IIC_DIR_IOP, AFIO_FUN_GPIO);//I2C DIR(用来设置SDA的方向)
   GPIO_DirectionConfig(IIC_DIR_IOG,IIC_DIR_IOP,GPIO_DIR_OUT);//配置为输出
	 GPIO_SetOutBits(IIC_DIR_IOG,IIC_DIR_IOP);//输出设置为0(从器件到主器件方向，使从器件到主器件之间开路)
	}
//设置SDA引脚的参数(模拟开漏)
static void SetSDAOD(bool IsPinHigh)	
  {
	GPIO_DirectionConfig(IIC_SDA_IOG,IIC_SDA_IOP,IsPinHigh?GPIO_DIR_IN:GPIO_DIR_OUT);//配置IO方向(如果SDA=1,则将IO设置为高阻)
	GPIO_InputConfig(IIC_SDA_IOG,IIC_SDA_IOP,IsPinHigh?ENABLE:DISABLE);//(如果SDA=1,则启用IDR允许读取输入)
	if(IsPinHigh)GPIO_ClearOutBits(IIC_DIR_IOG,IIC_DIR_IOP);//DIR输出设置为0(配置为从器件到主器件方向，此时SDA处于高阻输入,被电阻拉高到0)
	else GPIO_SetOutBits(IIC_DIR_IOG,IIC_DIR_IOP);//DIR输出设置为1（配置为主器件到从器件方向，强制拉低SDA）
	}

//产生IIC起始信号
void IIC_Start(void)
{
	IIC_SDA_Set();	  	  
	IIC_SCL_Set();
	IIC_delay();
 	IIC_SDA_Clr();//START:when CLK is high,DATA change form high to low 
	IIC_delay();
	IIC_SCL_Clr();//钳住I2C总线，准备发送或接收数据 
}	  
//产生IIC停止信号
void IIC_Stop(void)
{
	IIC_SCL_Clr();
	IIC_SDA_Clr();//STOP:when CLK is high DATA change form low to high
 	IIC_delay();
	IIC_SCL_Set(); 
	IIC_delay();
	IIC_SDA_Set();//发送I2C总线结束信号
	IIC_delay();							   	
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
char IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	IIC_SDA_Set();	 
	IIC_delay();	   
	IIC_SCL_Set();//时钟拉高
	IIC_delay();	
	while(READ_SDA)
	{
		IIC_delay();
		ucErrTime++;
		if(ucErrTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL_Clr();//时钟输出0 	  
	IIC_delay();		
	return 0;  
} 
//产生ACK应答
void IIC_Ack(void)
{
	IIC_SCL_Clr();
	IIC_SDA_Clr();
	IIC_delay();
	IIC_SCL_Set();
	IIC_delay();
	IIC_SCL_Clr();
}
//不产生ACK应答		    
void IIC_NAck(void)
{
	IIC_SCL_Clr();
	IIC_SDA_Set();
	IIC_delay();
	IIC_SCL_Set();
	IIC_delay();
	IIC_SCL_Clr();
}					 				     
//IIC发送一个字节	  
void IIC_Send_Byte(unsigned char txd)
{                        
    u8 t;      
    IIC_SCL_Clr();//拉低时钟开始数据传输
    IIC_delay();//等一等再开始传输
    for(t=0;t<8;t++)
    {              
    if(txd&0x80)IIC_SDA_Set();
		else IIC_SDA_Clr();
    txd<<=1; 	  
		IIC_delay();   //对TEA5767这三个延时都是必须的
		IIC_SCL_Set();
		IIC_delay(); 
		IIC_SCL_Clr();	
		IIC_delay();
    }	 
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
unsigned char IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	  IIC_SDA_Set();//释放总线
    for(i=0;i<8;i++ )
	{
        IIC_SCL_Clr(); 
        IIC_delay();
		    IIC_SCL_Set();
        receive<<=1;
        if(READ_SDA)receive++;   
		    IIC_delay(); 
    }	
    if (!ack)
        IIC_NAck();//发送nACK
    else
        IIC_Ack(); //发送ACK 
    IIC_delay(); //额外的延时		
    return receive;
}
