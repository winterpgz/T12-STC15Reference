#include<STC15F104E_PLUS.H>
#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include "main.h"
#include "delay.h"
#include "uart.h"

//���崮ת���˿ڿ�λѰַ
//UCHAR bdata PORT1, PORT2;

//�˿ڶ���
sbit RA1=P3^3; //���ݳ�ŵ�
sbit RA0=P3^2; //�Ƚ�����ƽ
sbit CTRL=P3^5; //T12���ȿ���

sbit LED=P3^4;  //led���� ����ʹ��

bit bSendData; //�������ݵ�����
unsigned int ADCResult;	//AD����ֵ

SysConfig config; //ϵͳ����
SPid pid; // PID ���ƽṹ

unsigned char high_time, max_time,count;//ռ�ձȵ��ڲ���
unsigned int tickCount; //ϵͳ��ʱ��

int pTerm, dTerm, iTerm, drive;

void main (void)
{
    //���жϿ���
    EA = 1;

    //��ʱ����ʼ�� ��ʱ��0����AD�����봮�ڲ��� ��ʱ��1����ϵͳ����pwm������
    //���ö�ʱ��0, 1������1Tģʽ
    AUXR = 0xC0;
    //�ö�ʱ��0, 1 ����ģʽ0 16λ�Զ���װ��ʱ��
    TMOD = 0x0;

    //��T12���ȿ��ƿ�����Ϊ������� ������20ma ������Ҫ��������
    P3M1 = P3M1 & 0x1F;
    P3M0 = P3M0 | 0x20;
    //�ر�T12����
    CTRL = 0;

    /*����: ��ʼ��AD���*/
    RA1 = 1; //���ݷŵ�
    //�����ݷŵ������Ϊ������� ������20ma ������Ҫ��������
    P3M1 = P3M1 & 0x30;
    P3M0 = P3M0 | 0x08;

    while(1)
    {
        //���ڳ�ʼ��
        UartInit();
        //�ȴ���������
        while( uartDataSize < 16);
        UART_Command();
        if (TR1)
        {
            //����һ��ʱ���ֹͣ����
            bSendData = 0;
            while (tickCount < 30000)
            {
                if (bSendData)
                {
                    bSendData = 0;
                    //�򴮿ڷ�������
                    UartInit();
                    UART_SendByte(0xff);
                    UART_SendByte(0x55);
                    UART_SendByte(0xc);
                    UART_SendByte(0xff);
                    UART_SendBuf( (unsigned char *)&ADCResult, 2);
                    UART_SendByte( high_time);	//����ʱ��
                    UART_SendByte( max_time);
                    UART_SendBuf( (unsigned char *)&pTerm, 2);
                    UART_SendBuf( (unsigned char *)&iTerm, 2);
                    UART_SendBuf( (unsigned char *)&dTerm, 2);
                    UART_SendBuf( (unsigned char *)&drive, 2);

                }
            }
            StopWork();
        }
    }
}

void StartWork(void)
{
    /*����: ��ʼ����*/
    tickCount = 0;
    count = 0;
    max_time = MAX_PWM_TIME;
    high_time = MAX_PWM_TIME; //0-100
    memset ( &pid,0,sizeof(SPid));
    /*pid.pGain = 40; // Set PID Coefficients  �������� Proportional Const
    pid.iGain = 1;    //���ֳ��� Integral Const
    pid.dGain = 50;   //΢�ֳ��� Derivative Const
    */
    pid.pGain = config.pGain; // Set PID Coefficients  �������� Proportional Const
    pid.iGain = config.iGain;    //���ֳ��� Integral Const
    pid.dGain = config.dGain;   //΢�ֳ��� Derivative Const

    //��ʱ��1��ʼ��
    //2����@22.1184MHz ��ʱ��ʱ��1Tģʽ	16λ�Զ���װ��ʱ��
    TL1 = 0x33;		//���ö�ʱ��ֵ
    TH1 = 0x53;		//���ö�ʱ��ֵ
    //1����@33.1776MHz
    //TL1 = 0x66;		//���ö�ʱ��ֵ
    //TH1 = 0x7E;		//���ö�ʱ��ֵ

    TF1 = 0;		//���TF1��־
    PT1	= 1;        //���ж����ȼ�
    ET1 = 1;        //�����ж�
    TR1 = 1;		//��ʱ��1��ʼ��ʱ
}

void StopWork(void)
{
    /*����: ֹͣ����*/
    TR1 = 0;
    //�ر�T12����
    CTRL = 0;
    LED = 1;	  //�ر�led
}

void UART_Command(void)
{
    /*����; ִ�д����������*/
    UCHAR cmd;

    uartDataSize = 0;

    //�������ͷ FF 55 00 00 ... 55 FF
    if (!((uartbuf[0] == 0xFF) && (uartbuf[1] == 0x55)	&& (uartbuf[14] == 0x55) && (uartbuf[15] == 0xFF)))
    {
        //����ͷ����
        return;
    }

    cmd = uartbuf[2];
    switch ( cmd)
    {
    case 0:  //��ʼ����
        //ϵͳ����
        memcpy(&config, &uartbuf[3], sizeof(SysConfig));
        uartbuf[2] = 0;	 //���ݴ�С
        uartbuf[3] = 0;	 //����ID
        UART_SendBuf(uartbuf, 4); //ȷ�ϼ���
        StartWork();
        break;
    case 1: //ֹͣ����
        uartbuf[2] = 0;	 //���ݴ�С
        uartbuf[3] = 1;
        UART_SendBuf(uartbuf, 4);
        StopWork();
        break;
    }
}

