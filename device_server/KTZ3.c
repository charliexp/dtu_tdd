//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//
#include <stdio.h>
#include <string.h>

#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "osObjects.h"                      // RTOS object definitions

#include "sdhError.h"


#include "gprs.h"
#include "serial485_uart.h"
#include "modbus_master.h"
#include "dtuConfig.h"

#include "config/tdd_cfg.h"
#include "KTZ3.h"

/*
���ڲɼ�KTZ3�豸;
����������������������,��ʱֻ���Ƕ��ŷ�ʽ������ͨ��

���õ�485���ڱ����Ѿ���ʼ������

ʵ�ֵĹ��ܣ�
1 KTZZ3 �����Բɼ�
2 ��modbus_wr_obj_tת����modbus ָ����豸
3 ���ŷ���Э�����
4 ��KTZ3��Ϣ��ʽ���ɶ���Э��涨�Ľ������

*/



//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------
//const char *ktz3_head_format = "KTZ3 Addr:%d \n";
//const char *ktz3_0x_1x_format = "0X=%xh,\t1x=%xh\n";
//const char *ktze_3x_format = "3001=%04xh,\t3002=%x04h\n3003=%xh,\t3004=%x04h\n3005=%x04h\n";
//const char *ktze_4x_format = "4001=%04xh,\t4002=%x04h\n4003=%xh,\t4004=%x04h\n4005=%x04h,\t4006=%x04h\n4007=%x04h,\t4008=%x04h\n";

//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// global function prototypes
//------------------------------------------------------------------------------

//============================================================================//
//            P R I V A T E   D E F I N I T I O N S                           //
//============================================================================//


//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------
#define KTZ3_NUM_WR_OBJ		16

//д����Ĵ�������������������ֵ���⣬����modbus���쳣��:1,2,3,4
#define KTZ3_OBJ_SUCCEED	0x80
#define KTZ3_OBJ_TX_ERR		0x81
#define KTZ3_OBJ_RX_ERR		0x82


#define KTZ3_0X_START_REG_ADDR		1
#define KTZ3_1X_START_REG_ADDR		1


#define KTZ3_FLAG_TIME_OUT			1

#define KTZ3_CMD_READ			0
#define KTZ3_CMD_WRITE			1
#define KTZ3_NUM_CMD			2

#define KTZ3_ERR_HEAD					-1
#define KTZ3_ERR_CMD					-2
#define KTZ3_ERR_ADDR_FORMAT			-3
#define KTZ3_ERR_ADDR_RANGE				-4

//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------

typedef int (*ktz3_deal_cmd)(char *msg,  uint8_t msg_src, uint8_t sld);
//��ΪĿǰ�Ƕ��ŷ����ܣ�������������Ĵ�����д���ܣ��Ͳ���������д��
typedef struct {
	uint8_t			flag;					//0 ���� 1 Щ��Ϣ��Ч
	uint8_t			slave_id;
	uint16_t		reg_addr;
	
	uint16_t		val;
	
	int					msg_src;		//��ϢԴ������дֵ����󷵻ؽ������ϢԴ
}modbus_wr_obj_t;


typedef struct {
	uint8_t			reg_0x;		//״̬λ �ɶ�д
	uint8_t			reg_1x;		//״̬λ,ֻ��
	uint16_t		reg_3x[5];		//ֻ��
	uint16_t		reg_4x[8];		//�ɶ�д
}ktz3_reg_t;


typedef struct {
	
	modbus_wr_obj_t		arr_wr_msg[KTZ3_NUM_WR_OBJ];
	ktz3_reg_t				arr_dev_reg[NUM_DVS_SLAVE_DEV];
	char							arr_dev_flag[NUM_DVS_SLAVE_DEV];		//0 �豸δ���� 1 �豸������
	ktz3_deal_cmd		arr_func_cmd[KTZ3_NUM_CMD];
	
}ktz3_data_t;
//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------


static void ktz3_run(Device_server *self);


