#include "I2C.h"
#include "IP2368.h"
#include "oled.h"
#include "delay.h"
#include <math.h>
#include "LEDMgmt.h"
#include "Config.h"

//内部全局变量
bool EnableXARIIMode = false; //内部flag位，是否启用协议版本V1.63的特殊寄存器

//IP2368读取寄存器
static bool IP2368_ReadReg(char RegAddr,char *Result)
  {
	char buf;
	//设置寄存器地址
	IIC_Start();
	IIC_Send_Byte(0xEA);
	if(IIC_Wait_Ack())return false; //发送从机地址等待响应
	IIC_Send_Byte(RegAddr);
	if(IIC_Wait_Ack())return false; //发送寄存器地址
	//读取数据
  IIC_Start();
	IIC_Send_Byte(0xEB);
	if(IIC_Wait_Ack())return false; //发送从机地址等待响应
	buf=IIC_Read_Byte(0);
	IIC_Stop(); //读一个字节的数据后发送NACK，停止
	//数据读取结束，返回结果		
	if(Result!=NULL)*Result=buf;
	return true;
	}

//IP2368写入寄存器
static bool IP2368_WriteReg(char RegAddr,char Data)
  {
	//设置寄存器地址
	IIC_Start();
	IIC_Send_Byte(0xEA);
	if(IIC_Wait_Ack())return false; //发送从机地址等待响应
	IIC_Send_Byte(RegAddr);
	if(IIC_Wait_Ack())return false; //发送寄存器地址
	IIC_Send_Byte(Data);
	if(IIC_Wait_Ack())return false; //发送数据
	//数据发送结束
	IIC_Stop(); //发送停止
	return true;
	}	
	
//设置充电功率
bool IP2368_SetChargePower(int power)
  {
	char buf;
	//判断参数
	if(power<10||power>100)return false;
	//读取并更改SYS-CTL1寄存器
	if(!IP2368_ReadReg(0x01,&buf))return false;
  buf|=0x02; //允许修改充电功率模式设置
  buf|=0x01; //设置为使用输入功率		
	if(!IP2368_WriteReg(0x01,buf))return false;
  //读取并更改SYS-CTL3寄存器
	if(!IP2368_ReadReg(0x03,&buf))return false;	
	buf|=0x80; //允许设置充电功率	
	if(!IP2368_WriteReg(0x03,buf))return false; //解锁充电功率使能操作
  delay_ms(1);			
	if(!IP2368_ReadReg(0x03,&buf))return false;
	buf&=0x80; //去除掉原始的数据	
	buf|=(char)power&0x7F; //设置充电功率为目标值(1LSB=1W)
	if(!IP2368_WriteReg(0x03,buf))return false;
	//操作完毕
	return true;
	}

//读取VBUS开关状态
bool IP2368_GetVBUSSwitchState(bool *IsSwitchOn)
  {
	char buf;
	//读取MOS—STATE寄存器
	if(!IP2368_ReadReg(0x35,&buf))return false;
	*IsSwitchOn=buf&0x40?true:false; //检测VBUS-MOS-State位
	//操作结束，返回true
	return true;
	}	
	
//读取目标充电功率
bool IP2368_GetChargePower(int *Power)
  {
	char buf;
	//读取SYS-CTL3寄存器
	if(!IP2368_ReadReg(0x03,&buf))return false;
  *Power=(int)(buf&0x7F); //1LSB=1W
	*Power&=0x7F;
	//操作完毕
	return true;
	}
	
//设置涓流充电结束电压
bool IP2368_SetPreChargeEndVoltage(PerChargeEndVoltDef EndVolt)
  {
	char buf;
	//读取并更改SYS-CTL7寄存器
	if(!IP2368_ReadReg(0x07,&buf))return false;
  buf&=0xF3; //清除原本的设置
  buf|=(char)EndVolt<<2; //更新为目标值		
	if(!IP2368_WriteReg(0x07,buf))return false;
  //操作完毕
	return true;
	}

//设置放电低电压关机电压
bool IP2368_SetLowVoltAlert(LVAlertDef LowVolt)
  {
  char buf;
	//读取并更改SYS-CTL10寄存器
	if(!IP2368_ReadReg(0x0A,&buf))return false;
  buf&=0x1F; //清除原本的设置
  buf|=(char)LowVolt<<5; //更新为目标值		
	if(!IP2368_WriteReg(0x0A,buf))return false;
  //操作完毕
	return true;	
	}

