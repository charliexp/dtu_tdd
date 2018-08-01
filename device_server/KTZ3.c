//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

#include <string.h>

#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "osObjects.h"                      // RTOS object definitions

#include "sdhError.h"


#include "gprs.h"
#include "serial485_uart.h"
#include "modbus_master.h"
#include "dtuConfig.h"

#include "KTZ3.h"

/*
���ڲɼ�KTZ3�豸;
����������������������,��ʱֻ���Ƕ��ŷ�ʽ������ͨ��

���õ�485���ڱ����Ѿ���ʼ������

*/



//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

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
#define NUM_WR_MSGS		16
//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------

//��ΪĿǰ�Ƕ��ŷ����ܣ�������������Ĵ�����д���ܣ��Ͳ���������д��
typedef struct {
	uint8_t			flag;					//0 ���� 1 Щ��Ϣ��Ч
	uint8_t			slave_id;
	uint8_t			reg_type;
	uint8_t			addr;
	
	uint16_t		val;
	
	int					msg_src;		//��ϢԴ������дֵ����󷵻ؽ������ϢԴ
}wr_modbus_msg_t;


typedef struct {
	uint8_t			reg_0x;		//״̬λ �ɶ�д
	uint8_t			reg_1x;		//״̬λ,ֻ��
	uint16_t		reg_3x[5];		//ֻ��
	uint16_t		reg_4x[8];		//�ɶ�д
}ktz3_reg_t;


typedef struct {
	
	wr_modbus_msg_t		arr_wr_msg[NUM_WR_MSGS];
	ktz3_reg_t				arr_dev_reg[NUM_DVS_SLAVE_DEV];
	char							arr_dev_flag[NUM_DVS_SLAVE_DEV];		//0 �豸δ���� 1 �豸������
	
	
}ktz3_data_t;
//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------


static void ktz3_run(Device_server *self);


osThreadId tid_ktz3;                                          // thread id
osThreadDef (ktz3_run, osPriorityNormal, 1, 0);                   // thread object

static ktz3_data_t *ktz3_data;


const uint16_t num_addr_x[4] = {5,7,5,8};//��Ӧ0X,1X,3X,4X		ÿ��Ĵ���������

	
//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static 	int ktz3_init(Device_server *self);
static int ktz3_data_down(Device_server *self, int data_src, void *data_buf, int data_len);
static int ktz3_deal_write_msg(Device_server *self, uint8_t *buf, int buf_size);
static void ktz3_poll_modbus_dev(Device_server *self, uint8_t *buf, int buf_size);


static int ktz3_get_datatype(char *s);
static int ktz3_get_datavalue(char *s);
static int ktz3_datatype_2_modbus_addr();
static int ktz3_put_modbus_val();
static int ktz3_datatype_2_string(char *buf, int buf_size);


static int ktz3_up_read_ack(uint8_t slave_addr, uint8_t func_code, uint8_t num_byte, uint8_t *data);
static int ktz3_up_write_ack(uint8_t slave_addr, uint8_t func_code, uint16_t reg_addr, uint16_t val);		
static int ktz3_up_err(uint8_t slave_addr, uint8_t func_code, uint8_t err_code);

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//



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
	
	uint8_t  modbus_buf[64];
	while(1)
	{
		threadActive();
		
		ktz3_deal_write_msg(self, modbus_buf, 64);
		ktz3_poll_modbus_dev(self, modbus_buf, 64);
		osDelay(500);
//		osThreadYield();         
		
	}
	
}

static int ktz3_data_down(Device_server *self, int data_src, void *data_buf, int data_len)
{
	//��ȡĿ���豸��ַ
	
	//��ȡ��Ϣ���ͣ���ȡ��д��
	
	//д��Ϣ����,Ҫ���ǿ��ܴ��ڶ��дֵ�����
		//��ȡһ��wr_modbus_msg_t
	
	
		//��������������ת����Modbus��ַ�ͼĴ�������
	
		//��ȡдֵ
	
		//������Դ��д��ַ���Ĵ������ͣ�дֵ����wr_modbus_msg_t
	
		
	
	
	//����Ϣ����Ҫ���Ƕ�����ݵĶ�ȡ���
			//�����ʵ��������͵�ֵת�����ַ���
	
			//������ַ�����ϳɸ�ʽ�����ظ�����Դ
	
			

	return 0;
	
}
static ktz3_reg_t *ktz3_get_reg(uint8_t SlaveID)
{
	
	
}

