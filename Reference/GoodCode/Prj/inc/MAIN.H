#ifndef __MAIN_H__
#define __MAIN_H__

typedef unsigned char UCHAR;
typedef unsigned int UINT;
typedef unsigned long ULONG;

//���Ź����� 5s ���
#define FeedDog WDT_CONTR = 0x3F;

#define SYSclk 22118400L
#define MODE1T                               //Timer clock mode, comment this line is 12T mode, uncomm
#ifdef MODE1T
#define T1MS (65536-SYSclk/1000)        //1ms timer calculation method in 1T mode
#else
#define T1MS (65536-SYSclk/12/1000)   //1ms timer calculation method in 12T mode
#endif

//����eeprom��ʼ��ַ
#define EEPROM_ADDRESS 0x2000

//���Ź�����
//���Ź����ʱ��	=	(12 �� Pre-scale �� 32768) / 12000000 = Pre-scale�� 393216 / 12000000
//#define FeedDog WDT_CONTR = 0x3F;

//ϵͳ����
typedef struct
{
  unsigned int set_temper;      	//�趨�¶�
  unsigned char ad_zeroValueFix;    //AD����ֵ �����ڵ���Դ����½�ad�����0
  unsigned char ad_voltageScale;    //ADֵ����ѹֵ���� ��λuv
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


//���PWM����ʱ��
#define MAX_PWM_TIME 100

void GetADCResult(void);
void UART_Command(void);
void StartWork(void);
void StopWork(void);
int UpdatePID(int error, int position);

#endif