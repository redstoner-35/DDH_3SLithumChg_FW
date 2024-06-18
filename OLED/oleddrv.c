#include "ht32.h"
#include "oled.h"
#include "delay.h"
#include "Pindefs.h"
#include "LEDMgmt.h"
#include <stdio.h>
#include <string.h>

//ȫ�ֱ���
unsigned char OLED_GRAM[OLEDHoriSize][OLEDVertSize/8];//OLED���Դ�
static bool RetryWrite=false;
extern bool EnteredMainApp;

//��ʱ
static void OLED_IIC_delay(void)
{
	u8 t=7;
	while(t--); //t=7 SMBUSԼ480KHz
}

//���������������Ļ��ʼ�����߹���ʧ�������˴�
static void ScreenNACKErrorHandler(void)
{
EnteredMainApp=false; //����˳���ѭ��
CurrentLEDIndex=10;//��Ļ����ʧ�ܣ���ʾ����
while(1); //����ѭ��
}

//���ô��䷽��
static void I2C_SetTransDir(bool IsOutput)
{
GPIO_DirectionConfig(OLED_SDA_IOG,OLED_SDA_IOP,IsOutput?GPIO_DIR_OUT:GPIO_DIR_IN);//����IO����
GPIO_InputConfig(OLED_SDA_IOG,OLED_SDA_IOP,IsOutput?DISABLE:ENABLE);//���û����IDR
}

//��ʼ�ź�
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

//�����ź�
static void I2C_Stop(void)
{
	I2C_SetTransDir(true);
	OLED_SDA_Clr();
	OLED_SCL_Set();
	OLED_IIC_delay();
	OLED_SDA_Set();
}

//�ȴ��ź���Ӧ
static void I2C_WaitAck(void) //�������źŵĵ�ƽ
{
	int err=0;
	OLED_SDA_Set();
	OLED_IIC_delay();
	OLED_SCL_Set();
  OLED_IIC_delay();
  I2C_SetTransDir(false); //�ͷ�SDAΪInput����ȡ��ĻACK���
	while(GPIO_ReadInBit(OLED_SDA_IOG,OLED_SDA_IOP))  //�ȴ�ACK
	  {
		err++;
		if(err==250) //�ȴ���ʱ
		  {
			I2C_Stop();
			RetryWrite=true; //��Ǳ��δ���ʧ�ܣ�����
			return;
			}
		OLED_IIC_delay();
		}
	I2C_SetTransDir(true);
	OLED_SCL_Clr();
	OLED_IIC_delay();
}

//д��һ���ֽ�
static void Send_Byte(u8 dat)
{
	u8 i;
  I2C_SetTransDir(true);
	for(i=0;i<8;i++)
	{
		if(dat&0x80)//��dat��8λ�����λ����д��
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
		OLED_SCL_Clr();//��ʱ���ź�����Ϊ�͵�ƽ
		dat<<=1;
  }
}

//����һ���ֽ�
//mode:����/�����־ 0,��ʾ����;1,��ʾ����;
static void OLED_WR_Byte(u8 dat,u8 mode)
{
	int RetryCount=0;
	RetryWrite=false;
	do
		{
		RetryCount++;
		if(RetryCount==30)ScreenNACKErrorHandler();//����ʧ�ܴ������࣬���������
		I2C_Start();
		Send_Byte(0x78); //������Ļ��ַ
		I2C_WaitAck();
	  if(RetryWrite)continue; //����ִ��ʧ��
		Send_Byte(mode?0x40:0x00); //�����������ͱ��λ
		I2C_WaitAck();
		if(RetryWrite)continue; //����ִ��ʧ��
		Send_Byte(dat); //����ʵ������
		I2C_WaitAck();
		if(RetryWrite)continue; //����ִ��ʧ��
		I2C_Stop();	
	  }
  while(RetryWrite);
}

