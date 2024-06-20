#include "Pindefs.h"
#include "IP2368.h"
#include "delay.h"
#include "oled.h"
#include "Config.h"

//��Դ���������Զ�����
#define LDO_EN_IOB STRCAT2(GPIO_P,LDO_EN_IOBank)
#define LDO_EN_IOG STRCAT2(HT_GPIO,LDO_EN_IOBank)
#define LDO_EN_IOP STRCAT2(GPIO_PIN_,LDO_EN_IOPinNum) 

//�ⲿ���ڲ�ȫ�ֱ���
extern int SleepTimer;  //˯�߶�ʱ��
static int IPStallTime=0; //оƬֹͣ��Ӧ��ʱ��

//��ʼ��IO����Ծٲ���
void PowerMgmtSetup(void)
  {
   AFIO_GPxConfig(LDO_EN_IOB,LDO_EN_IOP, AFIO_FUN_GPIO);
   GPIO_DirectionConfig(LDO_EN_IOG,LDO_EN_IOP,GPIO_DIR_OUT);//����Ϊ���
   GPIO_SetOutBits(LDO_EN_IOG,LDO_EN_IOP);//�������Ϊ1
	 GPIO_DriveConfig(LDO_EN_IOG,LDO_EN_IOP,GPIO_DV_16MA);	//����Ϊ16mA��������ָ֤ʾ�������㹻
	}

//�ر�MCU
void ForceShutOff(void)	
  {
	GPIO_ClearOutBits(LDO_EN_IOG,LDO_EN_IOP);//�������Ϊ0,�ر�LDO��Դǿ�ȵ�Ƭ������
	while(1);	
	}

//Type-C����ʧ��ʱ�������������ֵĲ���
static void IP2368StallRestore(void)
  {
	int wait;
	//��ʱ�ۼӲ���
	IPStallTime++;
	if(IPStallTime>=40) //IP2368���ߴ�Լ5���ʼ����
	  {
		IPStallTime=0;  //������λ���ȴ��´μ�ʱ
		GPIO_InputConfig(IP2368_INT_IOG,IP2368_INT_IOP,ENABLE); 
	  GPIO_DirectionConfig(IP2368_INT_IOG,IP2368_INT_IOP,GPIO_DIR_IN);//����Ϊ��������ʹ2368����
		wait=300; //���Ѽ����ʱ300mS
		do
		  {
			wait--;
      if(wait<=0)break;	//200mS��IP2368����ʧ�ܣ��˳�
			delay_ms(1);		
			}
		while(GPIO_ReadInBit(IP2368_INT_IOG,IP2368_INT_IOP)==SET); //IP2368λ�ڻ���״̬���ȴ����ѽ���
		if(wait>0)delay_ms(100); //�ȴ�100mS������INT
		GPIO_InputConfig(IP2368_INT_IOG,IP2368_INT_IOP,DISABLE); 
	  GPIO_DirectionConfig(IP2368_INT_IOG,IP2368_INT_IOP,GPIO_DIR_OUT);//����IDR������Ϊ���
	  GPIO_SetOutBits(IP2368_INT_IOG,IP2368_INT_IOP); //��INT������1,ʹIP2368��Զ���Ѳ���˯��	
		if(wait>0)delay_ms(200); //ǿ������INT֮��IP2368��200mS����ܷ��ʼĴ���
    }
	}	
	
//Typec����ʱ���г�ʼ���Ĳ���
void TypeCInsertInitHandler(void)
  {
	
	bool IsTypeCSrc=false,IsReset=false;
	static bool IsSrcDetachReInit=false; //��SRC���ؽ���ʱ������re-init
	QuickChargeCtrlDef QCtrl;
	//��IP2368��ȡ��ǰOTP��Type-C��״̬
	if(!IP2368_CheckIfOTPLoaded(&IsReset)||!IP2368_GetIsTypeCSrcConnected(&IsTypeCSrc))
	  {
		IP2368StallRestore();
		return;  //��ʼ�ۼ�ʱ�䣬���ʱ�䵽����лָ�����
		}  
	else IPStallTime=0; //IP2368�������У���ʱ����λ
	//����Ĵ������������û�з����䶯���򷵻�
	if(IsSrcDetachReInit!=IsTypeCSrc&&!IsReset)return; 
	IsSrcDetachReInit=IsTypeCSrc; //ͬ��״̬
	if(IsSrcDetachReInit)return; //Type-C sourceģʽ�µ���δ���뵽����ʱ������Ӧ��ʼ������
	//׼�����״̬��������
	QCtrl.IsEnableDPDM=Config.IsEnableDPDM;
	QCtrl.IsEnablePD=Config.IsEnablePD;
	QCtrl.IsEnableSCP=Config.IsEnableSCP;
	QCtrl.IsEnable9VPDO=Config.IsEnable9VPDO;
	QCtrl.IsEnable20VPDO=Config.IsEnable20VPDO;		
	//���³�ʼ��			
	IP2368_SetPreChargeEndVoltage(PreCharge_End2V9); //����Ԥ��������ѹΪ2.9V
	if(!IsTypeCSrc)
	  {		
		IP2368_SetChargePower(ConvertChagePower()); //���ó�繦��	
		IP2368_SetChargeParam(100,ReCharge_4V10); //���ó������������ٳ���ѹ
		}
	IP2368_SetQuickchargeState(&QCtrl); //���ÿ������
	IP2368_SetLowVoltAlert(LVAlert_2V8);	 //���õ�ѹ�澯
	IP2368_SetDischarge(Config.IsEnableOutput); //�����������
	}	
	
//����״̬�ж�
void PowermanagementSleepControl(void)
  {
	//��ǰδ����˯��״̬����ִ��
	if(SleepTimer!=-1)return;
	//��ʼ���
	GPIO_InputConfig(IP2368_INT_IOG,IP2368_INT_IOP,ENABLE); 
	GPIO_DirectionConfig(IP2368_INT_IOG,IP2368_INT_IOP,GPIO_DIR_IN);//����Ϊ��������ʹ2368����
	delay_ms(40);
  if(GPIO_ReadInBit(IP2368_INT_IOG,IP2368_INT_IOP)==SET)return; //IP2368��Ȼ���������У��˳�	
	OLED_Clear();
	OLED_Refresh(); //�����Ļ�ڵ���������
	GPIO_ClearOutBits(LDO_EN_IOG,LDO_EN_IOP);//�������Ϊ0,�ر�LDO��Դǿ�ȵ�Ƭ������
	while(1);		
	}
