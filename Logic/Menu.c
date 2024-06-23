#include "IP2368.h"
#include "oled.h"
#include <math.h>
#include "string.h"
#include "SideKey.h"
#include "delay.h"
#include "Config.h"

//版本信息
#define FWMajorVer 2
#define FWMinorVer 3

//固件参数配置
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
	
//全局变量
extern bool EnableXARIIMode; //内部flag位，是否启用协议版本V1.63的特殊寄存器
MenuStateDef MenuState=Menu_TypeC; //菜单状态
MenuStateDef LastMenuBeforeSleep=Menu_TypeC; //全局变量，用于存储在休眠自动切换之前的目标菜单
int SleepTimer=OLEDSleepTimeOut*8;  //睡眠定时器
static int MenuSwitchtimer=0; //菜单切换定时器
static int QCSetMenu=0; //默认设置的快充项目
int ConnectTimeoutCNT=-1; //连接失败定时器
void ForceShutOff(void); //外部关机函数
static volatile bool IsSystemWakedup=true; //标志位，系统是否已经退出睡眠状态	

//效率滤波函数
static float EfficiencyFilter(float IN)
  {
	static volatile float sumbuf=0,min=2000,max=-2000;
	static volatile int sumcount=0;
	static volatile float result=0;
	//计数次数超了
	if(sumcount>EfficiencySampleCount)
	  {
		min=2000;
		max=-2000;
		sumcount=0;
		result=0;
		sumbuf=0; //复位缓冲区
		}
	//计数次数已到
	else if(sumcount==EfficiencySampleCount)
	  {
		sumbuf-=(min+max); //去掉一个最低+最高			
		result=sumbuf/=(float)(sumcount-2); //除以剩下的结果
		sumbuf=0;
		sumcount=0;
		min=2000;
		max=-2000;	 //复位缓冲区
		}
  //计数次数未到，开始累加	
	else if(IN>10&&IN<98)
	  {
		if(IN>max)max=IN;
		if(IN<min)min=IN; //最大最小值采用
		sumbuf+=IN; //加上输入的数值
		sumcount++; //累加次数+1
		}
	//计算结束，返回结果
	return result;
	}
	
//恢复开场动画检测
void ResetConfigDetection(void)
  {
	int i;
	if(GPIO_ReadInBit(ExtKey_IOG,ExtKey_IOP))return; //按键处于松开状态，退出
  for(i=0;i<58;i++)
		{
		OLED_Clear();
		OLED_Printf(16,4,127,1,"RESET");
		OLED_Printf(8,11,127,1,"DEFAULT?");	
		OLED_DrawRectangle(1,22,62,30,1);
		OLED_Fill(3,24,i+1,5,1);  //进度条
		if(GPIO_ReadInBit(ExtKey_IOG,ExtKey_IOP))
		  {
		  OLED_Clear();
		  OLED_Refresh(); //刷屏
		  return; //按键处于松开状态，退出
			}
		OLED_Refresh(); //刷屏
		delay_ms(30); //延时30毫秒
		}
	//用户一直按住持续1秒，开始复位
	OLED_Clear();
	if(RestoreDefaultCfg())OLED_Printf(1,1,127,1,"Default settings has been loaded.");
	else OLED_Printf(1,1,127,1,"Failed to load Default.");
	OLED_Refresh(); //刷屏
  while(!GPIO_ReadInBit(ExtKey_IOG,ExtKey_IOP))delay_ms(1); //等待用户松开按键
	OLED_OldTVFade(); //画面消失
	}	
	
//简简单单的一个开场动画
void DisplaySplash(void)
  {
	int i;
	//第一行PROJECT
	for(i=1;i<9;i++)
	  {
		OLED_Clear();
		OLED_Printfn(11,6,127,i,1,"PROJECT\x7e");	
		OLED_Refresh(); //刷屏
		delay_ms(20); //延迟150mS
		}
	//第二行DIFFTORCH
	for(i=1;i<10;i++)
	  {
		OLED_Clear();
		OLED_Printf(11,6,127,1,"PROJECT\x7e");	
		OLED_Printfn(6,13,127,i,1,"DIFFTORCH");	
		OLED_Refresh(); //刷屏
		delay_ms(20); //延迟150mS
		}	 
	//第三行EXTREME
	for(i=1;i<8;i++)
	  {
		OLED_Clear();
		OLED_Printf(11,6,127,1,"PROJECT\x7e");	
		OLED_Printf(6,13,127,1,"DIFFTORCH");	
		OLED_Printfn(11,20,127,i,1,"EXTREME");	
		OLED_Refresh(); //刷屏
		delay_ms(20); //延迟150mS
		}	
  delay_ms(500);		
	OLED_OldTVFade();
	//显示作者和固件版本
	for(i=1;i<12;i++)
	  {
		OLED_Clear();
		if(Config.BatteryCount==0)OLED_Printfn(1,1,127,i,1,"D8B-?SPD3.0");
		else OLED_Printfn(1,1,127,i,1,"D8B-%dSPD3.0",Config.BatteryCount);	
		OLED_Refresh(); //刷屏
		delay_ms(30); //延迟150mS
		}
	for(i=1;i<30;i++)
	  {
		OLED_Clear();
		if(Config.BatteryCount==0)OLED_Printf(1,1,127,1,"D8B-?SPD3.0");
		else OLED_Printf(1,1,127,1,"D8B-%dSPD3.0",Config.BatteryCount);
		OLED_Printfn(1,9,127,i,1,"FW %d.%d BY:REDSTONER35",FWMajorVer,FWMinorVer);	
		OLED_Refresh(); //刷屏
		delay_ms(30); //延迟150mS
		}	
	//延迟500mS后逐渐消失
	delay_ms(300);
	OLED_ImageDisappear();
	}	
		
