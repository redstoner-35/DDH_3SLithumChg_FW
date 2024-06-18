#include "IP2368.h"
#include <math.h>
#include "string.h"
#include "LEDMgmt.h"
#include "ht32.h"
#include "Pindefs.h"
#include "Config.h"
#include "oled.h"
#include "delay.h"

//�ⲿ�����ͺ���
void ForceShutOff(void);
bool IsMenuAtBatteryTelem(void);
int iroundf(float IN); //����ȡ����������
extern int SleepTimer;  //˯�߶�ʱ��
static bool IsOTFaultAsserted=false; //�Ƿ��������ȱ���
static bool IsUTFaultAsserted=false; //�Ƿ��������±���
extern int ConnectTimeoutCNT;

//״̬LEDָʾ(�����¿��߼�)
void STATULEDHandler(void)
  {
	BatteryStatuDef BattState;
	FaultFlagStrDef Fault;
	TypeCStatusDef TypeCState;
	bool State,IsNTCOK,VDState;
	static bool IsDischargeEnabled=true; //�������ŵ��Ƿ�ʹ��
	float TempResult;
	//��ȡIP2368״̬	
	State=IP2368_GetBatteryState(&BattState);
	State&=IP2368_GetTypeCState(&TypeCState);
	State&=IP2368_GetAlertFlag(&Fault);
	State&=IP2368_GetNTCTemp(&TempResult,&IsNTCOK);
	//��ʼ����LED
	if(!State&&ConnectTimeoutCNT<250)CurrentLEDIndex=0; //�����������ӵȴ���ָʾ��Ϩ��	
	else if(!State||Fault.FaultAsserted)CurrentLEDIndex=5;//�����쳣
	else if(SleepTimer>0)CurrentLEDIndex=0; //LEDָʾ�����Ⱥܸߣ���Ļ����ʱΪ�˱���Ӱ���Ķ��ʹر�
	else switch(BattState.BattState)
	  {
		case Batt_CCCharge:CurrentLEDIndex=2;break; //������磬��ɫ����
		case Batt_ChgDone:CurrentLEDIndex=1;break; //����������ɫ����
		case Batt_ChgError:CurrentLEDIndex=5;break; //�������쳣		
		case Batt_ChgWait:CurrentLEDIndex=6;break; //���ڵȴ������̿�ʼ
		case Batt_PreChage:CurrentLEDIndex=3;break; //���Ԥ����У���ɫ����
		case Batt_CVCharge:CurrentLEDIndex=4;break; //��ѹ����У���缴�����
		case Batt_StandBy:CurrentLEDIndex=0;break; //����״̬��LEDϨ��
		case Batt_discharging: //��طŵ���
		   if(BattState.BatteryRSOC>20)CurrentLEDIndex=9; //�������� ��ɫ����
		   else if(BattState.BatteryRSOC>10)CurrentLEDIndex=8; //�������� ��ɫ����
		   else CurrentLEDIndex=7; //��ص������ز��㣬��ɫ����
       if(fabsf(TypeCState.BusCurrent)<0.2)CurrentLEDIndex+=7; //��ش��ڿ���״̬��ָʾ������
       break;		
		}
	//��ط����Ӳ�����������ܵĿ����߼�
  if(BattState.BattState==Batt_ChgWait)VDState=true; //������ڵȴ�Ԥ��磬�򿪶�����		
  else if(BattState.BattState==Batt_ChgError)VDState=false; //��س�糬ʱ���ߴ������棬�رն�����
	else if(BattState.BattState==Batt_StandBy&&IsMenuAtBatteryTelem())VDState=true; //����״̬��λ�ڵ�ؼ��˵��������ܴ�
	else VDState=true; //�����ܴ�
	GPIO_WriteOutBits(VDiode_IOG,VDiode_IOP,VDState?SET:RESET);//�������������״̬ 
	//�����¶ȹ��ȱ��������߼��͹�����ͣ�߼�
	if(IsNTCOK)
	  {
		//���±���
		if(Fault.UTFault)IsUTFaultAsserted=true;
		else if(TempResult>(LowTempFault+3))IsUTFaultAsserted=false; //������¾�����������رճ�磬���¶�����ȥ��ָ�
		//���ȱ���
		if(Fault.OTFault)IsOTFaultAsserted=true;
		else if(TempResult<(TemperatureFault-5))IsOTFaultAsserted=false;	 //������ȱ�����������رճ�磬���¶Ƚ�������ָ�
		State=(!IsOTFaultAsserted)&(!IsUTFaultAsserted); //��ŵ�ʹ��״̬
		if(IsDischargeEnabled!=State) //״̬�����䶯
		  {
			IsDischargeEnabled=State; //ͬ��״̬
			IP2368_SetChargerState(IsDischargeEnabled);	//���ó�繦��	
			IP2368_SetDischarge(IsDischargeEnabled&&Config.IsEnableOutput); //�����Ƿ�ʹ����������÷ŵ繦��
			}
		}
	}
