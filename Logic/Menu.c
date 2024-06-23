#include "IP2368.h"
#include "oled.h"
#include <math.h>
#include "string.h"
#include "SideKey.h"
#include "delay.h"
#include "Config.h"

//�汾��Ϣ
#define FWMajorVer 2
#define FWMinorVer 3

//�̼���������
#define EfficiencySampleCount 30

typedef enum
  {
	Menu_DCDC,
	Menu_TypeC,
	Menu_Batt,
	Menu_Error,
	Menu_Fault,
	Menu_TypeCMode,
  Menu_ChargePower,
	Menu_QuickChargeSet,
  Menu_WaitReconnect,
	Menu_Settings,
	Menu_Brightness,
	Menu_About,
	Menu_PSOC_Upgrade,
	}MenuStateDef;
	
//ȫ�ֱ���
extern bool EnableXARIIMode; //�ڲ�flagλ���Ƿ�����Э��汾V1.63������Ĵ���
MenuStateDef MenuState=Menu_TypeC; //�˵�״̬
MenuStateDef LastMenuBeforeSleep=Menu_TypeC; //ȫ�ֱ��������ڴ洢�������Զ��л�֮ǰ��Ŀ��˵�
int SleepTimer=OLEDSleepTimeOut*8;  //˯�߶�ʱ��
static int MenuSwitchtimer=0; //�˵��л���ʱ��
static int QCSetMenu=0; //Ĭ�����õĿ����Ŀ
int ConnectTimeoutCNT=-1; //����ʧ�ܶ�ʱ��
void ForceShutOff(void); //�ⲿ�ػ�����
static volatile bool IsSystemWakedup=true; //��־λ��ϵͳ�Ƿ��Ѿ��˳�˯��״̬	

//Ч���˲�����
static float EfficiencyFilter(float IN)
  {
	static volatile float sumbuf=0,min=2000,max=-2000;
	static volatile int sumcount=0;
	static volatile float result=0;
	//������������
	if(sumcount>EfficiencySampleCount)
	  {
		min=2000;
		max=-2000;
		sumcount=0;
		result=0;
		sumbuf=0; //��λ������
		}
	//���������ѵ�
	else if(sumcount==EfficiencySampleCount)
	  {
		sumbuf-=(min+max); //ȥ��һ�����+���			
		result=sumbuf/=(float)(sumcount-2); //����ʣ�µĽ��
		sumbuf=0;
		sumcount=0;
		min=2000;
		max=-2000;	 //��λ������
		}
  //��������δ������ʼ�ۼ�	
	else if(IN>10&&IN<98)
	  {
		if(IN>max)max=IN;
		if(IN<min)min=IN; //�����Сֵ����
		sumbuf+=IN; //�����������ֵ
		sumcount++; //�ۼӴ���+1
		}
	//������������ؽ��
	return result;
	}
	
//�ָ������������
void ResetConfigDetection(void)
  {
	int i;
	if(GPIO_ReadInBit(ExtKey_IOG,ExtKey_IOP))return; //���������ɿ�״̬���˳�
  for(i=0;i<58;i++)
		{
		OLED_Clear();
		OLED_Printf(16,4,127,1,"RESET");
		OLED_Printf(8,11,127,1,"DEFAULT?");	
		OLED_DrawRectangle(1,22,62,30,1);
		OLED_Fill(3,24,i+1,5,1);  //������
		if(GPIO_ReadInBit(ExtKey_IOG,ExtKey_IOP))
		  {
		  OLED_Clear();
		  OLED_Refresh(); //ˢ��
		  return; //���������ɿ�״̬���˳�
			}
		OLED_Refresh(); //ˢ��
		delay_ms(30); //��ʱ30����
		}
	//�û�һֱ��ס����1�룬��ʼ��λ
	OLED_Clear();
	if(RestoreDefaultCfg())OLED_Printf(1,1,127,1,"Default settings has been loaded.");
	else OLED_Printf(1,1,127,1,"Failed to load Default.");
	OLED_Refresh(); //ˢ��
  while(!GPIO_ReadInBit(ExtKey_IOG,ExtKey_IOP))delay_ms(1); //�ȴ��û��ɿ�����
	OLED_OldTVFade(); //������ʧ
	}	
	
//��򵥵���һ����������
void DisplaySplash(void)
  {
	int i;
	//��һ��PROJECT
	for(i=1;i<9;i++)
	  {
		OLED_Clear();
		OLED_Printfn(11,6,127,i,1,"PROJECT\x7e");	
		OLED_Refresh(); //ˢ��
		delay_ms(20); //�ӳ�150mS
		}
	//�ڶ���DIFFTORCH
	for(i=1;i<10;i++)
	  {
		OLED_Clear();
		OLED_Printf(11,6,127,1,"PROJECT\x7e");	
		OLED_Printfn(6,13,127,i,1,"DIFFTORCH");	
		OLED_Refresh(); //ˢ��
		delay_ms(20); //�ӳ�150mS
		}	 
	//������EXTREME
	for(i=1;i<8;i++)
	  {
		OLED_Clear();
		OLED_Printf(11,6,127,1,"PROJECT\x7e");	
		OLED_Printf(6,13,127,1,"DIFFTORCH");	
		OLED_Printfn(11,20,127,i,1,"EXTREME");	
		OLED_Refresh(); //ˢ��
		delay_ms(20); //�ӳ�150mS
		}	
  delay_ms(500);		
	OLED_OldTVFade();
	//��ʾ���ߺ͹̼��汾
	for(i=1;i<12;i++)
	  {
		OLED_Clear();
		if(Config.BatteryCount==0)OLED_Printfn(1,1,127,i,1,"D8B-?SPD3.0");
		else OLED_Printfn(1,1,127,i,1,"D8B-%dSPD3.0",Config.BatteryCount);	
		OLED_Refresh(); //ˢ��
		delay_ms(30); //�ӳ�150mS
		}
	for(i=1;i<30;i++)
	  {
		OLED_Clear();
		if(Config.BatteryCount==0)OLED_Printf(1,1,127,1,"D8B-?SPD3.0");
		else OLED_Printf(1,1,127,1,"D8B-%dSPD3.0",Config.BatteryCount);
		OLED_Printfn(1,9,127,i,1,"FW %d.%d BY:REDSTONER35",FWMajorVer,FWMinorVer);	
		OLED_Refresh(); //ˢ��
		delay_ms(30); //�ӳ�150mS
		}	
	//�ӳ�500mS������ʧ
	delay_ms(300);
	OLED_ImageDisappear();
	}	
		
