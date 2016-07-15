//#include "STC15F204EA.h"//单片机头文件,24MHz时钟频率
//#include "INTRINS.h"
#include <stc12.h>

//共阴数码管段码数据(0,1,2,3,4,5,6,7,8,9),倒数第二个是显示负号-的数据,倒数第一个是显示字母P的数据
//unsigned char code duanma[12] = { 0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x40,0x73 };
unsigned char duanma[12] = { 0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x40,0x73 };

//根据NTC电阻随温度变化进而引起电压变化得出的数据,用来查表计算室温(进而对热电偶冷端补偿)
//unsigned int code wendubiao[62] = { 924,959,996,1033,1071,1110,1150,1190,1232,1273,1315,1358,1401,1443,1487,1501,1574,1619,1663,1706,1751,1756,1776,1810,1853,1903,1958,2017,2078,2141,2204,2266,2327,2387,2444,2500,2554,2607,2657,2706,2738,2800,2844,2889,2931,2974,3016,3056,3098,3139,3179,3218,3257,3296,3333,3372,3408,3446,3484,3519,3554,3590};
unsigned int wendubiao[62] = { 924,959,996,1033,1071,1110,1150,1190,1232,1273,1315,1358,1401,1443,1487,1501,1574,1619,1663,1706,1751,1756,1776,1810,1853,1903,1958,2017,2078,2141,2204,2266,2327,2387,2444,2500,2554,2607,2657,2706,2738,2800,2844,2889,2931,2974,3016,3056,3098,3139,3179,3218,3257,3296,3333,3372,3408,3446,3484,3519,3554,3590};

//白光控制器的默认参数
//unsigned char code moren[9]= { 0,230,100,41,10,3,10,0,0 };
unsigned char moren[9]= { 0,230,100,41,10,3,10,0,0 };

sbit at 0x80 dot = P2 ^ 7; //数码管的小数点接P2.7
sbit t12 = P2 ^ 0; //T12通过P2.0控制
sbit bw = P3 ^ 4; //数码管百位位选为P3.4
sbit sw = P3 ^ 5; //数码管十位位选为P3.5
sbit gw = P3 ^ 6; //数码管个位位选为P3.6
sbit tihuan = P3 ^ 7; //数码管的a段本应该用P1.0控制,由于P1.0被用来控制T12,所以要用P3.7替代P1.0
sbit encoderb = P1 ^ 4; //编码器的b脚接P1.4
sbit encodera = P3 ^ 2; //编码器的a脚接P3.2
sbit zhendongkaiguan = P0 ^ 1; //震动开关接P0.1
sbit bianmaanniu = P3 ^ 3; //编码器的按键接P3.3
bit e = 1, f = 1, g; //e,f用来在interrupt 1中保存上一次的编码器状态,用于和现在的状态比较,从而得出左旋还是右旋
bit huancunkaiguan = 0; //用于改变设定温度后显示设定温度一段时间再显示t12温度(而不是立刻显示t12温度)
bit xiumiankaiguan = 0; //定义休眠开关
bit xiumianjishukaiguan = 0; //定义休眠计数开关
bit ganggangkaiji = 1; //定义刚刚开机(用于确保刚开机未震动手柄能进入休眠状态,因为每次休眠计时是通过震动传感器状态改变触发的,而刚开机手柄没有震动所以要特殊处理)
bit guanjikaiguan = 0; //定义关机开关
bit guanjijishukaiguan; //定义关机计数开关
bit huifumoren = 0; //是否恢复默认参数
bit huanxingfangshi = 0; //如何从关机模式唤醒
bit shezhimoshi = 0; //设置模式还是正常工作模式
bit shezhixianshijishukaiguan; //用于设置模式延时显示P01,P02等等菜单
bit jinzhicaozuo = 1; //用于设置模式下某时刻禁止编码器操作
bit xianship; //用于设置模式下显示菜单P01,P02等的字母P

signed char wenduxiuzheng; //温度修正参数
signed int huancun; //显示函数直接显示huancun,要显示一个数据将必须这个数据赋值给缓存(由于数码管只有三位,为了在显示三位数同时保持四位数的精度,所以实际显示的是数据除以10,支持显示负数.但是在参数设置模式下显示的是实际值,不除以10)
signed int shiwen; //10倍实际室温,即实际室温乘以10(为了精确)(允许的室温范围为-11度至50度)
signed int t12wendu; //T12烙铁头的实际温度(非热电偶的温差)(同样为10倍温度)
signed int shedingwendu; //设定温度(范围200~450度)
signed int leijiwencha;

