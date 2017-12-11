/**
* @file 		gprs_uart.c
* @brief		�ṩΪgprsģ�����Ĵ��ڹ���.
* @details	
* @author		sundh
* @date		16-09-20
* @version	A001
* @par Copyright (c): 
* 		XXX??
* @par History:         
*	version: author, date, desc\n
* 	A001:sundh,16-09-20,��ʼ������
*/
#include "chipset.h"
#include "stdint.h"
#include "gprs_uart.h"
#include "hardwareConfig.h"
#include "stm32f10x_usart.h"
#include "sdhError.h"
#include "osObjects.h"                      // RTOS object definitions
#include <stdarg.h>
#include <string.h>
#include "def.h"
#include "Ping_PongBuf.h"




static PPBuf_t	GprsUart_ppbuf;


osSemaphoreId SemId_txFinish;                         // Semaphore ID
//osSemaphoreDef(Sem_txFinish);                       // Semaphore definition
uint32_t os_semaphore_cb_Sem_txFinish[2] = { 0 }; 
const osSemaphoreDef_t os_semaphore_def_Sem_txFinish = { (os_semaphore_cb_Sem_txFinish) };



osSemaphoreId SemId_rxFrame;                         // Semaphore ID
//osSemaphoreDef(Sem_rxFrame);                       // Semaphore definition
uint32_t os_semaphore_cb_Sem_rxFrame[2] = { 0 }; 
const osSemaphoreDef_t os_semaphore_def_Sem_rxFrame = { (os_semaphore_cb_Sem_rxFrame) };


struct {
	rxirq_cb cb;
	void *arg;
	int  len;
}GprsRxirqCB;
char GprsUart_buf[GPRS_UART_BUF_LEN];			//����DMA���ջ���
char gpprs_Uart_Txbuf[ GPRS_UART_TXBUF_LEN];

static struct usart_control_t {
	short	tx_block;		//������־
	short	rx_block;
	
	uint16_t	recv_size;
	uint16_t	totle_size;
	short	tx_waittime_ms;
	short	rx_waittime_ms;
	
	char *rxbuf;		//Ϊ�˷����ڽ����ж��д���һЩ�¼�����ʱ�����ٻ�ý����ڴ�������
	
}Gprs_uart_ctl;


static void DMA_GprsUart_Init(void);


/*!
**
**
** @param txbuf ���ͻ����ַ
** @param rxbuf ���ܻ����ַ
** @return
**/
int gprs_uart_init(void)
{


	SemId_txFinish = osSemaphoreCreate(osSemaphore(Sem_txFinish), 1);
	SemId_rxFrame = osSemaphoreCreate(osSemaphore(Sem_rxFrame), 1);

	osSemaphoreWait( SemId_txFinish, 0 );
	osSemaphoreWait( SemId_rxFrame, 0 );
	
	
	USART_DeInit( GPRS_USART);

//	GprsUart_ppbuf.ping_buf = GprsUart_buf;
//	GprsUart_ppbuf.pong_buf = GprsUart_buf + GPRS_UART_BUF_LEN/2;
//	GprsUart_ppbuf.ping_len = GPRS_UART_BUF_LEN/2;
//	GprsUart_ppbuf.pong_len = GPRS_UART_BUF_LEN/2;
//	init_pingponfbuf( &GprsUart_ppbuf);


	init_pingponfbuf( &GprsUart_ppbuf, GprsUart_buf, GPRS_UART_BUF_LEN, 0);

	DMA_GprsUart_Init();

	USART_Init( GPRS_USART, &Conf_GprsUsart);
	
	
	USART_ITConfig( GPRS_USART, USART_IT_RXNE, ENABLE);
	USART_ITConfig( GPRS_USART, USART_IT_IDLE, ENABLE);
	USART_DMACmd(GPRS_USART, USART_DMAReq_Tx, ENABLE);  // ����DMA����
	USART_DMACmd(GPRS_USART, USART_DMAReq_Rx, ENABLE); // ����DMA����
	USART_Cmd(GPRS_USART, ENABLE);
	
	
	GprsRxirqCB.arg = NULL;
	GprsRxirqCB.cb = NULL;
	
	Gprs_uart_ctl.rx_block = 1;
	Gprs_uart_ctl.tx_block = 1;
	Gprs_uart_ctl.rx_waittime_ms = 100;
	Gprs_uart_ctl.tx_waittime_ms = 100;
	
	return ERR_OK;
}



void regRxIrq_cb(rxirq_cb cb, void *arg)
{
	
	GprsRxirqCB.arg = arg;
	GprsRxirqCB.cb = cb;
}

/*!
**
**
** @param data 
** @param size 
** @return
**/


