//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
#ifndef __INC_system_H__
#define __INC_system_H__
//------------------------------------------------------------------------------
// check for correct compilation options
//------------------------------------------------------------------------------
#include <stdint.h>
#include "stdbool.h"
#include "system_cfg.h"

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

#define SOFTER_VER	304
#define HARD_VER	1
/*
V34 171217:
	����汾�ڼ����ƶ�����ʧ��ʱ��������Ϊ������������ʱ5���Ժ�ʼ�������ӡ����俵�ĺ����е���̱�������
V302 171215:
�����ź�ǿ�Ⱥ͵�����⣻������ʱ��ȥ�������ַ���
V300 171211: 
system:����ϵͳģ�飬��ϵͳģ���м��룺LED��˸���ڿ��ƹ���;gprs ��һЩ״̬���¼�����;
gprs: ���¼��Ĵ洢�Ӷ�����ʽ�ĳɼ��ϴ洢��
dtu: ��gprs�ĸ��ֲ����׶Σ�ʹ�ò�ͬ��Led��˸����



*/

#define LED_CYCLE_MS		200
#define LED_LV_NORMAL			0
#define LED_FILESYS_ERR			5
//gprs�ڲ�ͬ��״̬�¿��Կ���LED��˸Ƶ��
#define LED_GPRS_RUN			0	
#define LED_GPRS_CHECK		3			
#define LED_GPRS_CNNTING	1		//������
#define LED_GPRS_DISCNNT	2
#define LED_GPRS_ERR			4
#define LED_GPRS_SMS	1


#define	SET_U8_BIT(set, bit) (set | (1 << bit))
#define	CLR_U8_BIT(set, bit) (set & ~(1 << bit))
#define	CHK_U8_BIT(set, bit) (set & (1 << bit))
#define	MAX_NUM_SMS		64
//------------------------------------------------------------------------------
// typedef
//------------------------------------------------------------------------------



typedef struct  {
	struct {
		uint16_t	led_cycle_ms;
		uint16_t	led_count_ms;
	}led;
	struct {
//		uint8_t	flag_cnt;		//�������ӹ����У����ܻ�ȥ�����ر�ǰ������ӣ�������ʱ�ر��¼�Ҫ���˵�
		uint8_t	flag_ready;		
		uint8_t	cip_mode;		
		uint8_t	cip_mux;
		uint8_t	ip_status;	
		
		uint8_t	cur_state;
		uint8_t	rx_sms_seq;
		uint8_t	set_tcp_close;		
		uint8_t	set_tcp_recv;
		
		uint8_t		signal_strength;			//0 - 31
		uint8_t		ber;						// 0 - 7
		
		uint8_t		status_gsm;	
		uint8_t		status_gprs;			
		
		uint8_t		bcs;			//0 ME δ��� 1 ME�ڳ�� 2 ������
		uint8_t		bcl;			//0 - 100 ���� 			
		uint16_t	voltage_mv;
		
		uint32_t	set_sms_recv[2];		//����¼64��		ʵ�������50
	}gprs;
	
	char			cfg_change;			//������Ϣ���޸ĵı�־
	char			res[3];
}dtu_system;
//------------------------------------------------------------------------------
// global variable declarations
//------------------------------------------------------------------------------
extern dtu_system 	dsys;
//------------------------------------------------------------------------------
// function prototypes
//------------------------------------------------------------------------------
extern void Led_level(int lv);
bool check_bit(uint8_t *data, int bit);
void clear_bit(uint8_t *data, int bit);
void set_bit(uint8_t *data, int bit);
int Get_str_data(char *s_data, char* separator, int num, uint8_t	*err);
#endif
