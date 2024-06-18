#include "oled.h"
#include "delay.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

//5x5 ASCII�ֿ�
const char MINI_ASCII[][5]=
{
  {0x00,0x00,0x00,0x00,0x00},//' '
  {0x00,0x00,0x17,0x00,0x00},//'!'
	{0x00,0x03,0x00,0x03,0x00},//'"'
  {0x0A,0x1F,0x0A,0x1F,0x0A},//'#'
	{0x02,0x15,0x1F,0x15,0x08},//'$'
	{0x13,0x0B,0x04,0x1A,0x19},//'%'
	{0x1D,0x12,0x0D,0x0D,0x12},//'&'
  {0x00,0x00,0x03,0x00,0x00},//'
  {0x00,0x1F,0x11,0x11,0x00},//'('
	{0x00,0x11,0x11,0x1F,0x00},//')'
	{0x00,0x0A,0x04,0x0A,0x00},//'*'
	{0x04,0x04,0x1F,0x04,0x04},//'+'
  {0x00,0x14,0x0C,0x00,0x00},//','
	{0x00,0x04,0x04,0x04,0x00},//'-'
	{0x10,0x00,0x00,0x00,0x00},//'.'
  {0x10,0x08,0x04,0x02,0x01},//'/'
	{0x0E,0x11,0x11,0x11,0x0E},//'0'
	{0x11,0x11,0x1F,0x10,0x10},//'1'
	{0x1D,0x15,0x15,0x15,0x17},//'2'
	{0x15,0x15,0x15,0x15,0x1F},//'3'
	{0x07,0x04,0x04,0x04,0x1F},//'4'
	{0x17,0x15,0x15,0x15,0x0D},//'5'
	{0x1F,0x15,0x15,0x15,0x1D},//'6'
	{0x03,0x01,0x01,0x01,0x1F},//'7'
	{0x1F,0x15,0x15,0x15,0x1F},//'8'
	{0x17,0x15,0x15,0x15,0x1F},//'9'
	{0x00,0x00,0x0A,0x00,0x00},//':'
	{0x00,0x15,0x0D,0x00,0x00},//';'
	{0x00,0x04,0x0A,0x11,0x00},//'<'
	{0x0A,0x0A,0x0A,0x0A,0x0A},//'='
	{0x00,0x11,0x0A,0x04,0x00},//'>'
	{0x02,0x01,0x15,0x05,0x02},//'?'
	{0x0E,0x15,0x0D,0x11,0x0E},//'@'
	{0x1F,0x05,0x05,0x05,0x1F},//'A'
	{0x1F,0x15,0x15,0x15,0x0A},//'B'
	{0x1F,0x11,0x11,0x11,0x11},//'C'
	{0x1F,0x11,0x11,0x11,0x0E},//'D'
	{0x1F,0x15,0x15,0x15,0x11},//'E'
	{0x1F,0x05,0x05,0x05,0x01},//'F'
	{0x1F,0x11,0x15,0x15,0x1D},//'G'
	{0x1F,0x04,0x04,0x04,0x1F},//'H'
	{0x00,0x11,0x1F,0x11,0x00},//'I'
	{0x09,0x11,0x11,0x0F,0x01},//'J'
	{0x1F,0x04,0x0A,0x11,0x00},//'K'
	{0x1F,0x10,0x10,0x10,0x10},//'L'
	{0x1F,0x02,0x04,0x02,0x1F},//'M'
	{0x1F,0x02,0x04,0x08,0x1F},//'N'
	{0x1F,0x11,0x11,0x11,0x1F},//'O'
	{0x1F,0x05,0x05,0x05,0x07},//'P'
	{0x1F,0x11,0x11,0x09,0x17},//'Q'
	{0x1F,0x01,0x05,0x05,0x1B},//'R'
	{0x17,0x15,0x15,0x15,0x1D},//'S'
	{0x01,0x01,0x1F,0x01,0x01},//'T'
	{0x1F,0x10,0x10,0x10,0x1F},//'U'
	{0x07,0x08,0x10,0x08,0x07},//'V'
	{0x0F,0x10,0x0F,0x10,0x0F},//'W'
	{0x11,0x0A,0x04,0x0A,0x11},//'X'
	{0x07,0x04,0x1C,0x04,0x07},//'Y'
	{0x11,0x19,0x15,0x13,0x11},//'Z'
	{0x01,0x00,0x1C,0x14,0x14},//'��'
	{0x14,0x00,0x00,0x00,0x00}//':'
};
#pragma push
#pragma Otime//�Ż��ú���ʹ��3�����Ż�
#pragma O3
//���� 
//x:0~127
//y:0~63
//t:1 ��� 0,���	
void OLED_DrawPoint(char x,char y,char t)
{
	char i,m,n;
	//�жϲ���
  if(x>(OLEDHoriSize-1))return;
	if(y>(OLEDVertSize-1))return;
	//ʵ�ʻ������
  i=y/8;
	m=y%8;
	n=1<<m;
	if(t){OLED_GRAM[x][i]|=n;}
	else
	{
		OLED_GRAM[x][i]=~OLED_GRAM[x][i];
		OLED_GRAM[x][i]|=n;
		OLED_GRAM[x][i]=~OLED_GRAM[x][i];
	}
}