int gprs_Uart_write(char *data, uint16_t size)
{
	int ret;
	char *sendbuf ;
	if( data == NULL)
		return ERR_BAD_PARAMETER;
	
	if( size < GPRS_UART_TXBUF_LEN)
	{
		memset( gpprs_Uart_Txbuf, 0, GPRS_UART_TXBUF_LEN);
		memcpy( gpprs_Uart_Txbuf, data, size);
		sendbuf = gpprs_Uart_Txbuf;
	
	}
	else
	{
		sendbuf = data;
		
	}
	
	
	DMA_gprs_usart.dma_tx_base->CMAR = (uint32_t)sendbuf;
	DMA_gprs_usart.dma_tx_base->CNDTR = (uint16_t)size; 
	DMA_Cmd( DMA_gprs_usart.dma_tx_base, ENABLE);        //��ʼDMA����

	
	if( Gprs_uart_ctl.tx_block)
	{
		if( Gprs_uart_ctl.tx_waittime_ms == 0)
			ret = osSemaphoreWait( SemId_txFinish, osWaitForever );
		else
			ret = osSemaphoreWait( SemId_txFinish, Gprs_uart_ctl.tx_waittime_ms );
		
		if ( ret > 0) 
		{
			return ERR_OK;
		}
		
		
		return ERR_DEV_TIMEOUT;
		
	}
	
	return ERR_OK;
}



/*!
**
**
** @param data 
** @param size 
** @return
**/
int gprs_Uart_read(char *data, uint16_t size)
{
	int  ret;
	int len = size;
	char *playloadbuf ;

	if( data == NULL)
		return ERR_BAD_PARAMETER;
		
	if( Gprs_uart_ctl.rx_block)
	{
		//��������Զ�ȴ�
		if( Gprs_uart_ctl.rx_waittime_ms == 0 || Gprs_uart_ctl.rx_waittime_ms == osWaitForever)
			ret = osSemaphoreWait( SemId_rxFrame, 0 );
		else
			ret = osSemaphoreWait( SemId_rxFrame, Gprs_uart_ctl.rx_waittime_ms );
		
		
		
	}
	else 
		ret = osSemaphoreWait( SemId_rxFrame, 0 );
	
	if( ret > 0)
	{
		if( len > Gprs_uart_ctl.recv_size)
			len = Gprs_uart_ctl.recv_size;
		memset( data, 0, size);

		playloadbuf = get_playloadbuf( &GprsUart_ppbuf);
		memcpy( data, playloadbuf, len);
		free_playloadbuf( &GprsUart_ppbuf);
		Gprs_uart_ctl.recv_size = 0;

		return len;
	}
	
	return 0;
}


/*!
**
**
** @param size
**
** @return
**/
void gprs_Uart_ioctl(int cmd, ...)
{
	int int_data;
	va_list arg_ptr; 
	va_start(arg_ptr, cmd); 
	
	switch(cmd)
	{
		case GPRS_UART_CMD_SET_TXBLOCK:
			Gprs_uart_ctl.tx_block = 1;
			break;
		case GPRS_UART_CMD_CLR_TXBLOCK:
			Gprs_uart_ctl.tx_block = 0;
			break;
		case GPRS_UART_CMD_SET_RXBLOCK:
			Gprs_uart_ctl.rx_block = 1;
			break;
		case GPRS_UART_CMD_CLR_RXBLOCK:
			Gprs_uart_ctl.rx_block = 0;
			break;
		case GPRSUART_SET_TXWAITTIME_MS:
			int_data = va_arg(arg_ptr, int);
			va_end(arg_ptr); 
			Gprs_uart_ctl.tx_waittime_ms = int_data;
			break;
		case GPRSUART_SET_RXWAITTIME_MS:
			int_data = va_arg(arg_ptr, int);
			va_end(arg_ptr); 
			Gprs_uart_ctl.rx_waittime_ms = int_data;
			break;
		default: break;
		
	}
}



/*!
**
**
** @param size
**
** @return
**/
int gprs_uart_test(char *buf, int size)
{
	char *pp = NULL;
    
	
	
	strcpy( buf, "ATE0\r\n" );
//	gprs_Uart_ioctl( GPRS_UART_CMD_SET_TXWAITTIME, 0);
//	gprs_Uart_ioctl( GPRS_UART_CMD_SET_RXWAITTIME, 0);
	gprs_Uart_write( buf, strlen(buf));
	gprs_Uart_read( buf, size);
	pp = strstr((const char*)buf,"OK");
    if(pp)
		return ERR_OK;
	
	return ERR_UNKOWN;
	
}




