void GetADCResult(void)
{
    /*
    ����:
    ��ȡADC�����ؽ�� ģʽ ���˿ڵ�ƽ
    ͨ��LM258�Ƚ���ʵ��AD
    ��ʱ�����ʱ����� ��λms ��ϵͳʱ�� SYSclk = 22118400
    1Tģʽ
    1000/(22118400/65536)	  = 2.96
    12Tģʽ
    1000/(22118400/12/65536)  = 35.55
    Multisim ������
    103 ��絽2.5v Լ0.952ms  T1�¸����ٶ�
    223 ��絽2.5v Լ2.104ms  T1�¸��߾���
    104 ��絽2.5v Լ9.947ms  T12ģʽ
    */

    //����׼��ѹ
    //EA = 0;   //�ر�ȫ���ж�
    TR0=0;	  //�رռ�ʱ��
    ET0 = 0; //�������ж�

    TF0=0;    //�����λ
    TH0=0x0;  //������ֵ
    TL0=0x0;
    RA1 = 0; //���ݳ��
    TR0=1; //��ʼ����
    //�ȴ����ݵ�ѹ���׼��ѹ��ͬ���߶�ʱ�����
    while(!RA0 && !TF0);

    TR0=0;	  //�رռ�ʱ��
    //EA = 1;   //����ȫ���ж�
    RA1 = 1; //���ݷŵ�

    if (TF0)
    {
        //��ʱ����� ��׼��ѹ̫��? ���ݳ���ѹ̫��? ����̫��?
        //ADCResult = 0xfffe;
        ADCResult = 0x400 - config.ad_zeroValueFix;
    }
    else
    {
        //ֻȡ����ֵ��10λ
        ADCResult = ((unsigned int)TH0 << 2) | (TL0 >> 6);
        /*
        ��������
        ��Ϊ�˷���ʧ����ѹlm358Ϊ3mv, ���Ե���Դ��Ҫ����. ��Ϊ3�Ž�10K��������Ҳ����ѹ��.
        ������T12����ͷΪ����ʱ��ֵ��Ϊ0ֵ. �� T=T0 �²����=0
        */
        if (ADCResult >= config.ad_zeroValueFix)
            ADCResult = ADCResult-config.ad_zeroValueFix;
        else
            ADCResult = 0;
    }
    //delayms( 2); //�ȴ����ݷŵ���� ��������������ÿ���ע�͵�

    //�ŵ��ӳ� ��ֵ��Χ
    //1ms 4
    //2ms 5
    //3ms 6
    //5ms 7
    //20ms 9
    //100ms 7

    //�����ѹ=�ο���ѹ*�����ѹ��ʱ��ֵ/�ο���ѹ��ʱ��ֵ
    //V1=Vref*T2/T1
}

int UpdatePID(int error, int position)
{
    /*����: PID����*/
    pTerm = pid.pGain * error;   // calculate the proportional term

    // calculate the integral state with appropriate limiting
    pid.iState += error;
    // Maximum and minimum allowable integrator state
    if (pid.iState < 0) pid.iState = 0;
    if (pid.iState > 500) pid.iState = 100;
    //else if (pid.iState < pid.iMin) pid.iState = pid.iMin;

    iTerm = (pid.iGain * pid.iState) / 100;  // calculate the integral term

    dTerm = pid.dGain * (pid.dState - position) ;
    pid.dState = position;

    //drive = pTerm + dTerm + iTerm;
    drive = pTerm + iTerm + dTerm;

    if (drive<0)
        return 0;
    else if (drive>100)
        return 100;
    else
        return drive;
}

void Timer1_Interrupt(void) interrupt 3
{
    /*����: ���ڼ�ʱ,PWM��*/
    int error;

    tickCount++;
    count++; //PWM������

    //PWM��PID����
    if (count<=max_time)
    {
        //PWM����
        if (count <= high_time)
        {
            CTRL = 1; //����
            LED = 0;	  //����led
        }
        else
        {
            CTRL = 0; //ֹͣ
            LED = 1;	  //�ر�led
        }
    }
    else
    {
        TR1 = 0; //�رն�ʱ��
        CTRL = 0; //ֹͣ����
        LED = 1;	  //�ر�led

        if (high_time > 98)
            delayms(4); //�ȴ�T12�ϵĵ���й�Ÿɾ�

        GetADCResult();	 //��ȡADֵ

        //PID����
        max_time = MAX_PWM_TIME;
        error = (int)config.set_temper-(int)ADCResult;
        if(error>40)  //���õ��¶ȱ�ʵ�ʵ��¶��Ƿ��Ǵ���n��
        {
            //ȫ���ʼ��� 500ms
            max_time = 250;
            high_time = 250;
        }
        else if(error<-40)
        {
            //������
            max_time = 100;
            high_time = 0;
        }
        else  //�������n�ȷ�Χ�ڣ�������PID����
        {
            high_time = UpdatePID(error,  ADCResult);
        }


        bSendData = 1;

        count = 0;
        TR1 = 1; //������ʱ��
    }
}