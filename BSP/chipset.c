/**
  ******************************************************************************
  * @file    chipset.c
  * @author  sundh
  * @version V0.1.1
  * @date    2016-7-31
  * @brief   ���������ʱ�Ӻ��ж�
  *   
  ******************************************************************************
  * @attention
  *
  *
  * <h2><center>&copy; sundh </center></h2>
  ******************************************************************************
	---------------------------------------------------------------------------- 

	  Change History :                                                           

	  <Date>      <Version>  <Author>       | <Description>                    

	---------------------------------------------------------------------------- 

	  2016/07/31 | 0.1.1   	| sundh     		| Create file                      

	---------------------------------------------------------------------------- 

  */
#include <stdio.h>
#include "string.h"
#include "chipset.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_flash.h"
#include "hardwareConfig.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_iwdg.h"

#include "misc.h" /* High level functions for NVIC and SysTick (add-on to CMSIS functions) */
static vu32 TimingDelay;




/*! system dog*/
//void IWDG_Configuration(void)
//{

//    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);   /* д��0x5555,�����������Ĵ���д�빦�� */
//    IWDG_SetPrescaler(IWDG_Prescaler_256);          /* ����ʱ�ӷ�Ƶ,40K/256=156HZ(6.4ms)*/
//    IWDG_SetReload(781);                            /* ι��ʱ�� 5s/6.4MS=781 .ע�ⲻ�ܴ���0xfff*/
//    IWDG_ReloadCounter();                           /* ι��*/
//    IWDG_Enable();                                  /* ʹ�ܹ���*/
//}


/** 
* @brief ϵͳʱ������
*
*
* @param void   
*
* @return void
*
*/
void RCC_Configuration(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 /*| RCC_APB1Periph_TIM3*/,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC  | RCC_APB2Periph_GPIOD  | RCC_APB2Periph_GPIOE, ENABLE);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);  //Enable UART4 clocks
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div8);	     //2014.5.18 
	// ADC Periph clock enable 

  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);   //2014.5.18
	
	
}

void IWDG_Configuration(void)
{
	
	
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);   /* д��0x5555,�����򹷵ļĴ���д�� */
    IWDG_SetPrescaler(IWDG_Prescaler_256);          /* ʱ�ӷ�Ƶ,40K/256=156HZ(6.4ms)*/
    IWDG_SetReload(468);                            /* ι��ʱ�� 3s/6.4MS=468 ���ó���0xfff*/
    IWDG_ReloadCounter();                           /* ι��*/
    IWDG_Enable();                                  /* ʹ�ܹ�*/
	
}

/*! System NVIC Configuration */
void NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

#ifdef VECT_TAB_RAM
    /* Set the Vector Table base location at 0x20000000 */
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else /* VECT_TAB_FLASH */
    /* Set the Vector Table base location at 0x08000000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
#endif

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);//�������ȼ�������ʽ1������ռ��ռһλ�����ȼ�ռ3λ

    /* Enable the USART1 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);


    /* Enable the USART2 Interrupt*/
    NVIC_InitStructure.NVIC_IRQChannel=USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=1;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable the USART3 Interrupt*/
    NVIC_InitStructure.NVIC_IRQChannel=USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_Init(&NVIC_InitStructure);



/* Enable the DMA Interrupt */

    NVIC_InitStructure.NVIC_IRQChannel = DMA_gprs_usart.dma_tx_irq;   // ����DMA����
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     // ���ȼ�����
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = DMA_s485_usart.dma_tx_irq;   // ����DMA����
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     // ���ȼ�����
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	
//	NVIC_InitStructure.NVIC_IRQChannel = DMA_s485_usart.dma_rx_irq;    
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = DMA_adc.dma_rx_irq;   // ADC����
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     // ���ȼ�����
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = W25Q_Spi.irq;			//spi�ж�
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1 ;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	
	
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}



///*! GPIO Configuration */
void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    // Configure the all GPIO port pins in Analog Input Mode(Floating input
    // trigger OFF), this will reduce the power consumption and increase the
    // device immunity against EMI/EMC
    // Enables or disables the High Speed APB(APB2) peripheral clock

    

	
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //LED   PinLED_run
	
    GPIO_InitStructure.GPIO_Pin = PinLED_run.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PinLED_run.Port, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = PinLED_com.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PinLED_com.Port, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Gprs_powerkey.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(Gprs_powerkey.Port, &GPIO_InitStructure);
	GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOC, &GPIO_InitStructure);

	//ADC pins
	
	GPIO_InitStructure.GPIO_Pin = ADC_pins_4051A1.pin|ADC_pins_4051B1.pin|ADC_pins_4051C1.pin;		 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  //2Mʱ���ٶ�
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = ADC_pins_control0.pin | ADC_pins_control1.pin | ADC_pins_control2.pin;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //2Mʱ���ٶ�
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#ifdef USE_STM3210E_EVAL
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG, ENABLE);

    GPIO_Init(GPIOF, &GPIO_InitStructure);
    GPIO_Init(GPIOG, &GPIO_InitStructure);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG, DISABLE);