//设置Type-C是否使能放电功能
bool IP2368_SetDischarge(bool IsDischage)
  {
	char buf;
	//读取并更改SYS-CTL11寄存器
	if(!IP2368_ReadReg(0x0B,&buf))return false;
	buf=IsDischage?buf|0x80:buf&0x7F; //设置En-DCDC-output	
	if(!IP2368_WriteReg(0x0B,buf))return false;
	//读取并更改TYPEC-CTL8寄存器
	if(!IP2368_ReadReg(0x22,&buf))return false;
  buf&=0x3F; //清除原本的设置
	buf|=IsDischage?0xC0:0x00; //设置vbus-mode-set(设置为UFP或者DFP)
	if(!IP2368_WriteReg(0x22,buf))return false;		
  //操作完毕
	return true;	
	}

//读取TypeC是否使能放电功能
bool IP2368_GetDischargeState(bool *IsDischage)
  {
	char buf;
  bool result;
	//读取SYS-CTL11寄存器
	if(!IP2368_ReadReg(0x0B,&buf))return false;
	result=buf&0x80?true:false; //检测En-DCDC-output位
	//读取TYPEC-CTL8寄存器
	if(!IP2368_ReadReg(0x22,&buf))return false;
	buf&=0xC0; //保留vbus mode set其余清除
	result&=buf==0x40?false:true; //检测vbus-mode-set，如果b01则说明模式为DFP，关闭放电
	//返回结果		
	if(IsDischage!=NULL)*IsDischage=result;
	return true;
	}

//读取电池温度传感器所测得的温度	
bool IP2368_GetNTCTemp(float *Temp,bool *IsResultOK)
  {
	char buf;
	unsigned short ADCWord;
	float calc,INTC;
	//获取NTC电压
	if(!IP2368_ReadReg(0x78,&buf))return false; //VGPIO0_NTC[7:0]
	ADCWord=(unsigned short)buf&0xFF;
	if(!IP2368_ReadReg(0x79,&buf))return false; ///VGPIO0_NTC[15:8]
	ADCWord|=buf<<8; 
	calc=(float)(ADCWord); //LSB=1mV,得到mV的电压
	//读取输出到NTC的电流从而得知NTC的阻值
  if(!IP2368_ReadReg(0x77,&buf))return false; //INTC_IADC_DAT0
	INTC=(float)(buf&0x80?80:20)/1000; //如果INTC_IADC_DAT=1，则NTC输出80uA，否则输出20uA(0.02/0.08mA)
	calc/=INTC; //R=U(mV)/I(mA),得到电阻值(Ω)
	//换算电阻值
	calc/=1000; //阻值换成(KΩ)
	calc=1/((1/(273.15+(float)NTCT0))+log(calc/(float)NTCT0Res)/(float)NTCBValue);//计算出温度
	calc-=273.15;//减去开氏温标常数变为摄氏度
	calc+=(float)NTCTRIM;//加上修正值	
	if(calc<(-40)||calc>125)	//温度传感器异常
	  {
		*IsResultOK=false;
		*Temp=0; //指示传感器异常
		}
	else
	  {
		*IsResultOK=true; //传感器正常
		*Temp=calc; //输出温度
		}
	//操作完毕，返回true
	return true;
	}	
	
//IP2368得到电池状态
bool IP2368_GetBatteryState(BatteryStatuDef *BatteryState)
  {
  char buf;
	unsigned short ADCWord;
	static bool IsStillCCCharge=false;
	//获取电池电压
	do
	{			
	if(!IP2368_ReadReg(0x50,&buf))return false; //Battvadc[7:0]
	ADCWord=(unsigned short)buf&0xFF;
	if(!IP2368_ReadReg(0x51,&buf))return false; //Battvadc[15:8]
	ADCWord|=buf<<8; 
	BatteryState->BatteryVoltage=(float)(ADCWord)/1000; //LSB=1mV，转换为V
	}
	while(BatteryState->BatteryVoltage>14.4); //电池电压异常则重试采样
	BatteryState->BatteryVoltage-=0.11; //电压需要减0.1才是正确值修正ADC的飘
  //获取电池电流
	if(!IP2368_ReadReg(0x6E,&buf))return false; //iBattiadc[7:0]
	ADCWord=(unsigned short)buf&0xFF;
	if(!IP2368_ReadReg(0x6F,&buf))return false; //iBattiadc[15:8]
	ADCWord|=((unsigned short)buf&0xFF)<<8; 
  if(ADCWord&0x8000) //电池电流为负
	  {
		ADCWord&=0x7FFF; //去除符号位
		BatteryState->BatteryCurrent=((float)(ADCWord)/1000)*-1; //读数为负，LSB=-1mA，转换为A
		}
	else BatteryState->BatteryCurrent=(float)(ADCWord)/1000; //读数为正，LSB=1mA，转换为A
	//获取RSOC
	if(!IP2368_ReadReg(0x30,&buf))return false; //SOC CAP DATA
	BatteryState->BatteryRSOC=(int)buf; 
	if(BatteryState->BatteryRSOC>100)BatteryState->BatteryRSOC=100;
	if(BatteryState->BatteryRSOC<0)BatteryState->BatteryRSOC=0; //限幅
  //获取充电状态
	if(!IP2368_ReadReg(0x31,&buf))return false; //STATE-CTL0
	if(buf&0x08)BatteryState->BattState=Batt_discharging; //输出已启用，电池正在向外放电
	else BatteryState->BattState=(BatteryStateDef)(buf&0x07); //获取电池状态
	if(BatteryState->BattState==Batt_CVCharge) //当前IP2368处于恒压模式，开始根据电流阈值判断是否位于恒流
	  {
		if(!IsStillCCCharge&&BatteryState->BatteryCurrent>2.65)
		   IsStillCCCharge=true;  //当前电流值大于2.65A，仍然处于恒流充电阶段
		else if(IsStillCCCharge&&BatteryState->BatteryCurrent<2.1)
		   IsStillCCCharge=false; //当前电流值小于2.1A，这时候才报告进入恒压充电
		if(IsStillCCCharge)BatteryState->BattState=Batt_CCCharge; //当前电流仍然过大，报告恒流状态
		}
  //处理完毕，返回true
  return true;
	}

