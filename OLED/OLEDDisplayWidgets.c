#include "oled.h"
#include "delay.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

//5x5 ASCII字库
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
	{0x01,0x00,0x1C,0x14,0x14},//'℃'
	{0x14,0x00,0x00,0x00,0x00}//':'
};
#pragma push
#pragma Otime//优化该函数使用3级别优化
#pragma O3
//画点 
//x:0~127
//y:0~63
//t:1 填充 0,清空	
void OLED_DrawPoint(char x,char y,char t)
{
	char i,m,n;
	//判断参数
  if(x>(OLEDHoriSize-1))return;
	if(y>(OLEDVertSize-1))return;
	//实际画点操作
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

//画线
//x1,y1:起点坐标
//x2,y2:结束坐标
void OLED_DrawLine(char x1,char y1,char x2,char y2,char mode)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
		//判断参数
  if(x1>(OLEDHoriSize-1))return;
	if(y1>(OLEDVertSize-1))return;
	if(x2>(OLEDHoriSize-1))return;
	if(y2>(OLEDVertSize-1))return;
	//开始画线
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1;
	uRow=x1;//画线起点坐标
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步方向 
	else if (delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//水平线 
	else {incy=-1;delta_y=-delta_x;}
	if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		OLED_DrawPoint(uRow,uCol,mode);//画点
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
//x,y:圆心坐标
//r:圆的半径
void OLED_DrawCircle(char x,char y,char r)
{
	int a, b,num;
    a = 0;
    b = r;
		//判断参数
    if(x>(OLEDHoriSize-1))return;
	  if(y>(OLEDVertSize-1))return;
	  //开始绘制
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
        num = (a * a + b * b) - r*r;//计算画的点离圆心的距离
        if(num > 0)
        {
            b--;
            a--;
        }
    }
}

//x,y：起点坐标
//sizex,sizey,图片长宽
//const char *BMP：要写入的图片数组
//mode:0,反色显示;1,正常显示
void OLED_ShowPicture(char x,char y,char sizex,char sizey,const char *BMP,char mode)
{
	u16 j=0;
	u8 i,n,temp,m;
	u8 x0=x,y0=y;
	sizey=sizey/8+((sizey%8)?1:0);
	//判断参数
  if(x>(OLEDHoriSize-1))return;
	if(y>(OLEDVertSize-1))return;
	//开始绘制
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

//渲染单个5x5 Mini字符的函数
void OLED_ShowSingleMINIASCII(char x,char y,char Code,char mode)
{
  int j=0;
	char i,n,temp,m;
	char x0=x,y0=y;
	int sizey;
	//判断参数
  if(x>(OLEDHoriSize-1))return;
	if(y>(OLEDVertSize-1))return;
	//判断ASCII代码是否合法	e
	if(Code==0x7C)Code=0x5B; //特殊字符0x7C，显示摄氏温标
	else if(Code==0x7E)Code=0x5C; //特殊字符0x7E，显示紧贴的冒号
	else if(Code>0x60&&Code<0x7B)Code=Code-0x20;//小写字母区域，因为字符集没有小写字母，使用大写字母表示
	else if(Code<0x20||Code>0x5A)Code=0x3F;//超出显示范围的非法字符，显示'?'	
	Code=Code-0x20;//将ASCII字符代码转换为数组位置
  //启动显示
	sizey=5/8+((5%8)?1:0);
	for(n=0;n<sizey;n++)
	{
		 for(i=0;i<5;i++)
		 {
				temp=MINI_ASCII[Code][j];//取bitmap
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
//用Mini ASCII显示任意字符串（带自动换行）
void OLED_ShowStringViaMiniASCII(char Startx,char Starty,char MaxmiumLenPerLine,char *StringIN,int len,int mode)
 {
	int POS=0,remaingap;//指针位置
	int ColumnPOS=Startx;//当前列的位置（X轴）
  //判断参数
  if(Startx>(OLEDHoriSize-1))return;
	if(Starty>(OLEDVertSize-1))return;
  //开始显示
	while(StringIN[POS]>0x1F&&StringIN[POS]<0x7F&&POS<len)//判断是否为非法字符
	 {
	 //计算下一个新的文字所需要的大小
	 if(StringIN[POS]=='I'||StringIN[POS]=='i')remaingap=4;
	 else if(StringIN[POS]=='.'||StringIN[POS]==0x61)remaingap=1;
	 else remaingap=5;
	 //判断是否需要换行
	 if(((Startx+MaxmiumLenPerLine)-ColumnPOS)<remaingap||(ColumnPOS+remaingap)>(OLEDHoriSize-1))//当前显示的内容到达行的末尾或者即将超出屏幕
	  {
		if(Starty+6>63)return;//没法换行了，直接退出
		Starty=Starty+6;//向下移动一行
		ColumnPOS=Startx;//回到开头
		} 
	 //显示内容，然后到下一个字符
	 OLED_ShowSingleMINIASCII(ColumnPOS,Starty,StringIN[POS],mode);//显示一个字符
	 if(StringIN[POS]=='I'||StringIN[POS]=='i')ColumnPOS+=5;
	 else if(StringIN[POS]=='.'||StringIN[POS]==0x7E)ColumnPOS+=2;
	 else ColumnPOS+=6;//光标（看不见那种）右移一个字符的空间
	 POS++;//显示下一个字符
	 }
 }
//用OLED实现Printf
void OLED_Printf(char Startx,char Starty,char MaxmiumLenPerLine,char mode,char *Format,...)
 {
 va_list arg;
 char SBUF[100]={0};
 __va_start(arg,Format);
 vsnprintf(SBUF,100,Format,arg);
 __va_end(arg);
 OLED_ShowStringViaMiniASCII(Startx,Starty,MaxmiumLenPerLine,SBUF,100,mode);
 }
//用OLED实现指定输出的Printf
void OLED_Printfn(char Startx,char Starty,char MaxmiumLenPerLine,int n,char mode,char *Format,...)
 {
 va_list arg;
 char SBUF[100]={0};
 if(n<=0)return;//长度数值非法
 __va_start(arg,Format);
 vsnprintf(SBUF,100,Format,arg);
 __va_end(arg);
 if(n>100)n=100;//设定数值
 OLED_ShowStringViaMiniASCII(Startx,Starty,MaxmiumLenPerLine,SBUF,n,mode);
 } 
//显示迷你数字（5x5）
void OLED_ShowMiniNum(char x,char y,char Number,char mode)
{
u16 j=0;
	u8 i,n,temp,m;
	u8 x0=x,y0=y;
	int sizey;
	//判断参数
  if(x>(OLEDHoriSize-1))return;
	if(y>(OLEDVertSize-1))return;
	//开始显示
	sizey=5/8+((5%8)?1:0);
	for(n=0;n<sizey;n++)
	{
		 for(i=0;i<5;i++)
		 {
				temp=MINI_ASCII[Number+0x11][j];//取bitmap
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

//显示整数数字（3位）
void OLED_Show3DigitInt(char x,char y,int Num)
{
int Hun;
int Shi;
int Ge;
//判断参数
if(x>(OLEDHoriSize-1))return;
if(y>(OLEDVertSize-1))return;
if(Num>999|Num<-999)return;//检查数值是否非法
//如果是负数那就转成正数
if(Num<0)
 {
  x=x+6;//偏移出负数前面的负号的位置
	OLED_ShowMiniNum(x-6,y,16,1);
	OLED_ShowMiniNum(x,y,16,1);
  OLED_ShowMiniNum(x+6,y,16,1);
  OLED_ShowMiniNum(x+12,y,16,1);//清空之前显示的区域
  Num=Num*-1;//负数转正数
	OLED_ShowMiniNum(x-6,y,18,1);//显示负号
 }
else
 {
 OLED_ShowMiniNum(x,y,16,1);
 OLED_ShowMiniNum(x+6,y,16,1);
 OLED_ShowMiniNum(x+12,y,16,1);//清空之前显示的区域
 }
//把各个位分离出来 
Hun=Num/100;//取百位
Shi=(Num%100)/10;//取十位
Ge=(Num%100)%10;//取个位

//如果百位不为0，则显示百位，十位，个位
if(Hun!=0)
 {
 OLED_ShowMiniNum(x,y,Hun,1);//显示百位
 OLED_ShowMiniNum(x+6,y,Shi,1);//显示十位
 OLED_ShowMiniNum(x+12,y,Ge,1);//显示个位
 return;//退出
 }
//如果十位不为0则显示十位个位
if(Shi!=0)
 {
 OLED_ShowMiniNum(x,y,Shi,1);//显示十位
 OLED_ShowMiniNum(x+6,y,Ge,1);//显示个位
 return;//退出
 }
if(Ge!=0)OLED_ShowMiniNum(x,y,Ge,1);//显示个位
}
//渲染进度条，5宽,最长100长，按百分比控制长度
void OLED_ShowProgressBar(char x,char y,char Presentage)
{
int ENDX;
int ENDY;
int i;
//首先检查长度是不是有效值
if(x>(OLEDHoriSize-1))return;
if(y>(OLEDVertSize-1))return;
if(Presentage<1)return;//小于1的不显示
if(Presentage>100)Presentage=100;//大于100%的按100算
//计算结束点
ENDX=x+Presentage;//X结束点为开始坐标+长度
ENDY=y+5;//结束点坐标为Y+5，因为进度条是5宽
i=y;//将y轴数据赋给i
//先清零
while(i<ENDY)
 {
 OLED_DrawLine(x,i,x+100,i,0);//将对应区域的进度条给划掉
 i++;
 }
//然后画进度条
i=y;//将y轴数据赋给i	
while(i<ENDY)
 {
 OLED_DrawLine(x,i,ENDX,i,1);//画出新的进度条	
 i++;
 }	
}

//渲染浮点数字，0-999
void OLED_ShowFloatNum(float Num,char x,char y)
{
float NumCopy;//复制一份数据
int INUM;//整数区域
int Xnum;//小数区域
//判断参数
if(x>(OLEDHoriSize-1))return;
if(y>(OLEDVertSize-1))return;
if(Num>999|Num<0)return;//检查数值是否非法
//分出整数和小数部分
NumCopy=(float)Num;
INUM=(int)NumCopy;//强制取整
Num=Num-(float)INUM;//去掉元数据中的整数部分
Num=Num*(float)100;//放大小数点后2位的数据
Xnum=(int)Num;//放大之后的小数区域取整,此时后两位的数据就在这个变量里面
Xnum=Xnum%100;//除100取余让小数区只能在0-99范围内，即保留两位小数
//渲染数字
if(INUM>99)//如果数字大于99
  {
	OLED_Show3DigitInt(x,y,INUM);//直接渲染数字，不渲染小数点后面的
	return;//退出
	}
if(INUM>9)//如果数字大于9
 {
 OLED_Show3DigitInt(x,y,INUM);//先渲染整数部分
 x=x+12;
 OLED_DrawPoint(x,y+4,1);//X偏移小数点后两位，画小数点
 x=x+2;
 Xnum=Xnum/10;//除10保留小数点后一位
 OLED_ShowMiniNum(x,y,Xnum,1);//渲染小数点后一位
 return;//退出
 }
OLED_ShowMiniNum(x,y,INUM,1);//渲染整数区域
x=x+6;
OLED_DrawPoint(x,y+4,1);//X偏移小数点后1位，画小数点
x=x+2;
//检测，如果小数区域为两个0则渲染00
if(Xnum==0)
  {
  OLED_ShowMiniNum(x,y,0,1);
  OLED_ShowMiniNum(x+6,y,0,1);
  return;
  }
//检测，如果小数区域只有小数后面1位则渲染一个0加小数后面一位
if((Xnum%10)!=0&&(Xnum/10)==0)
  {
	OLED_ShowMiniNum(x,y,0,1);
	Xnum=Xnum%10;//取小数点后第二位
  OLED_ShowMiniNum(x+6,y,Xnum,1);
  return;
	}
OLED_Show3DigitInt(x,y,Xnum);//显示两位小数
}

//填充函数
void OLED_Fill(char x,char y,char sizex,char sizey,char mode)
{
int i;
int endy=y+sizey;
//判断参数
if(x>(OLEDHoriSize-1))return;
if(y>(OLEDVertSize-1))return;
//开始填充
for(i=y;i<endy;i++)
OLED_DrawLine(x,i,x+sizex,i,mode);
}
//显示空心长方形
void OLED_DrawRectangle(char x1,char y1,char x2,char y2,char mode)
{
//判断参数
if(x1>(OLEDHoriSize-1))return;
if(y1>(OLEDVertSize-1))return;
if(x2>(OLEDHoriSize-1))return;
if(y2>(OLEDVertSize-1))return;
//开始绘制
OLED_DrawLine(x1,y1,x2,y1,mode);
OLED_DrawLine(x1,y1,x1,y2,mode);
OLED_DrawLine(x2,y1,x2,y2,mode);
OLED_DrawLine(x1,y2,x2+1,y2,mode);
}
//显示按钮
void OLED_DisplayButton(char x,char y,char *Text,bool IsSelect)
{
 //判断参数
 if(x>(OLEDHoriSize-1))return;
 if(y>(OLEDVertSize-1))return;
 //显示按钮文本
 if(Text==NULL)return;
 if(IsSelect)OLED_Fill(x,y,(strlen(Text)*6)+2,9,1);//先填充背景
 OLED_Printf(x+2,y+2,strlen(Text)*6,!IsSelect,"%s",Text);
 OLED_Fill(x+2,y+8,strlen(Text)*6,8,0);//反色字体下面的内容抠干净
 //外围画框
 OLED_DrawLine(x,y,x+(strlen(Text)*6)+2,y,!IsSelect);
 OLED_DrawLine(x+(strlen(Text)*6)+2,y,x+(strlen(Text)*6)+2,y+8,!IsSelect);
 OLED_DrawLine(x,y,x,y+8,!IsSelect);
 OLED_DrawLine(x,y+8,x+(strlen(Text)*6)+3,y+8,!IsSelect);
}

//显示四位整数（0-9999）
void OLED_Show4DigitInt(char x,char y,int Num)
{
int Hun;
int Shi;
int Ge;
int Qian;
//判断参数
if(x>(OLEDHoriSize-1))return;
if(y>(OLEDVertSize-1))return;
if(Num>9999|Num<0)return;//检查数值是否非法
//清空之前显示的区域
 OLED_ShowMiniNum(x,y,16,1);
 OLED_ShowMiniNum(x+6,y,16,1);
 OLED_ShowMiniNum(x+12,y,16,1);
 OLED_ShowMiniNum(x+18,y,16,1);//清空之前显示的区域
//把各个位分离出来
Qian=Num/1000;//取千位
Hun=(Num%1000)/100;//取百位
Shi=(Num%100)/10;//取十位
Ge=(Num%100)%10;//取个位

//如果千位不为0，则显示百位，十位，个位
if(Qian!=0)
 {
 OLED_ShowMiniNum(x,y,Qian,1);//显示千位
 OLED_ShowMiniNum(x+6,y,Hun,1);//显示百位
 OLED_ShowMiniNum(x+12,y,Shi,1);//显示十位
 OLED_ShowMiniNum(x+18,y,Ge,1);//显示个位
 return;//退出
 }

 //如果百位不为0，则显示百位，十位，个位
if(Hun!=0)
 {
 OLED_ShowMiniNum(x+3,y,Hun,1);//显示百位
 OLED_ShowMiniNum(x+9,y,Shi,1);//显示十位
 OLED_ShowMiniNum(x+15,y,Ge,1);//显示个位
 return;//退出
 }
//如果十位不为0则显示十位个位
if(Shi!=0)
 {
 OLED_ShowMiniNum(x+6,y,Shi,1);//显示十位
 OLED_ShowMiniNum(x+12,y,Ge,1);//显示个位
 return;//退出
 }
if(Ge!=0)OLED_ShowMiniNum(x+9,y,Ge,1);//显示个位
else OLED_ShowMiniNum(x+9,y,0,1);//显示0
}
//OLED老电视熄灭效果
void OLED_OldTVFade(void)
{
int i;
for(i=1;i<16;i++)
	{
	OLED_Fill(0,0,63,i,0); //上半部分
	OLED_Fill(0,32-i,63,i,0);//下半部分
	OLED_DrawLine(0,i,63,i,1); 
	OLED_DrawLine(0,32-i,63,32-i,1); //消失线
	OLED_Refresh(); 
	delay_ms(5); //剩下一条线
	}
OLED_Clear();
OLED_DrawLine(0,16,63,16,1); //消失线
OLED_Refresh(); 
for(i=0;i<31;i++)
	{
	OLED_Clear();
	OLED_DrawLine(i,16,63-i,16,1); //消失线
	OLED_Refresh(); 
	delay_ms(5); //剩下一条线
	}
//最后完成所有操作
OLED_Clear();
OLED_Refresh();
}

//检查OLED的GRAM里面是否还有图像数据
bool OLED_CheckIsGRAMHasImage(void)
{
long i,n;
for(i=0;i<OLEDVertSize/8;i++)
	{
	for(n=0;n<OLEDHoriSize;n++)
			{
			if(OLED_GRAM[n][i]!=0x00)return true;//当前GRAM里面还有图像
			}
  }
return false;
}

//OLED图像消失效果
void OLED_ImageDisappear(void)
{
 long i,n;
 char FreshCount=0;
	for(i=0;i<OLEDVertSize/8;i++)
	{
	   for(n=0;n<OLEDHoriSize;n++)
			{
			 OLED_GRAM[n][i]=0;//清除所有数据
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
