#include "Config.h"
#include "delay.h"

//�����ֽ���������
#define ConfigADDR 0xFC00

//���ýṹ��
SystemConfigStrDef Config;
extern bool EnableXARIIMode; //�ڲ�flagλ���Ƿ�����Э��汾V1.63������Ĵ���

//ת����繦��
int ConvertChagePower(void)
  {
	switch(Config.ChargePower)
	  {
		case ChargePower_30W:return 30;
		case ChargePower_45W:return 45;
	  case ChargePower_60W:return 60;
		case ChargePower_65W:return 66;
		}
	return 45; //�����������45
	}

//ת���������ò���
char ConvertBrightLevel(void)
  {
		switch(Config.Brightness)
	  {
		case Screen_MaxBright:return 127;
		case Screen_HighBright:return 70;
		case Screen_MidBright:return 45;
		case Screen_LowBright:return 10;
		}		
	return 0x4F;
	}	
	
//�ָ�Ĭ������
bool RestoreDefaultCfg(void)
  {
	Config.ChargePower=Config.BatteryCount<3?ChargePower_45W:ChargePower_65W;
	Config.IsEnableOutput=true;
  Config.IsEnableDPDM=true;
	Config.IsEnableSCP=true;
	Config.IsEnablePD=true;
	Config.IsEnable9VPDO=EnableXARIIMode?true:false; //������°汾���Զ����м��
	Config.IsEnable20VPDO=Config.BatteryCount<3?false:true;
	Config.Brightness=Screen_MidBright; //�������
	return SavingConfig(); 
	}
	
//��������
bool SavingConfig(void)
  {
	unsigned int sbuf,data;
	int i,zerocount=0;
	//��������
	data=0;
	if(Config.IsEnableOutput)data|=0x05;
	else data|=0x02; //bit0-2 �Ƿ�ʹ��˫�����
	sbuf=(unsigned int)Config.ChargePower; 
	sbuf&=0x03;
	data|=sbuf<<3; //bit3-4 ��繦��
	if(Config.IsEnableDPDM)data|=0x20; //bit 5 �Ƿ�ʹ��DPDM���
	if(Config.IsEnableSCP)data|=0x40; //bit 6 �Ƿ�ʹ��SCP���
	if(Config.IsEnablePD)data|=0x80;//bit 7 �Ƿ�ʹ��PD���
	sbuf=(unsigned int)Config.Brightness; 
	sbuf&=0x03;
	data|=sbuf<<8;	 //bit8-9 ��Ļ����	
	if(Config.IsEnable9VPDO)data|=0x400; //bit 10 �Ƿ�ʹ��9V PDO
	if(Config.IsEnable20VPDO)data|=0x800; //bit 11 �Ƿ�ʹ��20V PDO
	sbuf=(unsigned int)Config.BatteryCount;
	sbuf&=0x07;
	data|=sbuf<<12; //bit12-14 ��ؽ���	
	//�򵥵�У��
	sbuf=data;
	for(i=0;i<30;i++)
		{
		if(!(sbuf&0x1))zerocount++; 
		sbuf=sbuf>>1; //�ӵ�λ����λ�𲽼���		
		}
	if(zerocount%2)data|=0x80000000; //������0����Ϊ��������ʱ���λΪ10
  else data|=0x40000000; //������0����Ϊ��������ʱ���λΪ01
	//��ʼд������
	if(*((u32 *)ConfigADDR)==data)return true; //����û�з����ı䣬ֱ�ӽ���
	i=0;
  do
	 {
	 if(FLASH_ErasePage(ConfigADDR)!=FLASH_COMPLETE)return false;
   if(FLASH_ProgramWordData(ConfigADDR, data)!=FLASH_COMPLETE)return false;
	 i++;
	 if(i>=30)return false; //���Դ�������
	 }
  while(*((u32 *)ConfigADDR)!=data); //����Ƿ�ˢд�ɹ�����������
	//ˢд����
	return true;
	}

//��ȡ����
void ReadConfig(void)
  {
	bool CheckSumError=true;
	unsigned int sbuf,checksum;
	int i,zerocount=0;
	bool IsIllegalConfig;
	//У������
	sbuf=*((u32 *)ConfigADDR); //��ȡ����
  for(i=0;i<30;i++)
		{
		if(!(sbuf&0x1))zerocount++; 
		sbuf=sbuf>>1; //�ӵ�λ����λ�𲽼���		
		}
	if(zerocount%2)checksum=0x80000000; //������0����Ϊ��������ʱ���λΪ10
  else checksum=0x40000000; //������0����Ϊ��������ʱ���λΪ01	
	if(checksum==(*((u32 *)ConfigADDR)&0xC0000000))	//У�����ȷ
	  {
		CheckSumError=false;
		switch(*((u32 *)ConfigADDR)&0x07) //�ж�Outputmode[2:0]
			 {
			 case 0x05:Config.IsEnableOutput=true;break; //����
			 case 0x02:Config.IsEnableOutput=false;break; //�ر�
			 default :CheckSumError=true; //���ô���
			 }
		sbuf=(*((u32 *)ConfigADDR)>>3)&0x03; 
		Config.ChargePower=(ChargePowerDef)sbuf; //��ֵ��繦��enum(bit3-4)
		sbuf=(*((u32 *)ConfigADDR)>>8)&0x03; 
		Config.Brightness=(ScreenBrightDef)sbuf; //��ֵ��Ļ����enun(bit8-9)
		Config.IsEnableDPDM=(*((u32 *)ConfigADDR)&0x20)?true:false; //bit 5 �Ƿ�ʹ��DPDM���
		Config.IsEnableSCP=(*((u32 *)ConfigADDR)&0x40)?true:false; //bit 6 �Ƿ�ʹ��SCP���
		Config.IsEnablePD=(*((u32 *)ConfigADDR)&0x80)?true:false;	//bit 7 �Ƿ�ʹ��PD���	 
		Config.IsEnable9VPDO=(*((u32 *)ConfigADDR)&0x400)?true:false; //bit 10 �Ƿ�ʹ��9V PDO
		Config.IsEnable20VPDO=(*((u32 *)ConfigADDR)&0x800)?true:false; //bit 11 �Ƿ�ʹ��20V PDO
		sbuf=(*((u32 *)ConfigADDR)>>12)&0x07; 
		Config.BatteryCount=(char)sbuf; //bit12-14 ��ؽ���	
		//�������úϷ����ж�
		if(Config.BatteryCount<3&&Config.ChargePower==ChargePower_60W)IsIllegalConfig=true;	
		else if(Config.BatteryCount<3&&Config.ChargePower==ChargePower_65W)IsIllegalConfig=true;	 //ֻ��2�ڵ�ص�����³�繦�ʴ���60W
		else if(Config.BatteryCount<3&&Config.IsEnable20VPDO)IsIllegalConfig=true; //���ڵ���²�������PDO��λ
		else IsIllegalConfig=false;		
		//��⵽�Ƿ����ã������޸�
		if(IsIllegalConfig&&!CheckSumError)
			{
			Config.ChargePower=ChargePower_45W; 
			Config.IsEnable20VPDO=false;
			SavingConfig(); //��������	    		
			} 
		}	  
	//У��ͳ�����д����
	if(!CheckSumError)return;
	Config.BatteryCount=0; //��ؽ���Ϊ0
  RestoreDefaultCfg(); //�ָ�Ĭ������
	NVIC_SystemReset(); //����MCU
	}