static int ktz3_up_read_ack(uint8_t slave_addr, uint8_t func_code, uint8_t num_byte, uint8_t *data)
{
	
	ktz3_reg_t *p_reg;
	uint16_t tmp_u16;
	uint8_t i, j;
	
	p_reg = ktz3_get_reg(slave_addr);
	if(p_reg == NULL)
		return -1;
	
	
	switch(func_code)
	{
		case MDM_U8_READ_COILS:	
				p_reg->reg_0x = *data;
				break;
		case MDM_U8_READ_DISCRETEINPUTS:	
				p_reg->reg_1x =  *data;
				break;
		case MDM_U16_READ_HOLD_REG:	
				for(i = 0, j = 0; i < num_byte; i+= 2)
				{
					tmp_u16 = data[i] | (data[i + 1] << 8);
					p_reg->reg_3x[j++] = tmp_u16;
					
				}
			
			
				break;
		case MDM_U16_READ_INPUT_REG:	
				for(i = 0, j = 0; i < num_byte; i+= 2)
				{
					tmp_u16 = data[i] | (data[i + 1] << 8);
					p_reg->reg_4x[j++] = tmp_u16;
					
				}
				break;
		
	}
	
	return 0;
	
}

static int ktz3_up_write_ack(uint8_t slave_addr, uint8_t func_code, uint16_t reg_addr, uint16_t val)
{
	
	
}

static int ktz3_up_err(uint8_t slave_addr, uint8_t func_code, uint8_t err_code)
{
	
	return 0;
}
static int ktz3_deal_write_msg(Device_server *self, uint8_t *buf, int buf_size)
{
	
		//�Ի����д��Ϣ���д���
	
}

static void ktz3_read_dev(uint8_t mdb_id)
{
	
	uint8_t		mdb_buf[64];
	int			pdu_len;	

	
	
	//��ȡ0X�Ĵ���
	pdu_len = ModbusMaster_readCoils(mdb_id, 1, num_addr_x[0], mdb_buf, sizeof(mdb_buf));
	if(pdu_len < 0)
		return;
	if(s485_Uart_write((char		*)mdb_buf, pdu_len) != ERR_OK)
		goto cmm_err;
	pdu_len = s485_Uart_read((char		*)mdb_buf, sizeof(mdb_buf));
	if(pdu_len <= 0)
		goto cmm_err;
	ModbusMaster_decode_pkt(mdb_buf, pdu_len);
	
	//��ȡ1X�Ĵ���
	pdu_len = ModbusMaster_readCoils(mdb_id, 1, num_addr_x[1], mdb_buf, sizeof(mdb_buf));
	if(pdu_len < 0)
		return;
	if(s485_Uart_write((char		*)mdb_buf, pdu_len) != ERR_OK)
		goto cmm_err;
	pdu_len = s485_Uart_read((char		*)mdb_buf, sizeof(mdb_buf));
	if(pdu_len <= 0)
		goto cmm_err;
	ModbusMaster_decode_pkt(mdb_buf, pdu_len);
	
	//��ȡ3X�Ĵ���
	pdu_len = ModbusMaster_readHoldingRegisters(mdb_id, 1, num_addr_x[2], mdb_buf, sizeof(mdb_buf));
	if(pdu_len < 0)
		return;
	if(s485_Uart_write((char		*)mdb_buf, pdu_len) != ERR_OK)
		goto cmm_err;
	pdu_len = s485_Uart_read((char		*)mdb_buf, sizeof(mdb_buf));
	if(pdu_len <= 0)
		goto cmm_err;
	ModbusMaster_decode_pkt(mdb_buf, pdu_len);
	
	//��ȡ4X�Ĵ���
	pdu_len = ModbusMaster_readInputRegisters(mdb_id, 1, num_addr_x[3], mdb_buf, sizeof(mdb_buf));
	if(pdu_len < 0)
		return;
	if(s485_Uart_write((char		*)mdb_buf, pdu_len) != ERR_OK)
		goto cmm_err;
	pdu_len = s485_Uart_read((char		*)mdb_buf, sizeof(mdb_buf));
	if(pdu_len <= 0)
		goto cmm_err;
	ModbusMaster_decode_pkt(mdb_buf, pdu_len);
	
	cmm_err:
	
		return;
	
}

static void ktz3_poll_modbus_dev(Device_server *self, uint8_t *buf, int buf_size)
{

	int i;
	
	
	//��ȡ�豸�ļĴ���
	for(i = 0; i < NUM_DVS_SLAVE_DEV; i++)
	{
		ktz3_read_dev(i);
		
		
	}
	
	
}