//IP2368设置充电使能
bool IP2368_SetChargerState(bool IsChargerEnabled)
  {
	char buf;
	//读取并更改SYS-CTL0寄存器
	if(!IP2368_ReadReg(0x00,&buf))return false;
	buf=IsChargerEnabled?buf|0x01:buf&0xFE; //设置en-charger位
	buf=IsChargerEnabled?buf|0x02:buf&0xFD; //设置en-vbus_sinkpd位
	if(!IP2368_WriteReg(0x00,buf))return false;		
  //处理完毕，返回true
  return true;	
	}
	
//IP2368获取状态flag
bool IP2368_GetAlertFlag(FaultFlagStrDef *Fault)
  {
	char buf;
	bool IsResultOK;
  float Temp;
	//获取STATE-CTL2
	if(!IP2368_ReadReg(0x33,&buf))return false; 
	Fault->INOVFault=buf&0x40?true:false;
	//获取STATE-CTL3
  if(!IP2368_ReadReg(0x38,&buf))return false; 
	if(buf&0x20) //VSYSOC=1
	  {
		buf|=0x20;
		if(!IP2368_WriteReg(0x38,buf))return false;	//向该寄存器写1清零
		delay_ms(600);
		if(!IP2368_ReadReg(0x38,&buf))return false; 
		Fault->OCPFault=buf&0x20?true:false; //600mS内再次读取，如果仍为高则过流事件发生
    buf|=0x20;
		if(!IP2368_WriteReg(0x38,buf))return false;	//向该寄存器写1清零
		}
	else Fault->OCPFault=false; //未发生过流事件
	if(buf&0x10) //VSYSSCDT=1
    {
		buf|=0x10;
		if(!IP2368_WriteReg(0x38,buf))return false;	//向该寄存器写1清零
		delay_ms(600);
		if(!IP2368_ReadReg(0x38,&buf))return false; 
		Fault->SCPFault=buf&0x10?true:false; //600mS内再次读取，如果仍为高则短路事件发生
    buf|=0x10;
		if(!IP2368_WriteReg(0x38,buf))return false;	//向该寄存器写1清零
		}		
	else Fault->SCPFault=false; //短路事件未发生	
	//获取系统温度
	if(!IP2368_GetNTCTemp(&Temp,&IsResultOK))return false;	
	if(IsResultOK) //结果有效
	  {
		Fault->UTFault=Temp>LowTempFault?false:true; //低温保护
		if(Temp>TemperatureFault)//过热异常
		   {
		   Fault->OTAlert=false;
			 Fault->OTFault=true;
	     }
    else if(Temp>TemperatureAlert) //过热警报
		   {
		   Fault->OTAlert=true;
			 Fault->OTFault=false;
	     }
		else //无告警
			 {
		   Fault->OTAlert=false;
			 Fault->OTFault=false;
	     }
		}
	//进行告警汇总
	if(Fault->INOVFault||Fault->OCPFault)Fault->FaultAsserted=true;
	else if(Fault->OTFault||Fault->SCPFault)Fault->FaultAsserted=true;
	else if(Fault->UTFault)Fault->FaultAsserted=true;
	else Fault->FaultAsserted=false;
	//检测完毕，返回true
	return true;
	}
//IP2368获取Typec是否连接
bool IP2368_GetTypeCConnectedState(bool *IsTypeCConnected)
  {
	char buf;
	//获取TYPEC_State0
	if(!IP2368_ReadReg(0x34,&buf))return false; 
	if(Config.IsEnableOutput)*IsTypeCConnected=buf&0xC0?true:false; //如果Sink和source任意一位为1说明typec已连接，此时返回true
	else *IsTypeCConnected=buf&0x80?true:false; //如果并非输出模式，则Type-C不响应输出请求，仅sink模式下才进行检测
	//检测完毕，返回true
  return true;
	}	
	