osThreadId tid_ktz3;                                          // thread id
osThreadDef (ktz3_run, osPriorityNormal, 1, 0);                   // thread object

static ktz3_data_t *ktz3_data;


const uint16_t ktz3_num_regs[4] = {6,7,5,8};//��Ӧ0X,1X,3X,4X		ÿ��Ĵ���������

	
//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static 	int ktz3_init(Device_server *self);
static int ktz3_data_down(Device_server *self, int data_src, void *data_buf, int data_len);
static int ktz3_deal_write_msg(Device_server *self);
static void ktz3_poll_modbus_dev(Device_server *self);

static int ktz3_reg_val_2_string(uint8_t slave_addr, uint16_t addr, char *buf, uint16_t buf_size);
static void ktz3_read_dev(uint8_t mdb_id);
static void ktz3_write_dev(uint8_t mdb_id);
static void ktz3_wr_cmd_ack(uint8_t msg_src, uint8_t mdb_id);		//д������֮��Ӧ����û�


static modbus_wr_obj_t *ktz3_get_free_obj(void);
//ͨ��id�����������ݴ洢��λ��
static int ktz3_salve_id_2_num(uint8_t slave_id);

static void ktz3_test_write_rtu(void);

static int ktz3_msg_get_slave_addr(char *msg, uint8_t *p_sld);
static int ktz3_msg_get_cmd(char *msg, uint8_t *cmd);
static int ktz3_msg_deal_wr_cmd(char *msg, uint8_t msg_src, uint8_t sld);
static int ktz3_msg_deal_rd_cmd(char *msg,  uint8_t msg_src, uint8_t sld);

static int ktz3_up_read_ack(uint8_t slave_addr, uint8_t func_code, uint8_t num_byte, uint8_t *data);
static int ktz3_up_write_ack(uint8_t slave_addr, uint8_t func_code, uint16_t reg_addr, uint16_t val);		
static int ktz3_up_err(uint8_t slave_addr, uint8_t func_code, uint8_t err_code);

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

uint8_t Check_bit(uint8_t *data, int bit)
{
	int i, j ;
	i = bit/8;
	j = bit % 8;
	return ( data[i] & ( 1 << j));


}

void Clear_bit(uint8_t *data, int bit)
{
	int i, j ;
	i = bit/8;
	j = bit % 8;
	data[i] &= ~( 1 << j);




}

void Set_bit(uint8_t *data, int bit)
{
	int i, j ;
	i = bit/8;
	j = bit % 8;
	data[i] |=  1 << j;




}



//key=val or key = val ���򷵻�ֱ�ӷ���0
//�Կո��β
int Get_key_val( char *s, char *key, char *val, short size)
{
	char *pp = s;
	char i = 0;
	
	memset( val, 0, size);
	while(1)
	{
		pp =  strstr( pp, key);
		if( pp == NULL)
			return -1;
		//��ֹ���ֽضϵ��������Ҫ����ls=2, ȴ�ҵ�cols=2ȥ��
		//���߳���x=2 ʱ�ҵ�xail=2ȥ��
		if(pp[-1] == ' ' && pp[strlen(key)] == '=')
			break;
		else
			pp ++;
	}
	pp += strlen(key);
	//ȥ���ո�
	while(1)
	{
		if( *pp == ' ')
			pp++;
		else
			break;
		
	}
	if( *pp != '=')
		return -1;
	pp ++;
	//ȥ���ո�
	while(1)
	{
		if( *pp == ' ')
			pp++;
		else
			break;
		
	}
	i = 0;
	while(1)
	{
		val[ i] = pp[i];
		i ++;
		if( pp[i] == ' ')
				break;
		if( i > ( size - 2))
			break;
		
	}
//	val[ i] = 0;
	return i;
}


CTOR(Ktz3)
SUPER_CTOR(Device_server);
FUNCTION_SETTING(Device_server.init, ktz3_init);
FUNCTION_SETTING(Device_server.run, ktz3_run);
FUNCTION_SETTING(Device_server.data_down, ktz3_data_down);
END_CTOR