//����
//x1,y1:�������
//x2,y2:��������
void OLED_DrawLine(char x1,char y1,char x2,char y2,char mode)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
		//�жϲ���
  if(x1>(OLEDHoriSize-1))return;
	if(y1>(OLEDVertSize-1))return;
	if(x2>(OLEDHoriSize-1))return;
	if(y2>(OLEDVertSize-1))return;
	//��ʼ����
	delta_x=x2-x1; //������������ 
	delta_y=y2-y1;
	uRow=x1;//�����������
	uCol=y1;
	if(delta_x>0)incx=1; //���õ������� 
	else if (delta_x==0)incx=0;//��ֱ�� 
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//ˮƽ�� 
	else {incy=-1;delta_y=-delta_x;}
	if(delta_x>delta_y)distance=delta_x; //ѡȡ�������������� 
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		OLED_DrawPoint(uRow,uCol,mode);//����
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}
//x,y:Բ������
//r:Բ�İ뾶
void OLED_DrawCircle(char x,char y,char r)
{
	int a, b,num;
    a = 0;
    b = r;
		//�жϲ���
    if(x>(OLEDHoriSize-1))return;
	  if(y>(OLEDVertSize-1))return;
	  //��ʼ����
    while(2 * b * b >= r * r)      
    {
        OLED_DrawPoint(x + a, y - b,1);
        OLED_DrawPoint(x - a, y - b,1);
        OLED_DrawPoint(x - a, y + b,1);
        OLED_DrawPoint(x + a, y + b,1);
 
        OLED_DrawPoint(x + b, y + a,1);
        OLED_DrawPoint(x + b, y - a,1);
        OLED_DrawPoint(x - b, y - a,1);
        OLED_DrawPoint(x - b, y + a,1);
        
        a++;
        num = (a * a + b * b) - r*r;//���㻭�ĵ���Բ�ĵľ���
        if(num > 0)
        {
            b--;
            a--;
        }
    }
}

//x,y���������
//sizex,sizey,ͼƬ����
//const char *BMP��Ҫд���ͼƬ����
//mode:0,��ɫ��ʾ;1,������ʾ
void OLED_ShowPicture(char x,char y,char sizex,char sizey,const char *BMP,char mode)
{
	u16 j=0;
	u8 i,n,temp,m;
	u8 x0=x,y0=y;
	sizey=sizey/8+((sizey%8)?1:0);
	//�жϲ���
  if(x>(OLEDHoriSize-1))return;
	if(y>(OLEDVertSize-1))return;
	//��ʼ����
	for(n=0;n<sizey;n++)
	{
		 for(i=0;i<sizex;i++)
		 {
				temp=BMP[j];
				j++;
				for(m=0;m<8;m++)
				{
					if(temp&0x01)OLED_DrawPoint(x,y,mode);
					else OLED_DrawPoint(x,y,!mode);
					temp>>=1;
					y++;
				}
				x++;
				if((x-x0)==sizex)
				{
					x=x0;
					y0=y0+8;
				}
				y=y0;
     }
	 }
}

