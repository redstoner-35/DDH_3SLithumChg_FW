#include "Pindefs.h"
#include "IP2368.h"
#include "delay.h"
#include "oled.h"
#include "Config.h"

//电源管理引脚自动定义
#define LDO_EN_IOB STRCAT2(GPIO_P,LDO_EN_IOBank)
#define LDO_EN_IOG STRCAT2(HT_GPIO,LDO_EN_IOBank)
#define LDO_EN_IOP STRCAT2(GPIO_PIN_,LDO_EN_IOPinNum) 

//外部和内部全局变量
extern int SleepTimer;  //睡眠定时器
static int IPStallTime=0; //芯片停止响应的时间

//初始化IO完成自举操作
void PowerMgmtSetup(void)
  {
   AFIO_GPxConfig(LDO_EN_IOB,LDO_EN_IOP, AFIO_FUN_GPIO);
   GPIO_DirectionConfig(LDO_EN_IOG,LDO_EN_IOP,GPIO_DIR_OUT);//配置为输出
   GPIO_SetOutBits(LDO_EN_IOG,LDO_EN_IOP);//输出设置为1
	 GPIO_DriveConfig(LDO_EN_IOG,LDO_EN_IOP,GPIO_DV_16MA);	//设置为16mA最大输出保证指示灯亮度足够
	}

//关闭MCU
void ForceShutOff(void)	
  {
	GPIO_ClearOutBits(LDO_EN_IOG,LDO_EN_IOP);//输出设置为0,关闭LDO电源强迫单片机掉电
	while(1);	
	}

//Type-C连接失败时，进行重新握手的部分
static void IP2368StallRestore(void)
  {
	int wait;
	//计时累加部分
	IPStallTime++;
	if(IPStallTime>=40) //IP2368掉线大约5秒后开始重试
	  {
		IPStallTime=0;  //操作复位，等待下次计时
		GPIO_InputConfig(IP2368_INT_IOG,IP2368_INT_IOP,ENABLE); 
	  GPIO_DirectionConfig(IP2368_INT_IOG,IP2368_INT_IOP,GPIO_DIR_IN);//设置为高阻输入使2368休眠
		wait=300; //唤醒检测延时300mS
		do
		  {
			wait--;
      if(wait<=0)break;	//200mS后IP2368唤醒失败，退出
			delay_ms(1);		
			}
		while(GPIO_ReadInBit(IP2368_INT_IOG,IP2368_INT_IOP)==SET); //IP2368位于唤醒状态，等待唤醒结束
		if(wait>0)delay_ms(100); //等待100mS后拉高INT
		GPIO_InputConfig(IP2368_INT_IOG,IP2368_INT_IOP,DISABLE); 
	  GPIO_DirectionConfig(IP2368_INT_IOG,IP2368_INT_IOP,GPIO_DIR_OUT);//禁用IDR，设置为输出
	  GPIO_SetOutBits(IP2368_INT_IOG,IP2368_INT_IOP); //令INT保持在1,使IP2368永远唤醒不得睡眠	
		if(wait>0)delay_ms(200); //强制拉高INT之后IP2368在200mS后才能访问寄存器
    }
	}	
	
//Typec接入时进行初始化的部分
void TypeCInsertInitHandler(void)
  {
	
	bool IsTypeCSrc=false,IsReset=false;
	static bool IsSrcDetachReInit=false; //当SRC负载接入时，进行re-init
	QuickChargeCtrlDef QCtrl;
	//从IP2368获取当前OTP和Type-C的状态
	if(!IP2368_CheckIfOTPLoaded(&IsReset)||!IP2368_GetIsTypeCSrcConnected(&IsTypeCSrc))
	  {
		IP2368StallRestore();
		return;  //开始累加时间，如果时间到则进行恢复操作
		}  
	else IPStallTime=0; //IP2368正常运行，计时器复位
	//如果寄存器里面的内容没有发生变动，则返回
	if(IsSrcDetachReInit!=IsTypeCSrc&&!IsReset)return; 
	IsSrcDetachReInit=IsTypeCSrc; //同步状态
	if(IsSrcDetachReInit)return; //Type-C source模式下当从未插入到插入时，不响应初始化操作
	//准备快充状态控制数据
	QCtrl.IsEnableDPDM=Config.IsEnableDPDM;
	QCtrl.IsEnablePD=Config.IsEnablePD;
	QCtrl.IsEnableSCP=Config.IsEnableSCP;
	QCtrl.IsEnable9VPDO=Config.IsEnable9VPDO;
	QCtrl.IsEnable20VPDO=Config.IsEnable20VPDO;		
	//重新初始化			
	IP2368_SetPreChargeEndVoltage(PreCharge_End2V9); //设置预充电结束电压为2.9V
	if(!IsTypeCSrc)
	  {		
		IP2368_SetChargePower(ConvertChagePower()); //设置充电功率	
		IP2368_SetChargeParam(100,ReCharge_4V10); //设置充电结束电流和再冲电电压
		}
	IP2368_SetQuickchargeState(&QCtrl); //设置快充配置
	IP2368_SetLowVoltAlert(LVAlert_2V8);	 //设置低压告警
	IP2368_SetDischarge(Config.IsEnableOutput); //设置输出方向
	}	
	
//休眠状态判断
void PowermanagementSleepControl(void)
  {
	//当前未处于睡眠状态，不执行
	if(SleepTimer!=-1)return;
	//开始检测
	GPIO_InputConfig(IP2368_INT_IOG,IP2368_INT_IOP,ENABLE); 
	GPIO_DirectionConfig(IP2368_INT_IOG,IP2368_INT_IOP,GPIO_DIR_IN);//设置为高阻输入使2368休眠
	delay_ms(40);
  if(GPIO_ReadInBit(IP2368_INT_IOG,IP2368_INT_IOP)==SET)return; //IP2368仍然在正常运行，退出	
	OLED_Clear();
	OLED_Refresh(); //清除屏幕内的所有内容
	GPIO_ClearOutBits(LDO_EN_IOG,LDO_EN_IOP);//输出设置为0,关闭LDO电源强迫单片机掉电
	while(1);		
	}