#endif

	
}

///*! adc Configuration */
//void adc_Configuration(void)
//{
//	
//	GPIO_InitTypeDef GPIO_InitStructure;
//	
//	/*******PB5 PB6 PB7 �����л�ģ�⿪��(���)*********/
//     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;		 
//     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //�������
//	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  //2Mʱ���ٶ�
//	 GPIO_Init(GPIOB, &GPIO_InitStructure);
//   /**********************************************/	
//	
//	
//	
//	/*************PC1 PC2 PC3 ���õ��� ��ѹMOS****************/
//     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;
//	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //�������
//	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  //2Mʱ���ٶ�
//	 GPIO_Init(GPIOC, &GPIO_InitStructure);
//	 /*************************************/
//}

///*! USART Configuration */
void USART_Configuration(void)
{
    USART_InitTypeDef USART_InitStructure;

    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_4;        //tx
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;                   //rx
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;                   //tx
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;                  //rx
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;                   //tx
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;                  //rx
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_11|GPIO_Pin_5|GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl =USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    /* Configure USART1 basic and asynchronous paramters */
    USART_Init(USART1, &USART_InitStructure);
    USART_Init(USART2, &USART_InitStructure);
    USART_Init(USART3, &USART_InitStructure);

//    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
//    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
//    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    /* Enable USART1 */
    USART_Cmd(USART1, ENABLE);
    USART_Cmd(USART2, ENABLE);
    USART_Cmd(USART3, ENABLE);
}


//����10ms����һ���ж�
void Init_TIM2(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseSturcture;			 //����TIM�ṹ�����
    TIM_DeInit(TIM2);										 //��λʱ��TIM2

    TIM_TimeBaseSturcture.TIM_Period = 10000;				  //��ʱ������
    TIM_TimeBaseSturcture.TIM_Prescaler = 71;				  //72000000/72=1000000
    TIM_TimeBaseSturcture.TIM_ClockDivision = 0x00;				//TIM_CKD_DIV1    TIM2ʱ�ӷ�Ƶ
    TIM_TimeBaseSturcture.TIM_CounterMode = TIM_CounterMode_Up; //Ӌ����ʽ	   

    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseSturcture);
																//��ʼ��
    TIM_ClearFlag(TIM2,TIM_FLAG_Update);						//������I
    TIM_ITConfig(TIM2, TIM_IT_Update,ENABLE);
    TIM_Cmd(TIM2, ENABLE);										//ʹ��
}




void w25q_init_cs(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	//w25q cs
	GPIO_InitStructure.GPIO_Pin = W25Q_csPin.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(W25Q_csPin.Port, &GPIO_InitStructure);
}

void w25q_init_spi(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef spi_conf;
	

	///spi gpio init
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5   | GPIO_Pin_7 ;        //spi_sck spi_mosi
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;                   //spi_miso
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);




	SPI_StructInit( &spi_conf);
	/* Initialize the SPI_Direction member */
	spi_conf.SPI_Mode = SPI_Mode_Master;
	/* Initialize the SPI_CPOL member */
	spi_conf.SPI_CPOL = SPI_CPOL_High;
	/* Initialize the SPI_CPHA member */
	spi_conf.SPI_CPHA = SPI_CPHA_2Edge;
	/* Initialize the SPI_NSS member */
	spi_conf.SPI_NSS = SPI_NSS_Soft;
	
	/* Initialize the SPI_BaudRatePrescaler member */
	spi_conf.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	
	
	W25Q_Spi.config = &spi_conf;
	spi_init( &W25Q_Spi);
	spi_ioctl( &W25Q_Spi, CMD_SET_RXBLOCK);
	spi_ioctl( &W25Q_Spi, SET_RXWAITTIME_MS, 2000);
	spi_ioctl( &W25Q_Spi, CMD_SET_TXBLOCK);
	spi_ioctl( &W25Q_Spi, SET_TXWAITTIME_MS, 1000);
}
