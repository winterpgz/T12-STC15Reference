/*****************STC15F204EA单片机旋转编码器版白光T12控制器代码(by金向维)***********************/
#include "STC15F204EA.h"//单片机头文件,24MHz时钟频率
#include "INTRINS.h"//头文件

//共阴数码管段码数据(0,1,2,3,4,5,6,7,8,9),倒数第二个是显示负号-的数据,倒数第一个是显示字母P的数据
unsigned char code duanma[12]= { 0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x40,0x73 };

//根据NTC电阻随温度变化进而引起电压变化得出的数据,用来查表计算室温(进而对热电偶冷端补偿)
unsigned int code wendubiao[62] = { 924,959,996,1033,1071,1110,1150,1190,1232,1273,1315,1358,1401,1443,1487,1501,1574,1619,1663,1706,1751,1756,1776,1810,1853,1903,1958,2017,2078,2141,2204,2266,2327,2387,2444,2500,2554,2607,2657,2706,2738,2800,2844,2889,2931,2974,3016,3056,3098,3139,3179,3218,3257,3296,3333,3372,3408,3446,3484,3519,3554,3590 };

sbit dot = P2 ^ 7; //数码管的小数点接P2.7
sbit t12 = P2 ^ 0; //T12通过P2.0控制
sbit bw = P3 ^ 4; //数码管百位位选为P3.4
sbit sw = P3 ^ 5; //数码管十位位选为P3.5
sbit gw = P3 ^ 6; //数码管个位位选为P3.6

sbit tihuan = P3 ^ 7; //数码管的a段本应该用P1.0控制,由于P1.0被用来控制T12,所以要用P3.7替代P1.0
sbit encoderb = P1 ^ 4; //编码器的b脚接P1.4
sbit encodera = P3 ^ 2; //编码器的a脚接P3.2
sbit zhendongkaiguan = P0 ^ 1; //震动开关接P0.1
sbit bianmaanniu = P3 ^ 3; //编码器的按键接P3.3

sbit a7 = P2 ^ 7;
sbit a6 = P2 ^ 6;
sbit a5 = P2 ^ 5;
sbit a4 = P2 ^ 4;
sbit a3 = P2 ^ 3;
sbit a2 = P2 ^ 2;
sbit a1 = P2 ^ 1;
bit e = 1, f = 1; //e,f用来保存编码器上一次的状态
bit huancunkaiguan = 0; //用于改变设定温度后延时显示设定温度(而不是立刻显示t12温度)

signed int huancun; //显示函数直接显示huancun,要显示一个数据将必须这个数据赋值给缓存(由于数码管只有三位,为了在显示三位数同时保持四位数的精度,所以实际显示的是数据除以10,支持显示负数)
signed int shiwen; //10倍实际室温,即实际室温乘以10(为了精确)(允许的室温范围为-11度至50度)
signed int t12wendu; //T12烙铁头的实际温度(非热电偶的温差)(同样为10倍温度)
signed int shedingwendu; //设定温度(范围200~450度)
signed int wencha; //T12两个周期间的温差
signed int jiareshu; //每200ms加热周期内需要加热的次数(一次等于1ms,相当于加热占空比)
unsigned char zhouqijishu; //加热周期200ms计数
unsigned int huancunjishu; //用于改变设定温度后保持显示设定温度一段时间再显示t12温度(而不是立刻显示t12温度)
unsigned long cankaodianya0, t12dianya, ntcdianya, dianyuandianya;

/********************************1ms延时函数*************************************************/
void delay_ms(unsigned int a) //24MHz时钟时的1毫秒延时函数
{
    unsigned int b;
    while (a--)
    {
        for (b = 0; b < 1200; b++)
            ;
    }
}

/********************************10us延时函数************************************************/
void delay_10us(unsigned int a) //24MHz时钟时的10微秒延时函数
{
    unsigned int b;
    while (a--)
    {
        for (b = 0; b < 12; b++)
            ;
    }
}

/********************************数码管延时关断函数******************************************/
void guanduan(void) //用于关断数码管的位选
{
    delay_ms(1); //延时
    bw = 1; //关断百位
    sw = 1; //关断十位
    gw = 1; //关断个位
}

/********************************公共函数10(显示)********************************************/
void gonggonghanshu10(unsigned char a)
{
    a7 = a & 0x80;
    a6 = a & 0x40;
    a5 = a & 0x20;
    a4 = a & 0x10;
    a3 = a & 0x08;
    a2 = a & 0x04;
    a1 = a & 0x02;
    tihuan = a & 0x01;
}

