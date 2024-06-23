#include "I2C.h"
#include "IP2368.h"
#include "oled.h"
#include "delay.h"
#include <math.h>
#include "LEDMgmt.h"
#include "Config.h"

//�ڲ�ȫ�ֱ���
bool EnableXARIIMode = false; //�ڲ�flagλ���Ƿ�����Э��汾V1.63������Ĵ���

//IP2368��ȡ�Ĵ���
static bool IP2368_ReadReg(char RegAddr,char *Result)
  {
	char buf;
	//���üĴ�����ַ
	IIC_Start();
	IIC_Send_Byte(0xEA);
	if(IIC_Wait_Ack())return false; //���ʹӻ���ַ�ȴ���Ӧ
	IIC_Send_Byte(RegAddr);
	if(IIC_Wait_Ack())return false; //���ͼĴ�����ַ
	//��ȡ����
  IIC_Start();
	IIC_Send_Byte(0xEB);
	if(IIC_Wait_Ack())return false; //���ʹӻ���ַ�ȴ���Ӧ
	buf=IIC_Read_Byte(0);
	IIC_Stop(); //��һ���ֽڵ����ݺ���NACK��ֹͣ
	//���ݶ�ȡ���������ؽ��		
	if(Result!=NULL)*Result=buf;
	return true;
	}

//IP2368д��Ĵ���
static bool IP2368_WriteReg(char RegAddr,char Data)
  {
	//���üĴ�����ַ
	IIC_Start();
	IIC_Send_Byte(0xEA);
	if(IIC_Wait_Ack())return false; //���ʹӻ���ַ�ȴ���Ӧ
	IIC_Send_Byte(RegAddr);
	if(IIC_Wait_Ack())return false; //���ͼĴ�����ַ
	IIC_Send_Byte(Data);
	if(IIC_Wait_Ack())return false; //��������
	//���ݷ��ͽ���
	IIC_Stop(); //����ֹͣ
	return true;
	}	
	
//���ó�繦��
bool IP2368_SetChargePower(int power)
  {
	char buf;
	//�жϲ���
	if(power<10||power>100)return false;
	//��ȡ������SYS-CTL1�Ĵ���
	if(!IP2368_ReadReg(0x01,&buf))return false;
  buf|=0x02; //�����޸ĳ�繦��ģʽ����
  buf|=0x01; //����Ϊʹ�����빦��		
	if(!IP2368_WriteReg(0x01,buf))return false;
  //��ȡ������SYS-CTL3�Ĵ���
	if(!IP2368_ReadReg(0x03,&buf))return false;	
	buf|=0x80; //�������ó�繦��	
	if(!IP2368_WriteReg(0x03,buf))return false; //������繦��ʹ�ܲ���
  delay_ms(1);			
	if(!IP2368_ReadReg(0x03,&buf))return false;
	buf&=0x80; //ȥ����ԭʼ������	
	buf|=(char)power&0x7F; //���ó�繦��ΪĿ��ֵ(1LSB=1W)
	if(!IP2368_WriteReg(0x03,buf))return false;
	//�������
	return true;
	}

//��ȡVBUS����״̬
bool IP2368_GetVBUSSwitchState(bool *IsSwitchOn)
  {
	char buf;
	//��ȡMOS��STATE�Ĵ���
	if(!IP2368_ReadReg(0x35,&buf))return false;
	*IsSwitchOn=buf&0x40?true:false; //���VBUS-MOS-Stateλ
	//��������������true
	return true;
	}	
	
//��ȡĿ���繦��
bool IP2368_GetChargePower(int *Power)
  {
	char buf;
	//��ȡSYS-CTL3�Ĵ���
	if(!IP2368_ReadReg(0x03,&buf))return false;
  *Power=(int)(buf&0x7F); //1LSB=1W
	*Power&=0x7F;
	//�������
	return true;
	}
	
//���������������ѹ
bool IP2368_SetPreChargeEndVoltage(PerChargeEndVoltDef EndVolt)
  {
	char buf;
	//��ȡ������SYS-CTL7�Ĵ���
	if(!IP2368_ReadReg(0x07,&buf))return false;
  buf&=0xF3; //���ԭ��������
  buf|=(char)EndVolt<<2; //����ΪĿ��ֵ		
	if(!IP2368_WriteReg(0x07,buf))return false;
  //�������
	return true;
	}

//���÷ŵ�͵�ѹ�ػ���ѹ
bool IP2368_SetLowVoltAlert(LVAlertDef LowVolt)
  {
  char buf;
	//��ȡ������SYS-CTL10�Ĵ���
	if(!IP2368_ReadReg(0x0A,&buf))return false;
  buf&=0x1F; //���ԭ��������
  buf|=(char)LowVolt<<5; //����ΪĿ��ֵ		
	if(!IP2368_WriteReg(0x0A,buf))return false;
  //�������
	return true;	
	}

