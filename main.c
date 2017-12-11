/*----------------------------------------------------------------------------
 * CMSIS-RTOS 'main' function template
 *---------------------------------------------------------------------------*/

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions
#include "chipset.h"
#include "gprs.h"
#include "hardwareConfig.h"
#include "debug.h"
#ifdef TDD_GPRS_USART
#include "gprs_uart.h"
#endif
#include "serial485_uart.h"
#include "adc.h"
#include "sdhError.h"
#include "string.h"
#include "stm32f10x_usart.h"
#include "sw_filesys.h"
#include "stdio.h"
#include "times.h"
#include "led.h"
#include "adc.h"
#include "rtu.h"
#include "dtuConfig.h"
#include "TTextConfProt.h"
#include "system.h"

#define APPTYPE_NORMAL			0x25		//��������
#define APPTYPE_CONFIG			0x37		//��������	
#define APPTYPE_CALIBRATION		0xa9		//�궨����


static void Led_job();
static int Select_apptype();		//ѡ��������
int Init_Thread_rtu (void);

ShutDownJob g_shutdow = NULL;
#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
	int safe_count = 10000;
    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART */
    USART_SendData( DEBUG_USART, (uint8_t) ch);

    /* Loop until the end of transmission */
    while (USART_GetFlagStatus( DEBUG_USART, USART_FLAG_TC) == RESET)
    {
		if(safe_count)
			safe_count --;
		else
			break;
    }

    return ch;
}

int fgetc(FILE *f /*stream*/)
{
	int safe_count = 10000;
	 /* Loop until rx not empty */
    while (USART_GetFlagStatus( DEBUG_USART, USART_FLAG_RXNE) == RESET)
    {
		if(safe_count)
			safe_count --;
		else
			break;
    }
	
	return USART_ReceiveData( DEBUG_USART);
}


#if TDD_ON == 1
#define TEST_BUF_SIZE 1200
char Test_buf[TEST_BUF_SIZE];
#endif
/*
 * main: initialize and start the system
 */

