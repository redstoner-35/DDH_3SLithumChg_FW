#include "delay.h"
#include "LEDMgmt.h"
#include <string.h>

/*LED闪烁的pattern(每阶段0.25秒)
0=熄灭，
1=绿色，
2=红色  
E=强制结束当前pattern
*/
#define LEDPatternSize 32
const char *LEDPattern[LEDPatternSize]=
 {
   "00",//LED熄灭 0
	 "111",//绿灯常亮 1
	 "333",//黄灯常亮 2
   "222", //红灯常亮 3
	 //状态指示
   "10101010", //绿色快闪 充电即将完成 4
	 "20202020", //红色快闪 充/放电遇到异常 5
	 "30303030", //黄色快闪 正在等待充电中 6
	 "2D0D2D0D", //红色慢闪 电池即将耗尽 7
	 "3D0D3D0D", //黄色慢闪 电池电量已不足 8
	 "1D0D1D0D", //绿色慢闪 电池电量充足 9
	 //错误提示
	 "2020D10DD", //红色闪2次绿色闪1次 OLED屏幕启动失败 10
	 "2020020D100DD",//红灯闪3次绿灯一次 IP2368设置充电状态失败 11
	 "2020020D10100DD",//红灯闪3次绿灯2次 IP2368获取电池电压失败 12
	 "2020D10100DD", //IP2368 INT无反应启动失败 13
	 //放电空载提示
	 "2DDD020202DDD", //红色每6秒闪2次 指示放电中输出处于空载状态且电池即将耗尽 14
	 "3DDD030303DDD", //黄色每6秒闪2次 指示放电中输出处于空载状态且电池电量已不足 15
	 "1DDD010101DDD", //绿色每6秒闪2次 指示放电中输出处于空载状态且电池电量充足 16
	 NULL//结束符
 };
//变量
char LEDModeStr[64]; //LED模式的字符串
static int ConstReadPtr;
static int LastLEDIndex;
static bool IsLEDInitOK=false; //LED是否初始化完毕
volatile int CurrentLEDIndex;
volatile static short LEDDelayTimer=0;
char *ExtLEDIndex = NULL; //用于传入的外部序列
static char *LastExtLEDIdx = NULL;

//仅一次显示循环的LED内容 
void LED_ShowLoopOperationOnce(int index) 
  {
	LED_Reset();//复位LED状态机
	CurrentLEDIndex=index;//开始显示
	while(ConstReadPtr==0)delay_ms(1); //等待系统开始显示
	delay_ms(10);
	while(ConstReadPtr>0)delay_ms(1); //等待显示结束
  CurrentLEDIndex=0;//立即使LED熄灭
	}

//往自定义LED缓存里面加上闪烁字符
void LED_AddStrobe(int count,const char *ColorStr) 
  {
	int gapcnt;
	if(count<=0)return; //输入的次数非法
	//开始显示
	gapcnt=0;
  while(count>0)
   {
	 //附加闪烁次数
	 strncat(LEDModeStr,ColorStr,sizeof(LEDModeStr)-1);	//使用黄色代表电压个位数
	 //每2次闪烁之间插入额外停顿方便用户计数
	 if(gapcnt==1)
	    {
		  gapcnt=0;
		  strncat(LEDModeStr,"0",sizeof(LEDModeStr)-1);
		  }
	 else gapcnt++;
	 //处理完一轮，减去闪烁次数
	 count--;
	 }
	}
//复位LED管理器从0开始读取
void LED_Reset(void)
  {
  GPIO_ClearOutBits(LED_Green_IOG,LED_Green_IOP);//输出设置为0
  GPIO_ClearOutBits(LED_Red_IOG,LED_Red_IOP);//输出设置为0	 	
	ConstReadPtr=0;//从初始状态开始读取
	}