//˯�߹����Լ���Ļ�ֻ�������
void SleepTimerHandler(void)
  {
	//����ʧ�ܶ�ʱ��
	if(ConnectTimeoutCNT>-1)ConnectTimeoutCNT=ConnectTimeoutCNT<250?ConnectTimeoutCNT+1:250;
	//��ʱ���ۼ�
	if(SleepTimer>(OLEDSleepTimeOut*3))
	   { 
		 SleepTimer--;
     MenuSwitchtimer=0; //�û����������У�ֹͣ�ֻ�
		 LastMenuBeforeSleep=MenuState; //�洢˯��֮ǰ��Ŀ��˵�
		 }
	else if(SleepTimer>0)
	   {
	   SleepTimer--; //��ʱ���ۼ�
	   if(MenuSwitchtimer==16) //ÿ��һ��ʱ���ֻ������ֹ����
		   {
			 MenuSwitchtimer=0;
			 if(MenuState==Menu_TypeCMode||MenuState==Menu_Error)return; //������ڴ����������ģʽ�׶��򲻽����ֻ�
			 else if(MenuState==Menu_DCDC)MenuState=Menu_TypeC;
			 else if(MenuState==Menu_TypeC)MenuState=Menu_Batt;
			 else if(MenuState==Menu_Batt)MenuState=Menu_DCDC;//ʵ�ֲ˵��ֻ�
			 }
		 else MenuSwitchtimer++;
     } 		
	}
//��ÿ�ΰ�������֮��ָ�˯��֮ǰ���ڵĲ˵�
void RestoreMenuStateBeforeSleep(void)
  {
	if(SleepTimer>(OLEDSleepTimeOut*3))return; //˯��֮ǰ�˳�
	if(LastMenuBeforeSleep==Menu_Error||LastMenuBeforeSleep==Menu_WaitReconnect)
		MenuState=Menu_TypeC; //���˯��֮ǰ�Ǵ���˵���ָ���Ĭ��ֵ
  else 
		MenuState=LastMenuBeforeSleep;
	}