//����Type-C�Ƿ�ʹ�ܷŵ繦��
bool IP2368_SetDischarge(bool IsDischage)
  {
	char buf;
	//��ȡ������SYS-CTL11�Ĵ���
	if(!IP2368_ReadReg(0x0B,&buf))return false;
	buf=IsDischage?buf|0x80:buf&0x7F; //����En-DCDC-output	
	if(!IP2368_WriteReg(0x0B,buf))return false;
	//��ȡ������TYPEC-CTL8�Ĵ���
	if(!IP2368_ReadReg(0x22,&buf))return false;
  buf&=0x3F; //���ԭ��������
	buf|=IsDischage?0xC0:0x00; //����vbus-mode-set(����ΪUFP����DFP)
	if(!IP2368_WriteReg(0x22,buf))return false;		
  //�������
	return true;	
	}

//��ȡTypeC�Ƿ�ʹ�ܷŵ繦��
bool IP2368_GetDischargeState(bool *IsDischage)
  {
	char buf;
  bool result;
	//��ȡSYS-CTL11�Ĵ���
	if(!IP2368_ReadReg(0x0B,&buf))return false;
	result=buf&0x80?true:false; //���En-DCDC-outputλ
	//��ȡTYPEC-CTL8�Ĵ���
	if(!IP2368_ReadReg(0x22,&buf))return false;
	buf&=0xC0; //����vbus mode set�������
	result&=buf==0x40?false:true; //���vbus-mode-set�����b01��˵��ģʽΪDFP���رշŵ�
	//���ؽ��		
	if(IsDischage!=NULL)*IsDischage=result;
	return true;
	}

//��ȡ����¶ȴ���������õ��¶�	
bool IP2368_GetNTCTemp(float *Temp,bool *IsResultOK)
  {
	char buf;
	unsigned short ADCWord;
	float calc,INTC;
	//��ȡNTC��ѹ
	if(!IP2368_ReadReg(0x78,&buf))return false; //VGPIO0_NTC[7:0]
	ADCWord=(unsigned short)buf&0xFF;
	if(!IP2368_ReadReg(0x79,&buf))return false; ///VGPIO0_NTC[15:8]
	ADCWord|=buf<<8; 
	calc=(float)(ADCWord); //LSB=1mV,�õ�mV�ĵ�ѹ
	//��ȡ�����NTC�ĵ����Ӷ���֪NTC����ֵ
  if(!IP2368_ReadReg(0x77,&buf))return false; //INTC_IADC_DAT0
	INTC=(float)(buf&0x80?80:20)/1000; //���INTC_IADC_DAT=1����NTC���80uA���������20uA(0.02/0.08mA)
	calc/=INTC; //R=U(mV)/I(mA),�õ�����ֵ(��)
	//�������ֵ
	calc/=1000; //��ֵ����(K��)
	calc=1/((1/(273.15+(float)NTCT0))+log(calc/(float)NTCT0Res)/(float)NTCBValue);//������¶�
	calc-=273.15;//��ȥ�����±곣����Ϊ���϶�
	calc+=(float)NTCTRIM;//��������ֵ	
	if(calc<(-40)||calc>125)	//�¶ȴ������쳣
	  {
		*IsResultOK=false;
		*Temp=0; //ָʾ�������쳣
		}
	else
	  {
		*IsResultOK=true; //����������
		*Temp=calc; //����¶�
		}
	//������ϣ�����true
	return true;
	}	
	