//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//
/// \name Private Functions
/// \{
static 	int ktz3_init(Device_server *self)
{
	
	ktz3_data = malloc(sizeof(ktz3_data_t));
	if(ktz3_data == NULL)
		return -1;
	memset(ktz3_data, 0,  sizeof(ktz3_data_t));
	
	ktz3_data->arr_func_cmd[KTZ3_CMD_READ] = ktz3_msg_deal_rd_cmd;
	ktz3_data->arr_func_cmd[KTZ3_CMD_WRITE] = ktz3_msg_deal_wr_cmd;
	
	
	
	s485_Uart_ioctl(S485_UART_CMD_SET_RXBLOCK);
	s485_Uart_ioctl(S485UART_SET_RXWAITTIME_MS, 200);
	s485_Uart_ioctl(S485_UART_CMD_SET_TXBLOCK);
	s485_Uart_ioctl(S485UART_SET_TXWAITTIME_MS, 200);
	
	tid_ktz3 = osThreadCreate(osThread(ktz3_run), self);
	if (!tid_ktz3) return(-1);

	MDM_register_update(ktz3_up_read_ack, ktz3_up_write_ack, ktz3_up_err);
	return(0);
}

static void ktz3_run(Device_server *self)
{
#if TDD_KTZ3 == 1

	char  test_buf[64];
#endif
	while(1)
	{
		threadActive();
	#if TDD_KTZ3 == 1
		sprintf(test_buf, "KTZ3 ADDR=1\nwrite\n4001=0x18,4002=19\n0002=1\n");
		self->data_down(self, 0, test_buf, strlen(test_buf));
	#endif
		ktz3_poll_modbus_dev(self);
		ktz3_deal_write_msg(self);
		osDelay(1000);
//		osThreadYield();         
		
	}
	
}

 

static int ktz3_data_down(Device_server *self, int data_src, void *data_buf, int data_len)
{
	int		err_code = 0;
	uint8_t 	target_addr = 0;
	uint8_t		cmd;
	char		err_str[32];
	//��ȡĿ���豸��ַ������֤ͷ����Ϣ�Ƿ���ȷ
	if(ktz3_msg_get_slave_addr(data_buf, &target_addr) < 0)
	{
		err_code = KTZ3_ERR_HEAD;
		goto err_exit;
	}
	//��ȡ��Ϣ���ͣ���ȡ��д��
	if(ktz3_msg_get_cmd(data_buf, &cmd) < 0)
	{
		err_code = KTZ3_ERR_CMD;
		goto err_exit;
	}
	//����ָ��
	err_code = ktz3_data->arr_func_cmd[cmd](data_buf, data_src, target_addr);
	if(err_code == 0)
		return 0;

	
err_exit:
	
	

	switch(err_code)
	{
		case KTZ3_ERR_HEAD:
			sprintf(err_str,"\nERR:msg head err\n");
			break;
		case KTZ3_ERR_CMD:
			sprintf(err_str,"\nERR:unkonw cmd\n");
			break;

		case KTZ3_ERR_ADDR_RANGE:
			sprintf(err_str,"\nERR:reg addr out of range\n");
			break;
		case KTZ3_ERR_ADDR_FORMAT:
			sprintf(err_str,"\nERR:format err\n");
			break;
	}
	strcat(data_buf, err_str);
	GPRS_send_sms(data_src, data_buf);
	return 0;
	
}
static int ktz3_salve_id_2_num(uint8_t slave_id)
{
#if TDD_KTZ3 == 1
	return 0;
#endif
}