unsigned char shedingwenduh, shedingwendul; //设定温度高八位,第低八位,只在掉电存储,上电恢复时用到(因为温度值是两个字节,所以保存到EEPROM需要两个char数字)
unsigned char zhouqijishu; //加热周期200ms计数
unsigned char zengyih; //运算放大器增益高八位
unsigned char zengyil; //运算放大器增益低八位
unsigned char shitiaodianya; //运算放大器失调电压修正(指将放大器的输入接地,放大器输出的电压,单位mV)
unsigned char redianou; //热电偶的℃/mV数据(指热电偶电压每升高1mV需要温升多少度,本人测定的t12原装二手头的数据为41,供大家参考,不懂勿乱调)
unsigned char wendubujin; //温度步进(范围0,1,2,5,10其中步进设为0可以锁定烙铁温度,防止熊孩子或者不懂的人乱调高温度烧坏t12头子)
unsigned char xiumianshijian; //多长时间无动作进入休眠,单位min(分钟)
unsigned char jinduzhi; //设置模式中设置到了P几(P00,P01,P02等等)
unsigned char guanjishijian; //进入休眠模式后多长时间进入关机模式,单位min(分钟)

unsigned int jiareshu; //每200ms加热周期内需要加热的次数(一次等于1ms,相当于加热占空比)
unsigned int huancunjishu; //用于改变设定温度后保持显示设定温度一段时间再显示t12温度(而不是立刻显示t12温度)
unsigned int baocunwendu; //用于进入休眠时保存退出休眠时恢复原来的设定温度
unsigned int zengyi; //运算放大器增益
unsigned int shezhixianshijishu; //设置模式中用于延时显示菜单项
unsigned long cankaodianya0, t12dianya, ntcdianya, dianyuandianya; //用于参考电压(ADC后的直接数据,未转换为mV),计算电源电压,热电偶经放大器放大后的电压和NTC电阻确定的室温电压(单位均为mV)
unsigned long xiumianjishu, xiumianjishu2; //等待多长时间进入休眠状态(单位ms)
unsigned long guanjijishu; //等待多长时间进入关机状态(单位ms)

/**************************EEPROM公共代码*********************************/
void gonggongdaima(void) //所有的公共代码都是因为多次使用到了,而把它们整理起来再调用可以减小编译后的文件大小(迫不得已的做法,因为单片机的FLASH太小了,否则装不下程序)
{
    IAP_TRIG = 0x5a; //发送5ah到触发寄存器
    IAP_TRIG = 0xa5; //发送a5h到触发寄存器
    _nop_(); //延时
    IAP_CONTR = 0; //关闭IAP 功能
    IAP_CMD = 0; //清空命令寄存器
    IAP_TRIG = 0; //清空命令触发寄存器
    IAP_ADDRH = 0; //清空地址高位
    IAP_ADDRL = 0; //清空地址低位
}

/**************************字节读函数***************************************/
unsigned char Byte_Read(unsigned int add)
{
    IAP_DATA = 0x00; //清空数据
    IAP_CONTR = 0x81; //打开IAP,设置操作等待时间
    IAP_CMD = 0x01; //字节读命令
    IAP_ADDRH = add >> 8; //设置高8位地址
    IAP_ADDRL = add & 0x00ff; //设置低8位地址
    gonggongdaima();
    return (IAP_DATA); //返回读到的数据
}

/*****************************字节编程函数*****************************/
void Byte_Program(unsigned int add, unsigned char dat)
{
    IAP_CONTR = 0x81; //打开IAP,设置操作等待时间
    IAP_CMD = 0x02; //字节编程命令
    IAP_ADDRH = add >> 8; //设置高8位地址
    IAP_ADDRL = add & 0x00ff; //设置低8位地址
    IAP_DATA = dat; //要编程的数据先送进IAP_DATA 寄存器
    gonggongdaima();
}

/*****************************扇区擦除函数****************************/
void Sector_Erase(unsigned int add)
{
    IAP_CONTR = 0x81; //打开IAP,设置操作等待时间
    IAP_CMD = 0x03; //扇区擦除命令
    IAP_ADDRH = add >> 8; //设置高8位地址
    IAP_ADDRL = add & 0x00ff; //设置低8位地址
    gonggongdaima();
}

/****************************数码管关断函数*************************/
void guanduan(void) //用于关断数码管的位选
{
    bw = 1; //关断百位
    sw = 1; //关断十位
    gw = 1; //关断个位
}