//IP2368�õ����״̬
bool IP2368_GetBatteryState(BatteryStatuDef *BatteryState)
  {
  char buf;
	unsigned short ADCWord;
	static bool IsStillCCCharge=false;
	//��ȡ��ص�ѹ
	do
	{			
	if(!IP2368_ReadReg(0x50,&buf))return false; //Battvadc[7:0]
	ADCWord=(unsigned short)buf&0xFF;
	if(!IP2368_ReadReg(0x51,&buf))return false; //Battvadc[15:8]
	ADCWord|=buf<<8; 
	BatteryState->BatteryVoltage=(float)(ADCWord)/1000; //LSB=1mV��ת��ΪV
	}
	while(BatteryState->BatteryVoltage>14.4); //��ص�ѹ�쳣�����Բ���
	BatteryState->BatteryVoltage-=0.11; //��ѹ��Ҫ��0.1������ȷֵ����ADC��Ʈ
  //��ȡ��ص���
	if(!IP2368_ReadReg(0x6E,&buf))return false; //iBattiadc[7:0]
	ADCWord=(unsigned short)buf&0xFF;
	if(!IP2368_ReadReg(0x6F,&buf))return false; //iBattiadc[15:8]
	ADCWord|=((unsigned short)buf&0xFF)<<8; 
  if(ADCWord&0x8000) //��ص���Ϊ��
	  {
		ADCWord&=0x7FFF; //ȥ������λ
		BatteryState->BatteryCurrent=((float)(ADCWord)/1000)*-1; //����Ϊ����LSB=-1mA��ת��ΪA
		}
	else BatteryState->BatteryCurrent=(float)(ADCWord)/1000; //����Ϊ����LSB=1mA��ת��ΪA
	//��ȡRSOC
	if(!IP2368_ReadReg(0x30,&buf))return false; //SOC CAP DATA
	BatteryState->BatteryRSOC=(int)buf; 
	if(BatteryState->BatteryRSOC>100)BatteryState->BatteryRSOC=100;
	if(BatteryState->BatteryRSOC<0)BatteryState->BatteryRSOC=0; //�޷�
  //��ȡ���״̬
	if(!IP2368_ReadReg(0x31,&buf))return false; //STATE-CTL0
	if(buf&0x08)BatteryState->BattState=Batt_discharging; //��������ã������������ŵ�
	else BatteryState->BattState=(BatteryStateDef)(buf&0x07); //��ȡ���״̬
	if(BatteryState->BattState==Batt_CVCharge) //��ǰIP2368���ں�ѹģʽ����ʼ���ݵ�����ֵ�ж��Ƿ�λ�ں���
	  {
		if(!IsStillCCCharge&&BatteryState->BatteryCurrent>2.65)
		   IsStillCCCharge=true;  //��ǰ����ֵ����2.65A����Ȼ���ں������׶�
		else if(IsStillCCCharge&&BatteryState->BatteryCurrent<2.1)
		   IsStillCCCharge=false; //��ǰ����ֵС��2.1A����ʱ��ű�������ѹ���
		if(IsStillCCCharge)BatteryState->BattState=Batt_CCCharge; //��ǰ������Ȼ���󣬱������״̬
		}
  //������ϣ�����true
  return true;
	}

//IP2368���ó��ʹ��
bool IP2368_SetChargerState(bool IsChargerEnabled)
  {
	char buf;
	//��ȡ������SYS-CTL0�Ĵ���
	if(!IP2368_ReadReg(0x00,&buf))return false;
	buf=IsChargerEnabled?buf|0x01:buf&0xFE; //����en-chargerλ
	buf=IsChargerEnabled?buf|0x02:buf&0xFD; //����en-vbus_sinkpdλ
	if(!IP2368_WriteReg(0x00,buf))return false;		
  //������ϣ�����true
  return true;	
	}
	
//IP2368��ȡ״̬flag
bool IP2368_GetAlertFlag(FaultFlagStrDef *Fault)
  {
	char buf;
	bool IsResultOK;
  float Temp;
	//��ȡSTATE-CTL2
	if(!IP2368_ReadReg(0x33,&buf))return false; 
	Fault->INOVFault=buf&0x40?true:false;
	//��ȡSTATE-CTL3
  if(!IP2368_ReadReg(0x38,&buf))return false; 
	if(buf&0x20) //VSYSOC=1
	  {
		buf|=0x20;
		if(!IP2368_WriteReg(0x38,buf))return false;	//��üĴ���д1����
		delay_ms(600);
		if(!IP2368_ReadReg(0x38,&buf))return false; 
		Fault->OCPFault=buf&0x20?true:false; //600mS���ٴζ�ȡ�������Ϊ��������¼�����
    buf|=0x20;
		if(!IP2368_WriteReg(0x38,buf))return false;	//��üĴ���д1����
		}
	else Fault->OCPFault=false; //δ���������¼�
	if(buf&0x10) //VSYSSCDT=1
    {
		buf|=0x10;
		if(!IP2368_WriteReg(0x38,buf))return false;	//��üĴ���д1����
		delay_ms(600);
		if(!IP2368_ReadReg(0x38,&buf))return false; 
		Fault->SCPFault=buf&0x10?true:false; //600mS���ٴζ�ȡ�������Ϊ�����·�¼�����
    buf|=0x10;
		if(!IP2368_WriteReg(0x38,buf))return false;	//��üĴ���д1����
		}		
	else Fault->SCPFault=false; //��·�¼�δ����	
	//��ȡϵͳ�¶�
	if(!IP2368_GetNTCTemp(&Temp,&IsResultOK))return false;	
	if(IsResultOK) //�����Ч
	  {
		Fault->UTFault=Temp>LowTempFault?false:true; //���±���
		if(Temp>TemperatureFault)//�����쳣
		   {
		   Fault->OTAlert=false;
			 Fault->OTFault=true;
	     }
    else if(Temp>TemperatureAlert) //���Ⱦ���
		   {
		   Fault->OTAlert=true;
			 Fault->OTFault=false;
	     }
		else //�޸澯
			 {
		   Fault->OTAlert=false;
			 Fault->OTFault=false;
	     }
		}
	//���и澯����
	if(Fault->INOVFault||Fault->OCPFault)Fault->FaultAsserted=true;
	else if(Fault->OTFault||Fault->SCPFault)Fault->FaultAsserted=true;
	else if(Fault->UTFault)Fault->FaultAsserted=true;
	else Fault->FaultAsserted=false;
	//�����ϣ�����true
	return true;
	}