//��Ⱦ����5x5 Mini�ַ��ĺ���
void OLED_ShowSingleMINIASCII(char x,char y,char Code,char mode)
{
  int j=0;
	char i,n,temp,m;
	char x0=x,y0=y;
	int sizey;
	//�жϲ���
  if(x>(OLEDHoriSize-1))return;
	if(y>(OLEDVertSize-1))return;
	//�ж�ASCII�����Ƿ�Ϸ�	e
	if(Code==0x7C)Code=0x5B; //�����ַ�0x7C����ʾ�����±�
	else if(Code==0x7E)Code=0x5C; //�����ַ�0x7E����ʾ������ð��
	else if(Code>0x60&&Code<0x7B)Code=Code-0x20;//Сд��ĸ������Ϊ�ַ���û��Сд��ĸ��ʹ�ô�д��ĸ��ʾ
	else if(Code<0x20||Code>0x5A)Code=0x3F;//������ʾ��Χ�ķǷ��ַ�����ʾ'?'	
	Code=Code-0x20;//��ASCII�ַ�����ת��Ϊ����λ��
  //������ʾ
	sizey=5/8+((5%8)?1:0);
	for(n=0;n<sizey;n++)
	{
		 for(i=0;i<5;i++)
		 {
				temp=MINI_ASCII[Code][j];//ȡbitmap
				j++;
				for(m=0;m<8;m++)
				{
					if(temp&0x01)OLED_DrawPoint(x,y,mode);
					else OLED_DrawPoint(x,y,!mode);
					temp>>=1;
					y++;
				}
				x++;
				if((x-x0)==5)
				{
					x=x0;
					y0=y0+8;
				}
				y=y0;
     }
	 }
}
//��Mini ASCII��ʾ�����ַ��������Զ����У�
void OLED_ShowStringViaMiniASCII(char Startx,char Starty,char MaxmiumLenPerLine,char *StringIN,int len,int mode)
 {
	int POS=0,remaingap;//ָ��λ��
	int ColumnPOS=Startx;//��ǰ�е�λ�ã�X�ᣩ
  //�жϲ���
  if(Startx>(OLEDHoriSize-1))return;
	if(Starty>(OLEDVertSize-1))return;
  //��ʼ��ʾ
	while(StringIN[POS]>0x1F&&StringIN[POS]<0x7F&&POS<len)//�ж��Ƿ�Ϊ�Ƿ��ַ�
	 {
	 //������һ���µ���������Ҫ�Ĵ�С
	 if(StringIN[POS]=='I'||StringIN[POS]=='i')remaingap=4;
	 else if(StringIN[POS]=='.'||StringIN[POS]==0x61)remaingap=1;
	 else remaingap=5;
	 //�ж��Ƿ���Ҫ����
	 if(((Startx+MaxmiumLenPerLine)-ColumnPOS)<remaingap||(ColumnPOS+remaingap)>(OLEDHoriSize-1))//��ǰ��ʾ�����ݵ����е�ĩβ���߼���������Ļ
	  {
		if(Starty+6>63)return;//û�������ˣ�ֱ���˳�
		Starty=Starty+6;//�����ƶ�һ��
		ColumnPOS=Startx;//�ص���ͷ
		} 
	 //��ʾ���ݣ�Ȼ����һ���ַ�
	 OLED_ShowSingleMINIASCII(ColumnPOS,Starty,StringIN[POS],mode);//��ʾһ���ַ�
	 if(StringIN[POS]=='I'||StringIN[POS]=='i')ColumnPOS+=5;
	 else if(StringIN[POS]=='.'||StringIN[POS]==0x7E)ColumnPOS+=2;
	 else ColumnPOS+=6;//��꣨���������֣�����һ���ַ��Ŀռ�
	 POS++;//��ʾ��һ���ַ�
	 }
 }
//��OLEDʵ��Printf
void OLED_Printf(char Startx,char Starty,char MaxmiumLenPerLine,char mode,char *Format,...)
 {
 va_list arg;
 char SBUF[100]={0};
 __va_start(arg,Format);
 vsnprintf(SBUF,100,Format,arg);
 __va_end(arg);
 OLED_ShowStringViaMiniASCII(Startx,Starty,MaxmiumLenPerLine,SBUF,100,mode);
 }
//��OLEDʵ��ָ�������Printf
void OLED_Printfn(char Startx,char Starty,char MaxmiumLenPerLine,int n,char mode,char *Format,...)
 {
 va_list arg;
 char SBUF[100]={0};
 if(n<=0)return;//������ֵ�Ƿ�
 __va_start(arg,Format);
 vsnprintf(SBUF,100,Format,arg);
 __va_end(arg);
 if(n>100)n=100;//�趨��ֵ
 OLED_ShowStringViaMiniASCII(Startx,Starty,MaxmiumLenPerLine,SBUF,n,mode);
 } 