//LED控制器初始化	
void LED_Init(void)
  {
	 //配置GPIO(绿色LED)
   AFIO_GPxConfig(LED_Green_IOB,LED_Green_IOP, AFIO_FUN_GPIO);
   GPIO_DirectionConfig(LED_Green_IOG,LED_Green_IOP,GPIO_DIR_OUT);//配置为输出
   GPIO_ClearOutBits(LED_Green_IOG,LED_Green_IOP);//输出设置为0
	 GPIO_DriveConfig(LED_Green_IOG,LED_Green_IOP,GPIO_DV_16MA);	//设置为16mA最大输出保证指示灯亮度足够
	 //配置GPIO(红色LED)
   AFIO_GPxConfig(LED_Red_IOB,LED_Red_IOP, AFIO_FUN_GPIO);
   GPIO_DirectionConfig(LED_Red_IOG,LED_Red_IOP,GPIO_DIR_OUT);//配置为输出
   GPIO_ClearOutBits(LED_Red_IOG,LED_Red_IOP);//输出设置为0	 		
	 GPIO_DriveConfig(LED_Red_IOG,LED_Red_IOP,GPIO_DV_16MA);	//设置为16mA最大输出保证指示灯亮度足够
	 //初始化变量
	 ConstReadPtr=0;
	 LastLEDIndex=0;
	 CurrentLEDIndex=0;//LED熄灭
	 IsLEDInitOK=true; //LED GPIO初始化已完毕
	 memset(LEDModeStr,0,sizeof(LEDModeStr)); //初始化灯光引擎
	}
//在系统内控制LED的回调函数
void LEDMgmt_CallBack(void)
  {
	//安全措施，保证GPIO配置后才执行LED操作
	if(!IsLEDInitOK)return;
	//检测到主机配置了新的LED状态
  if(CurrentLEDIndex!=LastLEDIndex||LastExtLEDIdx!=ExtLEDIndex)
	  {
		if(LastExtLEDIdx!=ExtLEDIndex)LastExtLEDIdx=ExtLEDIndex;
		if(CurrentLEDIndex!=LastLEDIndex)LastLEDIndex=CurrentLEDIndex;//同步index 
		LED_Reset();//复位LED状态机
		}		
  //正常执行		
	else if(LEDDelayTimer>0)//当前正在等待中
	  {
		LEDDelayTimer--;
		return;
		}
	if(LEDPattern[LastLEDIndex<LEDPatternSize?LastLEDIndex:0]==NULL)return;//指针为空，不执行	
	switch(LastExtLEDIdx==NULL?LEDPattern[LastLEDIndex<LEDPatternSize?LastLEDIndex:0][ConstReadPtr]:LastExtLEDIdx[ConstReadPtr])
	  {
		case 'D'://延时1秒
		  {
			ConstReadPtr++;//指向下一个数字
			LEDDelayTimer=8;
			break;
			}
		case '0'://LED熄灭
		  {
			GPIO_ClearOutBits(LED_Green_IOG,LED_Green_IOP);
		  GPIO_ClearOutBits(LED_Red_IOG,LED_Red_IOP);
			ConstReadPtr++;//指向下一个数字
			break;
			}
		case '1'://绿色
		  {
			GPIO_SetOutBits(LED_Green_IOG,LED_Green_IOP);
		  GPIO_ClearOutBits(LED_Red_IOG,LED_Red_IOP);
			ConstReadPtr++;//指向下一个数字
			break;
			}		
		case '2'://红色
		  {
			GPIO_ClearOutBits(LED_Green_IOG,LED_Green_IOP);
		  GPIO_SetOutBits(LED_Red_IOG,LED_Red_IOP);
			ConstReadPtr++;//指向下一个数字
			break;
			}	
		case '3'://红绿一起亮(橙色)
		  {
			GPIO_SetOutBits(LED_Green_IOG,LED_Green_IOP);
		  GPIO_SetOutBits(LED_Red_IOG,LED_Red_IOP);
			ConstReadPtr++;//指向下一个数字
			break;				
			}
		case 'E'://结束显示
		  {
			GPIO_ClearOutBits(LED_Green_IOG,LED_Green_IOP);
		  GPIO_ClearOutBits(LED_Red_IOG,LED_Red_IOP);
			if(LastExtLEDIdx!=NULL)ExtLEDIndex=NULL;//使用的是外部传入的pattern，所以说需要清除指针
			else CurrentLEDIndex=0;//回到熄灭状态
			ConstReadPtr=0;//回到第一个参数
			break;
			}	
		default:ConstReadPtr=0;//其他任何非法值，直接回到开始点
		}
	}
