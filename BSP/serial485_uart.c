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
#include "serial485_uart.h"
#include "hardwareConfig.h"
#include "sdhError.h"
#include "osObjects.h"                      // RTOS object definitions
#include <stdarg.h>
#include <string.h>
#include "def.h"
#include "led.h"
#include "Ping_PongBuf.h"


//static u485RxirqCB *T485Rxirq_cb;

osSemaphoreId SemId_s485txFinish;                         // Semaphore ID
uint32_t os_semaphore_cb_Sem_s485txFinish[2] = { 0 }; 
const osSemaphoreDef_t os_semaphore_def_Sem_s485txFinish = { (os_semaphore_cb_Sem_s485txFinish) };



osSemaphoreId SemId_s485rxFrame;                         // Semaphore ID
uint32_t os_semaphore_cb_Sem_s485rxFrame[2] = { 0 }; 
const osSemaphoreDef_t os_semaphore_def_Sem_s485rxFrame = { (os_semaphore_cb_Sem_s485rxFrame) };

char	S485Uart_buf[S485RX_BUF_LEN];			//����DMA���ջ���
static PPBuf_t	g_S485_ppbuf;

#if ( SER485_SENDMODE == SENDMODE_INTR ) || ( SER485_SENDMODE == SENDMODE_DMA)
char	S485Uart_Txbuf[S485_UART_BUF_LEN];			//���ڷ���
short		g_sendCount = 0;
short		g_sendLen = 0;
#endif
static struct usart_control_t {
	short	tx_block;		//������־
	short	rx_block;
	
	uint16_t	recv_size;
	short	tx_waittime_ms;
	short	rx_waittime_ms;
	

	
}S485_uart_ctl;


static void DMA_s485Uart_Init(void);


/*!
**
**
** @param txbuf ���ͻ����ַ
** @param rxbuf ���ܻ����ַ
** @return
**/
int s485_uart_init(ser_485Cfg *cfg, u485RxirqCB *cb)
{
	static char first = 0;
	int i = 0;
	if( first == 0)
	{
		SemId_s485txFinish = osSemaphoreCreate(osSemaphore(Sem_s485txFinish), 1);
		SemId_s485rxFrame = osSemaphoreCreate(osSemaphore(Sem_s485rxFrame), NUM_PINGPONGBUF);

		osSemaphoreWait( SemId_s485txFinish, 0 );
		for( i = 0; i < NUM_PINGPONGBUF; i++)
			osSemaphoreWait( SemId_s485rxFrame, 0 );
		
		
		
		
		first = 1;
	}
	
	init_pingponfbuf( &g_S485_ppbuf, S485Uart_buf, S485RX_BUF_LEN, 1);
	USART_Cmd(SER485_USART, DISABLE);
	USART_DeInit( SER485_USART);
	
	
	USART_Init( SER485_USART, cfg);
	DMA_s485Uart_Init();

	
	
	
	USART_ITConfig( SER485_USART, USART_IT_RXNE, ENABLE);
	USART_ITConfig( SER485_USART, USART_IT_IDLE, ENABLE);
	
	
#if SER485_SENDMODE == SENDMODE_DMA	
	USART_DMACmd(SER485_USART, USART_DMAReq_Tx, ENABLE);  // ����DMA����
#endif
	USART_DMACmd(SER485_USART, USART_DMAReq_Rx, ENABLE); // ����DMA����
	
	USART_Cmd(SER485_USART, ENABLE);
	
	

	
	S485_uart_ctl.rx_block = 1;
	S485_uart_ctl.tx_block = 1;
	S485_uart_ctl.rx_waittime_ms = 100;
	S485_uart_ctl.tx_waittime_ms = 1000;
	
	return ERR_OK;
}