//�˵���������������
void MenuKeyHandler(void)
  {
	int Click;
	int power;
  bool SaveFailed;
  bool IsTypeCOK=false;
	QuickChargeCtrlDef QCtrl;
	//��ȡ�̰��������жϵ�ǰϵͳ�Ƿ�˯��,Ȼ���ȡtype-C��״̬	
	Click=getSideKeyShortPressCount(true);
	if(!IsSystemWakedup)return; //˯��״̬�²�����������
	IP2368_GetTypeCConnectedState(&IsTypeCOK);
	//״̬��
	switch(MenuState)
	  {
		//Ӣ��оSoC����
		case Menu_PSOC_Upgrade:
					break;
		//����˵�
		case Menu_WaitReconnect:
		case Menu_Error:
			    if(!IsTypeCOK&&Click==3){OLED_OldTVFade();ForceShutOff();} //��������ǿ�ƹػ�
		      break;
		//���ڲ˵�
		case Menu_About:
			  if(Click>0||getSideKeyLongPressEvent())
				   {
				   while(getSideKeyHoldEvent())SideKey_LogicHandler(); //�ȴ������ſ�
				   MenuState=Menu_Settings; //�ص������Ӳ˵�
			     }
		    break;
		//�������ò˵�
		case Menu_Brightness:
			  if(Click==1)switch(Config.Brightness) //��������
				  {
					case Screen_MaxBright:Config.Brightness=Screen_LowBright;break;
					case Screen_HighBright:Config.Brightness=Screen_MaxBright;break;
					case Screen_MidBright:Config.Brightness=Screen_HighBright;break;
					case Screen_LowBright:Config.Brightness=Screen_MidBright;break;				
					}
		     else if(getSideKeyLongPressEvent()) //���������˳�
				  {
					QCSetMenu=0; //����index
					MenuState=Menu_Settings; //�ص������Ӳ˵�
				  //��ʾ���ڱ���
					OLED_Clear();
					OLED_Printf(9,15,127,1,"SAVING...");
					OLED_Refresh(); 
					OLED_SetBrightness(ConvertBrightLevel());	//�������ļ����Ŀ������ȵȼ�������OLED
					while(getSideKeyHoldEvent())SideKey_LogicHandler(); //�ȴ������ſ�
					if(SavingConfig())break;
					OLED_Clear();
					OLED_Printf(7,15,127,1,"SAVE ERROR");
					OLED_Refresh();
          delay_Second(1); //��ʾ����ʧ��
					}
				 break;	
		//���Э�����ò˵�
		case Menu_QuickChargeSet: 
		    if(Config.BatteryCount<3)Config.IsEnable20VPDO=false; //��ؽ���С��3�ڣ���ֹʹ��20V PDO
		    if(!EnableXARIIMode&&Config.IsEnableSCP)Config.IsEnable9VPDO=false; //�ɰ汾�̼��������SCP��ǿ�ƽ���9V PDO
			  if(Click==1)QCSetMenu=QCSetMenu<(Config.BatteryCount>2?4:3)?QCSetMenu+1:0; //����ѡ��Э��
		    else if(Click==2)
				  {
					switch(QCSetMenu) //��������
						{
						case 0:Config.IsEnableDPDM=Config.IsEnableDPDM?false:true;break; //����DPDM
						case 1:Config.IsEnablePD=Config.IsEnablePD?false:true;break; //����PD
						case 2:Config.IsEnableSCP=Config.IsEnableSCP?false:true;break; //����SCP
						case 3:Config.IsEnable9VPDO=Config.IsEnable9VPDO?false:true;break; //����9V PDO�Ƿ�����
						case 4:Config.IsEnable20VPDO=Config.IsEnable20VPDO?false:true;break; //����20V PDO�Ƿ�����
						}
					//����Э��
					QCtrl.IsEnableDPDM=Config.IsEnableDPDM;
	        QCtrl.IsEnablePD=Config.IsEnablePD;
	        QCtrl.IsEnableSCP=Config.IsEnableSCP;
					QCtrl.IsEnable9VPDO=(!EnableXARIIMode&&Config.IsEnableSCP)?false:Config.IsEnable9VPDO;
					QCtrl.IsEnable20VPDO=Config.BatteryCount<3?false:Config.IsEnable20VPDO;
	        if(!IP2368_SetQuickchargeState(&QCtrl))MenuState=Menu_Error; //���Ա���Э��
					}
				else if(getSideKeyLongPressEvent()) //���������˳�
				  {
					QCSetMenu=0; //����index
					MenuState=Menu_Settings; //�ص������Ӳ˵�
				  //��ʾ���ڱ���
					OLED_Clear();
					OLED_Printf(9,15,127,1,"SAVING...");
					OLED_Refresh(); 
					while(getSideKeyHoldEvent())SideKey_LogicHandler(); //�ȴ������ſ�
					if(SavingConfig())break;
					OLED_Clear();
					OLED_Printf(7,15,127,1,"SAVE ERROR");
					OLED_Refresh();
          delay_Second(1); //��ʾ����ʧ��
					}
		    break;
		//��繦�����ò˵�
	  case Menu_ChargePower:	
	      if(Config.BatteryCount<3)		
				  {
					if(Config.ChargePower==ChargePower_60W)Config.ChargePower=ChargePower_45W;
					if(Config.ChargePower==ChargePower_65W)Config.ChargePower=ChargePower_45W; //���������ƣ������ؽ���С���������ֹ��45W���ϵĵ�λ
					}
		    if(Click==1)switch(Config.ChargePower) //����ѡ���繦��
				  {
					case ChargePower_65W:Config.ChargePower=ChargePower_30W;break;
					case ChargePower_60W:Config.ChargePower=ChargePower_65W;break;
					case ChargePower_45W:Config.ChargePower=Config.BatteryCount<3?ChargePower_30W:ChargePower_60W;break; //������ֻ��2S�����ֹ��45W���ϵĵ�λ
					case ChargePower_30W:Config.ChargePower=ChargePower_45W;break;
					}
				else if(Click==2) //˫�������沢�˳�
				  {
					OLED_Clear();
					OLED_Printf(4,15,127,1,"NOT SAVED");
					OLED_Refresh(); 
					MenuState=Menu_Settings; //�ص������Ӳ˵�
					if(!IP2368_GetChargePower(&power))MenuState=Menu_Error; 
          else if(power>60&&Config.BatteryCount>2)Config.ChargePower=ChargePower_65W;
          else if(power>45&&Config.BatteryCount>2)Config.ChargePower=ChargePower_60W; //���е�ش���2�ڲ�������60W����
          else if(power>30)Config.ChargePower=ChargePower_45W;
          else Config.ChargePower=ChargePower_30W; //���ݵ�ǰ��繦�ʼĴ������ó�繦��			
					delay_Second(1);	
					}
		    else if(getSideKeyLongPressEvent())//�������沢�˳�
				  {
					MenuState=Menu_Settings; //�ص������Ӳ˵�
				  //��ʾ���ڱ���
					OLED_Clear();
					OLED_Printf(9,15,127,1,"SAVING...");
					OLED_Refresh(); 
					while(getSideKeyHoldEvent())SideKey_LogicHandler(); //�ȴ������ſ�
					//д�Ĵ������ó�繦��
					if(!IP2368_SetChargePower(ConvertChagePower()))SaveFailed=true;
          else if(!SavingConfig())SaveFailed=true;
          else SaveFailed=false; //�ɹ�����		
          //����ɹ����˳����ʧ������ʾ
					if(!SaveFailed)break;
					OLED_Clear();
					OLED_Printf(7,15,127,1,"SAVE ERROR");
					OLED_Refresh();
          delay_Second(1);				
          if(!IP2368_GetChargePower(&power))MenuState=Menu_Error; 
					else if(power>60&&Config.BatteryCount>2)Config.ChargePower=ChargePower_65W;
          else if(power>45&&Config.BatteryCount>2)Config.ChargePower=ChargePower_60W; //���е�ش���2�ڲ�������60W����
          else if(power>30)Config.ChargePower=ChargePower_45W;
          else Config.ChargePower=ChargePower_30W; //���ݵ�ǰ��繦�ʼĴ������ó�繦��			
					}
		    break;
		//TypeCģʽ�˵�
		case Menu_TypeCMode:
			  if(Click==1)Config.IsEnableOutput=Config.IsEnableOutput?false:true; //�л�ģʽ	
		    else if(Click==2) //�������˳�
					  {
						MenuState=Menu_Settings; //�ص������Ӳ˵�
						OLED_Clear();
					  OLED_Printf(4,15,127,1,"NOT SAVED");
					  OLED_Refresh(); 
						delay_Second(1);	
					  //��ȡ�Ĵ���״̬������ǰDRP����ͬ��ΪоƬ����ֵ
						if(!IP2368_GetDischargeState(&Config.IsEnableOutput))
						   {
						   MenuState=Menu_Error; 
							 return; //���ִ���ָʾIP2368����
							 }
						}
		     else if(getSideKeyLongPressEvent()) //˫����������߳����˳�
						{		
            MenuState=Menu_Settings; //�ص������Ӳ˵�				
						//��ʾ���ڱ���
						OLED_Clear();
						OLED_Printf(9,15,127,1,"SAVING...");
						OLED_Refresh(); 
						while(getSideKeyHoldEvent())SideKey_LogicHandler(); //�ȴ������ſ�
						//д�Ĵ������õ�ǰDRP����
						if(!IP2368_SetDischarge(Config.IsEnableOutput))SaveFailed=true;
            else if(!SavingConfig())SaveFailed=true;
            else SaveFailed=false; //�ɹ�����							
						//����ɹ����˳����ʧ������ʾ
						if(!SaveFailed)break;
						OLED_Clear();
						OLED_Printf(7,15,127,1,"SAVE ERROR");
						OLED_Refresh();
            delay_Second(1);						
            if(!IP2368_GetDischargeState(&Config.IsEnableOutput))MenuState=Menu_Error; 	//����ǰDRP����ͬ��ΪоƬ����ֵ						
						}	
		      break;
		//����flag�˵�
		case Menu_Fault:
			  if(!IsTypeCOK&&Click==3){OLED_OldTVFade();ForceShutOff();} //��������ǿ�ƹػ�
			  else if(Click==1)MenuState=Menu_TypeC; //�����л���TypeC�˵�
				else if(Click==2) //�������˫���������ò˵�
				  {
				  QCSetMenu=0; //����index
					MenuState=Menu_Settings;
			    }
		    break;
		//DCDC�˵�
	  case Menu_DCDC:
			  if(!IsTypeCOK&&Click==3){OLED_OldTVFade();ForceShutOff();} //��������ǿ�ƹػ�
			  else if(Click==2) //�������˫���������ò˵�
				  {
				  QCSetMenu=0; //����index
					MenuState=Menu_Settings;
			    }
			  else if(Click==1)MenuState=Menu_Fault; //�����л�������flag�˵�
		    break;
		//��ز˵�
		case Menu_Batt:
			  if(!IsTypeCOK&&Click==3){OLED_OldTVFade();ForceShutOff();} //��������ǿ�ƹػ�
			  else if(Click==2) //�������˫���������ò˵�
				  {
				  QCSetMenu=0; //����index
					MenuState=Menu_Settings;
			    }
			  else if(Click==1)MenuState=Menu_DCDC; //�����л���DCDC�˵�
		    break;
	  //Type-C�˵�
		case Menu_TypeC:
		    if(!IsTypeCOK&&Click==3){OLED_OldTVFade();ForceShutOff();} //��������ǿ�ƹػ�
			  else if(Click==2) //�������˫���������ò˵�
				  {
				  QCSetMenu=0; //����index
					MenuState=Menu_Settings;
			    }
		    else if(Click==1)MenuState=Menu_Batt; //�����л���DCDC�˵�
		    break;
		//���ò˵�
		case Menu_Settings:
			 if(Click==1)QCSetMenu=QCSetMenu<6?QCSetMenu+1:0; //����ѡ��Э��
		   else if(Click==2)
			   {
			   switch(QCSetMenu)//˫�����뵽��Ӧ�����ò˵�
						{
						case 0:MenuState=Menu_QuickChargeSet;break;
						case 1:MenuState=Menu_TypeCMode;break;
						case 2:MenuState=Menu_ChargePower;break;
					  case 3:MenuState=Menu_Brightness;break;
					  case 4:MenuState=Menu_About;break;
					  case 5:MenuState=Menu_PSOC_Upgrade;break;
					  case 6:IP2368_DoRSOCCalibration();break;
						}
				 QCSetMenu=0; //��λindex
				 }
			 else if(getSideKeyLongPressEvent()) //�����˳�
				  {
					QCSetMenu=0; //��λindex
				  while(getSideKeyHoldEvent())SideKey_LogicHandler(); //�ȴ������ſ�
				  MenuState=Menu_DCDC; //�ص���ʼ�˵�
			    }
			 break;
		}
	}
