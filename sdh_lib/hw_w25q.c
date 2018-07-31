/*************************************************
Copyright (C), 
File name: w25q.c
Author:		sundh
Version:	v1.0
Date: 		14-02-27
Description: 
w25Q����������ʵ��W25Q�ı�׼SPI����ģʽ
ʹ��MCU��SPI0����w25qͨѶ
w25qҪ�ṩ�ܶิ�ӵĽӿڡ�
w25q��ʵ�������������ӿڡ�
Ϊ�˾������ٶ�flash�Ĳ��������ж�flash��д������������Ϊ��λ���С����ڶ�ȡ�ṩ�������ַ���ķ�����
Others: 
Function List: 
1. w25q_init
��ʼ��flash������ȡid��������Ϣ
History: 
1. Date:
Author:
Modification:
2. w25q_Erase_Sector
����flash�е�ѡ����ĳһ������
History: 
1. Date:
Author:
Modification:
3. w25q_Write_Sector_Data
��һ��ָ��������д������
History: 
1. Date:
Author:
Modification:
4. w25q_Read_Sector_Data
��ȡһ������
History: 
1. Date:
Author:
Modification:
5. w25q_rd_data
��ȡ�����ַ
History: 
1. Date:
Author:
Modification:
*************************************************/
#include "hw_w25q.h"
#include <string.h>
#include "sdhError.h"




static w25q_instance_t  W25Q_flash;


//--------------����˽������--------------------------------
static uint8_t	W25Q_tx_buf[16];
//static uint8_t	W25Q_rx_buf[16];
//------------����˽�к���------------------------------
static int w25q_wr_enable(void);
static uint8_t w25q_ReadSR(void);
//static uint8_t w25q_ReadSR2(void);
static int w25q_write_waitbusy(uint8_t *data, int len);
static int w25q_read_id(void);

static int w25q_rd_page(uint8_t *pBuffer, uint16_t pg_num, uint16_t len);
static int w25q_wr_page(uint8_t *pBuffer, uint16_t pg_num, uint16_t len);
//--------------------------------------------------------------

int w25q_init(void)
{
	static char first = 1;
	if( first)
	{
		w25q_init_cs();
		w25q_init_spi();
			
		
		W25Q_Disable_CS;
		
		W25Q_tx_buf[0] = 0xff;
		first = 0;
	}
		
	return w25q_read_id();


}


//���ṩ���������в��������������ŵķ�Χ��0 - 4096 ��w25q128��

int w25q_Erase_Sector(uint16_t Sector_Number)
{

	uint8_t Block_Num = Sector_Number / BLOCK_HAS_SECTORS;
	if( Sector_Number > W25Q_flash.sector_num)
			return ERR_BAD_PARAMETER;
	
	Sector_Number %= BLOCK_HAS_SECTORS;
	W25Q_tx_buf[0] = W25Q_INSTR_Sector_Erase_4K;
	W25Q_tx_buf[1] = Block_Num;
	W25Q_tx_buf[2] = Sector_Number<<4;
	W25Q_tx_buf[3] = 0;
	return w25q_write_waitbusy( W25Q_tx_buf, 4);
	
}

int W25Q_write_flash(uint32_t addr, uint8_t *buf, uint32_t bytes)
{
	int 			pg;
	int 			ret;
	uint32_t	wr_bytes = 0;
	pg = addr / PAGE_SIZE;
	
	while(bytes)
	{
		
		ret = w25q_wr_page(buf, pg, bytes);
		if( ret < 0)
			break;
		
		wr_bytes += ret;
		bytes -= ret;
		pg ++;
	}
	
	
	return wr_bytes;
	
}
//���ض�ȡ���ֽ��������ߴ���-1
int W25Q_read_flash(uint32_t addr, uint8_t *buf, uint32_t bytes)
{
	int 			pg;
	int 			ret;
	uint32_t	rd_bytes = 0;
	pg = addr / PAGE_SIZE;
	
	while(bytes)
	{
		
		ret = w25q_rd_page(buf, pg, bytes);
		if( ret < 0)
			break;
		
		rd_bytes += ret;
		bytes -= ret;
		pg++;
	}
	
	
	return rd_bytes;
	
}


#if NO_FILESYS == 0
///�������Ҫ��w25q_init�ɹ�֮����ò�����
void w25q_info(void *info)
{
	w25qInfo_t	*w25qinfo = ( w25qInfo_t *)info;
	
	w25qinfo->page_size = PAGE_SIZE;
	w25qinfo->total_pagenum = W25Q_flash.page_num;
	w25qinfo->sector_pagenum = SECTOR_HAS_PAGES;
	
	w25qinfo->block_pagenum = BLOCK_HAS_SECTORS * SECTOR_HAS_PAGES;

}



int w25q_close(void)
{
	
	return ERR_OK;
	
}



