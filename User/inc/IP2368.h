#ifndef _IP2368_
#define _IP2368_


//类型定义
#include <stdbool.h>
typedef enum
 {
 PreCharge_End2V8,
 PreCharge_End2V9,
 Precharge_End3V0,
 PreCharge_End3V1
 }PerChargeEndVoltDef;

typedef enum
 {
 LVAlert_2V8,
 LVAlert_2V9,
 LVAlert_3V0,
 LVAlert_3V1,
 LVAlert_3V2
 }LVAlertDef;

typedef enum
 {
 Batt_StandBy,
 Batt_PreChage,
 Batt_CCCharge,
 Batt_CVCharge,
 Batt_ChgWait,
 Batt_ChgDone,
 Batt_ChgError,
 Batt_discharging,
 }BatteryStateDef;	

typedef enum
 {
 QuickCharge_PD,
 QuickCharge_HV, //其余高压快充
 QuickCharge_HC, //低电压高电流快充
 QuickCharge_None //无快充
 }QuickChargeStateDef;	
 
typedef enum
 {
 PD_5VMode,
 PD_7VMode,
 PD_9VMode,
 PD_12VMode,
 PD_15VMode,
 PD_20VMode
 }PDStateDef;

typedef enum
 {
 ReCharge_Disabled, //无再冲电
 ReCharge_4V15, //再冲电阈值为4.15V每节电池
 ReCharge_4V10, //再冲电阈值为4.10V每节电池
 ReCharge_4V0, //再冲电阈值为4V每节电池
 }RechargeModeDef;	
 
typedef struct
 {
 float BatteryCurrent;
 float BatteryVoltage;
 int BatteryRSOC;
 BatteryStateDef BattState;
 }BatteryStatuDef;	
 
typedef struct
 {
 float busVoltage;
 float BusPower;
 float BusCurrent;
 QuickChargeStateDef QuickChargeState;
 PDStateDef PDState;	 
 }TypeCStatusDef;	
 
typedef struct
 {
 bool IsEnableDPDM; //是否启用基于USB D+D-协商的其余快充协议(例如FCP QC AFC等)
 bool IsEnableSCP; //是否启用华为SCP快充
 bool IsEnablePD; //是否启用PD快充
 bool IsEnable9VPDO; //是否启用9V PDO快充
 bool IsEnable20VPDO; //是否启用20V PDO快充
 }QuickChargeCtrlDef;	
 
typedef struct
 {
 bool SCPFault; //短路保护
 bool OCPFault; //过流保护
 bool INOVFault; //输入过电压保护
 bool OTFault; //温度过高保护
 bool UTFault; //温度过低保护
 bool OTAlert; //温度过高警报
 //Global Fault Flag
 bool FaultAsserted; //如果有告警事件发生，则此位置起
 }FaultFlagStrDef;	
 
//NTC参数设置
#define NTCT0Res 15 //NTC在T0温度时的标定阻值(KΩ)
#define NTCBValue 3450 //NTC B值
#define NTCT0 25 //NTC默认阻值标定温度(一般是25℃)
#define NTCTRIM 0.5 //NTC温度修正值 
 
//过热和低温保护配置
#define TemperatureAlert 47 //过热警报温度
#define TemperatureFault 56 //过热故障停机温度 
#define LowTempFault 8 //低温警报 
 
//int引脚自动定义
#define IP2368_INT_IOB STRCAT2(GPIO_P,IP2368_INT_IOBank)
#define IP2368_INT_IOG STRCAT2(HT_GPIO,IP2368_INT_IOBank)
#define IP2368_INT_IOP STRCAT2(GPIO_PIN_,IP2368_INT_IOPinNum) 

//虚拟二极管引脚自动定义
#define VDiode_IOB STRCAT2(GPIO_P,VDiode_IOBank)
#define VDiode_IOG STRCAT2(HT_GPIO,VDiode_IOBank)
#define VDiode_IOP STRCAT2(GPIO_PIN_,VDiode_IOPinNum) 

//初始化函数
void IP2368_init(void);
void IP2368_GPIOInit(void);

//特定宏操作函数
void IP2368_DoRSOCCalibration(void); //进行电量校准

//设置寄存器函数
bool IP2368_SetDischarge(bool IsDischage);
bool IP2368_SetPreChargeEndVoltage(PerChargeEndVoltDef EndVolt);
bool IP2368_SetChargePower(int power);
bool IP2368_SetChargerState(bool IsChargerEnabled); 
bool IP2368_SetLowVoltAlert(LVAlertDef LowVolt);  
bool IP2368_SetQuickchargeState(QuickChargeCtrlDef *QCState);	
bool IP2368_SetChargeParam(int IChargeEnd,RechargeModeDef RechargeThr);	
bool IP2368_SetOTPReloadFlag(void);	
bool IP2368_SendResetCommand(void);	
bool IP2368_SetRSOC(int Level);

//读取寄存器函数
bool IP2368_GetBatteryState(BatteryStatuDef *BatteryState);
bool IP2368_GetTypeCState(TypeCStatusDef *TypeCStat); 
bool IP2368_GetNTCTemp(float *Temp,bool *IsResultOK); 
bool IP2368_GetDischargeState(bool *IsDischage); 
bool IP2368_GetChargePower(int *Power); 
bool IP2368_GetVBUSSwitchState(bool *IsSwitchOn); 
bool IP2368_GetAlertFlag(FaultFlagStrDef *Fault);
bool IP2368_GetTypeCConnectedState(bool *IsTypeCConnected);
bool IP2368_GetQuickchargeState(QuickChargeCtrlDef *QCState);
bool IP2368_GetIsTypeCSrcConnected(bool *IsConnected);
bool IP2368_CheckIfOTPLoaded(bool *IsLoaded);

#endif