//睡眠管理以及屏幕轮换处理函数
void SleepTimerHandler(void)
  {
	//连接失败定时器
	if(ConnectTimeoutCNT>-1)ConnectTimeoutCNT=ConnectTimeoutCNT<250?ConnectTimeoutCNT+1:250;
	//定时器累减
	if(SleepTimer>(OLEDSleepTimeOut*3))
	   { 
		 SleepTimer--;
     MenuSwitchtimer=0; //用户正常操作中，停止轮换
		 LastMenuBeforeSleep=MenuState; //存储睡眠之前的目标菜单
		 }
	else if(SleepTimer>0)
	   {
	   SleepTimer--; //定时器累减
	   if(MenuSwitchtimer==16) //每隔一定时间轮换界面防止烧屏
		   {
			 MenuSwitchtimer=0;
			 if(MenuState==Menu_TypeCMode||MenuState==Menu_Error)return; //如果处于错误或者设置模式阶段则不进行轮换
			 else if(MenuState==Menu_DCDC)MenuState=Menu_TypeC;
			 else if(MenuState==Menu_TypeC)MenuState=Menu_Batt;
			 else if(MenuState==Menu_Batt)MenuState=Menu_DCDC;//实现菜单轮换
			 }
		 else MenuSwitchtimer++;
     } 		
	}
//在每次按键按下之后恢复睡眠之前所在的菜单
void RestoreMenuStateBeforeSleep(void)
  {
	if(SleepTimer>(OLEDSleepTimeOut*3))return; //睡眠之前退出
	if(LastMenuBeforeSleep==Menu_Error||LastMenuBeforeSleep==Menu_WaitReconnect)
		MenuState=Menu_TypeC; //如果睡眠之前是错误菜单则恢复到默认值
  else 
		MenuState=LastMenuBeforeSleep;
	}