static int ktz3_up_read_ack(uint8_t slave_addr, uint8_t func_code, uint8_t num_byte, uint8_t *data)
{
	
	int num;

	uint16_t tmp_u16;
	uint8_t i, j;
	
	num = ktz3_salve_id_2_num(slave_addr);
	if(num < 0)
		return -1;
	
	
	switch(func_code)
	{
		case MDM_U8_READ_COILS:	
				//��Ȧ����ʼ�Ĵ�����1����������Ҫ
				ktz3_data->arr_dev_reg[num].reg_0x = (*data) << KTZ3_0X_START_REG_ADDR;
				break;
		case MDM_U8_READ_DISCRETEINPUTS:	
				ktz3_data->arr_dev_reg[num].reg_1x =  (*data) << KTZ3_1X_START_REG_ADDR;
				break;
		case MDM_U16_READ_HOLD_REG:	
				for(i = 0, j = 0; i < num_byte; i+= 2)
				{
					//��ˣ���λ��ǰ
					tmp_u16 = data[i + 1] | (data[i] << 8);
					ktz3_data->arr_dev_reg[num].reg_4x[j++] = tmp_u16;
					
				}
				break;
		case MDM_U16_READ_INPUT_REG:	
				for(i = 0, j = 0; i < num_byte; i+= 2)
				{
					//��ˣ���λ��ǰ
					tmp_u16 = data[i + 1] | (data[i] << 8);
					ktz3_data->arr_dev_reg[num].reg_3x[j++] = tmp_u16;
					
				}
				break;
		
	}
	
	return 0;
	
}

static int ktz3_up_write_ack(uint8_t slave_addr, uint8_t func_code, uint16_t reg_addr, uint16_t val)
{
	int num;

	uint16_t tmp_u16;
	uint8_t i, j;
	ktz3_reg_t		*p_reg;
	
	
	
	
	num = ktz3_salve_id_2_num(slave_addr);
	if(num < 0)
		return -1;
	p_reg = &ktz3_data->arr_dev_reg[num];

	
	switch(func_code)
	{
		case MDM_WRITE_SINGLE_COIL:	
				//��Ȧ����ʼ�Ĵ�����1����������Ҫ
			if(reg_addr > ktz3_num_regs[0])
				break;
			if(val == 0xff00)
				Set_bit(&p_reg->reg_0x, reg_addr);
			else if(val == 0x0000)
				Clear_bit(&p_reg->reg_0x, reg_addr);
				break;
		case MDM_WRITE_SINGLE_REG:
			if(reg_addr > ktz3_num_regs[3])
				break;			
			p_reg->reg_4x[reg_addr - 1] =  val;
			break;
		
		
	}
	
	return 0;
	
}

static int ktz3_up_err(uint8_t slave_addr, uint8_t func_code, uint8_t err_code)
{
	
	return 0;
}
static int ktz3_deal_write_msg(Device_server *self)
{
	uint8_t i, j;

#if TDD_KTZ3 == 1

//	ktz3_test_write_rtu();
	ktz3_write_dev(1);
	ktz3_wr_cmd_ack(0, 1);
#else	
	//���յ�ַ�Ի����д��Ϣ���д���
	for(i = 0; i < NUM_DVS_SLAVE_DEV; i++)
	{
		ktz3_write_dev(Dtu_config.dvs_slave_id[i]);

		
	}
	for(i = 0; i <= NUM_DVS_SLAVE_DEV; i++)
	{
		for(j = 0; j < ADMIN_PHNOE_NUM; j++)
			ktz3_wr_cmd_ack(j, Dtu_config.dvs_slave_id[i]);

		
	}
#endif

	
}