int w25q_erase(uint32_t offset, uint32_t len)
{
	uint32_t start, end, erase_size;
	int ret = 0;
	
	erase_size = SECTOR_SIZE;
	W25Q_tx_buf[0] = W25Q_INSTR_Sector_Erase_4K;
	
	
	start = offset;
	end = start + len;
	
	while (offset < end) {
		W25Q_tx_buf[1] = offset >> 16;
		W25Q_tx_buf[2] = offset >> 8;
		W25Q_tx_buf[3] = offset >> 0;
		offset += erase_size;

		ret = w25q_write_waitbusy( W25Q_tx_buf, 4);
		if( ret != ERR_OK)
			goto exit;
		
		
	}
	return ERR_OK;
	exit:
	W25Q_Disable_CS;
	return ret;	
	
}




int w25q_Erase_block(uint16_t block_Number)
{

	if( block_Number > W25Q_flash.block_num)
		return ERR_BAD_PARAMETER;
	
	W25Q_tx_buf[0] = W25Q_INSTR_BLOCK_Erase_64K;
	W25Q_tx_buf[1] = block_Number;
	W25Q_tx_buf[2] = 0;
	W25Q_tx_buf[3] = 0;
	return w25q_write_waitbusy( W25Q_tx_buf, 4);
	
}

int w25q_Erase_chip_c7(void)
{

	W25Q_tx_buf[0] = W25Q_INSTR_Chip_Erase_C7;
	
	return w25q_write_waitbusy( W25Q_tx_buf, 1);
	
}
int w25q_Erase_chip_60(void)
{

	W25Q_tx_buf[0] = W25Q_INSTR_Chip_Erase_60;
	
	return w25q_write_waitbusy( W25Q_tx_buf, 1);
	
}



int w25q_Write(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t WriteBytesNum)
{

		short step = 0;
		short count = 100;
		int ret = -1;
		
		while(1)
		{
			switch( step)
			{
				case 0:
					if( w25q_wr_enable() != ERR_OK)
					{
						ret =  ERR_DRI_OPTFAIL;
						goto exit;
					}
					
					W25Q_Enable_CS;
					W25Q_tx_buf[0] = W25Q_INSTR_Page_Program;
					W25Q_tx_buf[1] = (uint8_t)((WriteAddr&0x00ff0000)>>16);
					W25Q_tx_buf[2] = (uint8_t)((WriteAddr&0x0000ff00)>>8);
					W25Q_tx_buf[3] = (uint8_t)WriteAddr;
					if( SPI_WRITE( W25Q_tx_buf, 4) != ERR_OK)
					{
						ret =  ERR_DRI_OPTFAIL;
						goto exit;
					}
					if( SPI_WRITE( pBuffer, WriteBytesNum) != ERR_OK)
					{
						ret =  ERR_DRI_OPTFAIL;
						goto exit;
					}
					W25Q_Disable_CS;
					
					step++;
					break;
				case 1:
					if( w25q_ReadSR() & W25Q_STATUS1_BUSYBIT)
					{
						W25Q_DELAY_MS(1);
						if( count)
							count --;
						else
						{
							ret =  ERR_DEV_TIMEOUT;
							goto exit;
						}
						break;
					}
					
					ret =  ERR_OK;
					goto exit;
			
				default:
					step = 0;
					break;
				
			}		//switch
			
			
		}		//while(1)
		
		exit:
		
		
		return ret;
}


int w25q_Write_Sector_Data(uint8_t *pBuffer, uint16_t Sector_Num)
{
	int wr_page = 0;
	int		ret;
	while(1)
	{
		ret = w25q_Write(pBuffer + wr_page*PAGE_SIZE, Sector_Num*SECTOR_SIZE + wr_page * PAGE_SIZE, PAGE_SIZE);
		if( ret != ERR_OK)
			return ret;
		wr_page ++;
		if( wr_page >= SECTOR_HAS_PAGES)
			return ERR_OK;
	}
   
}


int w25q_Read_Sector_Data(uint8_t *pBuffer, uint16_t Sector_Num)
{
	uint8_t Block_Num = Sector_Num / BLOCK_HAS_SECTORS;
	if( Sector_Num > W25Q_flash.sector_num)
		return ERR_BAD_PARAMETER;
	memset( pBuffer, 0xff, SECTOR_SIZE);
	
	W25Q_Enable_CS;
	W25Q_tx_buf[0] = W25Q_INSTR_READ_DATA;
	W25Q_tx_buf[1] = Block_Num;
	W25Q_tx_buf[2] = Sector_Num<<4;
	W25Q_tx_buf[3] = 0;
	if( SPI_WRITE( W25Q_tx_buf, 4) != ERR_OK)
		return ERR_DRI_OPTFAIL;
	
	if( SPI_READ( pBuffer, SECTOR_SIZE) != ERR_OK)
		return ERR_DRI_OPTFAIL;
	W25Q_Disable_CS;
	return ERR_OK;
}

