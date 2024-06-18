#ifndef _Oled_
#define _Oled_

//��������
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#include <stdbool.h>

//OLED����ʱ��

#define OLEDSleepTimeOut 90 //OLED��Ļ����ʱ��

//OLED��Ļ��С
#define OLEDHoriSize 64
#define OLEDVertSize 32 //OLED���������ߴ�

//��Ļ���Ŷ���
#define OLED_SCL_IOB STRCAT2(GPIO_P,OLED_SCL_IOBank)
#define OLED_SCL_IOG STRCAT2(HT_GPIO,OLED_SCL_IOBank)
#define OLED_SCL_IOP STRCAT2(GPIO_PIN_,OLED_SCL_IOPinNum) //SCL�Զ�Define

#define OLED_SDA_IOB STRCAT2(GPIO_P,OLED_SDA_IOBank)
#define OLED_SDA_IOG STRCAT2(HT_GPIO,OLED_SDA_IOBank)
#define OLED_SDA_IOP STRCAT2(GPIO_PIN_,OLED_SDA_IOPinNum) //SDA�Զ�Define

#define OLED_RST_IOB STRCAT2(GPIO_P,OLED_RST_IOBank)
#define OLED_RST_IOG STRCAT2(HT_GPIO,OLED_RST_IOBank)
#define OLED_RST_IOP STRCAT2(GPIO_PIN_,OLED_RST_IOPinNum) //RST�Զ�Define

//��Ļ��������
#define OLED_CMD  0	//д����
#define OLED_DATA 1	//д����

#define OLED_SCL_Clr() GPIO_ClearOutBits(OLED_SCL_IOG,OLED_SCL_IOP)//SCL
#define OLED_SCL_Set() GPIO_SetOutBits(OLED_SCL_IOG,OLED_SCL_IOP)

#define OLED_SDA_Clr() GPIO_ClearOutBits(OLED_SDA_IOG,OLED_SDA_IOP)//DIN
#define OLED_SDA_Set() GPIO_SetOutBits(OLED_SDA_IOG,OLED_SDA_IOP)

#define OLED_RES_Clr() GPIO_ClearOutBits(OLED_RST_IOG,OLED_RST_IOP)//RES
#define OLED_RES_Set() GPIO_SetOutBits(OLED_RST_IOG,OLED_RST_IOP)

//�ⲿ����
extern unsigned char OLED_GRAM[OLEDHoriSize][OLEDVertSize/8];//OLED���Դ�

//Ӳ����غ���
void OLED_Init(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_SetBrightness(char Brightness);

//�������봦��
int iroundf(float IN);

//��ʾ�ؼ���غ���
void OLED_DrawPoint(char x,char y,char t);//����
void OLED_DrawLine(char x1,char y1,char x2,char y2,char mode);//����
void OLED_DrawCircle(char x,char y,char r);//��Բ
void OLED_ShowPicture(char x,char y,char sizex,char sizey,const char *BMP,char mode);//��ʾͼƬ
void OLED_ShowSingleMINIASCII(char x,char y,char Code,char mode);//��ʾ����5x5�ַ�
void OLED_ShowMiniNum(char x,char y,char Number,char mode);//��ʾ����5x5����
void OLED_ShowStringViaMiniASCII(char Startx,char Starty,char MaxmiumLenPerLine,char *StringIN,int len,int mode);//��5x5�ַ���ʾ�ַ���
void OLED_Show3DigitInt(char x,char y,int Num);//��5x5�ַ���ʾ��λ����
void OLED_ShowProgressBar(char x,char y,char Presentage);//��ʾ5�߽�����
void OLED_ShowFloatNum(float Num,char x,char y);//��Ⱦ3λ��������(0-999)
void OLED_Fill(char x,char y,char sizex,char sizey,char mode);//���ָ������
void OLED_Printf(char Startx,char Starty,char MaxmiumLenPerLine,char mode,char *Format,...);//OLED���
void OLED_Printfn(char Startx,char Starty,char MaxmiumLenPerLine,int n,char mode,char *Format,...);//OLED���ָ����ֵ
void OLED_Show4DigitInt(char x,char y,int Num);//��ʾ��λ����
void OLED_DisplayButton(char x,char y,char *Text,bool IsSelect);//��ʾ��ť
void OLED_DrawRectangle(char x1,char y1,char x2,char y2,char mode);//��ʾ���ĳ�����
void OLED_ImageDisappear(void);//��Ļ��ʧ��Ч
void OLED_OldTVFade(void);//OLED�ϵ�����ʧЧ��
bool OLED_CheckIsGRAMHasImage(void);//����Դ������Ƿ񻹰�������

#endif