static void ktz3_poll_modbus_dev(Device_server *self)
{

	int i;
#if TDD_KTZ3 == 1
	ktz3_read_dev(1);
#else	
	//��ȡ�豸�ļĴ���
	for(i = 0; i < NUM_DVS_SLAVE_DEV; i++)
	{
		ktz3_read_dev(Dtu_config.dvs_slave_id[i]);
	}
#endif
	
}
static void ktz3_write_dev(uint8_t mdb_id)
{
	
	uint8_t		mdb_buf[64];
	int			pdu_len;
	int			ret;
	uint8_t 	i;
	
	
	
	for(i = 0; i < KTZ3_NUM_WR_OBJ; i ++)
	{
		
		if((ktz3_data->arr_wr_msg[i].flag == 0) || (ktz3_data->arr_wr_msg[i].slave_id != mdb_id))
			continue;
		if(ktz3_data->arr_wr_msg[i].reg_addr < 0x1000)
		{
			pdu_len = ModbusMaster_writeSingleCoil(ktz3_data->arr_wr_msg[i].slave_id,\
			ktz3_data->arr_wr_msg[i].reg_addr & 0xfff,ktz3_data->arr_wr_msg[i].val,mdb_buf, 64);
		}
		else 
		{
			pdu_len = ModbusMaster_writeSingleRegister(ktz3_data->arr_wr_msg[i].slave_id,\
			ktz3_data->arr_wr_msg[i].reg_addr & 0xfff,ktz3_data->arr_wr_msg[i].val, mdb_buf, 64);
			
		}
		
		
		
		if(s485_Uart_write((char*)mdb_buf, pdu_len) != ERR_OK)
		{
			ktz3_data->arr_wr_msg[i].flag = KTZ3_OBJ_RX_ERR;
			continue;
		}
		pdu_len = s485_Uart_read((char		*)mdb_buf, sizeof(mdb_buf));
		if(pdu_len > 0)
		{
			ret = ModbusMaster_decode_pkt(mdb_buf, pdu_len);
			if(ret)
			{
				
				ktz3_data->arr_wr_msg[i].flag = ret;
			}
			else
			{
				
				ktz3_data->arr_wr_msg[i].flag = KTZ3_OBJ_SUCCEED;
				
			}
		}
		else
		{
			
			ktz3_data->arr_wr_msg[i].flag = KTZ3_OBJ_RX_ERR;
	
			
		}
	}
	
}
//����modbus_wr_obj_t��flag�����ظ��û�
static void ktz3_wr_cmd_ack(uint8_t msg_src, uint8_t mdb_id)
{
	char		sms_buf[256];
	char		tmp_buf[32];
	int 		num;
	ktz3_reg_t		*p_reg;

	modbus_wr_obj_t	*p_obj;
	uint8_t 	i;
	uint8_t 	count = 0;
	uint8_t		has_wr_coil = 0;

	
	num = ktz3_salve_id_2_num(mdb_id);
	if(num < 0)
		return;
	p_reg = &ktz3_data->arr_dev_reg[num];
	
	sprintf(sms_buf,"KTZ3 ADDR=%d\nwrite\n", mdb_id);
	
	//����Ȧ��д��Ӧ��������Ӧ��
	for(i = 0, count = 0; i < KTZ3_NUM_WR_OBJ; i ++)
	{
		
		if((ktz3_data->arr_wr_msg[i].flag == 0) || (ktz3_data->arr_wr_msg[i].slave_id != mdb_id) || \
			(ktz3_data->arr_wr_msg[i].msg_src != msg_src))
			continue;
		p_obj = &ktz3_data->arr_wr_msg[i];
		count ++;
		
		sprintf(tmp_buf,"%x", ktz3_data->arr_wr_msg[i].reg_addr);
		strcat(sms_buf, tmp_buf);
		switch(ktz3_data->arr_wr_msg[i].flag)
		{
				
			case KTZ3_OBJ_SUCCEED:
				if(p_obj->reg_addr > 0x1000)
					sprintf(tmp_buf,"=0x%04X", p_reg->reg_4x[p_obj->reg_addr & 0xfff]);
				else
				{
					has_wr_coil = 1;
					ktz3_data->arr_wr_msg[i].flag = 0;
					continue;
				}
				break;
			case KTZ3_OBJ_TX_ERR:
				sprintf(tmp_buf,":485 tx err");
				break;
			case KTZ3_OBJ_RX_ERR:
				sprintf(tmp_buf,":485 rx err");
				break;
			default:
				sprintf(tmp_buf,":mdb err[%d]", ktz3_data->arr_wr_msg[i].flag);
				break;
			
		}
		
		
		if(count & 1)
			strcat(tmp_buf,",");
		else
			strcat(tmp_buf,"\n");		//ż�����ӻ���
		strcat(sms_buf, tmp_buf);
		
		ktz3_data->arr_wr_msg[i].slave_id = 0;
		ktz3_data->arr_wr_msg[i].flag = 0;
	}
	if(has_wr_coil)
		sprintf(tmp_buf,"=0x%X\n", p_reg->reg_0x);
	strcat(sms_buf, tmp_buf);
	
	if(count || has_wr_coil)
		GPRS_send_sms(msg_src, sms_buf);
}