/********************************显示函数****************************************************/
void display(signed int a) //显示函数(显示实际数据除以10,支持显示负数,负数显示方式为百位显示负号"-"十位和个位显示数据)
{
    unsigned char baiwei, shiwei, gewei, d; //定义百位,十位,个位,每次显示帧数
    signed int c; //用于处理数字a
    if (a < 0) //如果a是负数
        c = -a; //取a的相反数
    else
        //否则
        c = a; //就直接取a
    c = c / 10;
    baiwei = c / 100; //计算百位
    c = c % 100;
    shiwei = c / 10; //计算十位
    c = c % 10;
    gewei = c; //计算个位
    for (d = 0; d < 20; d++) //显示部分,每次显示20个循环(20帧)
    {
        if (a < 0) //如果a是负数,则百位显示负号
            gonggonghanshu10(duanma[10]);
        else
            //否则直接显示百位
            gonggonghanshu10(duanma[baiwei]); //显示百位
        bw = 0; //打开百位
        guanduan(); //延时关断百位
        gonggonghanshu10(duanma[shiwei]); //显示十位
        sw = 0; //打开十位
        guanduan(); //延时关断十位
        gonggonghanshu10(duanma[gewei]); //显示个位
        gw = 0; //打开个位
        guanduan(); //延时关断个位
    }
}

/********************************ADC公共函数**************************************************/
void gonggonghanshu2(void) //此函数测量单片机电源电压
{
    ADC_CONTR = 0x88; //ADC_POWER, SPEED1, SPEED0, ADC_FLAG---ADC_START, CHS2, CHS1, CHS0
    delay_10us(2); //延时等待转换结束
    ADC_RESL = ADC_RESL & 0x03; //取转换结果低八位中的低二位
    cankaodianya0 = (ADC_RES * 4 + ADC_RESL); //把结果转换成十进制数据(10位ADC,最大值1023)
    dianyuandianya = 2549760 / cankaodianya0; //计算电源电压,单位mV
}

/********************************ADC测电压函数************************************************/
void adc(void) //ADC函数,用于测量和计算各种电压
{
    signed char a; //查NTC表用
    gonggonghanshu2(); //公共函数2(此函数功能是测量电源电压,单位mV)

    ADC_CONTR = 0x89; //ADC_POWER, SPEED1, SPEED0, ADC_FLAG---ADC_START, CHS2, CHS1, CHS0 转换采用最低速度速,低速更精确(测量t12电压务必使用最低速度AD转换,实测高速误差大)
    delay_10us(2);
    ADC_RESL = ADC_RESL & 0x03;
    t12dianya = (ADC_RES * 4 + ADC_RESL);
    t12dianya = 2490 * t12dianya / cankaodianya0; //计算t12电压,单位mV
    ADC_CONTR = 0x8a; //ADC_POWER, SPEED1, SPEED0, ADC_FLAG---ADC_START, CHS2, CHS1, CHS0
    delay_10us(2);
    ADC_RESL = ADC_RESL & 0x03;
    ntcdianya = (ADC_RES * 4 + ADC_RESL);
    ntcdianya = 2490 * ntcdianya / cankaodianya0; //计算ntc电压,单位mV
    for (a = 0; wendubiao[a] < ntcdianya; a++) //查表计算室温
    {
        if (a >= 61) //如果超出表的范围就取允许的最高温度(50度)
            break; //并且退出查表
    }
    shiwen = (a - 11) * 10; //得出室温(实际室温乘以10)
    t12wendu = (t12dianya - 100) * 43 * 10 / 260 + shiwen; //计算t12的实际温度,其中260为运放增益
    if (t12wendu < shiwen) //如果t12温度小于室温
        t12wendu = shiwen; //就取室温
    if (t12wendu > 5000) //如果得出的温度超过500度,说明没有插入烙铁头或参数错误(因为烙铁头的温度不可能超过500度)
        t12wendu = 5000; //显示500作为错误指示(注意显示函数显示的是1/10,所以要显示500,需要赋值5000)
    if (huancunkaiguan == 1) //如果缓存开关开,说明刚刚改变了设定温度
        huancun = shedingwendu; //于是显示设定温度(而不是t12温度)
    else
        huancun = t12wendu; //否则直接显示t12温度
}

/********************************定时器0初始化函数*******************************************/
void timer0init(void) //定时器0初始化程序,24MHz频率下,每1ms中断一次
{
    TMOD = 0x00; //定时模式,16位自动重装
    TH0 = 0xf8; //计时1ms
    TL0 = 0x2f;
    ET0 = 1; //开启定时器0中断
    TR0 = 1; //启动定时器0
}

