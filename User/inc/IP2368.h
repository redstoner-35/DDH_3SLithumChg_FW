#ifndef _IP2368_
#define _IP2368_


//���Ͷ���
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
 QuickCharge_HV, //�����ѹ���
 QuickCharge_HC, //�͵�ѹ�ߵ������
 QuickCharge_None //�޿��
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
 ReCharge_Disabled, //���ٳ��
 ReCharge_4V15, //�ٳ����ֵΪ4.15Vÿ�ڵ��
 ReCharge_4V10, //�ٳ����ֵΪ4.10Vÿ�ڵ��
 ReCharge_4V0, //�ٳ����ֵΪ4Vÿ�ڵ��
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
 bool IsEnableDPDM; //�Ƿ����û���USB D+D-Э�̵�������Э��(����FCP QC AFC��)
 bool IsEnableSCP; //�Ƿ����û�ΪSCP���
 bool IsEnablePD; //�Ƿ�����PD���
 bool IsEnable9VPDO; //�Ƿ�����9V PDO���
 bool IsEnable20VPDO; //�Ƿ�����20V PDO���
 }QuickChargeCtrlDef;	
 
typedef struct
 {
 bool SCPFault; //��·����
 bool OCPFault; //��������
 bool INOVFault; //�������ѹ����
 bool OTFault; //�¶ȹ��߱���
 bool UTFault; //�¶ȹ��ͱ���
 bool OTAlert; //�¶ȹ��߾���
 //Global Fault Flag
 bool FaultAsserted; //����и澯�¼����������λ����
 }FaultFlagStrDef;	
 
//NTC��������
#define NTCT0Res 15 //NTC��T0�¶�ʱ�ı궨��ֵ(K��)
#define NTCBValue 3450 //NTC Bֵ
#define NTCT0 25 //NTCĬ����ֵ�궨�¶�(һ����25��)
#define NTCTRIM 0.5 //NTC�¶�����ֵ 
 
//���Ⱥ͵��±�������
#define TemperatureAlert 47 //���Ⱦ����¶�
#define TemperatureFault 56 //���ȹ���ͣ���¶� 
#define LowTempFault 8 //���¾��� 
 
//int�����Զ�����
#define IP2368_INT_IOB STRCAT2(GPIO_P,IP2368_INT_IOBank)
#define IP2368_INT_IOG STRCAT2(HT_GPIO,IP2368_INT_IOBank)
#define IP2368_INT_IOP STRCAT2(GPIO_PIN_,IP2368_INT_IOPinNum) 

//��������������Զ�����
#define VDiode_IOB STRCAT2(GPIO_P,VDiode_IOBank)
#define VDiode_IOG STRCAT2(HT_GPIO,VDiode_IOBank)
#define VDiode_IOP STRCAT2(GPIO_PIN_,VDiode_IOPinNum) 

//��ʼ������
void IP2368_init(void);
void IP2368_GPIOInit(void);

//�ض����������
void IP2368_DoRSOCCalibration(void); //���е���У׼

//���üĴ�������
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

//��ȡ�Ĵ�������
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