static void ktz3_read_dev(uint8_t mdb_id)
{
	
	uint8_t		mdb_buf[64];
	int			pdu_len;	

	if(mdb_id < 1)
		return;
	
	//��ȡ0X�Ĵ���
	pdu_len = ModbusMaster_readCoils(mdb_id, KTZ3_0X_START_REG_ADDR, ktz3_num_regs[0], mdb_buf, sizeof(mdb_buf));
	if(pdu_len < 0)
		return;
	if(s485_Uart_write((char		*)mdb_buf, pdu_len) != ERR_OK)
		goto cmm_err;
	pdu_len = s485_Uart_read((char		*)mdb_buf, sizeof(mdb_buf));
	if(pdu_len > 0)
	{
		ModbusMaster_decode_pkt(mdb_buf, pdu_len);
	}
	
	//��ȡ1X�Ĵ���
	pdu_len = ModbusMaster_readDiscreteInputs(mdb_id, KTZ3_1X_START_REG_ADDR, ktz3_num_regs[1], mdb_buf, sizeof(mdb_buf));
	if(pdu_len < 0)
		return;
	if(s485_Uart_write((char		*)mdb_buf, pdu_len) != ERR_OK)
		goto cmm_err;
	pdu_len = s485_Uart_read((char		*)mdb_buf, sizeof(mdb_buf));
	if(pdu_len > 0)
	{
		ModbusMaster_decode_pkt(mdb_buf, pdu_len);
	}
	
	//��ȡ3X�Ĵ���
	pdu_len = ModbusMaster_readHoldingRegisters(mdb_id, 1, ktz3_num_regs[3], mdb_buf, sizeof(mdb_buf));
	if(pdu_len < 0)
		return;
	if(s485_Uart_write((char		*)mdb_buf, pdu_len) != ERR_OK)
		goto cmm_err;
	pdu_len = s485_Uart_read((char		*)mdb_buf, sizeof(mdb_buf));
	if(pdu_len > 0)
	{
		ModbusMaster_decode_pkt(mdb_buf, pdu_len);
	}
	
	//��ȡ4X�Ĵ���
	pdu_len = ModbusMaster_readInputRegisters(mdb_id, 1, ktz3_num_regs[2], mdb_buf, sizeof(mdb_buf));
	if(pdu_len < 0)
		return;
	if(s485_Uart_write((char		*)mdb_buf, pdu_len) != ERR_OK)
		goto cmm_err;
	pdu_len = s485_Uart_read((char		*)mdb_buf, sizeof(mdb_buf));
	if(pdu_len > 0)
	{
		ModbusMaster_decode_pkt(mdb_buf, pdu_len);
	}
	
	cmm_err:
	
		return;
	
}