//IP2368��ȡTypec�Ƿ�����
bool IP2368_GetTypeCConnectedState(bool *IsTypeCConnected)
  {
	char buf;
	//��ȡTYPEC_State0
	if(!IP2368_ReadReg(0x34,&buf))return false; 
	if(Config.IsEnableOutput)*IsTypeCConnected=buf&0xC0?true:false; //���Sink��source����һλΪ1˵��typec�����ӣ���ʱ����true
	else *IsTypeCConnected=buf&0x80?true:false; //����������ģʽ����Type-C����Ӧ������󣬽�sinkģʽ�²Ž��м��
	//�����ϣ�����true
  return true;
	}	
	
//IP2368����ͣ��������ٳ�����
bool IP2368_SetChargeParam(int IChargeEnd,RechargeModeDef RechargeThr)	
  {
	char buf;
	//��ȡSYS-CTL8
	if(!IP2368_ReadReg(0x08,&buf))return false;	
	//�޸�ͣ�����
	IChargeEnd/=50;
	if(IChargeEnd<=0)return false; //�����Ƿ�
	IChargeEnd&=0x0F; //ȥ��bit0-3
	buf&=0x0F;
	buf|=IChargeEnd<<4; //�޸�bit7-4����ͣ�����
	//�޸��ٳ�����
	buf&=0xF3; 
	buf|=((char)RechargeThr<<2)&0x0C; //����enumֵ�޸Ķ�Ӧ��bit
  //��дSYS-CTL8
	if(!IP2368_WriteReg(0x08,buf))return false;		
	return true;
	}
//IP2368���ÿ��״̬
bool IP2368_SetQuickchargeState(QuickChargeCtrlDef *QCState)
  {
	char buf;
	//�޸�SYS-CTL0
	if(!IP2368_ReadReg(0x00,&buf))return false;
	buf=QCState->IsEnableDPDM?buf|0x10:buf&0xEF; //EN_VBUS_sinkDPDM
	buf=QCState->IsEnablePD?buf|0x08:buf&0xF7; //EN_VBUS_sinkPD
	buf=QCState->IsEnableSCP?buf|0x04:buf&0xfb; //EN_VBUS_sinkSCPVOOC
	if(!IP2368_WriteReg(0x00,buf))return false;		
	//�޸�SYS-CTL11
	if(!IP2368_ReadReg(0x0B,&buf))return false;
	buf=QCState->IsEnableDPDM?buf|0x40:buf&0xBF; //EN_VBUS_srcDPDM
	buf=QCState->IsEnablePD?buf|0x20:buf&0xDF; //EN_VBUS_srckPD
	buf=QCState->IsEnableSCP?buf|0x10:buf&0xEF; //EN_VBUS_srcSCP
	if(!IP2368_WriteReg(0x0B,buf))return false;			
	//����TypeC-CTL17
  if(!IP2368_ReadReg(0x2B,&buf))return false;		
	buf=QCState->IsEnable20VPDO?buf|0x10:0xEF; //EN_SRC_20VPDO
	buf=QCState->IsEnable9VPDO?buf|0x02:buf&0xFD; //EN_SRC_9VPDO
	if(!IP2368_WriteReg(0x2B,buf))return false;				
	//�޸���ϣ�����true
	return true;
	}	
//IP2368��ȡ���״̬	
bool IP2368_GetQuickchargeState(QuickChargeCtrlDef *QCState)
  {
	char buf;
	//��ȡSYS-CTL0
	if(!IP2368_ReadReg(0x00,&buf))return false;
	QCState->IsEnableDPDM=buf&0x10?true:false;//EN_VBUS_sinkDPDM
	QCState->IsEnablePD=buf&0x08?true:false;//EN_VBUS_sinkPD
	QCState->IsEnableSCP=buf&0x04?true:false;//EN_VBUS_sinkSCPVOOC
	//��ȡSYS-CTL11
	if(!IP2368_ReadReg(0x0B,&buf))return false;
	QCState->IsEnableDPDM&=buf&0x40?true:false;//EN_VBUS_srcDPDM
	QCState->IsEnablePD&=buf&0x20?true:false; //EN_VBUS_srckPD
	QCState->IsEnableSCP&=buf&0x10?true:false; //EN_VBUS_srcSCP
	//��ȡTypeC-CTL17
  if(!IP2368_ReadReg(0x2B,&buf))return false;		
	QCState->IsEnable20VPDO=buf&0x10?true:false; //EN_SRC_20VPDO
	QCState->IsEnable9VPDO=buf&0x02?true:false; //EN_SRC_9VPDO	
	//�����ϣ�����true
	return true;
	}
	