//��ʾ�������֣�5x5��
void OLED_ShowMiniNum(char x,char y,char Number,char mode)
{
u16 j=0;
	u8 i,n,temp,m;
	u8 x0=x,y0=y;
	int sizey;
	//�жϲ���
  if(x>(OLEDHoriSize-1))return;
	if(y>(OLEDVertSize-1))return;
	//��ʼ��ʾ
	sizey=5/8+((5%8)?1:0);
	for(n=0;n<sizey;n++)
	{
		 for(i=0;i<5;i++)
		 {
				temp=MINI_ASCII[Number+0x11][j];//ȡbitmap
				j++;
				for(m=0;m<8;m++)
				{
					if(temp&0x01)OLED_DrawPoint(x,y,mode);
					else OLED_DrawPoint(x,y,!mode);
					temp>>=1;
					y++;
				}
				x++;
				if((x-x0)==5)
				{
					x=x0;
					y0=y0+8;
				}
				y=y0;
     }
	 }
}

//��ʾ�������֣�3λ��
void OLED_Show3DigitInt(char x,char y,int Num)
{
int Hun;
int Shi;
int Ge;
//�жϲ���
if(x>(OLEDHoriSize-1))return;
if(y>(OLEDVertSize-1))return;
if(Num>999|Num<-999)return;//�����ֵ�Ƿ�Ƿ�
//����Ǹ����Ǿ�ת������
if(Num<0)
 {
  x=x+6;//ƫ�Ƴ�����ǰ��ĸ��ŵ�λ��
	OLED_ShowMiniNum(x-6,y,16,1);
	OLED_ShowMiniNum(x,y,16,1);
  OLED_ShowMiniNum(x+6,y,16,1);
  OLED_ShowMiniNum(x+12,y,16,1);//���֮ǰ��ʾ������
  Num=Num*-1;//����ת����
	OLED_ShowMiniNum(x-6,y,18,1);//��ʾ����
 }
else
 {
 OLED_ShowMiniNum(x,y,16,1);
 OLED_ShowMiniNum(x+6,y,16,1);
 OLED_ShowMiniNum(x+12,y,16,1);//���֮ǰ��ʾ������
 }
//�Ѹ���λ������� 
Hun=Num/100;//ȡ��λ
Shi=(Num%100)/10;//ȡʮλ
Ge=(Num%100)%10;//ȡ��λ

//�����λ��Ϊ0������ʾ��λ��ʮλ����λ
if(Hun!=0)
 {
 OLED_ShowMiniNum(x,y,Hun,1);//��ʾ��λ
 OLED_ShowMiniNum(x+6,y,Shi,1);//��ʾʮλ
 OLED_ShowMiniNum(x+12,y,Ge,1);//��ʾ��λ
 return;//�˳�
 }
//���ʮλ��Ϊ0����ʾʮλ��λ
if(Shi!=0)
 {
 OLED_ShowMiniNum(x,y,Shi,1);//��ʾʮλ
 OLED_ShowMiniNum(x+6,y,Ge,1);//��ʾ��λ
 return;//�˳�
 }
if(Ge!=0)OLED_ShowMiniNum(x,y,Ge,1);//��ʾ��λ
}
//��Ⱦ��������5��,�100�������ٷֱȿ��Ƴ���
void OLED_ShowProgressBar(char x,char y,char Presentage)
{
int ENDX;
int ENDY;
int i;
//���ȼ�鳤���ǲ�����Чֵ
if(x>(OLEDHoriSize-1))return;
if(y>(OLEDVertSize-1))return;
if(Presentage<1)return;//С��1�Ĳ���ʾ
if(Presentage>100)Presentage=100;//����100%�İ�100��
//���������
ENDX=x+Presentage;//X������Ϊ��ʼ����+����
ENDY=y+5;//����������ΪY+5����Ϊ��������5��
i=y;//��y�����ݸ���i
//������
while(i<ENDY)
 {
 OLED_DrawLine(x,i,x+100,i,0);//����Ӧ����Ľ�����������
 i++;
 }
//Ȼ�󻭽�����
i=y;//��y�����ݸ���i	
while(i<ENDY)
 {
 OLED_DrawLine(x,i,ENDX,i,1);//�����µĽ�����	
 i++;
 }	
}

