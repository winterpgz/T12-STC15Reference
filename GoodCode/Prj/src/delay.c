#include "delay.h"
#include <intrins.h>

/* 时钟频率 22.1184MHz */

void delayus(unsigned char t)
{
	unsigned char i;
	while(t--)
	{
		_nop_();
		_nop_();
		i = 2;
		while (--i);
	}
}

void delayms(unsigned int t)
{
	unsigned char i, j;
	while(t--)
	{
		i = 22;
		j = 128;
		do
		{
			while (--j);
		} while (--i);
	}
}