/*!
**
**
** @param data 
** @param size 
** @return
**/
int s485_Uart_write(char *data, uint16_t size)
{
#if ( SER485_SENDMODE == SENDMODE_INTR ) || ( SER485_SENDMODE == SENDMODE_DMA)
	int ret;
	char *sendbuf ;
#else
	int count = 0;
#endif
	if( data == NULL)
		return ERR_BAD_PARAMETER;
	
#if ( SER485_SENDMODE == SENDMODE_INTR ) || ( SER485_SENDMODE == SENDMODE_DMA)
	if( size < S485_UART_BUF_LEN)
	{
		memset( S485Uart_Txbuf, 0, S485_UART_BUF_LEN);
		memcpy( S485Uart_Txbuf, data, size);
		sendbuf = S485Uart_Txbuf;
	
	}
	else
	{
		sendbuf = data;
		
	}
#	if SER485_SENDMODE == SENDMODE_DMA		
	DMA_s485_usart.dma_tx_base->CMAR = (uint32_t)sendbuf;
	DMA_s485_usart.dma_tx_base->CNDTR = (uint16_t)size; 
	DMA_Cmd( DMA_s485_usart.dma_tx_base, ENABLE);        //��ʼDMA����
#	elif SER485_SENDMODE == SENDMODE_INTR		
	
	USART_SendData( SER485_USART, S485Uart_Txbuf[0]);
	USART_ITConfig( SER485_USART, USART_IT_TXE, ENABLE);
	g_sendCount  = 1;
	g_sendLen  = size;
#	endif
//	osDelay(1);
	if( S485_uart_ctl.tx_block)
	{
		if( S485_uart_ctl.tx_waittime_ms == 0)
			ret = osSemaphoreWait( SemId_s485txFinish, osWaitForever );
		else
			ret = osSemaphoreWait( SemId_s485txFinish, S485_uart_ctl.tx_waittime_ms );
		
		if ( ret > 0) 
		{
			return ERR_OK;
		}
		
		
		return ERR_DEV_TIMEOUT;
		
	}
	
#endif

#if SER485_SENDMODE == SENDMODE_CPU		
	while( count < size)
	{
		USART_SendData( SER485_USART, data[count]);
		while( USART_GetFlagStatus( SER485_USART, USART_FLAG_TXE) == RESET){};
		count ++;
	}

#endif	
	return ERR_OK;
}



/*!
**
**
** @param data 
** @param size 
** @return
**/
int s485_Uart_read(char *data, uint16_t size)
{
	int  ret;
	int len = size;
	char *playloadbuf ;
	
	if( data == NULL)
		return ERR_BAD_PARAMETER;
	
//	memset( data, 0, size);
	
	if( S485_uart_ctl.rx_block)
	{
		//��������Զ�ȵȴ�
		if( S485_uart_ctl.rx_waittime_ms == 0 || S485_uart_ctl.rx_waittime_ms == osWaitForever)
			ret = osSemaphoreWait( SemId_s485rxFrame, 0 );
//		if( S485_uart_ctl.rx_waittime_ms == 0)
//			ret = osSemaphoreWait( SemId_s485rxFrame, osWaitForever );
		else
			ret = osSemaphoreWait( SemId_s485rxFrame, S485_uart_ctl.rx_waittime_ms );
		
		
//		if( ret < 0)
//		{
//			return ERR_DEV_TIMEOUT;
//		}
		
	}
	else 
		ret = osSemaphoreWait( SemId_s485rxFrame, 0 );
	
	if( ret > 0)
	{
		if( len > S485_uart_ctl.recv_size)
			len = S485_uart_ctl.recv_size;
		playloadbuf = get_playloadbuf( &g_S485_ppbuf);
		memset( data, 0, size);
		memcpy( data, playloadbuf, len);
		memset( playloadbuf, 0, len);
		free_playloadbuf( &g_S485_ppbuf);
//		LED_com->turnon(LED_com);
//		if( T485Rxirq_cb != NULL && len)
//			T485Rxirq_cb->cb( NULL,  T485Rxirq_cb->arg);
		S485_uart_ctl.recv_size = 0;
		return len;
	}
	
	return 0;
}

//�����ڵĻ����������
//���ص�ǰ��������ݳ���
int s485Obtain_Playloadbuf(char **data)
{
	int  ret;
	int len = 0;
	char *playloadbuf ;

	if( S485_uart_ctl.rx_block)
	{
		if( S485_uart_ctl.rx_waittime_ms == 0)
			ret = osSemaphoreWait( SemId_s485rxFrame, osWaitForever );
		else
			ret = osSemaphoreWait( SemId_s485rxFrame, S485_uart_ctl.rx_waittime_ms );
		
		
		if( ret < 0)
		{
			return ERR_DEV_TIMEOUT;
		}
		
	}
	else 
		ret = osSemaphoreWait( SemId_s485rxFrame, 0 );
	
	if( ret > 0)
	{
		
		len = S485_uart_ctl.recv_size;
		playloadbuf = get_playloadbuf( &g_S485_ppbuf);
		S485_uart_ctl.recv_size = 0;
		*data = playloadbuf;
		return len;
	}
	
	return 0;
}

int s485GiveBack_Rxbuf(char *data)
{
	free_playloadbuf( &g_S485_ppbuf);
	return 0;
}


