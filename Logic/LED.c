#include "IP2368.h"
#include <math.h>
#include "string.h"
#include "LEDMgmt.h"
#include "ht32.h"
#include "Pindefs.h"
#include "Config.h"
#include "oled.h"
#include "delay.h"

//外部变量和函数
void ForceShutOff(void);
bool IsMenuAtBatteryTelem(void);
int iroundf(float IN); //浮点取整四舍五入
extern int SleepTimer;  //睡眠定时器
static bool IsOTFaultAsserted=false; //是否启动过热报警
static bool IsUTFaultAsserted=false; //是否启动低温报警
extern int ConnectTimeoutCNT;

//状态LED指示(包括温控逻辑)
void STATULEDHandler(void)
  {
	BatteryStatuDef BattState;
	FaultFlagStrDef Fault;
	TypeCStatusDef TypeCState;
	bool State,IsNTCOK,VDState;
	static bool IsDischargeEnabled=true; //充电包括放电是否使能
	float TempResult;
	//获取IP2368状态	
	State=IP2368_GetBatteryState(&BattState);
	State&=IP2368_GetTypeCState(&TypeCState);
	State&=IP2368_GetAlertFlag(&Fault);
	State&=IP2368_GetNTCTemp(&TempResult,&IsNTCOK);
	//开始控制LED
	if(!State&&ConnectTimeoutCNT<250)CurrentLEDIndex=0; //正在重新连接等待，指示灯熄灭	
	else if(!State||Fault.FaultAsserted)CurrentLEDIndex=5;//出现异常
	else if(SleepTimer>0)CurrentLEDIndex=0; //LED指示灯亮度很高，屏幕点亮时为了避免影响阅读故关闭
	else switch(BattState.BattState)
	  {
		case Batt_CCCharge:CurrentLEDIndex=2;break; //恒流充电，黄色常亮
		case Batt_ChgDone:CurrentLEDIndex=1;break; //充电结束，绿色常亮
		case Batt_ChgError:CurrentLEDIndex=5;break; //充电出现异常		
		case Batt_ChgWait:CurrentLEDIndex=6;break; //正在等待充电过程开始
		case Batt_PreChage:CurrentLEDIndex=3;break; //涓流预充电中，红色常亮
		case Batt_CVCharge:CurrentLEDIndex=4;break; //恒压充电中，充电即将完成
		case Batt_StandBy:CurrentLEDIndex=0;break; //待机状态，LED熄灭
		case Batt_discharging: //电池放电中
		   if(BattState.BatteryRSOC>20)CurrentLEDIndex=9; //电量充足 绿色慢闪
		   else if(BattState.BatteryRSOC>10)CurrentLEDIndex=8; //电量不足 黄色慢闪
		   else CurrentLEDIndex=7; //电池电量严重不足，红色慢闪
       if(fabsf(TypeCState.BusCurrent)<0.2)CurrentLEDIndex+=7; //电池处于空载状态，指示灯慢闪
       break;		
		}
	//电池防反接部分虚拟二极管的控制逻辑
  if(BattState.BattState==Batt_ChgWait)VDState=true; //电池正在等待预充电，打开二极管		
  else if(BattState.BattState==Batt_ChgError)VDState=false; //电池充电超时或者触发警告，关闭二极管
	else if(BattState.BattState==Batt_StandBy&&IsMenuAtBatteryTelem())VDState=true; //待机状态且位于电池检测菜单，二极管打开
	else VDState=true; //二极管打开
	GPIO_WriteOutBits(VDiode_IOG,VDiode_IOP,VDState?SET:RESET);//设置虚拟二极管状态 
	//线性温度过热保护控制逻辑和过热暂停逻辑
	if(IsNTCOK)
	  {
		//低温保护
		if(Fault.UTFault)IsUTFaultAsserted=true;
		else if(TempResult>(LowTempFault+3))IsUTFaultAsserted=false; //如果低温警报触发，则关闭充电，当温度升上去后恢复
		//过热保护
		if(Fault.OTFault)IsOTFaultAsserted=true;
		else if(TempResult<(TemperatureFault-5))IsOTFaultAsserted=false;	 //如果过热保护触发，则关闭充电，当温度降下来后恢复
		State=(!IsOTFaultAsserted)&(!IsUTFaultAsserted); //充放电使能状态
		if(IsDischargeEnabled!=State) //状态发生变动
		  {
			IsDischargeEnabled=State; //同步状态
			IP2368_SetChargerState(IsDischargeEnabled);	//设置充电功能	
			IP2368_SetDischarge(IsDischargeEnabled&&Config.IsEnableOutput); //根据是否使能输出来设置放电功能
			}
		}
	}