//�жϺ������жϲ˵��Ƿ�λ�ڵ�ز˵�
bool IsMenuAtBatteryTelem(void)
  {
	if(MenuState==Menu_Batt)return true;
	//�������������false
	return false;
	}
		
//�˵���Ⱦ����
static void MenuRenderHandler(void)
  {
	BatteryStatuDef BattState;
	TypeCStatusDef TypeCState;
	FaultFlagStrDef Fault;
	QuickChargeCtrlDef Qstate;
	static MenuStateDef LastMenu=Menu_TypeC; //����֮ǰ���ϸ��˵�
  const char *BattInfo;
	bool State,IsNTCOK,IsDisEN,VBUSMOSEN,IsTypeCOK,IsTypeCSrcOK;
	int ChargePower,y;
	float TempResult,HCCurrent,Efficiency;
  //��ѯIP2368�Ĵ�����ȡ״̬
	switch(MenuState)	
	  {
		//��书�����ò˵�
		case Menu_QuickChargeSet:
		   State=IP2368_GetQuickchargeState(&Qstate);	//��ȡ���״̬
		   break;
		//����flag�Ӳ˵�
		case Menu_Fault:
			 State=IP2368_GetAlertFlag(&Fault); //��ȡ����״̬
		   break;
	  //DCDC�Ӳ˵�		
		case Menu_DCDC:
		  State=IP2368_GetDischargeState(&IsDisEN); //�Ƿ�ʹ�ܷŵ�
		  State&=IP2368_GetIsTypeCSrcConnected(&IsTypeCSrcOK); //��ȡType-C�Ƿ���SRC��ʽ����
		  State&=IP2368_GetTypeCState(&TypeCState);
		  State&=IP2368_GetBatteryState(&BattState); //��ȡ��غ�Type-C״̬�����ڼ����ŵ�Ч�ʣ�
	    State&=IP2368_GetChargePower(&ChargePower); //��繦��
			State&=IP2368_GetNTCTemp(&TempResult,&IsNTCOK); //�¶�
		  State&=IP2368_GetVBUSSwitchState(&VBUSMOSEN); //MOSʹ��״̬
		  break;
		//����Ӳ˵�
	  case Menu_Batt:
			State=IP2368_GetBatteryState(&BattState); //��ȡ���״̬
		  break;
		//Type-C״̬
		case Menu_TypeC:
			State=IP2368_GetTypeCConnectedState(&IsTypeCOK);
		  State&=IP2368_GetTypeCState(&TypeCState); //��ȡType-C״̬�ļĴ����Լ�Type-C�Ƿ�����
		  break;
	  //����˵���������ȡtype-C����״̬�����ж�
		case Menu_Error:
		case Menu_WaitReconnect:
			State=IP2368_GetTypeCConnectedState(&IsTypeCOK);
		  break;
		case Menu_Settings:
			State=IP2368_GetBatteryState(&BattState); //��ȡ���״̬�����ڿ����Ƿ��������༭���ģʽ��
		  break;
		//�����ڲ����ò���Ҫ��ȡ�Ĵ����ı������ò˵�,State=1��ʾ�޹��ϡ�
		default:State=true;
		}
	if(!State) //IP2368�Ͽ�����
	  {
		SleepTimer=250; //���������쳣����ֹ˯��
		if(ConnectTimeoutCNT==-1) //��ʱ����һ������
		   {
			 LastMenu=MenuState; //�洢CONNECT��ʾ֮ǰ�Ĳ˵�
			 ConnectTimeoutCNT=0; //����ʱ���ӹػ�״̬����Ϊ����	
			 }
		//��ʱ��ʱ��δ�������˵�����Ϊ�ȴ�����
		else if(ConnectTimeoutCNT<250)
			 MenuState=Menu_WaitReconnect;
	  //��ʱ��ʱ�䵽����ʾ����
		else 
			 MenuState=Menu_Error; 
		}
  else //IP2368�ظ�����
	  {
    ConnectTimeoutCNT=-1;//��λ��ʱ��
	  if(MenuState==Menu_Error||MenuState==Menu_WaitReconnect)MenuState=LastMenu; //ָʾIP2368�ѻָ�����,�ص�ԭ���Ĳ˵�
		}
	//��ʼ��Ⱦ
	OLED_Clear();
	switch(MenuState)
	  {
		//PSOC����	
	  case Menu_PSOC_Upgrade:
			   if(!IP2368_SendResetCommand())
				   {
					 OLED_Printf(1,1,127,1,"Failed to reset PSoC.");
					 OLED_Refresh();
					 delay_Second(2);
					 MenuState=Menu_Settings; //�ص������Ӳ˵�
					 }
		     else while(1) //������ѭ��
				   {
					 OLED_Printf(1,1,127,1,"Entered Upgrade mode.");
					 OLED_Refresh();
					 }
         break;
		//�̼���Ϣ	
		case Menu_About:
			OLED_Printf(0,0,127,1,"V%d.%d DATE:%s %s",FWMajorVer,FWMinorVer,__DATE__,__TIME__);	 
		  OLED_Printf(0,18,127,1,"PSV:1.%s",EnableXARIIMode?"63":"2");
		  OLED_Printf(0,24,127,1,"D8B-%dSPD3.0",Config.BatteryCount);
		  break;
		//���ò˵�
		case Menu_Settings:
		   OLED_Printf(8,1,127,1,"SETTINGS");
		   if(QCSetMenu<3) //ǰ����ѡ��
			   {
				 //���ƿ������
				 y=QCSetMenu==0?10:9;
		     OLED_Printf(7,y,127,1,"Q-Chrg.");
		     if(QCSetMenu==0)OLED_DrawRectangle(5,8,56,16,1);
		     //����Type-Cģʽ����
		     y=QCSetMenu==1?17:18;
		     if(QCSetMenu==2)y-=2; //ѡ������������ݣ���Ҫ���ַ�����2������ͻ
		     OLED_Printf(7,y,127,1,"Type-C");
		     if(QCSetMenu==1)OLED_DrawRectangle(5,15,56,23,1);
		     //���Ƴ�繦������
			   if(BattState.BattState!=Batt_discharging)
			     {
				   y=QCSetMenu==2?24:25;
		       OLED_Printf(7,y,127,1,"Chrg PWR");
		       if(QCSetMenu==2)OLED_DrawRectangle(5,22,56,30,1);	
				   }
			   else if(QCSetMenu==2)QCSetMenu=3;//ʹѡ��ܱ�ѡ�У�ֱ������������
				 }
			 else if(QCSetMenu<6)//���ĸ���������
			   {
				 //������Ļ��������
				 y=QCSetMenu==3?10:9;
		     OLED_Printf(7,y,127,1,"Screen");
		     if(QCSetMenu==3)OLED_DrawRectangle(5,8,56,16,1);
					//�����˵�
		     y=QCSetMenu==4?17:18;
		     if(QCSetMenu==5)y-=2; //ѡ������������ݣ���Ҫ���ַ�����2������ͻ
		     OLED_Printf(7,y,127,1,"ABOUT");
		     if(QCSetMenu==4)OLED_DrawRectangle(5,15,56,23,1);					 
				 //�����˵�
				 y=QCSetMenu==5?24:25;
		     OLED_Printf(7,y,127,1,"PSOCUPD");
		     if(QCSetMenu==5)OLED_DrawRectangle(5,22,56,30,1);
				 }
			 else //���������ھŸ�
			   {
				 //ʣ������У׼
				 y=QCSetMenu==6?10:9;
		     OLED_Printf(7,y,127,1,"RSOC Cal.");
		     if(QCSetMenu==6)OLED_DrawRectangle(5,8,56,16,1);
				 }
		   break;
		//��书�����ò˵�
		case Menu_QuickChargeSet:			
			 OLED_Printf(7,1,127,1,"Q. CHARGE");
		   OLED_DrawLine(1,3,4,3,1);
		   OLED_DrawLine(58,3,62,3,1);
			 if(QCSetMenu<3)		//��һ����������
		      {
		      //����DPDM���״̬
					y=QCSetMenu==0?10:9;
					OLED_Printf(7,y,127,1,"DPDM");
					OLED_Printf(38,y,127,1,Qstate.IsEnableDPDM?"ON":"OFF");
					if(QCSetMenu==0)OLED_DrawRectangle(5,8,56,16,1);
					//����PD���״̬
					y=QCSetMenu==1?17:18;
					if(QCSetMenu==2)y-=2; //ѡ������������ݣ���Ҫ���ַ�����2������ͻ
					OLED_Printf(7,y,127,1,"PD");
					OLED_Printf(38,y,127,1,Qstate.IsEnablePD?"ON":"OFF");
					if(QCSetMenu==1)OLED_DrawRectangle(5,15,56,23,1);
		   		//����SCP ���״̬
					y=QCSetMenu==2?24:25;
					OLED_Printf(7,y,127,1,"SCP");
					OLED_Printf(38,y,127,1,Qstate.IsEnableSCP?"ON":"OFF");
					if(QCSetMenu==2)OLED_DrawRectangle(5,22,56,30,1);
         	}
			else //���ĸ���������
					{
					if(EnableXARIIMode||(!Config.IsEnableSCP&&!EnableXARIIMode))
					  {
						//����PD9V ���״̬
						y=QCSetMenu==3?10:9;
						OLED_Printf(7,y,127,1,"9VPDO");
						OLED_Printf(43,y,127,1,Qstate.IsEnable9VPDO?"ON":"OFF");
						if(QCSetMenu==3)OLED_DrawRectangle(5,8,61,16,1);
						}
					else if(QCSetMenu==3)QCSetMenu=4; //ʹѡ���ѡ��
				  //����20V PDO״̬
					if(Config.BatteryCount>2) //�����2���������������20V PDO
					  {
						y=QCSetMenu==4?17:18;
						if(QCSetMenu==5)y-=2; //ѡ������������ݣ���Ҫ���ַ�����2������ͻ
						OLED_Printf(7,y,127,1,"20VPD");
						OLED_Printf(43,y,127,1,Qstate.IsEnable20VPDO?"ON":"OFF");
						if(QCSetMenu==4)OLED_DrawRectangle(5,15,61,23,1);
						}
					else if(QCSetMenu==4)QCSetMenu=1; //ʹѡ���ѡ��
					}
		   break;	
		//�ȴ����Ӳ˵�
		case Menu_WaitReconnect:
       OLED_Printf(2,9,127,1,"CONNECTING TO PSOC...");
       break;			
		//����˵�	
		case Menu_Error:
       OLED_Printf(3,9,127,1,"PSOC I2C Offline!");
       break;
		//��Ļ�������ò˵�
	  case Menu_Brightness:
	     OLED_Printf(2,1,127,1,"BRIGHTNESS");
		   OLED_Printf(9,11,127,1,"LOW");
		   OLED_Printf(9,22,127,1,"MID");
		   OLED_Printf(38,11,127,1,"HIGH");
		   OLED_Printf(38,22,127,1,"MAX");
		   switch(Config.Brightness) //��ʾ��ǰѡ��ĳ�繦��
			   {
				 case Screen_LowBright:OLED_DrawRectangle(7,9,27,17,1);break;
				 case Screen_MidBright:OLED_DrawRectangle(7,20,27,28,1);break;
				 case Screen_HighBright:OLED_DrawRectangle(36,9,62,17,1);break;
				 case Screen_MaxBright:OLED_DrawRectangle(36,20,56,28,1);break; 
				 }		 
		   break;
		//��繦�����ò˵�
		case Menu_ChargePower:
			 if(BattState.BattState==Batt_discharging)
			   {
				 MenuState=Menu_Settings;
				 return; //��ؽ���ŵ�ģʽ����ʱ��ֹ���ó�繦�ʵĲ˵���
				 }
			 OLED_Printf(2,1,127,1,"CHARGE PWR.");
		   OLED_Printf(9,11,127,1,"30W");
		   OLED_Printf(9,22,127,1,"45W");
			 if(Config.BatteryCount>2) //�����ؽ�������2�ڲ���ʾ45W���ϵĵ�λ
			   {				 
				 OLED_Printf(38,11,127,1,"60W");
				 OLED_Printf(38,22,127,1,"66W");
				 }
		   switch(Config.ChargePower) //��ʾ��ǰѡ��ĳ�繦��
			   {
				 case ChargePower_30W:OLED_DrawRectangle(7,9,27,17,1);break;
				 case ChargePower_45W:OLED_DrawRectangle(7,20,27,28,1);break;
				 case ChargePower_60W:if(Config.BatteryCount>2)OLED_DrawRectangle(36,9,56,17,1);break;
				 case ChargePower_65W:if(Config.BatteryCount>2)OLED_DrawRectangle(36,20,56,28,1);break; 
				 }
			 break;
		//Type-Cģʽ�˵�
		case Menu_TypeCMode:
			 OLED_Printf(0,2,127,1,"TYPEC MODE");
	     OLED_Printf(6,11,127,1,"DRP-BIDIR");
       OLED_Printf(6,22,127,1,"DFP-INPUT");		
		   if(Config.IsEnableOutput)OLED_DrawRectangle(4,9,59,17,1); //ָʾѡ��DRP
		   else OLED_DrawRectangle(4,20,59,28,1); //ָʾѡ��DFP
		   break;
		//����flag�Ӳ˵�
		case Menu_Fault:
	     OLED_Printf(2,1,127,1,"FAULT STAT");
	     if(!Fault.FaultAsserted&&!Fault.OTAlert)OLED_Printf(8,13,127,1,"NO FAULT");
		   else //��ʾ�澯��Ŀ
			   {
				 if(Fault.SCPFault)OLED_Printf(2,9,127,1,"SCPF");
				 if(Fault.OCPFault)OLED_Printf(2,16,127,1,"OCPF");
				 if(Fault.INOVFault)OLED_Printf(2,23,127,1,"INOV");
				 if(Fault.OTFault)OLED_Printf(26,9,127,1,"OTMPF");
				 if(Fault.UTFault)OLED_Printf(26,16,127,1,"UTMPF");
				 if(Fault.OTAlert)OLED_Printf(26,23,127,1,"OTALM");
				 }
		   break;
    //DCDC�Ӳ˵�		
		case Menu_DCDC:
			 OLED_Printf(6,1,127,1,"DCDC STAT");
		   if(IsNTCOK)//������ʾ�¶�
			    {
					if(TempResult>=100)OLED_Printf(4,9,127,1,"%d\x7C",iroundf(TempResult)); //100����ʾ
					else if(TempResult>=10)OLED_Printf(4,9,127,1,"%.1f\x7C",TempResult); //99.9����ʾ
					else if(TempResult>=0)OLED_Printf(4,9,127,1,"%.2f\x7C",TempResult); //9.99����ʾ
					else if(TempResult>=-10)OLED_Printf(4,9,127,1,"%.1f\x7C",TempResult); //-9.9����ʾ
					else OLED_Printf(4,9,127,1,"%d\x7C",iroundf(TempResult));//-99����ʾ
					}
			 else OLED_Printf(4,9,127,1,"-- \x7C",iroundf(TempResult)); //�¶�δ֪
			 //��ʾ����		
			 if(IsTypeCSrcOK)OLED_Printf(4,17,127,1,Config.IsEnableOutput?"P%sW":"OFF",Config.IsEnable20VPDO?"60":"45");		//Type-C sourceģʽ���֣�ָʾ����Ϊ�̶�45����60W		
			 else OLED_Printf(4,17,127,1,"P%dW",ChargePower); //��ʾ��ǰ��繦��
			 //����Ч��
			 if(BattState.BattState==Batt_discharging)IsTypeCOK=true;		
			 else if(BattState.BattState==Batt_CCCharge)IsTypeCOK=true;
       else if(BattState.BattState==Batt_CVCharge)IsTypeCOK=true;					
			 else if(BattState.BattState==Batt_PreChage)IsTypeCOK=true;		
			 else IsTypeCOK=false; //���ŵ�����в���ʾЧ�ʣ�ƽʱ����ʾ
			 BattState.BatteryVoltage+=0.11; //����ѹ�ӻ���
			 if(BattState.BattState==Batt_discharging)Efficiency=TypeCState.BusPower/(BattState.BatteryCurrent*BattState.BatteryVoltage); //�ŵ磬ʹ�������������õ�Ч��
			 else Efficiency=(BattState.BatteryCurrent*BattState.BatteryVoltage)/TypeCState.BusPower; //��磬ʹ�������������õ�Ч��
			 Efficiency*=100; //תΪ�ٷֱ�
			 Efficiency=IsTypeCOK?EfficiencyFilter(fabsf(Efficiency)):0; //��Ч�����ݼ��뵽�˲����� 	
			 if(!IsTypeCOK||Efficiency<=10||Efficiency>=98)OLED_Printf(4,25,127,1,"-- %%");
			 else OLED_Printf(4,25,127,1,"%d%%",iroundf(Efficiency));//���Ч���쳣����TypeCû��������ʾ��Ч������				
			 //Typec�ӿ�״̬
			 OLED_Printf(39,9,127,1,"%s",IsDisEN?"DRP":"DFP");
		   //�ŵ�MOSFET״̬
			 OLED_Printf(39,16,127,1,"MOS");
			 OLED_Printf(VBUSMOSEN?43:39,22,127,1,VBUSMOSEN?"ON":"OFF");
			 OLED_DrawLine(35,18,38,18,1);
			 OLED_DrawLine(57,18,59,18,1);
			 OLED_DrawLine(35,18,35,29,1);
			 OLED_DrawLine(59,18,59,29,1);
			 OLED_DrawLine(35,29,60,29,1); //��Χ�Ŀ�
			 break;
	  //TypeC�Ӳ˵�
	  case Menu_TypeC:
		   OLED_Printf(0,1,127,1,"TYPEC STAT");
		   if(!IsTypeCOK||TypeCState.busVoltage<4.5) //USB Type-Cδ���ӻ��������˵���û�е�ѹ����ʾnot connected
			   {
				 OLED_Printf(21,11,127,1,"NOT");
				 OLED_Printf(5,19,127,1,"CONNECTED");
				 break;
				 }
		   OLED_Printf(2,9,127,1,"%.2fV",TypeCState.busVoltage);
		   OLED_Printf(2,16,127,1,fabsf(TypeCState.BusCurrent)<10?"%.2fA":"%.1fA",TypeCState.BusCurrent);
		   OLED_Printf(2,23,127,1,TypeCState.BusPower<100?"%.2fW":"%.1fW",TypeCState.BusPower);
       switch(TypeCState.PDState)				 
			   {
				 case PD_5VMode:BattInfo="5V";break;
				 case PD_7VMode:BattInfo="7V";break;
				 case PD_9VMode:BattInfo="9V";break;
				 case PD_12VMode:BattInfo="12V";break;
				 case PD_15VMode:BattInfo="15V";break;
				 case PD_20VMode:BattInfo="20V";break;
				 }  //��ȡPD״̬
			 if(TypeCState.QuickChargeState==QuickCharge_PD) //PDģʽ
			   {
				 OLED_Printf(44,11,127,1,"PD");
				 OLED_DrawRectangle(41,9,57,17,1);
				 OLED_Printf(strlen(BattInfo)>2?41:44,21,127,1,"%s",BattInfo);
				 }				 
			 else if(TypeCState.QuickChargeState==QuickCharge_HV) //�����ѹģʽ
			   {
				 OLED_Printf(44,11,127,1,"HV");
				 OLED_DrawRectangle(41,9,57,17,1);
				 OLED_Printf(41,21,127,1,"---");				 
				 }
			 else if(TypeCState.QuickChargeState==QuickCharge_HC) //��ѹ�ߵ���ģʽ
			   {
				 OLED_Printf(44,11,127,1,"HC");
				 OLED_DrawRectangle(41,9,57,17,1);
				 HCCurrent=fabsf(TypeCState.BusCurrent); //�Ե���ȡ����ֵ���㹦��
				 if(HCCurrent>5)HCCurrent=7;
				 else if(HCCurrent>3)HCCurrent=4;
				 else if(HCCurrent>2)HCCurrent=3; 
				 else HCCurrent=2; //Э��ʶ�𲿷�
				 OLED_Printf(44,21,127,1,"%dA",iroundf(HCCurrent));	//��ʾ����			 
				 }
			 else //��׼���
			   {
				 OLED_Printf(41,12,127,1,"STD");
				 OLED_Printf(41,19,127,1,"CHG");
				 OLED_DrawRectangle(38,9,60,26,1);
				 }
		   break;
		//����Ӳ˵�
		case Menu_Batt:
		   OLED_Printf(5,1,127,1,"BATT. STAT");
		   OLED_Printf(3,9,127,1,"%.2fV",BattState.BatteryVoltage);
			 OLED_Printf(3,16,127,1,fabsf(BattState.BatteryCurrent)<10?"%.2fA":"%.1fA",BattState.BatteryCurrent);
		   OLED_Printf(3,23,127,1,"%.2fW",fabsf(BattState.BatteryCurrent*BattState.BatteryVoltage));
		   OLED_Printf(37,9,127,1,"%d%%",BattState.BatteryRSOC);
		   switch(BattState.BattState)
			   {
				 case Batt_StandBy:BattInfo="STB";break;
				 case Batt_CCCharge:BattInfo="CC.";break;
				 case Batt_ChgDone:BattInfo="FUL";break;
				 case Batt_ChgError:BattInfo="ERR";break;
				 case Batt_ChgWait:BattInfo="WFE";break;
				 case Batt_CVCharge:BattInfo="CV.";break;
				 case Batt_discharging:BattInfo="DIS";break;
				 case Batt_PreChage:BattInfo="PRE";break;
				 }
			 OLED_Printf(40,20,127,1,"%s",BattInfo);
		   OLED_DrawRectangle(37,18,59,26,1);
			 OLED_DrawLine(1,3,3,3,1);
			 OLED_DrawLine(60,3,62,3,1);	
       break;		 
		}
	}	