//���ɸ�λ����
bool IP2368_SendResetCommand(void)
  {
	char buf;
	//�޸�SYS-CTL0
	if(!IP2368_ReadReg(0x00,&buf))return false;
	buf|=0x40; //set EN_RESETMCUΪ1ʹIP2368��λ
	if(!IP2368_WriteReg(0x00,buf))return false;			
	//�����ɹ����
	return true;
	}	

//��ȡType-C�Ƿ����ӵ������Source��ģʽ
bool IP2368_GetIsTypeCSrcConnected(bool *IsConnected)
  {
	char buf;
	//��ȡTYPEC_STATE0
	if(!IP2368_ReadReg(0x34,&buf))return false;
	*IsConnected=(buf&0x40)?true:false;
	//������ ����true
  return true;		
	}
	
//IP2368��ȡTypeC״̬
bool IP2368_GetTypeCState(TypeCStatusDef *TypeCStat)
  {
	char buf;
	unsigned int ADCWord;
	bool State;
	//��ȡ�����ѹ
	if(!IP2368_ReadReg(0x52,&buf))return false; //VSYSVADC[7:0]
	ADCWord=(unsigned short)buf&0xFF;
	if(!IP2368_ReadReg(0x53,&buf))return false; //VSYSVADC[15:8]
	ADCWord|=buf<<8; 
	TypeCStat->busVoltage=(float)(ADCWord)/1000; //LSB=1mV��ת��ΪV
	//��ȡ���빦�ʺ͵���
	if(!EnableXARIIMode)do	//V1.2�汾Э�飬��ȡ�����Ĵ���
	  {
		if(!IP2368_ReadReg(0x70,&buf))return false; // ISYSIADC[7:0]
		ADCWord=(unsigned int)buf&0xFF;
		if(!IP2368_ReadReg(0x71,&buf))return false; // ISYSIADC[15:8]
		ADCWord|=buf<<8; 
		if(ADCWord&0x8000) //��ص���Ϊ��
			{
			ADCWord&=0x7FFF; //ȥ������λ
			TypeCStat->BusCurrent=((float)(ADCWord)/1000)*-1; //����Ϊ����LSB=-1mA��ת��ΪA
			}
		else TypeCStat->BusCurrent=(float)(ADCWord)/1000; //����Ϊ����LSB=1mA��ת��ΪA	
		TypeCStat->BusPower=fabsf(TypeCStat->BusCurrent)*TypeCStat->busVoltage; //��ѹ�͵�������ֵ��˵õ�����
		}
	while(TypeCStat->BusPower>105); //�����ȡ���Ĺ���ֵ�Ƿ�������
	else do//V1.63�汾Э�飬��ȡ���ʼĴ���
	  {		
		if(!IP2368_ReadReg(0x74,&buf))return false; //VSYSPOWADC[7:0]
		ADCWord=(unsigned int)buf&0xFF;
		if(!IP2368_ReadReg(0x75,&buf))return false; //VSYSPOWADC[15:8]
		ADCWord|=buf<<8; 
		if(!IP2368_ReadReg(0x76,&buf))return false; //VSYSPOWADC[23:16]
		ADCWord|=buf<<16; 
		TypeCStat->BusPower=(float)(ADCWord)/1000; //LSB=1mW��ת��ΪW
		TypeCStat->BusCurrent=fabsf(TypeCStat->BusPower/TypeCStat->busVoltage); //ͨ���ܹ��ʳ��Ե�ѹ�õ�����
		}
	while(TypeCStat->BusPower>105); //�����ȡ���Ĺ���ֵ�Ƿ�������
	//���ݵ���Ƿ��ڷŵ�״̬�жϵ�����ʾ		
  if(!IP2368_ReadReg(0x31,&buf))return false; //STATE-CTL0	
	if(buf&0x08)TypeCStat->BusCurrent*=(float)-1; //���ڷŵ�ģʽ������Ϊ��
	//��ȡPD Input״̬
	if(!IP2368_ReadReg(0x34,&buf))return false; //TYPEC-STATE0
	if((buf&0x60)==0x60) //Type-C����SRCģʽ��PD�ɹ�����
	  {
		//��ȡSYS-CTL11
		if(!IP2368_ReadReg(0x0B,&buf))return false;
		if(buf&0x20) //PD�Ѿ�����
		    {
				State=true; //Ĭ��Ϊ��
				//��ȡTYPEC-CTL17
		    if(!IP2368_ReadReg(0x2B,&buf))return false;
				//���ݵ�ѹ�����ж�
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
				else TypeCStat->PDState=PD_5VMode; //ʶ���ѹ�ж����ģʽ
		    if(State)TypeCStat->QuickChargeState=TypeCStat->busVoltage>6.0?QuickCharge_PD:QuickCharge_None; //��ǰ��ѹģʽ����Ӧ��PDO�ǿ��ŵģ������ѹ����6V����PDģʽ	 
				else //��ǰ��ѹģʽ����Ӧ��PDO�ǹرյģ������ѹ����6V���Ǹ�ѹģʽ	 
				   {
					 TypeCStat->PDState=PD_5VMode; //��ѹ��䣬ָʾ��5V��PD��λ
				   TypeCStat->QuickChargeState=TypeCStat->busVoltage>6.0?QuickCharge_HV:QuickCharge_None; 
					 }
				}
		else TypeCStat->QuickChargeState=TypeCStat->busVoltage>6.0?QuickCharge_HV:QuickCharge_None; //���PD�Ѿ����رգ����ж�Ϊ��ѹ���
		}
	else if((buf&0x90)==0x90)//Type-C����SNKģʽ��PD�ɹ�����
	  {
		if(!IP2368_ReadReg(0x33,&buf))return false; //STATE-CTL2
		buf&=0x07;
		buf-=0x02; //��״̬λ����0x02ת��Ϊenumֵ
		TypeCStat->PDState=(PDStateDef)buf;
		TypeCStat->QuickChargeState=buf>0?QuickCharge_PD:QuickCharge_None; //�����ѹ����6V����PDģʽ
		}
	else  //����״��
	  {
		TypeCStat->PDState=PD_5VMode;
		if(TypeCStat->busVoltage>6.0||buf&0x04)TypeCStat->QuickChargeState=QuickCharge_HV;//��ѹ���
		else if(TypeCStat->busVoltage<=6.0&&fabsf(TypeCStat->BusCurrent)>2.0)TypeCStat->QuickChargeState=QuickCharge_HC;//��ѹ�������
		else TypeCStat->QuickChargeState=QuickCharge_None; //����״����Ϊδʶ����
		}
  //������ϣ�����true
  return true;	
	}