/********************************公共函数6(记录编码器状态)**********************************/
void gonggonghanshu6(void)
{
    e = encodera; //记录编码器a脚此次状态
    f = encoderb; //记录编码器b脚此次状态
}

/********************************编码器函数(正常加热模式调用)********************************/
void bianmaqi(void)
{
    if (e == 1 && f == 1 && encodera == 1 && encoderb == 0) //和前一次状态比较确定为右旋
    {
        shedingwendu = shedingwendu + 100; //步进
        if (shedingwendu > 4500) //最高允许450度
            shedingwendu = 4500;
        huancun = shedingwendu; //显示改变后的设定温度
        huancunkaiguan = 1; //打开缓存开关(用于延时显示设定温度1.5秒)
        huancunjishu = 0; //重新开始缓存计数
    }
    if (e == 1 && f == 1 && encodera == 0 && encoderb == 1) //和前一次状态比较确定为左旋
    {
        shedingwendu = shedingwendu - 100; //步进
        if (shedingwendu < 2000) //最低允许200度
            shedingwendu = 2000;
        huancun = shedingwendu; //显示改变后的设定温度
        huancunkaiguan = 1; //打开缓存开关(用于延时显示设定温度1.5秒)
        huancunjishu = 0; //重新开始缓存计数
    }
    gonggonghanshu6(); //记录编码器状态
}

/********************************定时器0中断函数********************************************/
void timer0(void)
interrupt 1 //定时0中断函数,用于检测旋转编码器,掉电存储等操作
{
    unsigned char buchang;
    bianmaqi(); //调用编码器函数
    if(huancunkaiguan==1)//延时显示计数
    huancunjishu++;
    zhouqijishu++;//加热周期计数
    if(jiareshu>190)//最多加热190ms
    jiareshu=190;
    if(zhouqijishu<=jiareshu)//如果当前计数小于等于加热数
    t12=1;//就加热
    else//否则
    t12=0;//不加热
    if(t12wendu==5000)//如果t12温度为500,说明没有插入烙铁头或参数严重错误
    t12=0;//停止加热
    if(huancunjishu==1500)//设定温度延时显示1.5秒
    {
        huancunkaiguan=0; //如果达到了1.5秒,关闭缓存开关
        huancunjishu=0;//停止缓存计数
        huancun=t12wendu;//由显示设定温度改为显示t12温度
    }
    if(zhouqijishu==200) //t12停止加热后10ms再检测温度
    {
        adc(); //检测电压,计算温度
        zhouqijishu=0;//重新开始加热周期计数    //////////////////////////以下为加热算法(请自行理解,不作注释)//////////////////////
        if(t12wendu>shedingwendu)
        {
            if(t12wendu-shedingwendu<=20)
            jiareshu=(shedingwendu-1500)/160;
            else
            jiareshu=0;
        }
        if(t12wendu<=shedingwendu)
        {
            wencha=shedingwendu-t12wendu;
            if(wencha>20)
            {
                buchang++;
                if(buchang>150)
                buchang=150;
            }
            else
            buchang=0;
            if(shedingwendu-t12wendu>=300)
            jiareshu=190;
            else if(shedingwendu-t12wendu>=200)
            jiareshu=160;
            else if(shedingwendu-t12wendu>=150)
            jiareshu=130;
            else if(shedingwendu-t12wendu>=100)
            jiareshu=90+wencha/2+buchang;
            else if(shedingwendu-t12wendu>=50)
            jiareshu=50+buchang*2;
            else
            jiareshu=(shedingwendu-1000)/80+wencha*2/3+buchang;
        }
    }
}

/********************************主函数*****************************************************/
void main(void) //主函数
{
    P1M0 = 0x00; //P1除P1.0,P1.1,P1.2为输入模式外均为正常模式
    P1M1 = 0x07;
    P1ASF = 0x07; //设置P1相应ADC转换的I/O口为ADC输入模式
    P2M0 = 0xff; //P2都是推挽模式
    P2M1 = 0x00;
    P3M0 = 0xf0; //P3.4,P3.5,P3.6,P3.7为推挽模式,P3.2,P3.3为输入模式,其余正常模式
    P3M1 = 0x06;
    ADC_CONTR = 0xe0; //打开ADC电源
    shedingwendu = 3000; //设为300度
    IE = 0x88; //打开定时器0中断,关闭定时器1中断
    timer0init(); //初始化定时器0
    while (1)
    {
        display(huancun); //数码管显示数据
    }
}