int w25q_Read_page_Data(uint8_t *pBuffer, uint16_t num_page)
{
	return w25q_rd_data( pBuffer, num_page * PAGE_SIZE, PAGE_SIZE);
//	uint8_t num_sector = num_page / SECTOR_HAS_PAGES;
//	uint8_t num_block = num_sector / ( BLOCK_HAS_SECTORS);
//	if( num_page > W25Q_flash.page_num)
//		return ERR_BAD_PARAMETER;
//	memset( pBuffer, 0xff, SECTOR_SIZE);
//	
//	W25Q_Enable_CS;
//	W25Q_tx_buf[0] = W25Q_INSTR_READ_DATA;
//	W25Q_tx_buf[1] = num_sector;
//	W25Q_tx_buf[2] = num_sector<<4;
//	W25Q_tx_buf[3] = num_page;
//	if( SPI_WRITE( W25Q_tx_buf, 4) != ERR_OK)
//		return ERR_DRI_OPTFAIL;
//	
//	if( SPI_READ( pBuffer, PAGE_SIZE) != ERR_OK)
//		return ERR_DRI_OPTFAIL;
//	W25Q_Disable_CS;
//	return ERR_OK;
}

int w25q_rd_data(uint8_t *pBuffer, uint32_t rd_add, int len)
{
	if( len > PAGE_SIZE)
		return ERR_BAD_PARAMETER;
	W25Q_tx_buf[0] = W25Q_INSTR_READ_DATA;
	W25Q_tx_buf[1] = (uint8_t)((rd_add&0x00ff0000)>>16);
	W25Q_tx_buf[2] = (uint8_t)((rd_add&0x0000ff00)>>8);
	W25Q_tx_buf[3] = (uint8_t)rd_add;
	W25Q_Enable_CS;
	
	if( SPI_WRITE( W25Q_tx_buf, 4) != ERR_OK)
		return ERR_DRI_OPTFAIL;
	if( SPI_READ( pBuffer, len) != ERR_OK)
		return ERR_DRI_OPTFAIL;
	W25Q_Disable_CS;
	return ERR_OK;

}








#endif


//-------------------------------------------------------------------
//���غ���
//--------------------------------------------------------------------
//static void w25q_WriteSR(uint8_t data)
//{
//    uint8_t d[2] ;
//    W25Q_Enable_CS;
//	
//	
//	d[0] = W25Q_INSTR_WR_Status_Reg;
//	d[1] = data;
//	SPI_WRITE( d, 2) ;
//	
//}


static int w25q_rd_page(uint8_t *pBuffer, uint16_t pg_num, uint16_t len)
{
	uint32_t  rd_add = pg_num * PAGE_SIZE;
	if( len > PAGE_SIZE)
			len = PAGE_SIZE;
	W25Q_tx_buf[0] = W25Q_INSTR_READ_DATA;
	W25Q_tx_buf[1] = (uint8_t)((rd_add&0x00ff0000)>>16);
	W25Q_tx_buf[2] = (uint8_t)((rd_add&0x0000ff00)>>8);
	W25Q_tx_buf[3] = (uint8_t)rd_add;
	W25Q_Enable_CS;
	
	if( SPI_WRITE( W25Q_tx_buf, 4) != ERR_OK)
		return ERR_DRI_OPTFAIL;
	if( SPI_READ( pBuffer, len) != ERR_OK)
		return ERR_DRI_OPTFAIL;
	W25Q_Disable_CS;
	return len;

}



static int w25q_wr_page(uint8_t *pBuffer, uint16_t pg_num, uint16_t len)
{

	short step = 0;
	short count = 100;
	int ret = -1;
	uint32_t WriteAddr = pg_num * PAGE_SIZE;


	if( len > PAGE_SIZE)
		len = PAGE_SIZE;
	
		while(1)
		{
			switch( step)
			{
				case 0:
					if( w25q_wr_enable() != ERR_OK)
					{
						ret =  ERR_DRI_OPTFAIL;
						goto exit;
					}
					
					W25Q_Enable_CS;
					W25Q_tx_buf[0] = W25Q_INSTR_Page_Program;
					W25Q_tx_buf[1] = (uint8_t)((WriteAddr&0x00ff0000)>>16);
					W25Q_tx_buf[2] = (uint8_t)((WriteAddr&0x0000ff00)>>8);
					W25Q_tx_buf[3] = (uint8_t)WriteAddr;
					if( SPI_WRITE( W25Q_tx_buf, 4) != ERR_OK)
					{
						ret =  ERR_DRI_OPTFAIL;
						goto exit;
					}
					if( SPI_WRITE( pBuffer, len) != ERR_OK)
					{
						ret =  ERR_DRI_OPTFAIL;
						goto exit;
					}
					W25Q_Disable_CS;
					
					step++;
					break;
				case 1:
					if( w25q_ReadSR() & W25Q_STATUS1_BUSYBIT)
					{
						W25Q_DELAY_MS(1);
						if( count)
							count --;
						else
						{
							ret =  ERR_DEV_TIMEOUT;
							goto exit;
						}
						break;
					}
					
					ret =  len;
					goto exit;
			
				default:
					step = 0;
					break;
				
			}		//switch
			
			
		}		//while(1)
		
		exit:
		
		
		return ret;
}