char ShutdownFlag = 0 ;
ShutDownJob g_shutdow;
int main (void) {
	gprs_t *sim800;
	int u32_val = 0;
	
#if TDD_ON == 1

	int i = 0;
	char c = 0;
	
	
#endif
	
	osKernelInitialize ();                    // initialize CMSIS-RTOS

	
	 
  // initialize peripherals here
//	RCC_Configuration();
//	IWDG_Configuration();

	NVIC_Configuration();
	
	GPIO_Configuration();
	Init_TIM2();
	USART_Configuration();
	
	LED_run = stm32LED_new();
	LED_com = stm32LED_new();
	LED_run->init( LED_run, &PinLED_run);
	LED_com->init( LED_com, &PinLED_com);
	dsys.led.led_cycle_ms = 200;
	dsys.led.led_count_ms = 0;
#if TDD_ON == 0
	printf(" DTU TDD start ...\n");
	if( filesys_init() != ERR_OK)
	{
		printf(" init filesystem fail \n");
		return ERR_FAIL;
		
	}
	if( filesys_mount() != ERR_OK)
	{
		printf(" mount filesystem fail \n");
		return ERR_FAIL;
		
	}
	printf(" mount filesystem succeed! \n");
	
	
	
	regist_timejob( 200, Led_job);
	
	
	Init_system_config();
	clean_time2_flags();
	s485_uart_init( &Conf_S485Usart_default, NULL);
	u32_val = Select_apptype();
	if( u32_val == APPTYPE_CONFIG)
	{
		
		Config_job();
	}
	else
	{
		
		
		if( NEED_ADC( Dtu_config.work_mode)) 
		{
			create_adc();
			regist_timejob( 50, ADC_50ms);
			Init_rtu();
		}
		
		
		
		Init_ThrdDtu();
		Init_Thread_rtu();
		osKernelStart (); 
		sim800 =  GprsGetInstance();
		u32_val = 0;
		while(1)
		{
			osDelay(5);
			if( ShutdownFlag)
			{
				LED_run->destory(LED_run);
				fs_flush();
				if( g_shutdow)
					g_shutdow();
				
				os_reboot();
			}
			threadActive();
			u32_val ++;
			if( NEED_GPRS( Dtu_config.work_mode)) 
				sim800->run( sim800);
			if( !NEED_ADC( Dtu_config.work_mode)) 
				continue;
			if( u32_val < 10)
				continue;
			u32_val = 0;
			Collect_job();
//			osThreadYield (); 
		}
	}
	
#endif	
	
#ifdef TDD_GPRS_BUF
	sim800 = GprsGetInstance();
	sim800->init( sim800);
	sim800->buf_test( sim800, Test_buf, TEST_BUF_SIZE);
#endif	
	
	
#ifdef TDD_LED
	LED_run->test( LED_run);
	LED_com->test( LED_com);
	while(1);
#endif
#ifdef TDD_TIMER
	time_test();
#endif
#ifdef TDD_ADC
	printf("enter ADC test ?Y/N \n");
	
	c = getchar();
	while(c != 'Y' && c != 'N')
	{
		c = getchar();
		
	}
	if( c == 'Y')
	{
		printf("select Yes, enter adc test \n");
		adc_test(Test_buf, TEST_BUF_SIZE);
	}
	else 
	{
		printf("select No, enter idle loop\n");
	}
	printf("idle loop \n");
	while(1);

#endif
	
	
#ifdef TDD_FILESYS_TEST	
	if( fs_test() == ERR_OK)
		printf(" file system test sccusseed \n");
	else
		printf(" file system test failed \n");
	while(1);
#endif
	
                        // start thread execution
#if  defined(TDD_GPRS_USART) ||  defined(TDD_GPRS_SMS ) || defined(TDD_GPRS_TCP ) 	
	sim800->init(sim800);
#endif	
#ifdef TDD_GPRS_ONOFF
	while(1)
	{
		
		sim800->startup(sim800);
		osDelay(3000);
		sim800->shutdown(sim800);
		osDelay(3000);
		
	}
#endif
	
#ifdef TDD_GPRS_USART
//	gprs_uart_init();
	sim800->startup(sim800);
	while(1)
	{
		i ++;
		if( gprs_uart_test(Test_buf, TEST_BUF_SIZE) == ERR_OK)
			printf(" gprs uart test  %d sccusseed \r\n", i);
		else
			printf(" gprs uart test  %d fail \r\n", i);
		
		memset( Test_buf, 0, 512);
		osDelay(1000);
		
	}
#endif

#ifdef TDD_GPRS_SMS
	
	while(1)
	{
		if( sim800->startup(sim800) != ERR_OK)
		{
			
			osDelay(1000);
		}
		else if( sim800->check_simCard(sim800) == ERR_OK)
		{	
			
			break;
		}
		else {
			
			osDelay(1000);
		}
		
	}
	
	while(1)
	{
		i ++;
		
		if( sim800->sms_test(sim800, "15858172663", Test_buf, TEST_BUF_SIZE) == ERR_OK)
			printf(" sim800 sms test  %d sccusseed \r\n", i);
		else
			printf(" sim800 sms test  %d fail \r\n", i);
		
		osDelay(1000);
	}


#endif	
	
#ifdef TDD_GPRS_TCP
	while(1)
	{
		if( sim800->startup(sim800) != ERR_OK)
		{
			
			osDelay(1000);
		}
		else if( sim800->check_simCard(sim800) == ERR_OK)
		{	
			
			break;
		}
		else {
			
			osDelay(1000);
		}
		
	}
	
	while(1)
	{
		i ++;
		
		if( sim800->tcp_test(sim800, IPADDR, PORTNUM, Test_buf, TEST_BUF_SIZE) == ERR_OK)
			printf(" sim800 tcp test  %d sccusseed \r\n", i);
		else
			printf(" sim800 tcp test  %d fail \r\n", i);
		
		osDelay(1000);

	}

#endif	
	
	
#if TDD_S485 == 1
	s485_uart_init( &Conf_S485Usart_default, NULL);
	while(1)
	{
		i ++;
		if( s485_uart_test(Test_buf, TEST_BUF_SIZE) == ERR_OK)
			printf(" 485 uart test  %d sccusseed \r\n", i);
		else
			printf(" 485 uart test  %d fail \r\n", i);
		
		memset( Test_buf, 0, 512);
		osDelay(10);
		
	}
#endif	
	
	
	
	
}


static void Led_job()
{
//	short i = 0;
//	short err_flag = 0;
	dsys.led.led_count_ms += 200;
//	for( i = 0; i < IPMUX_NUM; i++) {
//		if( Dtu_config.DateCenter_port[i] < 0) {
//			err_flag = 1;
//			break;
//		}
//		
//	}
//	if( err_flag)
//		LED_run->turnon(LED_run);
//	else
		if(dsys.led.led_count_ms >= dsys.led.led_cycle_ms)
		{
			LED_run->blink(LED_run);
			dsys.led.led_count_ms = 0;
		}
		
	
	LED_com->turnoff(LED_com);
}

static int Select_apptype()
{
	int i = 20;
	int ret = 0;
	char	uart_buf[32] = {0};
	while(i)
	{
		
		ret = s485_Uart_read( uart_buf, 32);
		if( ret <=  0)
		{
			i --;
			osDelay(10);
			continue;
		}
		if( enter_TTCP( uart_buf) == ERR_OK)
		{
			get_TTCPVer( uart_buf);
			while( s485_Uart_write(uart_buf, strlen(uart_buf) ) != ERR_OK)
					;
//			osDelay(100);
			return APPTYPE_CONFIG;
		}
		else
		{
			
		}
		
	}
	
	return APPTYPE_NORMAL;
	
}


void SystemShutdown()
{
	ShutdownFlag = 1;
	
}