/********************************1ms延时函数**********************************/
void delay_ms(unsigned int a) //24MHz时钟时的1毫秒延时函数
{
    unsigned int b;
    while (a--)
    {
        for (b = 0; b < 1200; b++)
            ;
    }
}

/*****************************10us延时函数****************************************/
void delay_10us(unsigned int a) //24MHz时钟时的10微秒延时函数
{
    unsigned int b;
    while (a--)
    {
        for (b = 0; b < 12; b++)
            ;
    }
}

/********************************显示函数*************************************/
void display(signed int a) //显示函数(显示实际数据除以10,支持显示负数,负数显示方式为百位显示负号"-"十位和个位显示数据)
{
    unsigned char baiwei, shiwei, gewei, d; //定义百位,十位,个位等
    signed int c; //用于处理数字a
    bit e; //用于保存t12的当前状态(加热还是关闭)
    if (a < 0) //如果a是负数,取a的相反数
        c = -a;
    else
        //否则就直接取a
        c = a;
    if (guanjikaiguan == 1) //如果是关机状态,就显示000
        c = 0;
    if (shezhimoshi == 0) //如果是正常模式就显示数据的十分之一,如果是设置模式直接显示数据
        c = c / 10;
    baiwei = c / 100; //计算百位
    c = c % 100;
    shiwei = c / 10; //计算十位
    c = c % 10;
    gewei = c; //计算个位
    for (d = 0; d < 30; d++) //显示部分,每次显示30个循环(30帧)
    {
        if (a < 0) //如果a是负数,则百位显示负号
        {
            e = t12; //保存t12状态到e
            P2 = duanma[10]; //此操作可能会关闭t12
            t12 = e; //所以立刻恢复t12状态
            tihuan = 0;
        }
        else if (shezhimoshi == 1 && xianship == 1) //在设置模式显示菜单项时,百位显示字母P
        {
            e = t12; //保存t12状态到e
            P2 = duanma[11]; //此操作可能会关闭t12
            t12 = e; //所以立刻恢复t12状态
            tihuan = 1;
        }
        else //否则直接显示百位
        {
            e = t12; //同上
            P2 = duanma[baiwei]; //显示段码
            t12 = e; //同上
            if (duanma[baiwei] & 0x01) //显示替换的段码
                tihuan = 1;
            else
                tihuan = 0;
        }
        bw = 0; //打开百位
        delay_ms(1); //延时
        guanduan(); //关断百位

        e = t12; //同上
        P2 = duanma[shiwei]; //显示段码
        t12 = e; //同上
        if (duanma[shiwei] & 0x01) //显示替换的段码
            tihuan = 1;
        else
            tihuan = 0;
        sw = 0; //打开十位
        delay_ms(1); //延时
        guanduan(); //关断十位

        e = t12; //同上
        P2 = duanma[gewei]; //显示段码
        t12 = e; //同上
        if (duanma[gewei] & 0x01) //显示替换的段码
            tihuan = 1;
        else
            tihuan = 0;
        if (xiumiankaiguan == 1 && guanjikaiguan == 0) //如果是休眠状态,就显示个位的小数点作为休眠指示
            dot = 1;
        gw = 0; //打开个位
        delay_ms(1); //延时
        guanduan(); //关断个位
    }
}

/***************************ADC公共函数**********************************/
void gonggonghanshu2(void) //此函数测量单片机电源电压
{
    ADC_CONTR = 0x88; //ADC_POWER, SPEED1, SPEED0, ADC_FLAG---ADC_START, CHS2, CHS1, CHS0
    delay_10us(3); //延时等待转换结束
    ADC_RESL = ADC_RESL & 0x03; //取转换结果第八位中的低二位
    cankaodianya0 = (ADC_RES * 4 + ADC_RESL); //把结果转换成十进制数据(10位ADC,最大值1024)
    dianyuandianya = 2549760 / cankaodianya0; //计算电源电压,单位mV
}