/*!
**
**
** @param size
**
** @return
**/
void s485_Uart_ioctl(int cmd, ...)
{
	int int_data;
	va_list arg_ptr; 
	va_start(arg_ptr, cmd); 
	
	switch(cmd)
	{
		case S485_UART_CMD_SET_TXBLOCK:
			S485_uart_ctl.tx_block = 1;
			break;
		case S485_UART_CMD_CLR_TXBLOCK:
			S485_uart_ctl.tx_block = 0;
			break;
		case S485_UART_CMD_SET_RXBLOCK:
			S485_uart_ctl.rx_block = 1;
			break;
		case S485_UART_CMD_CLR_RXBLOCK:
			S485_uart_ctl.rx_block = 0;
			break;
		case S485UART_SET_TXWAITTIME_MS:
			int_data = va_arg(arg_ptr, int);
			va_end(arg_ptr); 
			S485_uart_ctl.tx_waittime_ms = int_data;
			break;
		case S485UART_SET_RXWAITTIME_MS:
			int_data = va_arg(arg_ptr, int);
			va_end(arg_ptr); 
			S485_uart_ctl.rx_waittime_ms = int_data;
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
#if TDD_485 == 1
int s485_uart_test(char *buf, int size)
{

	char *pp = NULL;
    int len;
	
	
	strcpy( buf, "Serial 485 test\n" );
	s485_Uart_ioctl( S485UART_SET_TXWAITTIME_MS, 10);
	s485_Uart_ioctl( S485UART_SET_RXWAITTIME_MS, 10);
	s485_Uart_write( buf, strlen(buf));
	
	while(1)
	{
		len = s485_Uart_read( buf, size);
		pp = strstr((const char*)buf,"OK");
		if(pp)
			return ERR_OK;
		
		if( len > 0)
			s485_Uart_write( buf, len);
	}

}
#endif
/*! gprs uart dma Configuration*/
void DMA_s485Uart_Init(void)
{

	DMA_InitTypeDef DMA_InitStructure;	
	short rxbuflen;
	char *rxbuf;
//	static char first_invoke = 1;
//	if( first_invoke)
//		first_invoke = 0;
//	else
//		return;

    /* DMA clock enable */

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE); // ??DMA1??
//=DMA_Configuration==============================================================================//	
/*--- UART_Tx_DMA_Channel DMA Config ---*/
#if SER485_SENDMODE == SENDMODE_DMA	

    DMA_Cmd( DMA_s485_usart.dma_tx_base, DISABLE);                           // �ر�DMA
    DMA_DeInit(DMA_s485_usart.dma_tx_base);                                 // �ָ���ʼֵ
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&SER485_USART->DR);// �����ַ
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)S485Uart_buf;        
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;                      // ���ڴ浽����
    DMA_InitStructure.DMA_BufferSize = S485_UART_BUF_LEN;                    
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;        // �����ַ������
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                 // �ڴ��ַ����
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // �������ݿ��1B
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;         // �ڴ��ַ���1B
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                           // ���δ���ģʽ
    DMA_InitStructure.DMA_Priority = DMA_Priority_Low;                 // ���ȼ�
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                            // �ر��ڴ浽�ڴ�ģʽ
    DMA_Init(DMA_s485_usart.dma_tx_base, &DMA_InitStructure);               // 
    DMA_ClearFlag( DMA_s485_usart.dma_tx_flag );                                 // �����־
		DMA_Cmd(DMA_s485_usart.dma_tx_base, DISABLE); 												// �ر�DMA
    DMA_ITConfig(DMA_s485_usart.dma_tx_base, DMA_IT_TC, ENABLE);            // ����������ж�

 #endif  

/*--- UART_Rx_DMA_Channel DMA Config ---*/

 
		switch_receivebuf( &g_S485_ppbuf, &rxbuf, &rxbuflen);
    DMA_Cmd(DMA_s485_usart.dma_rx_base, DISABLE);                           
    DMA_DeInit(DMA_s485_usart.dma_rx_base);                                 
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&SER485_USART->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rxbuf;         
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;                     
    DMA_InitStructure.DMA_BufferSize = rxbuflen;                     
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;        
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                 
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;         
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                           
    DMA_InitStructure.DMA_Priority = DMA_Priority_Low;                 
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                            
    DMA_Init(DMA_s485_usart.dma_rx_base, &DMA_InitStructure);               
    DMA_ClearFlag( DMA_s485_usart.dma_rx_flag);      
//	DMA_ITConfig(DMA_s485_usart.dma_rx_base, DMA_IT_TC, ENABLE); 	 // ����������ж�

    DMA_Cmd(DMA_s485_usart.dma_rx_base, ENABLE);                            

   

}





