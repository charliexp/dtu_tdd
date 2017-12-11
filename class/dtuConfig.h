#ifndef __DTUCONFIG_H__
#define __DTUCONFIG_H__

#include <stdint.h>
#include "gprs.h"
#include "serial485_uart.h"
#include "sw_filesys.h"
#include "system.h"



#define MODE_PASSTHROUGH					0			//
#define MODE_SMS					1			//
#define MODE_REMOTERTU		2
#define MODE_LOCALRTU			3
#define MODE_BEGIN					0
#define MODE_END					4
#define NEED_ADC( mode)				( ( mode) > MODE_SMS)
#define NEED_GPRS( mode)				( ( mode) != MODE_LOCALRTU)

#define DTU_CONFGILE_MAIN_VER		2
#define DTU_CONFGILE_SUB_VER		1

#define DEF_PROTOTOCOL "TCP"
#define DEF_IPADDR "chitic.zicp.net"
#define DEF_PORTNUM 18897
#define	DTUCONF_filename	"sys.cfg"

#define SIGTYPE_0_5_V			10
#define SIGTYPE_1_5_V			11
#define SIGTYPE_4_20_MA			9
#define SIGTYPE_0_10_MA			8

#define PHONENO_LEN				16
#define ADMIN_PHNOE_NUM			4
#define REGPACKAGE_LEN			32
#define HEATBEATPACKAGE_LEN			32
#define IP_LEN		16
#define IP_ADDR_LEN		64



typedef struct {
	uint16_t		rangeH;
	uint16_t		rangeL;
	uint16_t		alarmH;
	uint16_t		alarmL;
}signRange_t;
typedef struct {
	
	char	multiCent_mode;			//������ģʽ
	char	work_mode;					//����ģʽ
	uint8_t	ver[2];						//�汾
	
	
	char	dns_ip[IP_LEN];
	char	smscAddr[16];
	char	apn[64];		//�ɽ���㣬�û��������� ���������
//	char	apn_username[32];
//	char	apn_passwd[16];
	char	DateCenter_ip[IPMUX_NUM][IP_ADDR_LEN];
	int		DateCenter_port[IPMUX_NUM];
	char	protocol[IPMUX_NUM][4];
	char	registry_package[32];
	
	char	sim_NO[PHONENO_LEN];
	char	admin_Phone[ADMIN_PHNOE_NUM][PHONENO_LEN];
	
	char			output_mode;
	char			chn_type[3];
	
	uint32_t	hartbeat_timespan_s;
	char		heatbeat_package[32];
	uint32_t		dtu_id;
	uint32_t		rtu_addr;
	
	
			
	ser_485Cfg	the_485cfg;
	
	signRange_t		sign_range[3];
}DtuCfg_t;

typedef void (* other_ack)( char *data, void *arg);

extern DtuCfg_t	Dtu_config;
extern sdhFile *DtuCfg_file;


int Init_system_config(void);
void set_default( DtuCfg_t *conf);
void Config_job(void);

int  Config_server( char *data, other_ack ack, void * arg);

#endif
