#ifndef _Cfg_
#define _Cfg_

#include <stdbool.h>

typedef enum
 {
 ChargePower_30W,
 ChargePower_45W,
 ChargePower_60W,
 ChargePower_65W
 }ChargePowerDef; //充电功率定义

typedef enum
 {
 Screen_MaxBright,
 Screen_HighBright,
 Screen_MidBright,
 Screen_LowBright
 }ScreenBrightDef;	
 
typedef struct
 {
 bool IsEnableOutput;  //是否启用输出功能
 ChargePowerDef ChargePower; //充电功率
 ScreenBrightDef Brightness; //屏幕亮度
 bool IsEnableDPDM; //是否启用基于USB D+D-协商的其余快充协议(例如FCP QC AFC等)
 bool IsEnableSCP; //是否启用华为SCP快充
 bool IsEnablePD; //是否启用PD快充
 bool IsEnable9VPDO; //是否启用9V PDO
 bool IsEnable20VPDO; //是否启用20W PDO
 char BatteryCount; //电池节数
 }SystemConfigStrDef;

//外部声明
extern SystemConfigStrDef Config;

//函数
bool SavingConfig(void); //保存配置 
void ReadConfig(void); //读取配置 
int ConvertChagePower(void);//转换充电功率 
bool RestoreDefaultCfg(void);//恢复默认配置
char ConvertBrightLevel(void);//转换亮度参数 
 
#endif