//���IP2368�Ƿ������OTP
bool IP2368_CheckIfOTPLoaded(bool *IsLoaded)
  {
	char buf;
	//��ȡTYPEC_CTL3
	if(!IP2368_ReadReg(0x03,&buf))return false;	
	*IsLoaded=(buf&0x80)?false:true; //�������Ĵ�������λ����˵��IP2368�Ѿ����¼�����OTP
	//������
	return true;
	}

//���üĴ�����NVRAM��Flag���м��	
bool IP2368_SetOTPReloadFlag(void)
  {
	char buf;
	//��ȡ���޸�SYS_CTL3
	if(!IP2368_ReadReg(0x03,&buf))return false;	 
	buf|=0x80;
	if(!IP2368_WriteReg(0x03,buf))return false;	 //ǿ�Ƹ�λ
	//������
	return true;
	}

//����ʣ������Ĵ���
bool IP2368_SetRSOC(int Level)	
  {
	char buf;
  //�������	
	if(Level>100||Level<0)return false;
	//д��SYS_CTL6
	buf=(char)Level&0x7F;
	if(!IP2368_WriteReg(0x06,buf))return false;	 
	//�������
	return true;
	}	

//��ʼ��IP2368��GPIO	
static bool IsForceWakeUp=false; //��Ǳ���������Ƿ�ǿ�ƻ���	
	
void IP2368_GPIOInit(void)	
  {
	//������������ܵ�IO
	AFIO_GPxConfig(VDiode_IOB,VDiode_IOP, AFIO_FUN_GPIO);//�������������
  GPIO_DirectionConfig(VDiode_IOG,VDiode_IOP,GPIO_DIR_OUT);//����Ϊ���
	 GPIO_ClearOutBits(VDiode_IOG,VDiode_IOP);//�������Ϊ0
	//����GPIO����ȡGPIO״̬�ж����������ѻ��Ǳ���
	AFIO_GPxConfig(IP2368_INT_IOB,IP2368_INT_IOP, AFIO_FUN_GPIO);//GPIO����
  GPIO_DirectionConfig(IP2368_INT_IOG,IP2368_INT_IOP,GPIO_DIR_IN);//����Ϊ����
	GPIO_InputConfig(IP2368_INT_IOG,IP2368_INT_IOP,ENABLE);//����IDR
	if(GPIO_ReadInBit(IP2368_INT_IOG,IP2368_INT_IOP)==RESET)IsForceWakeUp=true; //IP2368��˯������ʱǿ�ƻ���
  //����Ϊ�����INT=1��ʹIP2368����˯��
	GPIO_InputConfig(IP2368_INT_IOG,IP2368_INT_IOP,DISABLE); 
	GPIO_DirectionConfig(IP2368_INT_IOG,IP2368_INT_IOP,GPIO_DIR_OUT);//����IDR������Ϊ���
	GPIO_SetOutBits(IP2368_INT_IOG,IP2368_INT_IOP); //��INT=1��ǿ�ƻ���IP2368
	delay_ms(150); //����2368֮����Ҫ��150mS���ܿ�ʼ���ʼĴ���
	//���üĴ����رճ��ͷŵ�
	IP2368_SetChargerState(false); 
	IP2368_SetDischarge(false);
	}