//IP2368设置停充电流和再冲电参数
bool IP2368_SetChargeParam(int IChargeEnd,RechargeModeDef RechargeThr)	
  {
	char buf;
	//读取SYS-CTL8
	if(!IP2368_ReadReg(0x08,&buf))return false;	
	//修改停充电流
	IChargeEnd/=50;
	if(IChargeEnd<=0)return false; //参数非法
	IChargeEnd&=0x0F; //去除bit0-3
	buf&=0x0F;
	buf|=IChargeEnd<<4; //修改bit7-4设置停充电流
	//修改再冲电参数
	buf&=0xF3; 
	buf|=((char)RechargeThr<<2)&0x0C; //根据enum值修改对应的bit
  //回写SYS-CTL8
	if(!IP2368_WriteReg(0x08,buf))return false;		
	return true;
	}
//IP2368设置快充状态
bool IP2368_SetQuickchargeState(QuickChargeCtrlDef *QCState)
  {
	char buf;
	//修改SYS-CTL0
	if(!IP2368_ReadReg(0x00,&buf))return false;
	buf=QCState->IsEnableDPDM?buf|0x10:buf&0xEF; //EN_VBUS_sinkDPDM
	buf=QCState->IsEnablePD?buf|0x08:buf&0xF7; //EN_VBUS_sinkPD
	buf=QCState->IsEnableSCP?buf|0x04:buf&0xfb; //EN_VBUS_sinkSCPVOOC
	if(!IP2368_WriteReg(0x00,buf))return false;		
	//修改SYS-CTL11
	if(!IP2368_ReadReg(0x0B,&buf))return false;
	buf=QCState->IsEnableDPDM?buf|0x40:buf&0xBF; //EN_VBUS_srcDPDM
	buf=QCState->IsEnablePD?buf|0x20:buf&0xDF; //EN_VBUS_srckPD
	buf=QCState->IsEnableSCP?buf|0x10:buf&0xEF; //EN_VBUS_srcSCP
	if(!IP2368_WriteReg(0x0B,buf))return false;			
	//设置TypeC-CTL17
  if(!IP2368_ReadReg(0x2B,&buf))return false;		
	buf=QCState->IsEnable20VPDO?buf|0x10:0xEF; //EN_SRC_20VPDO
	buf=QCState->IsEnable9VPDO?buf|0x02:buf&0xFD; //EN_SRC_9VPDO
	if(!IP2368_WriteReg(0x2B,buf))return false;				
	//修改完毕，返回true
	return true;
	}	
//IP2368获取快充状态	
bool IP2368_GetQuickchargeState(QuickChargeCtrlDef *QCState)
  {
	char buf;
	//读取SYS-CTL0
	if(!IP2368_ReadReg(0x00,&buf))return false;
	QCState->IsEnableDPDM=buf&0x10?true:false;//EN_VBUS_sinkDPDM
	QCState->IsEnablePD=buf&0x08?true:false;//EN_VBUS_sinkPD
	QCState->IsEnableSCP=buf&0x04?true:false;//EN_VBUS_sinkSCPVOOC
	//读取SYS-CTL11
	if(!IP2368_ReadReg(0x0B,&buf))return false;
	QCState->IsEnableDPDM&=buf&0x40?true:false;//EN_VBUS_srcDPDM
	QCState->IsEnablePD&=buf&0x20?true:false; //EN_VBUS_srckPD
	QCState->IsEnableSCP&=buf&0x10?true:false; //EN_VBUS_srcSCP
	//读取TypeC-CTL17
  if(!IP2368_ReadReg(0x2B,&buf))return false;		
	QCState->IsEnable20VPDO=buf&0x10?true:false; //EN_SRC_20VPDO
	QCState->IsEnable9VPDO=buf&0x02?true:false; //EN_SRC_9VPDO	
	//检测完毕，返回true
	return true;
	}
	
//生成复位命令
bool IP2368_SendResetCommand(void)
  {
	char buf;
	//修改SYS-CTL0
	if(!IP2368_ReadReg(0x00,&buf))return false;
	buf|=0x40; //set EN_RESETMCU为1使IP2368复位
	if(!IP2368_WriteReg(0x00,buf))return false;			
	//操作成功完成
	return true;
	}	

//获取Type-C是否连接到输出（Source）模式
bool IP2368_GetIsTypeCSrcConnected(bool *IsConnected)
  {
	char buf;
	//读取TYPEC_STATE0
	if(!IP2368_ReadReg(0x34,&buf))return false;
	*IsConnected=(buf&0x40)?true:false;
	//检测完毕 返回true
  return true;		
	}
	