/**************************ADC测电压函数*********************************/
void adc(void) //ADC函数,用于测量和计算各种电压
{
    signed char a; //查NTC表用
    unsigned int b;
    gonggonghanshu2(); //公共函数2(此函数功能是测量电源电压,单位mV)

    ADC_CONTR = 0x89; //ADC_POWER, SPEED1, SPEED0, ADC_FLAG---ADC_START, CHS2, CHS1, CHS0 转换采用最低速度速,低速更精确(测量t12电压务必使用最低速度AD转换,实测高速误差大)
    delay_10us(3);
    ADC_RESL = ADC_RESL & 0x03;
    t12dianya = (ADC_RES * 4 + ADC_RESL);
    t12dianya = 2490 * t12dianya / cankaodianya0; //计算t12电压,单位mV

    ADC_CONTR = 0x8a; //ADC_POWER, SPEED1, SPEED0, ADC_FLAG---ADC_START, CHS2, CHS1, CHS0
    delay_10us(3);
    ADC_RESL = ADC_RESL & 0x03;
    ntcdianya = (ADC_RES * 4 + ADC_RESL);
    ntcdianya = 2490 * ntcdianya / cankaodianya0; //计算ntc电压,单位mV

    for (a = 0; wendubiao[a] < ntcdianya; a++) //查表计算室温
    {
        if (a >= 61) //如果超出表的范围就取允许的最高温度(50度)
            break; //并且退出查表
    }
    shiwen = (a - 11) * 10; //得出室温(实际室温乘以10)

    if (t12dianya < shitiaodianya) //计算t12的实际温度(由于硬件和软件产生的误差,所以可能会计算出t12温度小于室温的异常情况)
        t12wendu = shiwen; //如果出现这种情况,就认为t12温度等于室温
    else
        t12wendu = (t12dianya - shitiaodianya) * redianou * 10 / zengyi + shiwen
                - wenduxiuzheng * 10; //计算t12的实际温度
    if (t12wendu < 0) //不允许t12温度小于0度
        t12wendu = 0;
    if (t12wendu > 5000) //如果得出的温度超过500度,说明没有插入烙铁头或参数错误(因为烙铁头的温度不可能超过500度)
        t12wendu = 5000; //显示500作为错误指示(注意显示函数显示的是1/10,所以要显示500,需要赋值5000)
    t12wendu = (t12wendu + b) / 2;
    b = t12wendu;
    if (t12wendu > 4980)
        t12wendu = 5000;
    if (huancunkaiguan == 1) //如果缓存开关开,说明刚刚改变了设定温度
        huancun = shedingwendu; //于是显示设定温度(而不是t12温度)
    else
        huancun = t12wendu; //否则直接显示t12温度
}

/***************************定时器0初始化函数********************************/
void timer0init(void) //定时器0初始化程序,24MHz频率下,每1ms中断一次
{
    TMOD = 0x00; //定时模式,16位自动重装
    TH0 = 0xf8; //计时1ms
    TL0 = 0x2f;
    ET0 = 1; //开启定时器0中断
    TR0 = 1; //启动定时器0
}

/***************************定时器1初始化函数********************************/
void timer1init(void) //定时器0初始化程序,24MHz频率下,每1ms中断一次
{
    TMOD = 0x00; //定时模式,16位自动重装
    TH1 = 0xf8; //计时1ms
    TL1 = 0x2f;
    ET1 = 1; //开启定时器1中断
    TR1 = 1; //启动定时器1
}

/****************************恢复默认值函数******************************/
void morenzhi(void) //恢复控制器的默认参数
{
    unsigned char a;
    Sector_Erase(0x0000); //擦除扇区
    for (a = 0; a < 9; a++) //一项一项的查表并存储参数
        Byte_Program(a, moren[a]);
}

/******************************编码器公共函数*********************************/
void gonggonghanshu3(void)
{
    huancun = shedingwendu; //显示改变后的设定温度
    huancunkaiguan = 1; //打开缓存开关(用于延时显示设定温度1.5秒)
    huancunjishu = 0; //重新开始缓存计数
    ganggangkaiji = 0; //已经开机
    if (xiumiankaiguan == 1) //如果原来在休眠状态
    {
        xiumiankaiguan = 0; //停止休眠
        shedingwendu = baocunwendu; //恢复休眠前的设定温度
    }
    xiumianjishukaiguan = 1; //允许新的一次休眠计时
    xiumianjishu = 0; //从0开始计时
    guanjijishukaiguan = 0; //停止关机计数
    guanjikaiguan = 0; //退出关机状态
}

/*******************************编码器函数(正常加热模式调用)************************************/
void bianmaqi(void)
{
    if (e == 1 && f == 1 && encodera == 1 && encoderb == 0) //和前一次状态比较确定为右旋
    {
        shedingwendu = shedingwendu + wendubujin * 10; //步进
        if (shedingwendu > 4500) //最高允许450度
            shedingwendu = 4500;
        gonggonghanshu3();
    }
    if (e == 1 && f == 1 && encodera == 0 && encoderb == 1) //和前一次状态比较确定为左旋
    {
        shedingwendu = shedingwendu - wendubujin * 10; //步进
        if (shedingwendu < 2000) //最低允许200度
            shedingwendu = 2000;
        gonggonghanshu3();
    }
    e = encodera; //记录编码器a脚此次状态
    f = encoderb; //记录编码器b脚此次状态
}

