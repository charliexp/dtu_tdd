#ifndef __SERAIL_485_UART_H__
#define __SERAIL_485_UART_H__
#include "stdint.h"
#include "stm32f10x_usart.h"

#define SENDMODE_CPU			0
#define SENDMODE_INTR			1
#define SENDMODE_DMA			2

#define SER485_SENDMODE 		SENDMODE_DMA

typedef void (*uart_cb)(void *rxbuf, void *arg);

typedef struct {
	uart_cb cb;
	void *arg;
}u485RxirqCB;

typedef USART_InitTypeDef	ser_485Cfg;
/**
 * @brief 485���ڳ�ʼ��
 * 
 * @return ERR_OK �ɹ�
 * @return 
 */
int s485_uart_init(ser_485Cfg *cfg, u485RxirqCB *cb);



/**
 * @brief 485���ڷ��͹���
 * 
 * @param data �������ݵ��ڴ��ַ
 * @param size �������ݵĳ���
 * @return ERR_OK �ɹ�
 * @return 
 */
int s485_Uart_write(char *data, uint16_t size);

/**
 * @brief 584���ڽ��ճ���
 * 
 * @param data 
 * @param size 
 * @return >= 0 �ֽ���; < 0 :����
 * @return 
 */
int s485_Uart_read(char *data, uint16_t size);
int s485Obtain_Playloadbuf(char **data);
int s485GiveBack_Rxbuf(char *data);

/**
 * @brief ���ƴ��ڵ���Ϊ
 * 
 * @param data 
 * @param size 
 * @return ERR_OK �ɹ�
 * @return 
 */
void s485_Uart_ioctl(int cmd, ...);

/**
 * @brief ����gprs uart���շ�����
 * 
 * @param size �������ݵĳ���
 * @return 0xFF if error, else return 0
 */
int s485_uart_test(char *buf, int size);

#define S485RX_BUF_LEN		1024
#define S485_UART_BUF_LEN		256

#define S485_UART_CMD_SET_TXBLOCK	1
#define S485_UART_CMD_CLR_TXBLOCK	2
#define S485_UART_CMD_SET_RXBLOCK	3
#define S485_UART_CMD_CLR_RXBLOCK	4
#define S485UART_SET_TXWAITTIME_MS	5
#define S485UART_SET_RXWAITTIME_MS	6

#endif