//��Ⱦ�������֣�0-999
void OLED_ShowFloatNum(float Num,char x,char y)
{
float NumCopy;//����һ������
int INUM;//��������
int Xnum;//С������
//�жϲ���
if(x>(OLEDHoriSize-1))return;
if(y>(OLEDVertSize-1))return;
if(Num>999|Num<0)return;//�����ֵ�Ƿ�Ƿ�
//�ֳ�������С������
NumCopy=(float)Num;
INUM=(int)NumCopy;//ǿ��ȡ��
Num=Num-(float)INUM;//ȥ��Ԫ�����е���������
Num=Num*(float)100;//�Ŵ�С�����2λ������
Xnum=(int)Num;//�Ŵ�֮���С������ȡ��,��ʱ����λ�����ݾ��������������
Xnum=Xnum%100;//��100ȡ����С����ֻ����0-99��Χ�ڣ���������λС��
//��Ⱦ����
if(INUM>99)//������ִ���99
  {
	OLED_Show3DigitInt(x,y,INUM);//ֱ����Ⱦ���֣�����ȾС��������
	return;//�˳�
	}
if(INUM>9)//������ִ���9
 {
 OLED_Show3DigitInt(x,y,INUM);//����Ⱦ��������
 x=x+12;
 OLED_DrawPoint(x,y+4,1);//Xƫ��С�������λ����С����
 x=x+2;
 Xnum=Xnum/10;//��10����С�����һλ
 OLED_ShowMiniNum(x,y,Xnum,1);//��ȾС�����һλ
 return;//�˳�
 }
OLED_ShowMiniNum(x,y,INUM,1);//��Ⱦ��������
x=x+6;
OLED_DrawPoint(x,y+4,1);//Xƫ��С�����1λ����С����
x=x+2;
//��⣬���С������Ϊ����0����Ⱦ00
if(Xnum==0)
  {
  OLED_ShowMiniNum(x,y,0,1);
  OLED_ShowMiniNum(x+6,y,0,1);
  return;
  }
//��⣬���С������ֻ��С������1λ����Ⱦһ��0��С������һλ
if((Xnum%10)!=0&&(Xnum/10)==0)
  {
	OLED_ShowMiniNum(x,y,0,1);
	Xnum=Xnum%10;//ȡС�����ڶ�λ
  OLED_ShowMiniNum(x+6,y,Xnum,1);
  return;
	}
OLED_Show3DigitInt(x,y,Xnum);//��ʾ��λС��
}

//��亯��
void OLED_Fill(char x,char y,char sizex,char sizey,char mode)
{
int i;
int endy=y+sizey;
//�жϲ���
if(x>(OLEDHoriSize-1))return;
if(y>(OLEDVertSize-1))return;
//��ʼ���
for(i=y;i<endy;i++)
OLED_DrawLine(x,i,x+sizex,i,mode);
}
//��ʾ���ĳ�����
void OLED_DrawRectangle(char x1,char y1,char x2,char y2,char mode)
{
//�жϲ���
if(x1>(OLEDHoriSize-1))return;
if(y1>(OLEDVertSize-1))return;
if(x2>(OLEDHoriSize-1))return;
if(y2>(OLEDVertSize-1))return;
//��ʼ����
OLED_DrawLine(x1,y1,x2,y1,mode);
OLED_DrawLine(x1,y1,x1,y2,mode);
OLED_DrawLine(x2,y1,x2,y2,mode);
OLED_DrawLine(x1,y2,x2+1,y2,mode);
}
//��ʾ��ť
void OLED_DisplayButton(char x,char y,char *Text,bool IsSelect)
{
 //�жϲ���
 if(x>(OLEDHoriSize-1))return;
 if(y>(OLEDVertSize-1))return;
 //��ʾ��ť�ı�
 if(Text==NULL)return;
 if(IsSelect)OLED_Fill(x,y,(strlen(Text)*6)+2,9,1);//����䱳��
 OLED_Printf(x+2,y+2,strlen(Text)*6,!IsSelect,"%s",Text);
 OLED_Fill(x+2,y+8,strlen(Text)*6,8,0);//��ɫ������������ݿٸɾ�
 //��Χ����
 OLED_DrawLine(x,y,x+(strlen(Text)*6)+2,y,!IsSelect);
 OLED_DrawLine(x+(strlen(Text)*6)+2,y,x+(strlen(Text)*6)+2,y+8,!IsSelect);
 OLED_DrawLine(x,y,x,y+8,!IsSelect);
 OLED_DrawLine(x,y+8,x+(strlen(Text)*6)+3,y+8,!IsSelect);
}