/****************************读取设置数据函数************************/
void duqushezhishuju(void) //读取控制器的参数
{
    zengyih = Byte_Read(0x0000); //放大器增益高八位
    zengyil = Byte_Read(0x0001); //放大器增益低八位
    zengyi = zengyih * 256 + zengyil; //计算十进制的放大器增益(倍)
    shitiaodianya = Byte_Read(0x0002); //放大器失调电压(mV)
    redianou = Byte_Read(0x0003); //热电偶℃/mV数据(30~50)
    wendubujin = Byte_Read(0x0004); //温度步进(0,1,2,5,10)
    xiumianshijian = Byte_Read(0x0005); //休眠时间(分钟,0~60,0为取消休眠)
    guanjishijian = Byte_Read(0x0006); //关机时间(分钟,0~180,0为取消关机)
    wenduxiuzheng = Byte_Read(0x0007); //温度修正(-30~+30度)
    huanxingfangshi = Byte_Read(0x0008); //关机模式唤醒方式(0指既能用震动传感器唤醒又能用编码器唤醒;1指只能用编码器唤醒)
}

/****************************保存数据函数****************************/
void baocunshuju(void) //保存控制器数据到EEPROM
{
    Sector_Erase(0x0000); //擦除扇区
    Byte_Program(0x0000, zengyih); //保存增益高八位
    Byte_Program(0x0001, zengyil); //增益低八位
    Byte_Program(0x0002, shitiaodianya); //失调电压
    Byte_Program(0x0003, redianou); //热电偶
    Byte_Program(0x0004, wendubujin); //温度步进
    Byte_Program(0x0005, xiumianshijian); //休眠时间
    Byte_Program(0x0006, guanjishijian); //关机时间
    Byte_Program(0x0007, wenduxiuzheng); //温度修正
    Byte_Program(0x0008, huanxingfangshi); //唤醒方式
}

