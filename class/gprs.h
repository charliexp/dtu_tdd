#ifndef __GPRS_H__
#define __GPRS_H__
#include "lw_oopc.h"
#include "stdint.h"
#include "CircularBuffer.h"

#define RETRY_TIMES	5

#define IPMUX_NUM	4		//֧��4·����,���ó���7����Ϊ�õ�u8�ļ�������¼tcp close�¼�
#define EVENT_MAX	16		//��󻺴��¼���

#define COPS_CHINA_MOBILE		0x33
#define COPS_CHINA_UNICOM		0x55
#define COPS_UNKOWN				0xaa


#define CIPMODE_OPAQUE		0		//��͸������
#define CIPMODE_TRSP		  1		//͸������

//SIM900 ATCMD
#define AT_SET_DNSIP  "AT+CDNSCFG=" 

//typedef struct {
//	char 		used;
//	char		type;
//	short		arg;
////	uint8_t 	*data;
//}gprs_event_t;

CLASS(gprs_t)
{

	sCircularBuffer		*event_cbuf;
	int					operator;
	// public
	int ( *init)( gprs_t *self);
	
	int ( *lock)(  gprs_t *self);
	int ( *unlock)(  gprs_t *self);
	
	void (*startup)( gprs_t *self);
	void (*shutdown)( gprs_t *self);
	
	int	(*check_simCard)( gprs_t *self);
	int	(*get_sim_info)(gprs_t *self);
	
	int	(*send_text_sms)(  gprs_t *self, char *phnNmbr, char *sms);
	int	(*read_phnNmbr_TextSMS)(  gprs_t *self, char *phnNmbr, char *in_buf, char *out_buf, int *len);
	int	(*read_seq_TextSMS)( gprs_t *self, char *phnNmbr, int seq, char *buf, int *len);
	int (*delete_sms)( gprs_t *self, int seq);
	
	int (*read_smscAddr)(gprs_t *self, char *addr);
	int (*set_smscAddr)(gprs_t *self, char *addr);
	int (*get_apn)( gprs_t *self, char *buf);
	int (*set_dns_ip)( gprs_t *self, char *dns_ip);
	int (*tcpip_cnnt)( gprs_t *self, int cnnt_num, char *prtl, char *addr, int portnum);
	int ( *tcpClose)( gprs_t *self, int cnntNum); 
	int (*sendto_tcp)( gprs_t *self, int cnnt_num, char *data, int len);
	int (*sendto_tcp_buf)( gprs_t *self, char *data, int len);
	int (*recvform_tcp)( gprs_t *self, char *buf, int *lsize);
	


	
	int (*sms_test)( gprs_t *self, char *phnNmbr, char *buf, int bufsize);
	int (*buf_test)( gprs_t *self, char *buf, int len);
	int (*tcp_test)( gprs_t *self, char *tets_addr, int portnum, char *buf, int bufsize);
	
	int (*report_event)( gprs_t *self, char *buf, int *lsize);
//	void (*free_event)( gprs_t *self, void *event);
	int (*deal_tcpclose_event)(gprs_t *self);
	int (*deal_tcprecv_event)(gprs_t *self,  char *buf, int *len);
	int (*deal_smsrecv_event)(gprs_t *self, char *buf, int *lsize, char *phno);
	
	int	(*get_firstDiscnt_seq)(gprs_t *self);
	int	(*get_firstCnt_seq)(gprs_t *self);
	
	void ( *run)(gprs_t *self);
	

};



typedef enum {
	GPRSERROR,
	SHUTDOWN,
	TCP_IP_ERROR,
	STARTUP,
    GPRS_OPEN_FINISH,       /// GPRS �򿪳ɹ���
	INIT_FINISH_OK,
    TCP_IP_OK,
}SIM_STATUS ;
// { |len| data[0]| ... | data[len - 1]|}
#define EXTRASPACE		2		//ά������Ķ���ռ䣬�����г����ֶ�������u16
#define ADD_RECVBUF_WR(recvbuf) { \
	recvbuf->write ++;	\
	recvbuf->write &= ( recvbuf->buf_len - 1);\
}

#define ADD_RECVBUF_RD(recvbuf) { \
	recvbuf->read ++;	\
	recvbuf->read &= ( recvbuf->buf_len - 1);\
}


typedef struct {
	uint16_t	read;
	uint16_t	write;
	uint16_t	free_size;
	uint16_t	buf_len;
	char*		buf;
}RecvdataBuf;

//typedef enum {
//	sms_urc = 1,
//	tcp_receive,
//	tcp_close,
//	
//}SIM_Event;

#define SET_EVENT( event, flag)  ( event | ( 1 << flag) )
#define CKECK_EVENT( target, flag)  ( ( ( gprs_event_t*)target)->type ==  flag)
#define CLR_EVENT( event, flag)  ( event & ~( 1 << flag) )


gprs_t *GprsGetInstance(void);

int compare_phoneNO(char *NO1, char *NO2);
int check_phoneNO(char *NO);
int copy_phoneNO(char *dest_NO, char *src_NO);
int check_ip(char *ip);
int get_apn( gprs_t *self, char *buf);
int Grps_SetCipmode( short mode);
int Grps_SetCipmux( short mux);
void GprsTcpCnnectBeagin();
void GprsTcpCnnectFinish();

int GPRS_send_sms(int fd, char *data);

#endif