//���ƶ��ļĴ�����ֵת�����ַ���
//addr = 0 ȫ����ַ������
//addr={0134}{Xx} ĳһ�������
//addr=abcd ����ĵ�ַ
//todo:Ŀǰֻʵ��addr=0�Ĺ���
//�����ַ����ĳ���
static int ktz3_reg_val_2_string(uint8_t slave_addr, uint16_t addr, char *buf, uint16_t buf_size)
{
	
	

	int num;

	ktz3_reg_t		*p_reg;
	
	num = ktz3_salve_id_2_num(slave_addr);
	if(num < 0)
		return -1;
	
	p_reg = &ktz3_data->arr_dev_reg[num];
//�ַ�������д����һ�в����пո�,�����ѿո�Ҳ������
	sprintf(buf, "KTZ3 Addr:%d\r\n\
0X=0x%X,1X=0x%X\r\n\
3001=0x%04X,3002=0x%04X\r\n\
3003=0x%04X,3004=0x%04X\r\n\
3005=0x%04X\r\n\
4001=0x%04X,4002=0x%04X\r\n\
4003=0x%04X,4004=0x%04X\r\n\
4005=0x%04X,4006=0x%04X\r\n\
4007=0x%04X,4008=0x%04X\r\n", \
	slave_addr,
	p_reg->reg_0x, p_reg->reg_1x, \
	p_reg->reg_3x[0], p_reg->reg_3x[1], p_reg->reg_3x[2], p_reg->reg_3x[3], p_reg->reg_3x[4], \
	p_reg->reg_4x[0], p_reg->reg_4x[1], p_reg->reg_4x[2], p_reg->reg_4x[3],\
	p_reg->reg_4x[4], p_reg->reg_4x[5], p_reg->reg_4x[6], p_reg->reg_4x[7]);
	
}


static void ktz3_test_write_rtu(void)
{
	
	uint8_t		mdb_buf[64];
	int			pdu_len;	
	ktz3_reg_t		*p_reg;
	
	
	
	p_reg = &ktz3_data->arr_dev_reg[0];
	
	if(p_reg->reg_0x & 0x2)
		pdu_len = ModbusMaster_writeSingleCoil(1,1,0,mdb_buf, 64);
	else
		pdu_len = ModbusMaster_writeSingleCoil(1,1,1,mdb_buf, 64);
	
	if(s485_Uart_write((char*)mdb_buf, pdu_len) != ERR_OK)
		goto wr_err;
	pdu_len = s485_Uart_read((char		*)mdb_buf, sizeof(mdb_buf));
	if(pdu_len > 0)
	{
		ModbusMaster_decode_pkt(mdb_buf, pdu_len);
	}
	
	pdu_len = ModbusMaster_writeSingleRegister(1,1,p_reg->reg_4x[0] + 1, mdb_buf, 64);

	
	if(s485_Uart_write((char*)mdb_buf, pdu_len) != ERR_OK)
		goto wr_err;
	pdu_len = s485_Uart_read((char		*)mdb_buf, sizeof(mdb_buf));
	if(pdu_len > 0)
	{
		ModbusMaster_decode_pkt(mdb_buf, pdu_len);
	}
	
wr_err:
	return;
}

//�ж���Ϣ�Ƿ���:KTZ3 ��ͷ
//��ȡmodbus rtu��ַ
static int ktz3_msg_get_slave_addr(char *msg, uint8_t *p_sld)
{
	char *p_key;
	char s_sld[4];
	uint8_t sld;
	uint8_t	i;
	
	
	p_key = strstr( msg, "KTZ3");
	if(p_key == NULL)
		return -1;
	
	if(Get_key_val(msg,"addr", s_sld, 4) < 0)
	{
		if(Get_key_val(msg,"ADDR", s_sld, 4) < 0)
			return -1;
	}
	sld = atoi(s_sld);
	
	//��ַ�Ϸ��Լ��
	for(i = 0; i < NUM_DVS_SLAVE_DEV; i++)
	{
		if((sld == Dtu_config.dvs_slave_id[i]) && (sld >= 1) &&(sld <= 247))
		{
			*p_sld = sld;
			return 0;
		}
		
	}
	
	return -1;
	
}
static int ktz3_msg_get_cmd(char *msg, uint8_t *cmd)
{
	char *p_key;
	
	
	p_key = strstr( msg, "read");
	if(p_key)
	{
		*cmd = KTZ3_CMD_READ;
		return 0;
		
	}
	p_key = strstr( msg, "Read");
	if(p_key)
	{
		*cmd = KTZ3_CMD_READ;
		return 0;
		
	}
	p_key = strstr( msg, "READ");
	if(p_key)
	{
		*cmd = KTZ3_CMD_READ;
		return 0;
		
	}
	
	p_key = strstr( msg, "write");
	if(p_key)
	{
		*cmd = KTZ3_CMD_WRITE;
		return 0;
		
	}
	p_key = strstr( msg, "Write");
	if(p_key)
	{
		*cmd = KTZ3_CMD_WRITE;
		return 0;
		
	}
	p_key = strstr( msg, "WRITE");
	if(p_key)
	{
		*cmd = KTZ3_CMD_WRITE;
		return 0;
		
	}
	
	
	return -1;
	
}