/********************************编码器函数2*************************************/
void bianmaqi2(void)
{
    if (e == 1 && f == 1 && encodera == 1 && encoderb == 0 && jinzhicaozuo == 0) //和前一次状态比较确定为右旋
    {
        if (jinduzhi == 0) //菜单P00
        {
            huifumoren = 1;
            huancun = huifumoren; //每改变一次数值,刷新一次显示
        }
        if (jinduzhi == 1) //菜单P01
        {
            if (zengyi < 350)
                zengyi++;
            zengyih = zengyi >> 8;
            zengyil = zengyi & 0x00ff;
            huancun = zengyi; //同上,不再赘述
        }
        if (jinduzhi == 2) //菜单P02
        {
            if (shitiaodianya < 250)
                shitiaodianya = shitiaodianya + 2;
            huancun = shitiaodianya;
        }
        if (jinduzhi == 3) //菜单P03
        {
            if (redianou < 50)
                redianou++;
            huancun = redianou;
        }
        if (jinduzhi == 4) //菜单P04
        {
            if (wendubujin == 0)
            {
                wendubujin = 1;
                huancun = wendubujin;
            }
            else if (wendubujin == 1)
            {
                wendubujin = 2;
                huancun = wendubujin;
            }
            else if (wendubujin == 2)
            {
                wendubujin = 5;
                huancun = wendubujin;
            }
            else if (wendubujin == 5)
            {
                wendubujin = 10;
                huancun = wendubujin;
            }
        }

        if (jinduzhi == 5) //菜单P05
        {
            if (xiumianshijian < 60)
                xiumianshijian++;
            huancun = xiumianshijian;
        }
        if (jinduzhi == 6) //菜单P06
        {
            if (guanjishijian < 180)
            {
                if (guanjishijian < 30)
                    guanjishijian++;
                else
                    guanjishijian = guanjishijian + 10;
            }
            huancun = guanjishijian;
        }

        if (jinduzhi == 7) //菜单P07
        {
            if (wenduxiuzheng < 30)
                wenduxiuzheng++;
            huancun = wenduxiuzheng;
        }
        if (jinduzhi == 8) //菜单P08
        {
            huanxingfangshi = 1;
            huancun = huanxingfangshi;
        }
    }
    if (e == 1 && f == 1 && encodera == 0 && encoderb == 1 && jinzhicaozuo == 0) //和前一次状态比较确定为左旋
    {
        if (jinduzhi == 0) //同上,不再赘述
        {
            huifumoren = 0;
            huancun = huifumoren; //同上,不再赘述
        }
        if (jinduzhi == 1)
        {
            if (zengyi > 200)
                zengyi--;
            zengyih = zengyi >> 8;
            zengyil = zengyi & 0x00ff;
            huancun = zengyi;
        }
        if (jinduzhi == 2)
        {
            if (shitiaodianya >= 2)
                shitiaodianya = shitiaodianya - 2;
            huancun = shitiaodianya;
        }
        if (jinduzhi == 3)
        {
            if (redianou > 30)
                redianou--;
            huancun = redianou;
        }
        if (jinduzhi == 4)
        {
            if (wendubujin == 1)
            {
                wendubujin = 0;
                huancun = wendubujin;
            }
            else if (wendubujin == 2)
            {
                wendubujin = 1;
                huancun = wendubujin;
            }
            else if (wendubujin == 5)
            {
                wendubujin = 2;
                huancun = wendubujin;
            }
            else if (wendubujin == 10)
            {
                wendubujin = 5;
                huancun = wendubujin;
            }
        }

        if (jinduzhi == 5)
        {
            if (xiumianshijian > 0)
                xiumianshijian--;
            huancun = xiumianshijian;
        }
        if (jinduzhi == 6)
        {
            if (guanjishijian > 0)
            {
                if (guanjishijian < 31)
                    guanjishijian--;
                else
                    guanjishijian = guanjishijian - 10;
            }
            huancun = guanjishijian;
        }

        if (jinduzhi == 7)
        {
            if (wenduxiuzheng > -30)
                wenduxiuzheng--;
            huancun = wenduxiuzheng;
        }
        if (jinduzhi == 8)
        {
            huanxingfangshi = 0;
            huancun = huanxingfangshi;
        }
    }
    e = encodera; //记录编码器a脚此次状态
    f = encoderb; //记录编码器b脚此次状态

    if (bianmaanniu == 0 && jinzhicaozuo == 0) //如果按下编码按钮,并且此时允许操作编码器
    {
        delay_ms(1);
        if (bianmaanniu == 0) //去抖动
        {
            while (bianmaanniu == 0)
                ;
            if (jinduzhi == 0 && huifumoren == 1) //如果在P00菜单选择了恢复默认值
            {
                morenzhi(); //就恢复默认值
                duqushezhishuju(); //并重新读取数据到RAM
            }
            jinduzhi++; //菜单数加一(即下一项菜单)
            xianship = 1; //百位显示字母P
            huancun = jinduzhi; //显示新的菜单项(指P几菜单)
            jinzhicaozuo = 1; //由于菜单项要延迟显示1.5秒才显示菜单对应的参数,所以显示菜单项的时候禁止操作编码器改变参数
            shezhixianshijishukaiguan = 1; //允许开始新的延迟计数(1.5秒)
            shezhixianshijishu = 0; //从0开始计数
            baocunshuju(); //保存数据到EEPROM
            if (jinduzhi == 9) //菜单项最大为P08,如果到了P09说明所有的菜单都设置完了
            {
                xianship = 0; //所以百位不再显示字母P
                shezhimoshi = 0; //退出设置模式
                ET1 = 0; //关闭定时器1
                TR1 = 0;
                IE = 0x88; //关闭定时器1中断,开启定时器0中断
                timer0init(); //初始化定时器0,进入正常工作模式!!!!!!!!!!!
                ganggangkaiji = 1; //认为是刚刚开机
                xiumianjishukaiguan = 1; //允许新的一次休眠计时
                xiumianjishu = 0; //从0开始计时
            }
        }
    }
}

/******************************掉电存储函数***************************************/
void diaodiancunchu(void) //用于关机时存储温度数据,以便下次开机时恢复温度
{
    gonggonghanshu2(); //此函数用于检测电源电压
    if (dianyuandianya < 4850) //如果电源电压降低到4.85V说明掉电了
    {
        P2 = 0x00; //关闭数码管位选和t12(即停止显示,停止加热),节约电量
        P3 = 0x70; //关闭替换脚,关闭位选脚,节约电量
        if (xiumiankaiguan == 0) //如果是正常状态,保存当前设定温度
        {
            shedingwenduh = shedingwendu >> 8; //
            shedingwendul = shedingwendu & 0x00ff;
        }
        else //如果是休眠状态,保存休眠前的设定温度
        {
            shedingwenduh = baocunwendu >> 8; //
            shedingwendul = baocunwendu & 0x00ff;
        }
        Sector_Erase(0x0200); //擦除扇区
        Byte_Program(0x0200, shedingwenduh); //写入温度高八位到EEPROM
        Byte_Program(0x0201, shedingwendul); //写入温度低八位到EEPROM
        delay_ms(300);
        t12 = 1; //设定温度已经保存,于是加热t12以尽快耗尽电容电量,加速关机
        delay_ms(1000); //最多加热1000ms
        IAP_CONTR = 0x20; //如果1000ms后单片机仍在工作,说明单片机没有真的掉电,于是复位单片机,重新开机,避免陷入加热的死循环
    }
}