//��ʾ��λ������0-9999��
void OLED_Show4DigitInt(char x,char y,int Num)
{
int Hun;
int Shi;
int Ge;
int Qian;
//�жϲ���
if(x>(OLEDHoriSize-1))return;
if(y>(OLEDVertSize-1))return;
if(Num>9999|Num<0)return;//�����ֵ�Ƿ�Ƿ�
//���֮ǰ��ʾ������
 OLED_ShowMiniNum(x,y,16,1);
 OLED_ShowMiniNum(x+6,y,16,1);
 OLED_ShowMiniNum(x+12,y,16,1);
 OLED_ShowMiniNum(x+18,y,16,1);//���֮ǰ��ʾ������
//�Ѹ���λ�������
Qian=Num/1000;//ȡǧλ
Hun=(Num%1000)/100;//ȡ��λ
Shi=(Num%100)/10;//ȡʮλ
Ge=(Num%100)%10;//ȡ��λ

//���ǧλ��Ϊ0������ʾ��λ��ʮλ����λ
if(Qian!=0)
 {
 OLED_ShowMiniNum(x,y,Qian,1);//��ʾǧλ
 OLED_ShowMiniNum(x+6,y,Hun,1);//��ʾ��λ
 OLED_ShowMiniNum(x+12,y,Shi,1);//��ʾʮλ
 OLED_ShowMiniNum(x+18,y,Ge,1);//��ʾ��λ
 return;//�˳�
 }

 //�����λ��Ϊ0������ʾ��λ��ʮλ����λ
if(Hun!=0)
 {
 OLED_ShowMiniNum(x+3,y,Hun,1);//��ʾ��λ
 OLED_ShowMiniNum(x+9,y,Shi,1);//��ʾʮλ
 OLED_ShowMiniNum(x+15,y,Ge,1);//��ʾ��λ
 return;//�˳�
 }
//���ʮλ��Ϊ0����ʾʮλ��λ
if(Shi!=0)
 {
 OLED_ShowMiniNum(x+6,y,Shi,1);//��ʾʮλ
 OLED_ShowMiniNum(x+12,y,Ge,1);//��ʾ��λ
 return;//�˳�
 }
if(Ge!=0)OLED_ShowMiniNum(x+9,y,Ge,1);//��ʾ��λ
else OLED_ShowMiniNum(x+9,y,0,1);//��ʾ0
}
//OLED�ϵ���Ϩ��Ч��
void OLED_OldTVFade(void)
{
int i;
for(i=1;i<16;i++)
	{
	OLED_Fill(0,0,63,i,0); //�ϰ벿��
	OLED_Fill(0,32-i,63,i,0);//�°벿��
	OLED_DrawLine(0,i,63,i,1); 
	OLED_DrawLine(0,32-i,63,32-i,1); //��ʧ��
	OLED_Refresh(); 
	delay_ms(5); //ʣ��һ����
	}
OLED_Clear();
OLED_DrawLine(0,16,63,16,1); //��ʧ��
OLED_Refresh(); 
for(i=0;i<31;i++)
	{
	OLED_Clear();
	OLED_DrawLine(i,16,63-i,16,1); //��ʧ��
	OLED_Refresh(); 
	delay_ms(5); //ʣ��һ����
	}
//���������в���
OLED_Clear();
OLED_Refresh();
}

//���OLED��GRAM�����Ƿ���ͼ������
bool OLED_CheckIsGRAMHasImage(void)
{
long i,n;
for(i=0;i<OLEDVertSize/8;i++)
	{
	for(n=0;n<OLEDHoriSize;n++)
			{
			if(OLED_GRAM[n][i]!=0x00)return true;//��ǰGRAM���滹��ͼ��
			}
  }
return false;
}

//OLEDͼ����ʧЧ��
void OLED_ImageDisappear(void)
{
 long i,n;
 char FreshCount=0;
	for(i=0;i<OLEDVertSize/8;i++)
	{
	   for(n=0;n<OLEDHoriSize;n++)
			{
			 OLED_GRAM[n][i]=0;//�����������
			 FreshCount++;
			 if(FreshCount==4)
			  {
				FreshCount=0;
				OLED_Refresh();
				}
			delay_ms(2);
			}
  }
}
#pragma pop