//IP2368获取TypeC状态
bool IP2368_GetTypeCState(TypeCStatusDef *TypeCStat)
  {
	char buf;
	unsigned int ADCWord;
	bool State;
	//获取输入电压
	if(!IP2368_ReadReg(0x52,&buf))return false; //VSYSVADC[7:0]
	ADCWord=(unsigned short)buf&0xFF;
	if(!IP2368_ReadReg(0x53,&buf))return false; //VSYSVADC[15:8]
	ADCWord|=buf<<8; 
	TypeCStat->busVoltage=(float)(ADCWord)/1000; //LSB=1mV，转换为V
	//获取输入功率和电流
	if(!EnableXARIIMode)do	//V1.2版本协议，读取电流寄存器
	  {
		if(!IP2368_ReadReg(0x70,&buf))return false; // ISYSIADC[7:0]
		ADCWord=(unsigned int)buf&0xFF;
		if(!IP2368_ReadReg(0x71,&buf))return false; // ISYSIADC[15:8]
		ADCWord|=buf<<8; 
		if(ADCWord&0x8000) //电池电流为负
			{
			ADCWord&=0x7FFF; //去除符号位
			TypeCStat->BusCurrent=((float)(ADCWord)/1000)*-1; //读数为负，LSB=-1mA，转换为A
			}
		else TypeCStat->BusCurrent=(float)(ADCWord)/1000; //读数为正，LSB=1mA，转换为A	
		TypeCStat->BusPower=fabsf(TypeCStat->BusCurrent)*TypeCStat->busVoltage; //电压和电流绝对值相乘得到功率
		}
	while(TypeCStat->BusPower>105); //如果读取到的功率值非法则重试
	else do//V1.63版本协议，读取功率寄存器
	  {		
		if(!IP2368_ReadReg(0x74,&buf))return false; //VSYSPOWADC[7:0]
		ADCWord=(unsigned int)buf&0xFF;
		if(!IP2368_ReadReg(0x75,&buf))return false; //VSYSPOWADC[15:8]
		ADCWord|=buf<<8; 
		if(!IP2368_ReadReg(0x76,&buf))return false; //VSYSPOWADC[23:16]
		ADCWord|=buf<<16; 
		TypeCStat->BusPower=(float)(ADCWord)/1000; //LSB=1mW，转换为W
		TypeCStat->BusCurrent=fabsf(TypeCStat->BusPower/TypeCStat->busVoltage); //通过总功率除以电压得到电流
		}
	while(TypeCStat->BusPower>105); //如果读取到的功率值非法则重试
	//根据电池是否处于放电状态判断电流显示		
  if(!IP2368_ReadReg(0x31,&buf))return false; //STATE-CTL0	
	if(buf&0x08)TypeCStat->BusCurrent*=(float)-1; //处于放电模式，电流为负
	//获取PD Input状态
	if(!IP2368_ReadReg(0x34,&buf))return false; //TYPEC-STATE0
	if((buf&0x60)==0x60) //Type-C处于SRC模式且PD成功握手
	  {
		//读取SYS-CTL11
		if(!IP2368_ReadReg(0x0B,&buf))return false;
		if(buf&0x20) //PD已经启用
		    {
				State=true; //默认为真
				//读取TYPEC-CTL17
		    if(!IP2368_ReadReg(0x2B,&buf))return false;
				//根据电压进行判断
				if(TypeCStat->busVoltage>19.0) //20V
				   {
					 State=buf&0x10?true:false; 
				   TypeCStat->PDState=PD_20VMode; 
		       }
		    else if(TypeCStat->busVoltage>14.0) //15V
				   {
				   State=buf&0x08?true:false; 
					 TypeCStat->PDState=PD_15VMode;
			     }
				else if(TypeCStat->busVoltage>11.0) //12V
				   {
				   State=buf&0x04?true:false; 
				   TypeCStat->PDState=PD_12VMode;
			     }
				else if(TypeCStat->busVoltage>8.0) //9V
				   {
				   State=buf&0x02?true:false; 
					 TypeCStat->PDState=PD_9VMode;
		       }
				else if(TypeCStat->busVoltage>6.0)TypeCStat->PDState=PD_7VMode;
				else TypeCStat->PDState=PD_5VMode; //识别电压判断输出模式
		    if(State)TypeCStat->QuickChargeState=TypeCStat->busVoltage>6.0?QuickCharge_PD:QuickCharge_None; //当前电压模式所对应的PDO是开着的，如果电压大于6V则是PD模式	 
				else //当前电压模式所对应的PDO是关闭的，如果电压大于6V则是高压模式	 
				   {
					 TypeCStat->PDState=PD_5VMode; //高压快充，指示非5V的PD挡位
				   TypeCStat->QuickChargeState=TypeCStat->busVoltage>6.0?QuickCharge_HV:QuickCharge_None; 
					 }
				}
		else TypeCStat->QuickChargeState=TypeCStat->busVoltage>6.0?QuickCharge_HV:QuickCharge_None; //如果PD已经被关闭，则判断为高压快充
		}
	else if((buf&0x90)==0x90)//Type-C处于SNK模式且PD成功握手
	  {
		if(!IP2368_ReadReg(0x33,&buf))return false; //STATE-CTL2
		buf&=0x07;
		buf-=0x02; //将状态位减掉0x02转换为enum值
		TypeCStat->PDState=(PDStateDef)buf;
		TypeCStat->QuickChargeState=buf>0?QuickCharge_PD:QuickCharge_None; //如果电压大于6V则是PD模式
		}
	else  //其余状况
	  {
		TypeCStat->PDState=PD_5VMode;
		if(TypeCStat->busVoltage>6.0||buf&0x04)TypeCStat->QuickChargeState=QuickCharge_HV;//高压快充
		else if(TypeCStat->busVoltage<=6.0&&fabsf(TypeCStat->BusCurrent)>2.0)TypeCStat->QuickChargeState=QuickCharge_HC;//低压电流快充
		else TypeCStat->QuickChargeState=QuickCharge_None; //其余状况均为未识别快充
		}
  //处理完毕，返回true
  return true;	
	}