/******************************震动休眠函数********************************/
void zhendongxiumian(void)
{
    if (zhendongkaiguan != g) //震动开关状态改变,说明晃动了手柄
    {
        ganggangkaiji = 0; //已经开机
        if ((xiumiankaiguan == 1 && guanjikaiguan == 0)
                || (xiumiankaiguan == 1 && guanjikaiguan == 1 && huanxingfangshi == 0)) //
        {
            xiumiankaiguan = 0; //停止休眠
            shedingwendu = baocunwendu; //恢复原来的温度
            xiumianjishukaiguan = 1; //允许新一次休眠计数
            xiumianjishu = 0; //从0开始计数
            guanjijishukaiguan = 0; //停止关机计数
            guanjikaiguan = 0; //退出关机状态(如果原来是关机状态的话)
        }
    }
    g = zhendongkaiguan; //保存当前震动传感器状态(用于和下一次比较,以便确定是否震动了)
}

/*****************************定时器0中断函数********************************/
void timer0(void)
interrupt 1 //定时0中断函数,用于检测旋转编码器,掉电存储等操作(仅用于正常工作模式)
{
    unsigned int a;
    diaodiancunchu(); //调用掉电存储函数
    if(bianmaanniu==0)//正常加热模式按住编码器开关不动,就显示室温
    huancun=shiwen;
    bianmaqi();//调用编码器函数
    zhendongxiumian();//调用震动休眠函数
    if(huancunkaiguan==1)//延时显示计数
    huancunjishu++;
    if(xiumianjishukaiguan==1||ganggangkaiji==1)//休眠计数
    xiumianjishu++;
    if(guanjijishukaiguan==1||guanjishijian==0)//关机计数
    guanjijishu++;
    if(xiumianjishu==xiumianshijian*60000)//如果达到设定时间就休眠
    {
        ganggangkaiji=0; //已经开机
        baocunwendu=shedingwendu;//记录当前设定温度
        shedingwendu=2000;//设定休眠温度
        xiumiankaiguan=1;//进入休眠状态
        xiumianjishukaiguan=0;//停止休眠计数
        xiumianjishu=0;//清零休眠计数
        guanjijishukaiguan=1;//允许关机计数
    }

    if(guanjijishu==guanjishijian*60000) //如果关机计数达到了设定的时间
    {
        guanjikaiguan=1; //进入关机状态
        guanjijishukaiguan=0;//停止关机计数
        guanjijishu=0;//清零关机计数
    }
    zhouqijishu++; //加热周期计数
    if(jiareshu>198)//最多加热198ms
    jiareshu=198;
    if((zhouqijishu<=jiareshu)&&(guanjikaiguan==0))//如果当前计数小于等于加热数并且不是关机模式,就加热
    t12=1;
    else
    t12=0;
    if(t12wendu==5000)//如果t12温度为500,说明没有插入烙铁头或参数严重错误
    t12=0;//停止加热
    if(huancunjishu==1500)//设定温度延时显示1.5秒
    {
        huancunkaiguan=0; //如果达到了1.5秒,关闭缓存开关
        huancunjishu=0;//停止缓存计数
        huancun=t12wendu;//由显示设定温度改为显示t12温度
    }
    if(zhouqijishu==198) //最多加热到198ms
    t12=0;//停止加热
    if(zhouqijishu==200)//t12停止加热后2ms再检测温度(给电容留出放电时间,防止检测的温度偏高)
    {
        adc(); //检测电压,计算温度
        zhouqijishu=0;//重新开始加热周期计数
        a=shedingwendu+10;//////////////////////////以下为加热算法(请自行理解,不作注释)//////////////////////
        if(t12wendu>a)
        {
            leijiwencha=0;
            if(t12wendu-a<=30)
            jiareshu=(t12wendu-a)/4+a/1000;
            else
            jiareshu=0;
        }
        if(t12wendu<=a)
        {
            leijiwencha=a-t12wendu;
            if(t12wendu==a)
            leijiwencha=0;
            if(a-t12wendu>=300)
            jiareshu=198;
            else if(a-t12wendu>=200)
            jiareshu=160;
            else if(a-t12wendu>=150)
            jiareshu=130;
            else if(a-t12wendu>=100)
            jiareshu=90+leijiwencha;
            else if(a-t12wendu>=50)
            jiareshu=50;
            else
            jiareshu=(a-t12wendu)/2+shedingwendu/500+leijiwencha;
        }
    }
}