//菜单按键处理主函数
void MenuKeyHandler(void)
  {
	int Click;
	int power;
  bool SaveFailed;
  bool IsTypeCOK=false;
	QuickChargeCtrlDef QCtrl;
	//获取短按次数、判断当前系统是否睡眠,然后获取type-C的状态	
	Click=getSideKeyShortPressCount(true);
	if(!IsSystemWakedup)return; //睡眠状态下不处理按键操作
	IP2368_GetTypeCConnectedState(&IsTypeCOK);
	//状态机
	switch(MenuState)
	  {
		//英集芯SoC升级
		case Menu_PSOC_Upgrade:
					break;
		//错误菜单
		case Menu_WaitReconnect:
		case Menu_Error:
			    if(!IsTypeCOK&&Click==3){OLED_OldTVFade();ForceShutOff();} //三击开关强制关机
		      break;
		//关于菜单
		case Menu_About:
			  if(Click>0||getSideKeyLongPressEvent())
				   {
				   while(getSideKeyHoldEvent())SideKey_LogicHandler(); //等待按键放开
				   MenuState=Menu_Settings; //回到设置子菜单
			     }
		    break;
		//亮度设置菜单
		case Menu_Brightness:
			  if(Click==1)switch(Config.Brightness) //更改亮度
				  {
					case Screen_MaxBright:Config.Brightness=Screen_LowBright;break;
					case Screen_HighBright:Config.Brightness=Screen_MaxBright;break;
					case Screen_MidBright:Config.Brightness=Screen_HighBright;break;
					case Screen_LowBright:Config.Brightness=Screen_MidBright;break;				
					}
		     else if(getSideKeyLongPressEvent()) //长按保存退出
				  {
					QCSetMenu=0; //重置index
					MenuState=Menu_Settings; //回到设置子菜单
				  //显示正在保存
					OLED_Clear();
					OLED_Printf(9,15,127,1,"SAVING...");
					OLED_Refresh(); 
					OLED_SetBrightness(ConvertBrightLevel());	//从配置文件获得目标的亮度等级并设置OLED
					while(getSideKeyHoldEvent())SideKey_LogicHandler(); //等待按键放开
					if(SavingConfig())break;
					OLED_Clear();
					OLED_Printf(7,15,127,1,"SAVE ERROR");
					OLED_Refresh();
          delay_Second(1); //显示保存失败
					}
				 break;	
		//快充协议设置菜单
		case Menu_QuickChargeSet: 
		    if(Config.BatteryCount<3)Config.IsEnable20VPDO=false; //电池节数小于3节，禁止使用20V PDO
		    if(!EnableXARIIMode&&Config.IsEnableSCP)Config.IsEnable9VPDO=false; //旧版本固件如果启用SCP则强制禁用9V PDO
			  if(Click==1)QCSetMenu=QCSetMenu<(Config.BatteryCount>2?4:3)?QCSetMenu+1:0; //单击选择协议
		    else if(Click==2)
				  {
					switch(QCSetMenu) //更新数据
						{
						case 0:Config.IsEnableDPDM=Config.IsEnableDPDM?false:true;break; //设置DPDM
						case 1:Config.IsEnablePD=Config.IsEnablePD?false:true;break; //设置PD
						case 2:Config.IsEnableSCP=Config.IsEnableSCP?false:true;break; //设置SCP
						case 3:Config.IsEnable9VPDO=Config.IsEnable9VPDO?false:true;break; //设置9V PDO是否启用
						case 4:Config.IsEnable20VPDO=Config.IsEnable20VPDO?false:true;break; //设置20V PDO是否启用
						}
					//设置协议
					QCtrl.IsEnableDPDM=Config.IsEnableDPDM;
	        QCtrl.IsEnablePD=Config.IsEnablePD;
	        QCtrl.IsEnableSCP=Config.IsEnableSCP;
					QCtrl.IsEnable9VPDO=(!EnableXARIIMode&&Config.IsEnableSCP)?false:Config.IsEnable9VPDO;
					QCtrl.IsEnable20VPDO=Config.BatteryCount<3?false:Config.IsEnable20VPDO;
	        if(!IP2368_SetQuickchargeState(&QCtrl))MenuState=Menu_Error; //尝试保存协议
					}
				else if(getSideKeyLongPressEvent()) //长按保存退出
				  {
					QCSetMenu=0; //重置index
					MenuState=Menu_Settings; //回到设置子菜单
				  //显示正在保存
					OLED_Clear();
					OLED_Printf(9,15,127,1,"SAVING...");
					OLED_Refresh(); 
					while(getSideKeyHoldEvent())SideKey_LogicHandler(); //等待按键放开
					if(SavingConfig())break;
					OLED_Clear();
					OLED_Printf(7,15,127,1,"SAVE ERROR");
					OLED_Refresh();
          delay_Second(1); //显示保存失败
					}
		    break;
		//充电功率设置菜单
	  case Menu_ChargePower:	
	      if(Config.BatteryCount<3)		
				  {
					if(Config.ChargePower==ChargePower_60W)Config.ChargePower=ChargePower_45W;
					if(Config.ChargePower==ChargePower_65W)Config.ChargePower=ChargePower_45W; //黑名单机制，如果电池节数小于三节则禁止打开45W以上的挡位
					}
		    if(Click==1)switch(Config.ChargePower) //单击选择充电功率
				  {
					case ChargePower_65W:Config.ChargePower=ChargePower_30W;break;
					case ChargePower_60W:Config.ChargePower=ChargePower_65W;break;
					case ChargePower_45W:Config.ChargePower=Config.BatteryCount<3?ChargePower_30W:ChargePower_60W;break; //如果电池只有2S，则禁止打开45W以上的挡位
					case ChargePower_30W:Config.ChargePower=ChargePower_45W;break;
					}
				else if(Click==2) //双击不保存并退出
				  {
					OLED_Clear();
					OLED_Printf(4,15,127,1,"NOT SAVED");
					OLED_Refresh(); 
					MenuState=Menu_Settings; //回到设置子菜单
					if(!IP2368_GetChargePower(&power))MenuState=Menu_Error; 
          else if(power>60&&Config.BatteryCount>2)Config.ChargePower=ChargePower_65W;
          else if(power>45&&Config.BatteryCount>2)Config.ChargePower=ChargePower_60W; //仅有电池大于2节才允许激活60W功率
          else if(power>30)Config.ChargePower=ChargePower_45W;
          else Config.ChargePower=ChargePower_30W; //根据当前充电功率寄存器设置充电功率			
					delay_Second(1);	
					}
		    else if(getSideKeyLongPressEvent())//长按保存并退出
				  {
					MenuState=Menu_Settings; //回到设置子菜单
				  //显示正在保存
					OLED_Clear();
					OLED_Printf(9,15,127,1,"SAVING...");
					OLED_Refresh(); 
					while(getSideKeyHoldEvent())SideKey_LogicHandler(); //等待按键放开
					//写寄存器设置充电功率
					if(!IP2368_SetChargePower(ConvertChagePower()))SaveFailed=true;
          else if(!SavingConfig())SaveFailed=true;
          else SaveFailed=false; //成功保存		
          //保存成功则退出如果失败则提示
					if(!SaveFailed)break;
					OLED_Clear();
					OLED_Printf(7,15,127,1,"SAVE ERROR");
					OLED_Refresh();
          delay_Second(1);				
          if(!IP2368_GetChargePower(&power))MenuState=Menu_Error; 
					else if(power>60&&Config.BatteryCount>2)Config.ChargePower=ChargePower_65W;
          else if(power>45&&Config.BatteryCount>2)Config.ChargePower=ChargePower_60W; //仅有电池大于2节才允许激活60W功率
          else if(power>30)Config.ChargePower=ChargePower_45W;
          else Config.ChargePower=ChargePower_30W; //根据当前充电功率寄存器设置充电功率			
					}
		    break;
		//TypeC模式菜单
		case Menu_TypeCMode:
			  if(Click==1)Config.IsEnableOutput=Config.IsEnableOutput?false:true; //切换模式	
		    else if(Click==2) //不保存退出
					  {
						MenuState=Menu_Settings; //回到设置子菜单
						OLED_Clear();
					  OLED_Printf(4,15,127,1,"NOT SAVED");
					  OLED_Refresh(); 
						delay_Second(1);	
					  //读取寄存器状态，将当前DRP设置同步为芯片设置值
						if(!IP2368_GetDischargeState(&Config.IsEnableOutput))
						   {
						   MenuState=Menu_Error; 
							 return; //出现错误指示IP2368离线
							 }
						}
		     else if(getSideKeyLongPressEvent()) //双击不保存或者长按退出
						{		
            MenuState=Menu_Settings; //回到设置子菜单				
						//显示正在保存
						OLED_Clear();
						OLED_Printf(9,15,127,1,"SAVING...");
						OLED_Refresh(); 
						while(getSideKeyHoldEvent())SideKey_LogicHandler(); //等待按键放开
						//写寄存器设置当前DRP参数
						if(!IP2368_SetDischarge(Config.IsEnableOutput))SaveFailed=true;
            else if(!SavingConfig())SaveFailed=true;
            else SaveFailed=false; //成功保存							
						//保存成功则退出如果失败则提示
						if(!SaveFailed)break;
						OLED_Clear();
						OLED_Printf(7,15,127,1,"SAVE ERROR");
						OLED_Refresh();
            delay_Second(1);						
            if(!IP2368_GetDischargeState(&Config.IsEnableOutput))MenuState=Menu_Error; 	//将当前DRP设置同步为芯片设置值						
						}	
		      break;
		//故障flag菜单
		case Menu_Fault:
			  if(!IsTypeCOK&&Click==3){OLED_OldTVFade();ForceShutOff();} //三击开关强制关机
			  else if(Click==1)MenuState=Menu_TypeC; //单击切换到TypeC菜单
				else if(Click==2) //任意界面双击进入设置菜单
				  {
				  QCSetMenu=0; //重置index
					MenuState=Menu_Settings;
			    }
		    break;
		//DCDC菜单
	  case Menu_DCDC:
			  if(!IsTypeCOK&&Click==3){OLED_OldTVFade();ForceShutOff();} //三击开关强制关机
			  else if(Click==2) //任意界面双击进入设置菜单
				  {
				  QCSetMenu=0; //重置index
					MenuState=Menu_Settings;
			    }
			  else if(Click==1)MenuState=Menu_Fault; //单击切换到故障flag菜单
		    break;
		//电池菜单
		case Menu_Batt:
			  if(!IsTypeCOK&&Click==3){OLED_OldTVFade();ForceShutOff();} //三击开关强制关机
			  else if(Click==2) //任意界面双击进入设置菜单
				  {
				  QCSetMenu=0; //重置index
					MenuState=Menu_Settings;
			    }
			  else if(Click==1)MenuState=Menu_DCDC; //单击切换到DCDC菜单
		    break;
	  //Type-C菜单
		case Menu_TypeC:
		    if(!IsTypeCOK&&Click==3){OLED_OldTVFade();ForceShutOff();} //三击开关强制关机
			  else if(Click==2) //任意界面双击进入设置菜单
				  {
				  QCSetMenu=0; //重置index
					MenuState=Menu_Settings;
			    }
		    else if(Click==1)MenuState=Menu_Batt; //单击切换到DCDC菜单
		    break;
		//设置菜单
		case Menu_Settings:
			 if(Click==1)QCSetMenu=QCSetMenu<6?QCSetMenu+1:0; //单击选择协议
		   else if(Click==2)
			   {
			   switch(QCSetMenu)//双击进入到对应的设置菜单
						{
						case 0:MenuState=Menu_QuickChargeSet;break;
						case 1:MenuState=Menu_TypeCMode;break;
						case 2:MenuState=Menu_ChargePower;break;
					  case 3:MenuState=Menu_Brightness;break;
					  case 4:MenuState=Menu_About;break;
					  case 5:MenuState=Menu_PSOC_Upgrade;break;
					  case 6:IP2368_DoRSOCCalibration();break;
						}
				 QCSetMenu=0; //复位index
				 }
			 else if(getSideKeyLongPressEvent()) //长按退出
				  {
					QCSetMenu=0; //复位index
				  while(getSideKeyHoldEvent())SideKey_LogicHandler(); //等待按键放开
				  MenuState=Menu_DCDC; //回到初始菜单
			    }
			 break;
		}
	}