//检查IP2368是否加载了OTP
bool IP2368_CheckIfOTPLoaded(bool *IsLoaded)
  {
	char buf;
	//读取TYPEC_CTL3
	if(!IP2368_ReadReg(0x03,&buf))return false;	
	*IsLoaded=(buf&0x80)?false:true; //如果这个寄存器被复位，则说明IP2368已经重新加载了OTP
	//检测完毕
	return true;
	}

//设置寄存器对NVRAM的Flag进行检查	
bool IP2368_SetOTPReloadFlag(void)
  {
	char buf;
	//读取并修改SYS_CTL3
	if(!IP2368_ReadReg(0x03,&buf))return false;	 
	buf|=0x80;
	if(!IP2368_WriteReg(0x03,buf))return false;	 //强制复位
	//检测完毕
	return true;
	}

//设置剩余电量寄存器
bool IP2368_SetRSOC(int Level)	
  {
	char buf;
  //参数检查	
	if(Level>100||Level<0)return false;
	//写入SYS_CTL6
	buf=(char)Level&0x7F;
	if(!IP2368_WriteReg(0x06,buf))return false;	 
	//操作完成
	return true;
	}	

//初始化IP2368的GPIO	
static bool IsForceWakeUp=false; //标记变量，标记是否强制唤醒	
	
void IP2368_GPIOInit(void)	
  {
	//配置虚拟二极管的IO
	AFIO_GPxConfig(VDiode_IOB,VDiode_IOP, AFIO_FUN_GPIO);//虚拟二极管引脚
  GPIO_DirectionConfig(VDiode_IOG,VDiode_IOP,GPIO_DIR_OUT);//配置为输出
	 GPIO_ClearOutBits(VDiode_IOG,VDiode_IOP);//输出设置为0
	//配置GPIO，读取GPIO状态判断是主动唤醒还是被动
	AFIO_GPxConfig(IP2368_INT_IOB,IP2368_INT_IOP, AFIO_FUN_GPIO);//GPIO功能
  GPIO_DirectionConfig(IP2368_INT_IOG,IP2368_INT_IOP,GPIO_DIR_IN);//配置为输入
	GPIO_InputConfig(IP2368_INT_IOG,IP2368_INT_IOP,ENABLE);//启用IDR
	if(GPIO_ReadInBit(IP2368_INT_IOG,IP2368_INT_IOP)==RESET)IsForceWakeUp=true; //IP2368在睡觉，此时强制唤醒
  //配置为输出令INT=1，使IP2368不能睡眠
	GPIO_InputConfig(IP2368_INT_IOG,IP2368_INT_IOP,DISABLE); 
	GPIO_DirectionConfig(IP2368_INT_IOG,IP2368_INT_IOP,GPIO_DIR_OUT);//禁用IDR，设置为输出
	GPIO_SetOutBits(IP2368_INT_IOG,IP2368_INT_IOP); //令INT=1，强制唤醒IP2368
	delay_ms(150); //唤醒2368之后需要等150mS才能开始访问寄存器
	//设置寄存器关闭充电和放电
	IP2368_SetChargerState(false); 
	IP2368_SetDischarge(false);
	}

