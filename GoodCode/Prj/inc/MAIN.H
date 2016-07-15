#ifndef __MAIN_H__
#define __MAIN_H__

typedef unsigned char UCHAR;
typedef unsigned int UINT;
typedef unsigned long ULONG;

//看门狗定义 5s 溢出
#define FeedDog WDT_CONTR = 0x3F;

#define SYSclk 22118400L
#define MODE1T                               //Timer clock mode, comment this line is 12T mode, uncomm
#ifdef MODE1T
#define T1MS (65536-SYSclk/1000)        //1ms timer calculation method in 1T mode
#else
#define T1MS (65536-SYSclk/12/1000)   //1ms timer calculation method in 12T mode
#endif

//定义eeprom起始地址
#define EEPROM_ADDRESS 0x2000

//看门狗定义
//看门狗溢出时间	=	(12 × Pre-scale × 32768) / 12000000 = Pre-scale× 393216 / 12000000
//#define FeedDog WDT_CONTR = 0x3F;

//系统参数
typedef struct
{
  unsigned int set_temper;      	//设定温度
  unsigned char ad_zeroValueFix;    //AD调零值 用于在单电源情况下将ad输出调0
  unsigned char ad_voltageScale;    //AD值到电压值比例 单位uv
  int	
        	pGain,    	// proportional gain
			iGain,    	// integral gain
         	dGain;     	// derivative gain
} SysConfig;

typedef struct
{
  int dState;      	// Last position input
  int iState;      	// Integrator state
  int	iGain,    	// integral gain
        	pGain,    	// proportional gain
         	dGain;     	// derivative gain
} SPid;


//最大PWM加热时间
#define MAX_PWM_TIME 100

void GetADCResult(void);
void UART_Command(void);
void StartWork(void);
void StopWork(void);
int UpdatePID(int error, int position);

#endif