//判断函数，判断菜单是否位于电池菜单
bool IsMenuAtBatteryTelem(void)
  {
	if(MenuState==Menu_Batt)return true;
	//其他情况，返回false
	return false;
	}
		
//菜单渲染函数
static void MenuRenderHandler(void)
  {
	BatteryStatuDef BattState;
	TypeCStatusDef TypeCState;
	FaultFlagStrDef Fault;
	QuickChargeCtrlDef Qstate;
	static MenuStateDef LastMenu=Menu_TypeC; //出错之前的上个菜单
  const char *BattInfo;
	bool State,IsNTCOK,IsDisEN,VBUSMOSEN,IsTypeCOK,IsTypeCSrcOK;
	int ChargePower,y;
	float TempResult,HCCurrent,Efficiency;
  //轮询IP2368寄存器获取状态
	switch(MenuState)	
	  {
		//快充功率设置菜单
		case Menu_QuickChargeSet:
		   State=IP2368_GetQuickchargeState(&Qstate);	//获取快充状态
		   break;
		//错误flag子菜单
		case Menu_Fault:
			 State=IP2368_GetAlertFlag(&Fault); //获取故障状态
		   break;
	  //DCDC子菜单		
		case Menu_DCDC:
		  State=IP2368_GetDischargeState(&IsDisEN); //是否使能放电
		  State&=IP2368_GetIsTypeCSrcConnected(&IsTypeCSrcOK); //获取Type-C是否以SRC方式工作
		  State&=IP2368_GetTypeCState(&TypeCState);
		  State&=IP2368_GetBatteryState(&BattState); //获取电池和Type-C状态（用于计算充放电效率）
	    State&=IP2368_GetChargePower(&ChargePower); //充电功率
			State&=IP2368_GetNTCTemp(&TempResult,&IsNTCOK); //温度
		  State&=IP2368_GetVBUSSwitchState(&VBUSMOSEN); //MOS使能状态
		  break;
		//电池子菜单
	  case Menu_Batt:
			State=IP2368_GetBatteryState(&BattState); //获取电池状态
		  break;
		//Type-C状态
		case Menu_TypeC:
			State=IP2368_GetTypeCConnectedState(&IsTypeCOK);
		  State&=IP2368_GetTypeCState(&TypeCState); //获取Type-C状态的寄存器以及Type-C是否连接
		  break;
	  //错误菜单，反复获取type-C连接状态进行判断
		case Menu_Error:
		case Menu_WaitReconnect:
			State=IP2368_GetTypeCConnectedState(&IsTypeCOK);
		  break;
		case Menu_Settings:
			State=IP2368_GetBatteryState(&BattState); //获取电池状态（用于控制是否允许进入编辑充电模式）
		  break;
		//其余内部设置不需要读取寄存器的本地设置菜单,State=1表示无故障。
		default:State=true;
		}
	if(!State) //IP2368断开连接
	  {
		SleepTimer=250; //出现致命异常，禁止睡眠
		if(ConnectTimeoutCNT==-1) //定时器第一次启动
		   {
			 LastMenu=MenuState; //存储CONNECT提示之前的菜单
			 ConnectTimeoutCNT=0; //将定时器从关机状态设置为激活	
			 }
		//定时器时间未到，将菜单设置为等待连接
		else if(ConnectTimeoutCNT<250)
			 MenuState=Menu_WaitReconnect;
	  //定时器时间到，显示错误
		else 
			 MenuState=Menu_Error; 
		}
  else //IP2368回复正常
	  {
    ConnectTimeoutCNT=-1;//复位定时器
	  if(MenuState==Menu_Error||MenuState==Menu_WaitReconnect)MenuState=LastMenu; //指示IP2368已恢复正常,回到原来的菜单
		}
	//开始渲染
	OLED_Clear();
	switch(MenuState)
	  {
		//PSOC升级	
	  case Menu_PSOC_Upgrade:
			   if(!IP2368_SendResetCommand())
				   {
					 OLED_Printf(1,1,127,1,"Failed to reset PSoC.");
					 OLED_Refresh();
					 delay_Second(2);
					 MenuState=Menu_Settings; //回到设置子菜单
					 }
		     else while(1) //进入死循环
				   {
					 OLED_Printf(1,1,127,1,"Entered Upgrade mode.");
					 OLED_Refresh();
					 }
         break;
		//固件信息	
		case Menu_About:
			OLED_Printf(0,0,127,1,"V%d.%d DATE:%s %s",FWMajorVer,FWMinorVer,__DATE__,__TIME__);	 
		  OLED_Printf(0,18,127,1,"PSV:1.%s",EnableXARIIMode?"63":"2");
		  OLED_Printf(0,24,127,1,"D8B-%dSPD3.0",Config.BatteryCount);
		  break;
		//设置菜单
		case Menu_Settings:
		   OLED_Printf(8,1,127,1,"SETTINGS");
		   if(QCSetMenu<3) //前三个选项
			   {
				 //绘制快充设置
				 y=QCSetMenu==0?10:9;
		     OLED_Printf(7,y,127,1,"Q-Chrg.");
		     if(QCSetMenu==0)OLED_DrawRectangle(5,8,56,16,1);
		     //绘制Type-C模式设置
		     y=QCSetMenu==1?17:18;
		     if(QCSetMenu==2)y-=2; //选中最下面的内容，需要让字符上移2格避免冲突
		     OLED_Printf(7,y,127,1,"Type-C");
		     if(QCSetMenu==1)OLED_DrawRectangle(5,15,56,23,1);
		     //绘制充电功率设置
			   if(BattState.BattState!=Batt_discharging)
			     {
				   y=QCSetMenu==2?24:25;
		       OLED_Printf(7,y,127,1,"Chrg PWR");
		       if(QCSetMenu==2)OLED_DrawRectangle(5,22,56,30,1);	
				   }
			   else if(QCSetMenu==2)QCSetMenu=3;//使选项不能被选中，直接跳到第三个
				 }
			 else if(QCSetMenu<6)//第四个到第六个
			   {
				 //绘制屏幕亮度设置
				 y=QCSetMenu==3?10:9;
		     OLED_Printf(7,y,127,1,"Screen");
		     if(QCSetMenu==3)OLED_DrawRectangle(5,8,56,16,1);
					//帮助菜单
		     y=QCSetMenu==4?17:18;
		     if(QCSetMenu==5)y-=2; //选中最下面的内容，需要让字符上移2格避免冲突
		     OLED_Printf(7,y,127,1,"ABOUT");
		     if(QCSetMenu==4)OLED_DrawRectangle(5,15,56,23,1);					 
				 //升级菜单
				 y=QCSetMenu==5?24:25;
		     OLED_Printf(7,y,127,1,"PSOCUPD");
		     if(QCSetMenu==5)OLED_DrawRectangle(5,22,56,30,1);
				 }
			 else //第六个到第九个
			   {
				 //剩余容量校准
				 y=QCSetMenu==6?10:9;
		     OLED_Printf(7,y,127,1,"RSOC Cal.");
		     if(QCSetMenu==6)OLED_DrawRectangle(5,8,56,16,1);
				 }
		   break;
		//快充功率设置菜单
		case Menu_QuickChargeSet:			
			 OLED_Printf(7,1,127,1,"Q. CHARGE");
		   OLED_DrawLine(1,3,4,3,1);
		   OLED_DrawLine(58,3,62,3,1);
			 if(QCSetMenu<3)		//第一个到第三个
		      {
		      //绘制DPDM快充状态
					y=QCSetMenu==0?10:9;
					OLED_Printf(7,y,127,1,"DPDM");
					OLED_Printf(38,y,127,1,Qstate.IsEnableDPDM?"ON":"OFF");
					if(QCSetMenu==0)OLED_DrawRectangle(5,8,56,16,1);
					//绘制PD快充状态
					y=QCSetMenu==1?17:18;
					if(QCSetMenu==2)y-=2; //选中最下面的内容，需要让字符上移2格避免冲突
					OLED_Printf(7,y,127,1,"PD");
					OLED_Printf(38,y,127,1,Qstate.IsEnablePD?"ON":"OFF");
					if(QCSetMenu==1)OLED_DrawRectangle(5,15,56,23,1);
		   		//绘制SCP 快充状态
					y=QCSetMenu==2?24:25;
					OLED_Printf(7,y,127,1,"SCP");
					OLED_Printf(38,y,127,1,Qstate.IsEnableSCP?"ON":"OFF");
					if(QCSetMenu==2)OLED_DrawRectangle(5,22,56,30,1);
         	}
			else //第四个到第六个
					{
					if(EnableXARIIMode||(!Config.IsEnableSCP&&!EnableXARIIMode))
					  {
						//绘制PD9V 快充状态
						y=QCSetMenu==3?10:9;
						OLED_Printf(7,y,127,1,"9VPDO");
						OLED_Printf(43,y,127,1,Qstate.IsEnable9VPDO?"ON":"OFF");
						if(QCSetMenu==3)OLED_DrawRectangle(5,8,61,16,1);
						}
					else if(QCSetMenu==3)QCSetMenu=4; //使选项不可选中
				  //绘制20V PDO状态
					if(Config.BatteryCount>2) //电池有2节以上则才允许开启20V PDO
					  {
						y=QCSetMenu==4?17:18;
						if(QCSetMenu==5)y-=2; //选中最下面的内容，需要让字符上移2格避免冲突
						OLED_Printf(7,y,127,1,"20VPD");
						OLED_Printf(43,y,127,1,Qstate.IsEnable20VPDO?"ON":"OFF");
						if(QCSetMenu==4)OLED_DrawRectangle(5,15,61,23,1);
						}
					else if(QCSetMenu==4)QCSetMenu=1; //使选项不可选中
					}
		   break;	
		//等待连接菜单
		case Menu_WaitReconnect:
       OLED_Printf(2,9,127,1,"CONNECTING TO PSOC...");
       break;			
		//错误菜单	
		case Menu_Error:
       OLED_Printf(3,9,127,1,"PSOC I2C Offline!");
       break;
		//屏幕亮度设置菜单
	  case Menu_Brightness:
	     OLED_Printf(2,1,127,1,"BRIGHTNESS");
		   OLED_Printf(9,11,127,1,"LOW");
		   OLED_Printf(9,22,127,1,"MID");
		   OLED_Printf(38,11,127,1,"HIGH");
		   OLED_Printf(38,22,127,1,"MAX");
		   switch(Config.Brightness) //显示当前选择的充电功率
			   {
				 case Screen_LowBright:OLED_DrawRectangle(7,9,27,17,1);break;
				 case Screen_MidBright:OLED_DrawRectangle(7,20,27,28,1);break;
				 case Screen_HighBright:OLED_DrawRectangle(36,9,62,17,1);break;
				 case Screen_MaxBright:OLED_DrawRectangle(36,20,56,28,1);break; 
				 }		 
		   break;
		//充电功率设置菜单
		case Menu_ChargePower:
			 if(BattState.BattState==Batt_discharging)
			   {
				 MenuState=Menu_Settings;
				 return; //电池进入放电模式，此时禁止设置充电功率的菜单项
				 }
			 OLED_Printf(2,1,127,1,"CHARGE PWR.");
		   OLED_Printf(9,11,127,1,"30W");
		   OLED_Printf(9,22,127,1,"45W");
			 if(Config.BatteryCount>2) //如果电池节数大于2节才显示45W以上的挡位
			   {				 
				 OLED_Printf(38,11,127,1,"60W");
				 OLED_Printf(38,22,127,1,"66W");
				 }
		   switch(Config.ChargePower) //显示当前选择的充电功率
			   {
				 case ChargePower_30W:OLED_DrawRectangle(7,9,27,17,1);break;
				 case ChargePower_45W:OLED_DrawRectangle(7,20,27,28,1);break;
				 case ChargePower_60W:if(Config.BatteryCount>2)OLED_DrawRectangle(36,9,56,17,1);break;
				 case ChargePower_65W:if(Config.BatteryCount>2)OLED_DrawRectangle(36,20,56,28,1);break; 
				 }
			 break;
		//Type-C模式菜单
		case Menu_TypeCMode:
			 OLED_Printf(0,2,127,1,"TYPEC MODE");
	     OLED_Printf(6,11,127,1,"DRP-BIDIR");
       OLED_Printf(6,22,127,1,"DFP-INPUT");		
		   if(Config.IsEnableOutput)OLED_DrawRectangle(4,9,59,17,1); //指示选中DRP
		   else OLED_DrawRectangle(4,20,59,28,1); //指示选中DFP
		   break;
		//错误flag子菜单
		case Menu_Fault:
	     OLED_Printf(2,1,127,1,"FAULT STAT");
	     if(!Fault.FaultAsserted&&!Fault.OTAlert)OLED_Printf(8,13,127,1,"NO FAULT");
		   else //显示告警条目
			   {
				 if(Fault.SCPFault)OLED_Printf(2,9,127,1,"SCPF");
				 if(Fault.OCPFault)OLED_Printf(2,16,127,1,"OCPF");
				 if(Fault.INOVFault)OLED_Printf(2,23,127,1,"INOV");
				 if(Fault.OTFault)OLED_Printf(26,9,127,1,"OTMPF");
				 if(Fault.UTFault)OLED_Printf(26,16,127,1,"UTMPF");
				 if(Fault.OTAlert)OLED_Printf(26,23,127,1,"OTALM");
				 }
		   break;
    //DCDC子菜单		
		case Menu_DCDC:
			 OLED_Printf(6,1,127,1,"DCDC STAT");
		   if(IsNTCOK)//正常显示温度
			    {
					if(TempResult>=100)OLED_Printf(4,9,127,1,"%d\x7C",iroundf(TempResult)); //100度显示
					else if(TempResult>=10)OLED_Printf(4,9,127,1,"%.1f\x7C",TempResult); //99.9度显示
					else if(TempResult>=0)OLED_Printf(4,9,127,1,"%.2f\x7C",TempResult); //9.99度显示
					else if(TempResult>=-10)OLED_Printf(4,9,127,1,"%.1f\x7C",TempResult); //-9.9度显示
					else OLED_Printf(4,9,127,1,"%d\x7C",iroundf(TempResult));//-99度显示
					}
			 else OLED_Printf(4,9,127,1,"-- \x7C",iroundf(TempResult)); //温度未知
			 //显示功率		
			 if(IsTypeCSrcOK)OLED_Printf(4,17,127,1,Config.IsEnableOutput?"P%sW":"OFF",Config.IsEnable20VPDO?"60":"45");		//Type-C source模式握手，指示功率为固定45或者60W		
			 else OLED_Printf(4,17,127,1,"P%dW",ChargePower); //显示当前充电功率
			 //计算效率
			 if(BattState.BattState==Batt_discharging)IsTypeCOK=true;		
			 else if(BattState.BattState==Batt_CCCharge)IsTypeCOK=true;
       else if(BattState.BattState==Batt_CVCharge)IsTypeCOK=true;					
			 else if(BattState.BattState==Batt_PreChage)IsTypeCOK=true;		
			 else IsTypeCOK=false; //充电放电过程中才显示效率，平时不显示
			 BattState.BatteryVoltage+=0.11; //将电压加回来
			 if(BattState.BattState==Batt_discharging)Efficiency=TypeCState.BusPower/(BattState.BatteryCurrent*BattState.BatteryVoltage); //放电，使用输出除以输入得到效率
			 else Efficiency=(BattState.BatteryCurrent*BattState.BatteryVoltage)/TypeCState.BusPower; //充电，使用输入除以输出得到效率
			 Efficiency*=100; //转为百分比
			 Efficiency=IsTypeCOK?EfficiencyFilter(fabsf(Efficiency)):0; //将效率数据加入到滤波器内 	
			 if(!IsTypeCOK||Efficiency<=10||Efficiency>=98)OLED_Printf(4,25,127,1,"-- %%");
			 else OLED_Printf(4,25,127,1,"%d%%",iroundf(Efficiency));//如果效率异常或者TypeC没链接则显示无效率数据				
			 //Typec接口状态
			 OLED_Printf(39,9,127,1,"%s",IsDisEN?"DRP":"DFP");
		   //放电MOSFET状态
			 OLED_Printf(39,16,127,1,"MOS");
			 OLED_Printf(VBUSMOSEN?43:39,22,127,1,VBUSMOSEN?"ON":"OFF");
			 OLED_DrawLine(35,18,38,18,1);
			 OLED_DrawLine(57,18,59,18,1);
			 OLED_DrawLine(35,18,35,29,1);
			 OLED_DrawLine(59,18,59,29,1);
			 OLED_DrawLine(35,29,60,29,1); //外围的框
			 break;
	  //TypeC子菜单
	  case Menu_TypeC:
		   OLED_Printf(0,1,127,1,"TYPEC STAT");
		   if(!IsTypeCOK||TypeCState.busVoltage<4.5) //USB Type-C未连接或者连接了但是没有电压，提示not connected
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
				 }  //获取PD状态
			 if(TypeCState.QuickChargeState==QuickCharge_PD) //PD模式
			   {
				 OLED_Printf(44,11,127,1,"PD");
				 OLED_DrawRectangle(41,9,57,17,1);
				 OLED_Printf(strlen(BattInfo)>2?41:44,21,127,1,"%s",BattInfo);
				 }				 
			 else if(TypeCState.QuickChargeState==QuickCharge_HV) //其余高压模式
			   {
				 OLED_Printf(44,11,127,1,"HV");
				 OLED_DrawRectangle(41,9,57,17,1);
				 OLED_Printf(41,21,127,1,"---");				 
				 }
			 else if(TypeCState.QuickChargeState==QuickCharge_HC) //低压高电流模式
			   {
				 OLED_Printf(44,11,127,1,"HC");
				 OLED_DrawRectangle(41,9,57,17,1);
				 HCCurrent=fabsf(TypeCState.BusCurrent); //对电流取绝对值计算功率
				 if(HCCurrent>5)HCCurrent=7;
				 else if(HCCurrent>3)HCCurrent=4;
				 else if(HCCurrent>2)HCCurrent=3; 
				 else HCCurrent=2; //协议识别部分
				 OLED_Printf(44,21,127,1,"%dA",iroundf(HCCurrent));	//显示电流			 
				 }
			 else //标准充电
			   {
				 OLED_Printf(41,12,127,1,"STD");
				 OLED_Printf(41,19,127,1,"CHG");
				 OLED_DrawRectangle(38,9,60,26,1);
				 }
		   break;
		//电池子菜单
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
//菜单初始化功能
void MenuInit(void)
  {
	int i;
	bool IsTypeCOK;
	char BrightNess,CurrentBrightness;
	if(!IP2368_GetTypeCConnectedState(&IsTypeCOK))MenuState=Menu_Error;
	if(!IsTypeCOK)MenuState=Menu_Batt;
	else MenuState=Menu_TypeC; //判断Type-C连接状态，如果未连接则跳转到电池
	SideKey_ResetModule();//在进入主APP之前复位整个侧按检测模块
	for(i=31;i>=0;i--)//播放消失线重新出现
	  {
		OLED_Clear();
		OLED_DrawLine(i,16,63-i,16,1); 
		OLED_Refresh(); 
		delay_ms(1);
		}
	for(i=16;i>0;i--) //播放菜单重新出现的动画
		{
		MenuRenderHandler();//执行菜单渲染
		OLED_Fill(0,0,63,i,0); //上半部分
		OLED_Fill(0,32-i,63,i,0);//下半部分
		OLED_DrawLine(0,i,63,i,1); 
		OLED_DrawLine(0,32-i,63,32-i,1); //消失线
		OLED_Refresh(); 
		}	
	//设置亮度
  BrightNess=ConvertBrightLevel();//从配置文件获得目标的亮度等级
	CurrentBrightness=0x7F; //初始化的目标亮度			
	do
	  {
		MenuKeyHandler();//完成菜单操作的按键处理
		MenuRenderHandler();//执行菜单渲染
	  OLED_Refresh(); 
		if(CurrentBrightness<BrightNess)CurrentBrightness++;
		if(CurrentBrightness>BrightNess)CurrentBrightness--;
		OLED_SetBrightness(CurrentBrightness);
    delay_ms(10); //做一个亮度逐渐变化的特效
		}
	while(CurrentBrightness!=BrightNess);
	}
	
//菜单处理主函数	
void MenuHandler(void)
  {
	static bool IsGUIInRender=true;
	bool SleepState;
	int i;
	//判断是否处于睡眠	
	SleepState=SleepTimer>0?true:false;
  if(IsGUIInRender!=SleepState)
	  {
		IsGUIInRender=SleepState; //同步状态
		if(!IsGUIInRender)return; //屏幕从亮处到熄灭，不执行
		for(i=31;i>=0;i--)//播放消失线重新出现
	   {
			OLED_Clear();
			OLED_DrawLine(i,16,63-i,16,1); 
			OLED_Refresh(); 
			delay_ms(1);
			}
		for(i=16;i>0;i--) //播放菜单重新出现的动画
			{
			MenuRenderHandler();//执行菜单渲染
			OLED_Fill(0,0,63,i,0); //上半部分
			OLED_Fill(0,32-i,63,i,0);//下半部分
			OLED_DrawLine(0,i,63,i,1); 
			OLED_DrawLine(0,32-i,63,32-i,1); //消失线
			OLED_Refresh(); 
			}	
		//复位按键
		getSideKeyShortPressCount(true);
		while(getSideKeyHoldEvent())SideKey_LogicHandler(); //等待按键放开
		IsSystemWakedup=true; //成功退出睡眠，此时系统对按键操作进行处理
		}
	//当前定时器处于正常渲染阶段
	if(SleepTimer>0)MenuRenderHandler();//执行菜单渲染
	//时间到，播放菜单消失特效
	else if(SleepTimer==0)	 
	  {
		IsSystemWakedup=false; //标记系统已进入睡眠
		SleepTimer=-1; //标记结束计数
		OLED_OldTVFade();//播放老电视消失特效
	  }
	//完成动画播放，令GRAM为空进入睡眠
	else OLED_Clear(); 
	OLED_Refresh(); //刷屏
	}