static modbus_wr_obj_t *ktz3_get_free_obj(void)
{
	uint8_t i;
	
	for(i = 0; i < KTZ3_NUM_WR_OBJ; i++)
	{
		
		if(ktz3_data->arr_wr_msg[i].flag == 0)
		{
			ktz3_data->arr_wr_msg[i].flag = 1;
			return &ktz3_data->arr_wr_msg[i];
		}
		
	}
	return NULL;
	
}

//���ص�ַ������ߵ�ַ��Χ����Ĵ�����
static int ktz3_msg_deal_wr_cmd(char *msg,  uint8_t msg_src, uint8_t sld)
{
	
	
	char *p_key;
	char *p_data;
	modbus_wr_obj_t *wr_obj;
	uint16_t reg_addr[10], reg_val[10];
	short	err_code;
	uint8_t n;
	uint8_t i;
	//д�Ĵ���=дֵ ���д�ָ�����һ�п�ʼ
	p_key = strstr( msg, "Write");
	if(p_key)
	{
		goto move_first_rea_addr;
	}
	p_key = strstr( msg, "write");
	if(p_key)
	{
		goto move_first_rea_addr;
	}
	p_key = strstr( msg, "WRITE");
	if(p_key)
	{
		goto move_first_rea_addr;
	}
move_first_rea_addr:
	
	while(*p_key != '\0')
	{
		p_key ++;
		if(*p_key == '\n')
		{
			break;
		}
		
	}

	n = 0;
	p_data = p_key;
	//��ȡд�Ĵ�����ַ��дֵ
	while(*p_data != '\0')
	{
		//��ȡ�Ĵ�����ַ
		reg_addr[n] = strtoul(p_data, &p_data, 16);
		
		//����ַ��Χ
		if(reg_addr[n] == 0)
			break;
		
		if(((reg_addr[n] > ktz3_num_regs[0]) && ( reg_addr[n] < 0x4000)) || (reg_addr[n] > (0x4000 + ktz3_num_regs[3])))
		{
			err_code = KTZ3_ERR_ADDR_RANGE;
			goto err;
		}
		
		//��ȡ�Ĵ�����ֵ, ������0x��ͷ����16���ƣ���1-9��ͷ����10���ƣ���0��ͷ���ǰ˽���
		reg_val[n]= strtoul(p_data + 1, &p_data, 0);
		p_data ++;
		n ++;
		if(n >= 10)
			break;
	}
	
	
	//������Դ��д��ַ���Ĵ������ͣ�дֵ����modbus_wr_obj_t
	for(i = 0; i < n; i++)
	{
		wr_obj = ktz3_get_free_obj();
		if(wr_obj == NULL)
		{
//			err_code = KTZ3_ERR_BUSY;
//			goto err;
			break;
		}
		wr_obj->slave_id = sld;
		wr_obj->msg_src = msg_src;
		wr_obj->reg_addr = reg_addr[i];
		wr_obj->val = reg_val[i];
	}
		

	return 0;
	err:
	
	return err_code;
	
}

//��ʱ���������ļĴ�����ѯ
static int ktz3_msg_deal_rd_cmd(char *msg,  uint8_t msg_src, uint8_t sld)
{

	
		
	
	
	//����Ϣ����Ҫ���Ƕ�����ݵĶ�ȡ���
	
	ktz3_reg_val_2_string(sld, KTZ3_CMD_READ, msg,0);		
	GPRS_send_sms(msg_src, msg);
			//�����ʵ��������͵�ֵת�����ַ���
	
			//������ַ�����ϳɸ�ʽ�����ظ�����Դ
	return 0;
}

