#ifndef _Cfg_
#define _Cfg_

#include <stdbool.h>

typedef enum
 {
 ChargePower_30W,
 ChargePower_45W,
 ChargePower_60W,
 ChargePower_65W
 }ChargePowerDef; //��繦�ʶ���

typedef enum
 {
 Screen_MaxBright,
 Screen_HighBright,
 Screen_MidBright,
 Screen_LowBright
 }ScreenBrightDef;	
 
typedef struct
 {
 bool IsEnableOutput;  //�Ƿ������������
 ChargePowerDef ChargePower; //��繦��
 ScreenBrightDef Brightness; //��Ļ����
 bool IsEnableDPDM; //�Ƿ����û���USB D+D-Э�̵�������Э��(����FCP QC AFC��)
 bool IsEnableSCP; //�Ƿ����û�ΪSCP���
 bool IsEnablePD; //�Ƿ�����PD���
 bool IsEnable9VPDO; //�Ƿ�����9V PDO
 bool IsEnable20VPDO; //�Ƿ�����20W PDO
 char BatteryCount; //��ؽ���
 }SystemConfigStrDef;

//�ⲿ����
extern SystemConfigStrDef Config;

//����
bool SavingConfig(void); //�������� 
void ReadConfig(void); //��ȡ���� 
int ConvertChagePower(void);//ת����繦�� 
bool RestoreDefaultCfg(void);//�ָ�Ĭ������
char ConvertBrightLevel(void);//ת�����Ȳ��� 
 
#endif
