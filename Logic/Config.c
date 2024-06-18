#include "Config.h"
#include "delay.h"

#define ConfigADDR 0xFC00

SystemConfigStrDef Config;

//转换充电功率
int ConvertChagePower(void)
  {
	switch(Config.ChargePower)
	  {
		case ChargePower_30W:return 30;
		case ChargePower_45W:return 45;
	  case ChargePower_60W:return 60;
		case ChargePower_65W:return 66;
		}
	return 45; //其他情况返回45
	}

//转换亮度设置参数
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
	
//恢复默认配置
bool RestoreDefaultCfg(void)
  {
  Config.ChargePower=ChargePower_65W;
	Config.IsEnableOutput=true;
  Config.IsEnableDPDM=true;
	Config.IsEnableSCP=true;
	Config.IsEnablePD=true;
	Config.Brightness=Screen_MidBright; //最高亮度
	return SavingConfig(); 
	}
	
//保存配置
bool SavingConfig(void)
  {
	unsigned int sbuf,data;
	int i,zerocount=0;
	//构建数据
	data=0;
	if(Config.IsEnableOutput)data|=0x05;
	else data|=0x02; //bit0-2 是否使能双向输出
	sbuf=(unsigned int)Config.ChargePower; 
	sbuf&=0x03;
	data|=sbuf<<3; //bit3-4 充电功率
	if(Config.IsEnableDPDM)data|=0x20; //bit 5 是否使能DPDM快充
	if(Config.IsEnableSCP)data|=0x40; //bit 6 是否使能SCP快充
	if(Config.IsEnablePD)data|=0x80;//bit 7 是否使能PD快充
	sbuf=(unsigned int)Config.Brightness; 
	sbuf&=0x03;
	data|=sbuf<<8;	 //bit8-9 屏幕亮度	
	//简单的校验
	sbuf=data;
	for(i=0;i<30;i++)
		{
		if(!(sbuf&0x1))zerocount++; 
		sbuf=sbuf>>1; //从低位到高位逐步计算		
		}
	if(zerocount%2)data|=0x80000000; //包含的0数量为奇数，此时最高位为10
  else data|=0x40000000; //包含的0数量为奇数，此时最高位为01
	//开始写入数据
	if(*((u32 *)ConfigADDR)==data)return true; //数据没有发生改变，直接结束
	i=0;
  do
	 {
	 if(FLASH_ErasePage(ConfigADDR)!=FLASH_COMPLETE)return false;
   if(FLASH_ProgramWordData(ConfigADDR, data)!=FLASH_COMPLETE)return false;
	 i++;
	 if(i>=30)return false; //重试次数过多
	 }
  while(*((u32 *)ConfigADDR)!=data); //检查是否刷写成功并反复重试
	//刷写结束
	return true;
	}

//读取配置
void ReadConfig(void)
  {
	bool CheckSumError=true;
	unsigned int sbuf,checksum;
	int i,zerocount=0;
	//校验数据
	sbuf=*((u32 *)ConfigADDR); //读取数据
  for(i=0;i<30;i++)
		{
		if(!(sbuf&0x1))zerocount++; 
		sbuf=sbuf>>1; //从低位到高位逐步计算		
		}
	if(zerocount%2)checksum=0x80000000; //包含的0数量为奇数，此时最高位为10
  else checksum=0x40000000; //包含的0数量为奇数，此时最高位为01	
	if(checksum==(*((u32 *)ConfigADDR)&0xC0000000))	//校验和正确
	  {
		CheckSumError=false;
		switch(*((u32 *)ConfigADDR)&0x07) //判断Outputmode[2:0]
			 {
			 case 0x05:Config.IsEnableOutput=true;break; //开启
			 case 0x02:Config.IsEnableOutput=false;break; //关闭
			 default :CheckSumError=true; //配置错误
			 }
		sbuf=(*((u32 *)ConfigADDR)>>3)&0x03; 
		Config.ChargePower=(ChargePowerDef)sbuf; //赋值充电功率enum(bit3-4)
		sbuf=(*((u32 *)ConfigADDR)>>8)&0x03; 
		Config.Brightness=(ScreenBrightDef)sbuf; //赋值屏幕亮度enun(bit8-9)
		Config.IsEnableDPDM=(*((u32 *)ConfigADDR)&0x20)?true:false; //bit 5 是否使能DPDM快充
		Config.IsEnableSCP=(*((u32 *)ConfigADDR)&0x40)?true:false; //bit 6 是否使能SCP快充
		Config.IsEnablePD=(*((u32 *)ConfigADDR)&0x80)?true:false;	//bit 7 是否使能PD快充	 
		}
	//校验和出错，重写数据
	if(!CheckSumError)return;
  Config.ChargePower=ChargePower_65W;
	Config.IsEnableOutput=true;
  Config.IsEnableDPDM=true;
	Config.IsEnableSCP=true;
	Config.IsEnablePD=true;
	Config.Brightness=Screen_MidBright; //默认使用中等亮度
	SavingConfig(); //恢复默认
	}