//进行RSOC校准
void IP2368_DoRSOCCalibration(void)
  {
	int retry,i;
	float RSOC,FullVolt,EmptyVolt,LastRSOC[3]={0},delta;
	BatteryStatuDef BatteryState;
	//开始校准
	OLED_Clear();
	OLED_Printf(0,0,64,1,"RSOC Cal Start...");
	OLED_Refresh();
	GPIO_SetOutBits(VDiode_IOG,VDiode_IOP);//输出设置为1，打开虚拟二极管  	
	retry=0;
	do
		{
		delay_ms(50);
		if(!IP2368_GetBatteryState(&BatteryState)) //获取电池电压
		   {
			 OLED_Printf(0,12,64,1,"Failed,ERR:0");
	     OLED_Refresh();
			 delay_Second(1);
			 return;
			 }
		FullVolt=(float)Config.BatteryCount*4.2;	
		EmptyVolt=(float)Config.BatteryCount*2.8; //计算满电和电池电压	
		RSOC=(BatteryState.BatteryVoltage-EmptyVolt)/(FullVolt-EmptyVolt); //根据电压数据计算RSOC
		RSOC*=100; //转成100%
		//转移采样数据
		for(i=2;i>0;i--)LastRSOC[i]=LastRSOC[i-1];
    LastRSOC[0]=RSOC;
    retry++;
		//计算三个sample之间的均差
		delta=0;
    for(i=0;i<3;i++)delta+=LastRSOC[i];	
		EmptyVolt=delta/(float)3; //求平均并且暂存一下
		delta=-2000;
    for(i=0;i<3;i++)if(delta<fabsf(LastRSOC[i]-EmptyVolt))delta=fabsf(LastRSOC[i]-EmptyVolt); //取均差最大的元素			
		}
	while(delta>1.00&&retry<20); //反复采样求RSOC直到三个sample之间的均差小于等于1%容量差
	if(RSOC<0)RSOC=0;
  if(RSOC>100)RSOC=100; //数值限幅，因为RSOC只能是0-100%	
	if(!IP2368_SetRSOC(iroundf(RSOC)))OLED_Printf(0,12,64,1,"Failed,ERR:1");
	else OLED_Printf(0,12,64,1,"OK,RSOC=%d%%",iroundf(RSOC));//成功完成
	//操作完毕，更新数据
	OLED_Refresh();
	delay_Second(1);	
	}	
	