static int w25q_wr_enable(void)
{
	int ret;
//	int count = 50;
	uint8_t cmd = W25Q_INSTR_WR_ENABLE;
	
	
	
	W25Q_Enable_CS;
	ret = SPI_WRITE( &cmd,1);
	W25Q_Disable_CS;
	return ret;
//	if( ret != ERR_OK)
//		return ret;

//	while(1)
//	{

//		cmd = w25q_ReadSR();
//		if( ( cmd & W25Q_STATUS1_WEL)) 
//		{
//			return ERR_OK;
//		}
//		count --;
//	}
//	
//	return ERR_DEV_TIMEOUT;
	
	
}


static uint8_t w25q_ReadSR(void)
{
    uint8_t cmd = 0;
	uint8_t	status;
    W25Q_Enable_CS;
	
	
	cmd = W25Q_INSTR_RD_Status_Reg1;
	
	if( SPI_WRITE( &cmd, 1) != ERR_OK)
		return 0xff;
	if( SPI_READ( &status, 1) != ERR_OK)
		return 0xff;
    W25Q_Disable_CS;
    return status;
}

//static uint8_t w25q_ReadSR2(void)
//{
//    uint8_t retValue = 0;
//	
//    W25Q_Enable_CS;
//	
//	
//	retValue = W25Q_INSTR_RD_Status_Reg2;
//	
//	if( SPI_WRITE( &retValue, 1) != ERR_OK)
//		return 0xff;
//	if( SPI_READ( &retValue, 1) != ERR_OK)
//		return 0xff;
//    W25Q_Disable_CS;
//    return retValue;
//}

static int w25q_write_waitbusy(uint8_t *data, int len)
{
	short step = 0;
	short count = 100;
	int ret = -1;
	
	
	while(1)
	{
		switch( step)
		{	
			case 0:
				
				if( w25q_wr_enable() != ERR_OK)
				{
					ret =  ERR_DRI_OPTFAIL;
					goto exit;
				}
				
				W25Q_Enable_CS;
				if( SPI_WRITE( data, len) != ERR_OK)
				{
					ret =  ERR_DRI_OPTFAIL;
					goto exit;
				}
				step ++;
				W25Q_Disable_CS;

//				W25Q_DELAY_MS(1);
				break;
			case 1:
				if( w25q_ReadSR() & W25Q_STATUS1_BUSYBIT)
				{
					W25Q_DELAY_MS(1);
					if( count)
						count --;
					else
					{
						ret =  ERR_DEV_TIMEOUT;
						goto exit;
					}
					break;
				}
				
				ret =  ERR_OK;
				goto exit;
			default:
				step = 0;
				break;
		}
	}
		
	exit:
	W25Q_Disable_CS;
	return ret;

}


static int w25q_read_id(void)
{
	uint8_t id[2];
	//read id
	W25Q_tx_buf[0] = 0x90;
	W25Q_tx_buf[1] = 0;
	W25Q_tx_buf[2] = 0;
	W25Q_tx_buf[3] = 0;
	W25Q_Enable_CS;
	
	SPI_WRITE( W25Q_tx_buf, 4);
	SPI_READ( id, 2);
	
	W25Q_Disable_CS;
	W25Q_flash.id[0] =  id[0];
	W25Q_flash.id[1] =  id[1];
	if( id[0] == 0xEF && id[1] == 0x17)		//w25Q128
	{
		W25Q_flash.page_num = 65536;
		W25Q_flash.sector_num = W25Q_flash.page_num/SECTOR_HAS_PAGES;
		W25Q_flash.block_num = W25Q_flash.sector_num/BLOCK_HAS_SECTORS;
		return ERR_OK;
	}
	
	if( id[0] == 0xEF && id[1] == 0x16)		//w25Q64
	{
		W25Q_flash.page_num = 32768;
		W25Q_flash.sector_num = W25Q_flash.page_num/SECTOR_HAS_PAGES;
		W25Q_flash.block_num = W25Q_flash.sector_num/BLOCK_HAS_SECTORS;
		return ERR_OK;
	}
	

	return ERR_FAIL;
	
	
}






