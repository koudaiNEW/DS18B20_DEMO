#include "stm32f10x_gpio.h"
#include "stm32f10x.h"
#include "delay.h"

//用于定义DQ引脚
#define PIN_DQ                 GPIO_Pin_7

//IO组
#define PIN_PORT                GPIOB

#define DQ_H                   GPIO_SetBits(PIN_PORT,PIN_DQ)
#define DQ_L                   GPIO_ResetBits(PIN_PORT,PIN_DQ)
#define DQ                     GPIO_ReadInputDataBit(PIN_PORT,PIN_DQ)


static void DQ_OUT(void)
{
	GPIO_InitTypeDef DQ_Pin = {0};

	DQ_Pin.GPIO_Pin = PIN_DQ;
	DQ_Pin.GPIO_Speed = GPIO_Speed_50MHz;
	DQ_Pin.GPIO_Mode = GPIO_Mode_Out_PP;

	GPIO_Init(PIN_PORT, &DQ_Pin);
}

static void DQ_IN(void)
{
	GPIO_InitTypeDef DQ_Pin = {0};

	DQ_Pin.GPIO_Pin = PIN_DQ;
	DQ_Pin.GPIO_Speed = GPIO_Speed_50MHz;
	DQ_Pin.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	
	GPIO_Init(PIN_PORT, &DQ_Pin);
}

//复位DS18B20
static unsigned char DS18B20_RST(void)
{
	unsigned char hold = 0;

	DQ_OUT();
	DQ_L;
	delay_us(750);
	DQ_H;
	delay_us(15);
	DQ_IN();
	while(DQ && hold < 240)
	{
		hold++;
		delay_us(1);
	}
	if(hold == 240)
	{
		return 1;
	}
	hold = 0;
	while(!DQ && hold < 240)
	{
		hold++;
		delay_us(1);
	}
	if(hold == 240)
	{
		return 2;
	}
	delay_us(500);
	return 0;
}

//字节写入
static void Write_Byte(unsigned char byte)
{
	unsigned char i = 0;

	DQ_OUT();
	for(i = 0; i < 8; i++)
	{
		DQ_L;
		delay_us(2);
		if(byte & 0x01)
		{
			DQ_H;
		}
		byte >>= 1;
		delay_us(60);
		DQ_H;
		delay_us(2);
	}
}

//字节读
static unsigned char Read_Byte(void)
{
	unsigned char byte = 0;
	unsigned char i = 0;

	DQ_OUT();
	for(i = 0; i < 8; i++)
	{
		DQ_L;
		delay_us(2);
		DQ_IN();
		byte >>= 1;
		if(DQ)
		{
			byte |= 0x80;
		}
		delay_us(60);
		DQ_OUT();
		DQ_H;
		delay_us(2);
	}
	delay_us(1);
	return byte;
}

//读取温度
float Get_Temp(void)
{
	unsigned char temp_L = 0;
	unsigned char temp_H = 0;
	unsigned short temp_raw = 0;
	float temp = 0;

	//复位DS18B20
	DS18B20_RST();
	//跳过ROM地址
	Write_Byte(0xCC);
	//启动温度转换
	Write_Byte(0x44);
	delay_ms(750);
	//复位DS18B20
	DS18B20_RST();
	//跳过ROM地址
	Write_Byte(0xCC);
	//读暂存寄存器
	Write_Byte(0xBE);
	temp_L = Read_Byte();
	temp_H = Read_Byte();
	if(temp_H & 0x80)
	{
		temp_raw = (((unsigned short)temp_H << 8) | temp_L) & 0x07FF;
		temp = (-temp_raw + 1) * 0.0625;
	}
	else
	{
		temp_raw = ((unsigned short)temp_H << 8) | temp_L;
		temp = temp_raw * 0.0625;
	}
	return temp;
}

