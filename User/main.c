#include "ht32.h"
#include "delay.h"
#include "SideKey.h"
#include "LEDMgmt.h"
#include "I2C.h"
#include "oled.h"
#include "IP2368.h"
#include "Config.h"

//函数声明
void CheckForFlashLock(void);
void SleepTimerHandler(void);
void MenuHandler(void);
void MenuKeyHandler(void);
void STATULEDHandler(void);
void DisplaySplash(void);
void PowermanagementSleepControl(void);
void PowerMgmtSetup(void);
void MenuInit(void);
void TypeCInsertInitHandler(void);
void ResetConfigDetection(void);

//常量
bool SensorRefreshFlag=false;
bool EnteredMainApp=false;

int main(void)
 {
//初始化系统时钟
 CKCU_PeripClockConfig_TypeDef CLKConfig={{0}};
 CLKConfig.Bit.PA=1;
 CLKConfig.Bit.PB=1;
 CLKConfig.Bit.PC=1;
 CLKConfig.Bit.AFIO=1;
 CLKConfig.Bit.EXTI=1;
 CLKConfig.Bit.BKP = 1;
 CKCU_PeripClockConfig(CLKConfig,ENABLE);
 //启动SYSTICK和电源锁定引脚
 delay_init(); //启动延时函数
 PowerMgmtSetup();//立即打开LDO使能IO完成自举
 //初始化LSI和RTC
 while((PWRCU_CheckReadyAccessed() != PWRCU_OK)){}; //等待BKP可以访问
 RTC_LSILoadTrimData();//加载LSI的修正值
 RTC_DeInit();//复位RTC确保RTC不要在运行  
 //其余外设初始化
 CheckForFlashLock();//锁定存储区
 EnableHBTimer(); //初始化systick和定时器
 LED_Init(); //初始化LED管理器
 SideKeyInit();//初始化侧按按钮
 SMBUS_Init(); //初始化SMBUS
 ReadConfig();//从Flash里面读取配置
 IP2368_GPIOInit();//初始化IP2368的GPIO等
 OLED_Init();//初始化OLED
 DisplaySplash();//显示开场动画
 IP2368_init();//初始化IP2368
 ResetConfigDetection();//初始化配置
 MenuInit();//对菜单进行初始化
 EnteredMainApp=true; //已进入主APP
 //主循环
 while(1)
   {
	 //处理充电节自身的逻辑事务
	 SideKey_LogicHandler();//处理侧按按键事务 
	 PowermanagementSleepControl();//处理电源管理和睡眠
	 MenuHandler();//完成IP2368的轮询和菜单处理
	 MenuKeyHandler();//完成菜单操作的按键处理
	 STATULEDHandler();//完成状态指示灯的处理以及线性充电功率控制	  
	 //0.125S软件定时处理
	 if(!SensorRefreshFlag)continue;	
	 TypeCInsertInitHandler();//执行Typec接入的初始化   	 
	 SleepTimerHandler();//处理睡眠指示灯的控制
	 LEDMgmt_CallBack();//处理电量指示灯的逻辑
	 SensorRefreshFlag=false;
	 //额外的延时避免主循环运行过快
	 delay_ms(5);
	 }
 return 0;
 }