/****************************定时器1中断函数************************/
void timer1(void)
interrupt 3 //定时器1中断仅用于参数设置模式
{
    bianmaqi2(); //调用编码器2函数
    if(ganggangkaiji==1)//如果刚刚开机
    {
        xianship=1; //百位显示字母P
        huancun=jinduzhi;//显示菜单P00(因为此时进度值为0)
        ganggangkaiji=0;//已经开机
        shezhixianshijishukaiguan=1;//允许设置显示计数
    }
    if(shezhixianshijishukaiguan==1) //如果允许设置显示计数,就开始计数
    shezhixianshijishu++;
    if(shezhixianshijishu==1500)//如果计数到规定的1.5s
    {
        xianship=0; //停止显示百位的字母P
        shezhixianshijishukaiguan=0;//停止设置显示计数
        shezhixianshijishu=0;//清零计数
        jinzhicaozuo=0;//此时禁止操作编码器
        if(jinduzhi==0)//显示当前菜单项对应的参数(P00的参数)
        huancun=huifumoren;
        if(jinduzhi==1)//同上,不再赘述
        huancun=zengyi;
        if(jinduzhi==2)
        huancun=shitiaodianya;
        if(jinduzhi==3)
        huancun=redianou;
        if(jinduzhi==4)
        huancun=wendubujin;
        if(jinduzhi==5)
        huancun=xiumianshijian;
        if(jinduzhi==6)
        huancun=guanjishijian;
        if(jinduzhi==7)
        huancun=wenduxiuzheng;
        if(jinduzhi==8)
        huancun=huanxingfangshi;
        jinzhicaozuo=0;
    }
}

/***************************主函数***********************************/
void main(void) //主函数
{
    P2 = 0x00; //关闭数码管位选
    t12 = 0; //开机时输出关,不加热
    P3 = 0x70; //关闭替换脚,关闭位选脚
    P0M0 = 0x00; //P0为正常模式
    P0M0 = 0x00;
    zhendongkaiguan = 1; //震动开关默认高电平
    P1M0 = 0x00; //P1除P1.0,P1.1,P1.2为输入模式外均为正常模式
    P1M1 = 0x07;
    P1ASF = 0x07; //设置P1相应ADC转换的I/O口为ADC输入模式
    P2M0 = 0xff; //P2都是推挽模式
    P2M1 = 0x00;
    P3M0 = 0xf0; //P3.4,P3.5,P3.6,P3.7为推挽模式,P3.2,P3.3为输入模式,其余正常模式
    P3M1 = 0x06;
    ADC_CONTR = 0xe0; //打开ADC电源
    shedingwenduh = Byte_Read(0x0200); //读取上次关机时保存的设定温度的高八位
    shedingwendul = Byte_Read(0x0201); //读取上次关机时保存的设定温度的低八位
    shedingwendu = shedingwenduh * 256 + shedingwendul; //计算十进制的设定温度
    if (shedingwendu > 4500 || shedingwendu < 2000) //如果读取的设定温度超出范围
        shedingwendu = 2500; //就设为250度
    g = zhendongkaiguan; //检测震动休眠开关的当前状态
    duqushezhishuju(); //读取控制器的设置参数
    bianmaanniu = 1; //编码按钮高电平
    if (bianmaanniu == 0) //如果开机时编码按钮是低电平(即按下了按钮)
    {
        shezhimoshi = 1; //进入参数设置模式
        delay_ms(300); //延时一段时间(可以松开手了,嘿嘿)
        IE = 0x82; //打开定时器1中断,关闭定时器0中断
        timer1init(); //初始化定时器1
    }
    else //如果开机时没有按下编码器按钮
    {
        shezhimoshi = 0; //进入正常加热模式
        IE = 0x88; //打开定时器0中断,关闭定时器1中断
        timer0init(); //初始化定时器0
    }
    while (1)
    {
        display(huancun); //数码管显示数据
    }
}
