#include "ht32.h"
#include "delay.h"

#define ProgramSize 0x9FFF  //程序的大小
#define CRCWordAddress 0xA000  //存储CRC字的存储器

//计算主程序区域的CRC-32
unsigned int MainProgramRegionCRC(void)
 {
 unsigned int DATACRCResult;
 int i;
 CKCU_PeripClockConfig_TypeDef CLKConfig={{0}};
 //初始化CRC32      
 CLKConfig.Bit.CRC = 1;
 CKCU_PeripClockConfig(CLKConfig,ENABLE);//启用CRC-32时钟  
 CRC_DeInit(HT_CRC);//清除配置
 HT_CRC->SDR = 0x0;//CRC-32 poly: 0x04C11DB7  
 HT_CRC->CR = CRC_32_POLY | CRC_BIT_RVS_WR | CRC_BIT_RVS_SUM | CRC_BYTE_RVS_SUM | CRC_CMPL_SUM;
 //开始校验
 for(i=0;i<ProgramSize;i+=4)HT_CRC->DR=*(u32 *)i;//将内容写入到CRC寄存器内
 for(i=4;i<8;i++)HT_CRC->DR=i<4?*(u32*)(0x40080310+(i*4)):~*(u32*)(0x40080310+((i-4)*4));//写入FMC UID
 //校验完毕计算结果
 DATACRCResult=HT_CRC->CSR;
 CRC_DeInit(HT_CRC);//清除CRC结果
 CLKConfig.Bit.CRC = 1;
 CKCU_PeripClockConfig(CLKConfig,DISABLE);//禁用CRC-32时钟节省电力
 DATACRCResult^=0x351A53DD;//输出结果异或一下
 return DATACRCResult;
 }	

//启用flash锁定
void CheckForFlashLock(void)
 { 
 int i=0;
 FLASH_OptionByte Option;
 unsigned int ProgramAreaCRC; 
 //检查option byte是否开启
 GPIO_DisableDebugPort();//关闭debug口
 FLASH_GetOptionByteStatus(&Option);
 if(Option.MainSecurity != 0)
    {
    ProgramAreaCRC=MainProgramRegionCRC();
		if(*(u32 *)CRCWordAddress!=ProgramAreaCRC)while(1); //检查不通过
    return;
    }
 //启用HSI(给flash设置option byte需要HSI启用)
 CKCU_HSICmd(ENABLE);
 while(CKCU_GetClockReadyStatus(CKCU_FLAG_HSIRDY) != SET)
   {
	 delay_ms(1);
	 i++;
	 if(i==50)return; //等待超时
	 }
 //编程程序的CRC32值
 ProgramAreaCRC=MainProgramRegionCRC();
 if(FLASH_ErasePage(CRCWordAddress)!=FLASH_COMPLETE)return;
 if(FLASH_ProgramWordData(CRCWordAddress, ProgramAreaCRC)!=FLASH_COMPLETE)return;
 if(*((u32 *)CRCWordAddress)!=ProgramAreaCRC)return;
 //打开主安全功能
 Option.OptionProtect=1; //锁定选项byte
 Option.MainSecurity=1;  //打开ROP
 Option.WriteProtect[0]=0xFFFFFFFF;//整个程序区域锁死禁止编程和访问(0-32KB)
	 Option.WriteProtect[1]=0x000FFFFF;//CRC字区域写保护,最高位不保护	 (32-64KB)
 Option.WriteProtect[2]=0;
 Option.WriteProtect[3]=0; //其他区域解锁	 
 FLASH_EraseOptionByte();
 if(FLASH_ProgramOptionByte(&Option)!=FLASH_COMPLETE)return;
 NVIC_SystemReset();  //刷完之后重启
 while(1);
 }