//����RSOCУ׼
void IP2368_DoRSOCCalibration(void)
  {
	int retry,i;
	float RSOC,FullVolt,EmptyVolt,LastRSOC[3]={0},delta;
	BatteryStatuDef BatteryState;
	//��ʼУ׼
	OLED_Clear();
	OLED_Printf(0,0,64,1,"RSOC Cal Start...");
	OLED_Refresh();
	GPIO_SetOutBits(VDiode_IOG,VDiode_IOP);//�������Ϊ1�������������  	
	retry=0;
	do
		{
		delay_ms(50);
		if(!IP2368_GetBatteryState(&BatteryState)) //��ȡ��ص�ѹ
		   {
			 OLED_Printf(0,12,64,1,"Failed,ERR:0");
	     OLED_Refresh();
			 delay_Second(1);
			 return;
			 }
		FullVolt=(float)Config.BatteryCount*4.2;	
		EmptyVolt=(float)Config.BatteryCount*2.8; //��������͵�ص�ѹ	
		RSOC=(BatteryState.BatteryVoltage-EmptyVolt)/(FullVolt-EmptyVolt); //���ݵ�ѹ���ݼ���RSOC
		RSOC*=100; //ת��100%
		//ת�Ʋ�������
		for(i=2;i>0;i--)LastRSOC[i]=LastRSOC[i-1];
    LastRSOC[0]=RSOC;
    retry++;
		//��������sample֮��ľ���
		delta=0;
    for(i=0;i<3;i++)delta+=LastRSOC[i];	
		EmptyVolt=delta/(float)3; //��ƽ�������ݴ�һ��
		delta=-2000;
    for(i=0;i<3;i++)if(delta<fabsf(LastRSOC[i]-EmptyVolt))delta=fabsf(LastRSOC[i]-EmptyVolt); //ȡ��������Ԫ��			
		}
	while(delta>1.00&&retry<20); //����������RSOCֱ������sample֮��ľ���С�ڵ���1%������
	if(RSOC<0)RSOC=0;
  if(RSOC>100)RSOC=100; //��ֵ�޷�����ΪRSOCֻ����0-100%	
	if(!IP2368_SetRSOC(iroundf(RSOC)))OLED_Printf(0,12,64,1,"Failed,ERR:1");
	else OLED_Printf(0,12,64,1,"OK,RSOC=%d%%",iroundf(RSOC));//�ɹ����
	//������ϣ���������
	OLED_Refresh();
	delay_Second(1);	
	}	
	