//�����Դ浽OLED	
void OLED_Refresh(void)
{
	u8 i=0,n;
	int ErrorCount=0;
	while(i<OLEDVertSize/8)
	{
		//����������࣬��Ļ���Ͻ��������
		if(ErrorCount>=30)ScreenNACKErrorHandler(); //���д���
		//���õ�ַ����
		OLED_WR_Byte(0xb0+i,OLED_CMD); //��������ʼ��ַ
		OLED_WR_Byte(0x00,OLED_CMD);   //���õ�����ʼ��ַ
		OLED_WR_Byte(0x12,OLED_CMD);   //���ø�����ʼ��ַ
		//��ʼ����frame data
		I2C_Start();
		Send_Byte(0x78);
		I2C_WaitAck();
		if(RetryWrite){ErrorCount++;continue;} //����ִ��ʧ��
		Send_Byte(0x40);
		I2C_WaitAck();
		if(RetryWrite){ErrorCount++;continue;} //����ִ��ʧ��,���Ӵ������
		for(n=0;n<OLEDHoriSize;n++)
		  {
			Send_Byte(OLED_GRAM[n][i]);
			I2C_WaitAck();
			if(RetryWrite)break; //������������ѭ��
		  }
		//���ͽ�������
		if(n==OLEDHoriSize)
		  {
		  I2C_Stop();
			i++;  //���е�����˳����ɷ��ͣ�������һ��	
			}	
		else ErrorCount++; //���з���ʧ�ܣ�����
  }
}

//������Ļ����
void OLED_SetBrightness(char Brightness)
 {
 if(Brightness&0x80||Brightness==0x00)return; //�Ƿ���ֵ
 OLED_WR_Byte(0x81,OLED_CMD); /*contract control*/ 
 OLED_WR_Byte(Brightness,OLED_CMD); /*128*/ 
 }

//��ʼ����Ļ
void OLED_Init(void)
 {
 //����GPIO(SCL)
 AFIO_GPxConfig(OLED_SCL_IOB,OLED_SCL_IOP, AFIO_FUN_GPIO);//I2C SCL(������ʱ��)
 GPIO_DirectionConfig(OLED_SCL_IOG,OLED_SCL_IOP,GPIO_DIR_OUT);//����Ϊ���
 GPIO_SetOutBits(OLED_SCL_IOG,OLED_SCL_IOP);//�������Ϊ1
 GPIO_PullResistorConfig(OLED_SCL_IOG,OLED_SCL_IOP,GPIO_PR_UP);//��������
 //����GPIO(SDA)
 AFIO_GPxConfig(OLED_SDA_IOB,OLED_SDA_IOP, AFIO_FUN_GPIO);//I2C SDA(����������)
 GPIO_DirectionConfig(OLED_SDA_IOG,OLED_SDA_IOP,GPIO_DIR_OUT);//����Ϊ���
 GPIO_SetOutBits(OLED_SDA_IOG,OLED_SDA_IOP);//�������Ϊ1	 
 GPIO_PullResistorConfig(OLED_SDA_IOG,OLED_SDA_IOP,GPIO_PR_UP);//��������
 //����GPIO(RST)
 AFIO_GPxConfig(OLED_RST_IOB,OLED_RST_IOP, AFIO_FUN_GPIO);//��Ļ��λ����
 GPIO_DirectionConfig(OLED_RST_IOG,OLED_RST_IOP,GPIO_DIR_OUT);//����Ϊ���
 GPIO_SetOutBits(OLED_RST_IOG,OLED_RST_IOP);//�������Ϊ1 
 //���ɸ�λ����
 delay_ms(10);	 
 OLED_RES_Clr();
 delay_ms(200);
 OLED_RES_Set();
 delay_ms(10);  //���͸�λ����֮���ӳ�10mS�������ݿ�ʼ��ʼ��
 //���ͳ�ʼ��ָ��
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

//��������
void OLED_Clear(void)
{
unsigned char i;
for(i=0;i<OLEDHoriSize;i++)memset(&OLED_GRAM[i][0],0,OLEDVertSize/8);
}