//初始化IP2368的寄存器之类的
void IP2368_init(void)
  {
	BatteryStatuDef BatteryState;
  QuickChargeCtrlDef QCtrl;
	bool Result=false;
	int retry=0,BattCount,i;
	float RSOC,FullVolt,EmptyVolt,LastRSOC[3]={0},delta;
	//开始初始化流程检测协议版本
	OLED_Printf(0,0,64,1,"PSoC Init...");
	OLED_Refresh();
  do
	 {
	 if(IP2368_ReadReg(0x76,NULL))break;
	 retry++;
	 }		
	while(retry<5); //通过读取0x76寄存器检测协议版本	
	EnableXARIIMode=(retry==5)?false:true; //如果重试了五次寄存器都无法读取说明芯片为旧版本，此时通过读取电流来得到功率
  //开始进行电池检测
	if(!IP2368_GetTypeCConnectedState(&Result)) //尝试进行通信
	  {
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368-I2C Connection Failed.");
	  OLED_Refresh();		
		CurrentLEDIndex=11; //禁止充电器充电失败
		while(1);
		}
	if(!IP2368_GetBatteryState(&BatteryState)) //读取电池电压
	  {
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368 BTELEM ERR.");
	  OLED_Refresh();		
		CurrentLEDIndex=12; //电池遥测失败
		while(1);
		}		
  if(BatteryState.BatteryVoltage<5.5||BatteryState.BatteryVoltage>16.9) //电池电压异常
 		{
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"Battery");
		OLED_Printf(0,6,64,1,BatteryState.BatteryVoltage<8.1?"disconnected!":"OverVolt!");			
	  OLED_Refresh();		
		CurrentLEDIndex=3; //红灯常亮
		while(1);
		}		
	if(Config.BatteryCount==0) //电池节数未配置
	  {
    if(BatteryState.BatteryVoltage>12.7)BattCount=4;
		else if(BatteryState.BatteryVoltage>8.5)BattCount=3;
    else BattCount=2; //根据电压识别电池节数		
		OLED_Printf(0,6,64,1,"PSoC BattCfg=%dCell",BattCount);
		OLED_Refresh();
		delay_Second(1);
		//开始重新构建配置
		Config.BatteryCount=BattCount;
		RestoreDefaultCfg();
		//重新显示
		OLED_OldTVFade();
		OLED_Printf(0,0,64,1,"PSoC Init...");
	  OLED_Refresh();
		}			
  //启用虚拟二极管并检测RSOC
	GPIO_SetOutBits(VDiode_IOG,VDiode_IOP);//输出设置为1，打开虚拟二极管  	
	retry=0;
	do
		{
		delay_ms(50);
		IP2368_GetBatteryState(&BatteryState); //获取电池电压
		FullVolt=(float)Config.BatteryCount*4.2;	
		EmptyVolt=(float)Config.BatteryCount*2.8; //计算满电和电池电压	
		RSOC=(BatteryState.BatteryVoltage-EmptyVolt)/(FullVolt-EmptyVolt); //根据电压数据计算RSOC
		RSOC*=100; //转成100%
		//转移采样数据
		for(i=2;i>0;i--)LastRSOC[i]=LastRSOC[i-1];
    LastRSOC[0]=RSOC;
    retry++;
		//计算三个sample之间的均差
		delta=0;
    for(i=0;i<3;i++)delta+=LastRSOC[i];	
		EmptyVolt=delta/(float)3; //求平均并且暂存一下
		delta=-2000;
    for(i=0;i<3;i++)if(delta<fabsf(LastRSOC[i]-EmptyVolt))delta=fabsf(LastRSOC[i]-EmptyVolt); //取均差最大的元素			
		}
	while(delta>1.00&&retry<20); //反复采样求RSOC直到三个sample之间的均差小于等于1%容量差
	if(RSOC<0)RSOC=0;
  if(RSOC>100)RSOC=100; //数值限幅，因为RSOC只能是0-100%	
	if(!IP2368_SetRSOC(iroundf(RSOC)))	
	  {
		OLED_Clear();
		OLED_Printf(0,0,64,1,"IP2368 SetRSOC ERR.");
		OLED_Refresh();		
		while(1);
		}
	//配置寄存器
	if(!Result||IsForceWakeUp) //如果是强制唤醒,或者typec未连接，则不执行IP2368的自检
	  {
		OLED_Printf(0,6,64,1,"PSoC is Ready,Using V1.%s Protocol.",EnableXARIIMode?"63":"2");
		OLED_Refresh();		
		GPIO_SetOutBits(VDiode_IOG,VDiode_IOP);//输出设置为1，打开虚拟二极管
		delay_Second(1);
		OLED_OldTVFade();		
		IP2368_SetChargerState(true);	//启用充电模块
		IP2368_SetDischarge(Config.IsEnableOutput); //按照设置重新启用或者禁用放电模块
		return;	
		}
	//进行Type-C连接状态侦测
	IP2368_GetIsTypeCSrcConnected(&Result);
	if(!Result) //检测Type-C是否以Source形式连接，如果是，则跳过下面的额外初始化
	  {
	  if(!IP2368_SetChargePower(ConvertChagePower())) //设置充电功率
			{
			OLED_Clear();
			OLED_Printf(0,0,64,1,"IP2368 IchgSET ERR.");
			OLED_Refresh();		
			while(1);
			}	
		if(!IP2368_SetPreChargeEndVoltage(PreCharge_End2V9))
			{
			OLED_Clear();
			OLED_Printf(0,0,64,1,"IP2368 PCSET ERR.");
			OLED_Refresh();		
			while(1);
			}		
		if(!IP2368_SetChargeParam(100,ReCharge_4V10))
			{
			OLED_Clear();
			OLED_Printf(0,0,64,1,"IP2368 ChgParamSet ERR.");
			OLED_Refresh();		
			while(1);
			}		
		}
	//设置低电压告警
	if(!IP2368_SetLowVoltAlert(LVAlert_2V8))
		{
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368 LVSET ERR.");
	  OLED_Refresh();		
		while(1);
		}	
	//设置快充模式
	QCtrl.IsEnableDPDM=Config.IsEnableDPDM;
	QCtrl.IsEnablePD=Config.IsEnablePD;
	QCtrl.IsEnableSCP=Config.IsEnableSCP;
	QCtrl.IsEnable9VPDO=Config.IsEnable9VPDO;
	QCtrl.IsEnable20VPDO=Config.IsEnable20VPDO;				
	if(!IP2368_SetQuickchargeState(&QCtrl))
		{
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368 QCSet ERR.");
	  OLED_Refresh();		
		while(1);
		}			
	//加载OTP检测nibble
	if(!IP2368_SetOTPReloadFlag()) //重新加载flag
		{
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368 OTPDCTP ERR.");
	  OLED_Refresh();		
		while(1);
		}	
	//重新启用充电模块
	retry=0;
  while(retry<300)//反复尝试启用充电模块
	  {
		if(IP2368_SetChargerState(true))break;
		delay_ms(100);
		retry++;
		}		
	if(retry==300) //重试超时
		{
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368 ENCHG ERR.");
	  OLED_Refresh();		
		while(1);
		}		

	//设置是否允许放电
	IP2368_SetDischarge(true);	
	delay_ms(5);	
	if(!IP2368_SetDischarge(Config.IsEnableOutput)) //这里是因为IP2368的bug，需要通过短时间设置Type-C接口为UFP使IP2368重新握手
	  {
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368 DRPSET ERR.");
	  OLED_Refresh();		
		while(1);
		}	
	//所有操作完成，指示成功完成
	OLED_Printf(0,6,64,1,"PSoC is Ready,Using V1.%s Protocol.",EnableXARIIMode?"63":"2");
	OLED_Refresh();	
	delay_Second(1);
	OLED_OldTVFade();
	}