//��ʼ��IP2368�ļĴ���֮���
void IP2368_init(void)
  {
	BatteryStatuDef BatteryState;
  QuickChargeCtrlDef QCtrl;
	bool Result=false;
	int retry=0,BattCount,i;
	float RSOC,FullVolt,EmptyVolt,LastRSOC[3]={0},delta;
	//��ʼ��ʼ�����̼��Э��汾
	OLED_Printf(0,0,64,1,"PSoC Init...");
	OLED_Refresh();
  do
	 {
	 if(IP2368_ReadReg(0x76,NULL))break;
	 retry++;
	 }		
	while(retry<5); //ͨ����ȡ0x76�Ĵ������Э��汾	
	EnableXARIIMode=(retry==5)?false:true; //�����������μĴ������޷���ȡ˵��оƬΪ�ɰ汾����ʱͨ����ȡ�������õ�����
  //��ʼ���е�ؼ��
	if(!IP2368_GetTypeCConnectedState(&Result)) //���Խ���ͨ��
	  {
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368-I2C Connection Failed.");
	  OLED_Refresh();		
		CurrentLEDIndex=11; //��ֹ��������ʧ��
		while(1);
		}
	if(!IP2368_GetBatteryState(&BatteryState)) //��ȡ��ص�ѹ
	  {
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368 BTELEM ERR.");
	  OLED_Refresh();		
		CurrentLEDIndex=12; //���ң��ʧ��
		while(1);
		}		
  if(BatteryState.BatteryVoltage<5.5||BatteryState.BatteryVoltage>16.9) //��ص�ѹ�쳣
 		{
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"Battery");
		OLED_Printf(0,6,64,1,BatteryState.BatteryVoltage<8.1?"disconnected!":"OverVolt!");			
	  OLED_Refresh();		
		CurrentLEDIndex=3; //��Ƴ���
		while(1);
		}		
	if(Config.BatteryCount==0) //��ؽ���δ����
	  {
    if(BatteryState.BatteryVoltage>12.7)BattCount=4;
		else if(BatteryState.BatteryVoltage>8.5)BattCount=3;
    else BattCount=2; //���ݵ�ѹʶ���ؽ���		
		OLED_Printf(0,6,64,1,"PSoC BattCfg=%dCell",BattCount);
		OLED_Refresh();
		delay_Second(1);
		//��ʼ���¹�������
		Config.BatteryCount=BattCount;
		RestoreDefaultCfg();
		//������ʾ
		OLED_OldTVFade();
		OLED_Printf(0,0,64,1,"PSoC Init...");
	  OLED_Refresh();
		}			
  //������������ܲ����RSOC
	GPIO_SetOutBits(VDiode_IOG,VDiode_IOP);//�������Ϊ1�������������  	
	retry=0;
	do
		{
		delay_ms(50);
		IP2368_GetBatteryState(&BatteryState); //��ȡ��ص�ѹ
		FullVolt=(float)Config.BatteryCount*4.2;	
		EmptyVolt=(float)Config.BatteryCount*2.8; //��������͵�ص�ѹ	
		RSOC=(BatteryState.BatteryVoltage-EmptyVolt)/(FullVolt-EmptyVolt); //���ݵ�ѹ���ݼ���RSOC
		RSOC*=100; //ת��100%
		//ת�Ʋ�������
		for(i=2;i>0;i--)LastRSOC[i]=LastRSOC[i-1];
    LastRSOC[0]=RSOC;
    retry++;
		//��������sample֮��ľ���
		delta=0;
    for(i=0;i<3;i++)delta+=LastRSOC[i];	
		EmptyVolt=delta/(float)3; //��ƽ�������ݴ�һ��
		delta=-2000;
    for(i=0;i<3;i++)if(delta<fabsf(LastRSOC[i]-EmptyVolt))delta=fabsf(LastRSOC[i]-EmptyVolt); //ȡ��������Ԫ��			
		}
	while(delta>1.00&&retry<20); //����������RSOCֱ������sample֮��ľ���С�ڵ���1%������
	if(RSOC<0)RSOC=0;
  if(RSOC>100)RSOC=100; //��ֵ�޷�����ΪRSOCֻ����0-100%	
	if(!IP2368_SetRSOC(iroundf(RSOC)))	
	  {
		OLED_Clear();
		OLED_Printf(0,0,64,1,"IP2368 SetRSOC ERR.");
		OLED_Refresh();		
		while(1);
		}
	//���üĴ���
	if(!Result||IsForceWakeUp) //�����ǿ�ƻ���,����typecδ���ӣ���ִ��IP2368���Լ�
	  {
		OLED_Printf(0,6,64,1,"PSoC is Ready,Using V1.%s Protocol.",EnableXARIIMode?"63":"2");
		OLED_Refresh();		
		GPIO_SetOutBits(VDiode_IOG,VDiode_IOP);//�������Ϊ1�������������
		delay_Second(1);
		OLED_OldTVFade();		
		IP2368_SetChargerState(true);	//���ó��ģ��
		IP2368_SetDischarge(Config.IsEnableOutput); //���������������û��߽��÷ŵ�ģ��
		return;	
		}
	//����Type-C����״̬���
	IP2368_GetIsTypeCSrcConnected(&Result);
	if(!Result) //���Type-C�Ƿ���Source��ʽ���ӣ�����ǣ�����������Ķ����ʼ��
	  {
	  if(!IP2368_SetChargePower(ConvertChagePower())) //���ó�繦��
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
	//���õ͵�ѹ�澯
	if(!IP2368_SetLowVoltAlert(LVAlert_2V8))
		{
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368 LVSET ERR.");
	  OLED_Refresh();		
		while(1);
		}	
	//���ÿ��ģʽ
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
	//����OTP���nibble
	if(!IP2368_SetOTPReloadFlag()) //���¼���flag
		{
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368 OTPDCTP ERR.");
	  OLED_Refresh();		
		while(1);
		}	
	//�������ó��ģ��
	retry=0;
  while(retry<300)//�����������ó��ģ��
	  {
		if(IP2368_SetChargerState(true))break;
		delay_ms(100);
		retry++;
		}		
	if(retry==300) //���Գ�ʱ
		{
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368 ENCHG ERR.");
	  OLED_Refresh();		
		while(1);
		}		

	//�����Ƿ�����ŵ�
	IP2368_SetDischarge(true);	
	delay_ms(5);	
	if(!IP2368_SetDischarge(Config.IsEnableOutput)) //��������ΪIP2368��bug����Ҫͨ����ʱ������Type-C�ӿ�ΪUFPʹIP2368��������
	  {
		OLED_Clear();
	  OLED_Printf(0,0,64,1,"IP2368 DRPSET ERR.");
	  OLED_Refresh();		
		while(1);
		}	
	//���в�����ɣ�ָʾ�ɹ����
	OLED_Printf(0,6,64,1,"PSoC is Ready,Using V1.%s Protocol.",EnableXARIIMode?"63":"2");
	OLED_Refresh();	
	delay_Second(1);
	OLED_OldTVFade();
	}