/*! gprs uart dma Configuration*/
void DMA_GprsUart_Init(void)
{

	DMA_InitTypeDef DMA_InitStructure;	
	short rxbuflen;
	char *rxbuf;
    /* DMA clock enable */

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE); // ??DMA1??
//=DMA_Configuration==============================================================================//	
/*--- UART_Tx_DMA_Channel DMA Config ---*/
    DMA_Cmd( DMA_gprs_usart.dma_tx_base, DISABLE);                           // �ر�DMA
    DMA_DeInit(DMA_gprs_usart.dma_tx_base);                                 // �ָ���ʼֵ
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&GPRS_USART->DR);// �����ַ
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)GprsUart_buf;        
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;                      // ���ڴ浽����
    DMA_InitStructure.DMA_BufferSize = GPRS_UART_BUF_LEN;                    
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;        // �����ַ������
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                 // �ڴ��ַ����
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // �������ݿ��1B
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;         // �ڴ��ַ���1B
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                           // ���δ���ģʽ
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;                 // ���ȼ�
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                            // �ر��ڴ浽�ڴ�ģʽ
    DMA_Init(DMA_gprs_usart.dma_tx_base, &DMA_InitStructure);               // 

    DMA_ClearFlag( DMA_gprs_usart.dma_tx_flag );                                 // �����־

	DMA_Cmd(DMA_gprs_usart.dma_tx_base, DISABLE); 												// �ر�DMA

    DMA_ITConfig(DMA_gprs_usart.dma_tx_base, DMA_IT_TC, ENABLE);            // ��������DMA�ж�

   

/*--- UART_Rx_DMA_Channel DMA Config ---*/

	switch_receivebuf( &GprsUart_ppbuf, &rxbuf, &rxbuflen);

    DMA_Cmd(DMA_gprs_usart.dma_rx_base, DISABLE);                           
    DMA_DeInit(DMA_gprs_usart.dma_rx_base);                                 
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&GPRS_USART->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rxbuf;         
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;                     
    DMA_InitStructure.DMA_BufferSize = rxbuflen;                     
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;        
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                 
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;         
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                           
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;                 
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                            
    DMA_Init(DMA_gprs_usart.dma_rx_base, &DMA_InitStructure);               
    DMA_ClearFlag( DMA_gprs_usart.dma_rx_flag);                                
    DMA_Cmd(DMA_gprs_usart.dma_rx_base, ENABLE);                            

   Gprs_uart_ctl.rxbuf = rxbuf;

}





void DMA1_Channel2_IRQHandler(void)
{

    if(DMA_GetITStatus(DMA1_FLAG_TC2))
    {
		
			DMA_ClearFlag(DMA_gprs_usart.dma_tx_flag);         // �����־
			DMA_Cmd( DMA_gprs_usart.dma_tx_base, DISABLE);   // �ر�DMAͨ��
			
			USART_ClearFlag(USART3,USART_IT_TC );
			USART_ITConfig( USART3, USART_IT_TC, ENABLE);
    }
}


void USART3_IRQHandler(void)
{
	uint8_t clear_idle = clear_idle;
	short rxbuflen;
	char *rxbuf;
	if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)  // �����ж�
	{


		
		DMA_Cmd(DMA_gprs_usart.dma_rx_base, DISABLE);       // �ر�DMA
		DMA_ClearFlag( DMA_gprs_usart.dma_rx_flag );           // ���DMA��־
		
		

		
		Gprs_uart_ctl.recv_size = get_loadbuflen( &GprsUart_ppbuf)  - \
		DMA_GetCurrDataCounter(DMA_gprs_usart.dma_rx_base); //��ý��յ����ֽ�
		
		if( GprsRxirqCB.cb != NULL)
			GprsRxirqCB.cb( Gprs_uart_ctl.rxbuf,  GprsRxirqCB.arg, Gprs_uart_ctl.recv_size);
		
		switch_receivebuf( &GprsUart_ppbuf, &rxbuf, &rxbuflen);
		DMA_gprs_usart.dma_rx_base->CMAR = (uint32_t)rxbuf;
		DMA_gprs_usart.dma_rx_base->CNDTR = rxbuflen;
		DMA_Cmd( DMA_gprs_usart.dma_rx_base, ENABLE);
		clear_idle = GPRS_USART->SR;
		clear_idle = GPRS_USART->DR;
		USART_ReceiveData( USART3 ); // Clear IDLE interrupt flag bit
		
		Gprs_uart_ctl.rxbuf = rxbuf;
		osSemaphoreRelease( SemId_rxFrame);

		
		
		
		
	}
	
	if(USART_GetITStatus( USART3, USART_IT_RXNE) == SET) 
	{
		//���RXNE��־����ֹ�ڽ��յ����ݳ��ȳ���dma�Ľ��ճ���֮��û���κ�Ӧ��ȥ�������ݶ���ɴ�����Զ�����ж�״̬
		USART_ReceiveData( USART3);
	}
	//����ж���DMA��������ж��п���
	if(USART_GetITStatus( USART3, USART_IT_TC) == SET) 
	{
		USART_ClearFlag( USART3,USART_IT_TC );
		USART_ITConfig( USART3, USART_IT_TC, DISABLE);
		
		osSemaphoreRelease( SemId_txFinish);
		
	}

}