//�˵���ʼ������
void MenuInit(void)
  {
	int i;
	bool IsTypeCOK;
	char BrightNess,CurrentBrightness;
	if(!IP2368_GetTypeCConnectedState(&IsTypeCOK))MenuState=Menu_Error;
	if(!IsTypeCOK)MenuState=Menu_Batt;
	else MenuState=Menu_TypeC; //�ж�Type-C����״̬�����δ��������ת�����
	SideKey_ResetModule();//�ڽ�����APP֮ǰ��λ�����ఴ���ģ��
	for(i=31;i>=0;i--)//������ʧ�����³���
	  {
		OLED_Clear();
		OLED_DrawLine(i,16,63-i,16,1); 
		OLED_Refresh(); 
		delay_ms(1);
		}
	for(i=16;i>0;i--) //���Ų˵����³��ֵĶ���
		{
		MenuRenderHandler();//ִ�в˵���Ⱦ
		OLED_Fill(0,0,63,i,0); //�ϰ벿��
		OLED_Fill(0,32-i,63,i,0);//�°벿��
		OLED_DrawLine(0,i,63,i,1); 
		OLED_DrawLine(0,32-i,63,32-i,1); //��ʧ��
		OLED_Refresh(); 
		}	
	//��������
  BrightNess=ConvertBrightLevel();//�������ļ����Ŀ������ȵȼ�
	CurrentBrightness=0x7F; //��ʼ����Ŀ������			
	do
	  {
		MenuKeyHandler();//��ɲ˵������İ�������
		MenuRenderHandler();//ִ�в˵���Ⱦ
	  OLED_Refresh(); 
		if(CurrentBrightness<BrightNess)CurrentBrightness++;
		if(CurrentBrightness>BrightNess)CurrentBrightness--;
		OLED_SetBrightness(CurrentBrightness);
    delay_ms(10); //��һ�������𽥱仯����Ч
		}
	while(CurrentBrightness!=BrightNess);
	}
	