void DMA1_Channel7_IRQHandler(void)
{

    if(DMA_GetITStatus(DMA1_FLAG_TC7))
    {
			
			DMA_ClearFlag(DMA_s485_usart.dma_tx_flag);         // �����־
			DMA_Cmd( DMA_s485_usart.dma_tx_base, DISABLE);   // �ر�DMAͨ��
//			osSemaphoreRelease( SemId_s485txFinish);
		
		USART_ClearFlag(USART2,USART_IT_TC );
		USART_ITConfig( USART2, USART_IT_TC, ENABLE);
		
    }
}

//dma�����������Ժ�,�л����ջ���
void DMA1_Channel6_IRQHandler(void)
{
	short rxbuflen;
	char *rxbuf;
	
    if(DMA_GetITStatus(DMA1_FLAG_TC6))
    {
		DMA_Cmd(DMA_s485_usart.dma_rx_base, DISABLE); 
        DMA_ClearFlag(DMA_s485_usart.dma_rx_flag);         // �����־
		S485_uart_ctl.recv_size = get_loadbuflen( &g_S485_ppbuf)  - \
		DMA_GetCurrDataCounter( DMA_s485_usart.dma_rx_base); //��ý��յ����ֽ�
		
		switch_receivebuf( &g_S485_ppbuf, &rxbuf, &rxbuflen);
		DMA_s485_usart.dma_rx_base->CMAR = (uint32_t)rxbuf;
		DMA_s485_usart.dma_rx_base->CNDTR = rxbuflen;
		DMA_Cmd( DMA_s485_usart.dma_rx_base, ENABLE);
		LED_com->turnon(LED_com);
		osSemaphoreRelease( SemId_s485rxFrame);
    }
}

void USART2_IRQHandler(void)
{
	uint8_t clear_idle = clear_idle;
	short rxbuflen;
	char *rxbuf;
	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)  // �����ж�
	{
		
		DMA_Cmd(DMA_s485_usart.dma_rx_base, DISABLE);       // �ر�DMA
		DMA_ClearFlag( DMA_s485_usart.dma_rx_flag );           // ���DMA��־
		S485_uart_ctl.recv_size = get_loadbuflen( &g_S485_ppbuf)  - \
		DMA_GetCurrDataCounter( DMA_s485_usart.dma_rx_base); //��ý��յ����ֽ�
//		S485_uart_ctl.recv_size = S485_UART_BUF_LEN - DMA_GetCurrDataCounter(DMA_s485_usart.dma_rx_base); //��ý��յ����ֽ�
		
		switch_receivebuf( &g_S485_ppbuf, &rxbuf, &rxbuflen);
		DMA_s485_usart.dma_rx_base->CMAR = (uint32_t)rxbuf;
		DMA_s485_usart.dma_rx_base->CNDTR = rxbuflen;
		DMA_Cmd( DMA_s485_usart.dma_rx_base, ENABLE);
		
		clear_idle = SER485_USART->SR;
		clear_idle = SER485_USART->DR;
		USART_ReceiveData( USART2 ); // Clear IDLE interrupt flag bit
		LED_com->turnon(LED_com);
		osSemaphoreRelease( SemId_s485rxFrame);
	}
	if(USART_GetITStatus( USART2, USART_IT_RXNE) == SET) 
	{
		//���RXNE��־����ֹ�ڽ��յ����ݳ��ȳ���dma�Ľ��ճ���֮��û���κ�Ӧ��ȥ�������ݶ���ɴ�����Զ�����ж�״̬
		USART_ReceiveData( USART2);
	}
	//����ж���DMA��������ж��п���
	if(USART_GetITStatus( USART2, USART_IT_TC) == SET) 
	{
		USART_ClearFlag( USART2,USART_IT_TC );
		USART_ITConfig( USART2, USART_IT_TC, DISABLE);
		
		osSemaphoreRelease( SemId_s485txFinish);
		
	}
	
#if SER485_SENDMODE == SENDMODE_INTR	
	if(USART_GetITStatus(USART2, USART_IT_TXE) == SET)  // �����ж�
	{
			g_sendCount ++;
	
		//������ɺ�رշ�չ�жϣ����ⷢ�ͼĴ������˾ͻ�����ж�
		if( g_sendCount >= g_sendLen)
		{
			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
			osSemaphoreRelease( SemId_s485txFinish);
		}			
		else
			USART_SendData(USART2, S485Uart_Txbuf[ g_sendCount] );
		
		
		
	}
	
#endif

}