//�˵�����������	
void MenuHandler(void)
  {
	static bool IsGUIInRender=true;
	bool SleepState;
	int i;
	//�ж��Ƿ���˯��	
	SleepState=SleepTimer>0?true:false;
  if(IsGUIInRender!=SleepState)
	  {
		IsGUIInRender=SleepState; //ͬ��״̬
		if(!IsGUIInRender)return; //��Ļ��������Ϩ�𣬲�ִ��
		for(i=31;i>=0;i--)//������ʧ�����³���
	   {
			OLED_Clear();
			OLED_DrawLine(i,16,63-i,16,1); 
			OLED_Refresh(); 
			delay_ms(1);
			}
		for(i=16;i>0;i--) //���Ų˵����³��ֵĶ���
			{
			MenuRenderHandler();//ִ�в˵���Ⱦ
			OLED_Fill(0,0,63,i,0); //�ϰ벿��
			OLED_Fill(0,32-i,63,i,0);//�°벿��
			OLED_DrawLine(0,i,63,i,1); 
			OLED_DrawLine(0,32-i,63,32-i,1); //��ʧ��
			OLED_Refresh(); 
			}	
		//��λ����
		getSideKeyShortPressCount(true);
		while(getSideKeyHoldEvent())SideKey_LogicHandler(); //�ȴ������ſ�
		IsSystemWakedup=true; //�ɹ��˳�˯�ߣ���ʱϵͳ�԰����������д���
		}
	//��ǰ��ʱ������������Ⱦ�׶�
	if(SleepTimer>0)MenuRenderHandler();//ִ�в˵���Ⱦ
	//ʱ�䵽�����Ų˵���ʧ��Ч
	else if(SleepTimer==0)	 
	  {
		IsSystemWakedup=false; //���ϵͳ�ѽ���˯��
		SleepTimer=-1; //��ǽ�������
		OLED_OldTVFade();//�����ϵ�����ʧ��Ч
	  }
	//��ɶ������ţ���GRAMΪ�ս���˯��
	else OLED_Clear(); 
	OLED_Refresh(); //ˢ��
	}
