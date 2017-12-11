
#include "adc.h"
#include "hardwareConfig.h"
#include "sdhError.h"
#include <string.h>
#define ADC_BUFLEN	200


unsigned char  ConfigRecFinish;

unsigned long   TimingDelay;
unsigned short  ConfigCheckCnt;
unsigned short  ADC_timcount;                     //ADC����ʱ��
unsigned short  comout_count; 
unsigned char   BusCheckEN;
unsigned char   HalfSecCnt500mS;
unsigned short  HalfSecCnt5000mS;
unsigned char   Falg_HalfSec;
unsigned char   RunStat=0;                        //LED������״̬����
unsigned char   sec_10;	

unsigned char  collectChannel;                    //��ǰ�Ĳɼ�ͨ��  
unsigned char  RUNorBD;                           //�궨�������б�־(RUNorBD =1��ʾΪ�궨״̬  RUNorBD ==0��ʾΪ����״̬)
unsigned char  sampledote0,sampleolddote0;        //ͨ��0������
unsigned char  sampledote1,sampleolddote1;        //ͨ��1������
unsigned char  sampledote2,sampleolddote2;        //ͨ��2������

unsigned char  BD_sampledote,BD_sampleolddote;    //�궨��ǰͨ���Ĳ����źŵ�
unsigned short BUSADdelta;                       
unsigned char  BUSADdelataCnt;  
unsigned char  CHannel0finish, CHannel1finish, CHannel2finish;
unsigned short ADCval0 =0, ADCval1 =0,ADCval2=0;
float   AV_BUF0,AV_BUF1,AV_BUF2;
float   rang_f;
float	  ma_tie=1;
unsigned short	sample_V11;
unsigned short	sample_V12;
unsigned short	sample_V13;
unsigned short	sample_HIGH1;
unsigned short	sample_LOW1;
struct  _RemoteRTU_Module_  RTU[3];  
struct  _SampleValue_     SAMPLE[3];
struct  _BDSampleVal_     BDSAMPLE[3];

get_realVAl I_get_val = NULL;
get_rsv I__get_rsv = NULL;
get_alarm I_get_alarm = NULL;
static uint16_t ADCConvertedValue[ADC_BUFLEN];	//�������ݲ���200��(�˳�50HZ����)
static void DMA1_Configuration(void);
static void DealwithCollect(unsigned char channel);
static void init_stm32adc(void *arg);
static void FinishCollect(void);
static void system_para_init(void);

void Regist_get_val( get_realVAl get_val)
{
	
	I_get_val = get_val;
}

void Regist_get_rsv( get_rsv get_rsv)
{
	
	I__get_rsv = get_rsv;
}

void Regist_get_alarm( get_alarm get_alarm)
{
	
	I_get_alarm = get_alarm;
}

void Collect_job(void)
{
	
	DealwithCollect(collectChannel);
	FinishCollect();  
}


void ADC_50ms()
{
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//����ADC
	ADC_Cmd(ADC1, ENABLE);                    //ADCת��ʹ��
}

void ADCValUpdate( char chn)
{
	if( I_get_val)
		I_get_val( chn, RTU[chn].value);
	if( I__get_rsv)
		I__get_rsv( chn, RTU[chn].RSV);
	if( I_get_alarm)
		I_get_alarm( chn, RTU[chn].Alarm);
	
}

void Set_chnType( char chn, uint8_t type)
{
	if( chn > 2) 
		return;
	
	RTU[chn].Type = type;
	
}
void Set_rangH( char chn, uint16_t rang)
{
	if( chn > 2) 
		return;
	
	RTU[chn].RangeH = rang;
}
void Set_rangL( char chn, uint16_t rang)
{
	if( chn > 2) 
		return;
	
	RTU[chn].RangeL = rang;
}
void Set_alarmH( char chn, uint16_t alarm)
{
	if( chn > 2) 
		return;
	
	RTU[chn].HiAlm = alarm;
	
}
void Set_alarmL( char chn, uint16_t alarm)
{
	if( chn > 2) 
		return;
	
	RTU[chn].LowAlm = alarm;
	
}

void create_adc(void)
{
	init_stm32adc(NULL);
	system_para_init();
}


static void init_stm32adc(void *arg)
{   
	ADC_InitTypeDef ADC_InitStructure;
//	GPIO_InitTypeDef GPIO_InitStructure;
//	DMA_InitTypeDef DMA_InitStructure;
	

  	// ADC1 configuration -------------------------------
	ADC_StructInit( &ADC_InitStructure);
	
  	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  	ADC_Init(ADC1, &ADC_InitStructure);	               //ʹ��ADC1

    
    //����ת������1��ͨ��10,MCP6S21�����ͨ������ 
    ADC_RegularChannelConfig(ADC1, ADC_chn, 1, ADC_SampleTime_71Cycles5); 
  
  	//Enable ADC1 DMA 
  	ADC_DMACmd(ADC1, ENABLE);
	// ����ADC��DMA֧�֣�Ҫʵ��DMA���ܣ������������DMAͨ���Ȳ�����
  
  	// Enable ADC1 
  	ADC_Cmd(ADC1, ENABLE);

	// ������ADC�Զ�У׼����������ִ��һ�Σ���֤���� 
  	// Enable ADC1 reset calibaration register    
  	ADC_ResetCalibration(ADC1);
  	// Check the end of ADC1 reset calibration register 
  	while(ADC_GetResetCalibrationStatus(ADC1));

  	// Start ADC1 calibaration 
  	ADC_StartCalibration(ADC1);
  	// Check the end of ADC1 calibration 
  	while(ADC_GetCalibrationStatus(ADC1));
	// ADC�Զ�У׼����---------------
	
	
	
	DMA1_Configuration();                        
	
}


static void DMA1_Configuration( )
{
	DMA_InitTypeDef DMA_InitStructure;
	//DMA1 channel1 configuration ---------------------------------------------	
	DMA_DeInit( DMA_adc.dma_rx_base);
  	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&ADC1->DR);;
  	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&ADCConvertedValue;
  	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  	DMA_InitStructure.DMA_BufferSize = ADC_BUFLEN;
  	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
  	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  	DMA_Init(DMA1_Channel1, &DMA_InitStructure);	 
  
  	DMA_ITConfig( DMA_adc.dma_rx_base,DMA_IT_TC,ENABLE);
	// Enable DMA1 channel1 
  	DMA_Cmd( DMA_adc.dma_rx_base, ENABLE);			 
}



/********************************************************************************************************
** Function name:        void series_se(unsigned char data)         
** Modified by:          ͨ���л�����(����ģ�⿪��ѡͨ��ͬ���ź�����)
** Modified date:        2016-11-28
*********************************************************************************************************/
static void series_se(unsigned char data)
{
   switch(data)
   {
     case 0:  
		SelectCLR4051_A;	    //ѡͨ�ź�VX32
		SelectCLR4051_B;
		SelectCLR4051_C;
		break;   	  
	case 1:
		SelectSET4051_A;      //ѡͨ�ź�VX22
		SelectCLR4051_B;
		SelectCLR4051_C;
		break;

	case 2: 
		SelectCLR4051_A;	     //ѡͨVX21�ź�
		SelectSET4051_B;
		SelectCLR4051_C; 
		break;

	case 3:
		SelectSET4051_A;  	   //ѡͨVX31�ź�
		SelectSET4051_B;
		SelectCLR4051_C;  
		break;

	case 4:                            //ѡͨVX12�ź�
		SelectCLR4051_A;
		SelectCLR4051_B;
		SelectSET4051_C;
		break;

	case 5:   
		SelectSET4051_A;       //ѡͨGND�ź�
		SelectCLR4051_B;
		SelectSET4051_C;
		break;

	case 6: 
		SelectCLR4051_A;       //ѡͨVX11�ź�
		SelectSET4051_B;
		SelectSET4051_C;
		break;

	case 7:                        //ѡͨVR0��׼�ź�
		SelectSET4051_A;
		SelectSET4051_B;
		SelectSET4051_C;
		break;

	default:                         //ѡͨGND�ź�
		SelectSET4051_A;       
		SelectCLR4051_B;
		SelectSET4051_C;
		break;
	}	

}

/*********************************************************************************************************
** Function name:        void sw_select(unsigned char channel)        
** Modified by:          ѡ��ͬ���ź�ͨ��channel0-channel2
** Modified date:        2016-11-28
** intput value:         channelΪRTU��ͨ��ȡֵ��ΧΪ 0-2     
*********************************************************************************************************/

static void sw_select(unsigned char channel)
{
	
	if(channel==0)
	{
		switch(RTU[channel].Type)       //ѡ��channel0
		{

		  case V_5:                     //��ѹ�ź�����
			case V_1_5:
			CLR_currentnum0;              //�ض�Q1 MOS��
			
		   if(sampledote0<=2)           
			{
				 if(sampledote0==0)	          //����VX11
				 {
					 series_se(VX11);
				 

				 }
				 if(sampledote0==1)            //����GND
				 {
					 series_se(GND);
				 }
				 if(sampledote0==2)            //����VR0_A
				 {
					 series_se(VR0_A);  
				 }
			}
			else 
			{
			     sampledote0 = 0;
			}
			
		    break;

			case mA0_10:	          //0-10mA �����ź�  
			case mA4_20:		        //4-20mA �����ź�
			SET_currentnum0;        //�� Q1 MOS
				if(sampledote0<=3)
				{	
				   
					if(sampledote0==0)	series_se(VX12);  //����VX12
					if(sampledote0==1)	series_se(VX11);  //����VX11	
					if(sampledote0==2)	series_se(GND);   //����GND	
					if(sampledote0==3)	series_se(VR0_A); //������׼VR0
				}
				else
					sampledote0=0;
			break;

			default :
				break;
		}	
	}	
	
	else if(channel==1)
	{	
		switch(RTU[channel].Type)       //ѡ��channel1
		{

		  case V_5:                     //��ѹ�ź�����
			case V_1_5:
			CLR_currentnum1;              //�ض�Q3 MOS��
			
		   if(sampledote1<=2)           
			{
				 if(sampledote1==0)	          //����VX21
				 {
					 series_se(VX21);
				 

				 }
				 if(sampledote1==1)           //����GND
				 {
					 series_se(GND);
				 }
				 if(sampledote1==2)           //����VR0_A
				 {
					 series_se(VR0_A);  
				 }
			}
			else 
			{
			     sampledote1 = 0;
			}
			
		    break;

			case mA0_10:	          //0-10mA �����ź�  
			case mA4_20:		        //4-20mA �����ź�
				
			SET_currentnum1;        //�� Q3 MOS��
				if(sampledote1<=3)
				{	
				   
					if(sampledote1==0)	series_se(VX22);  //����VX22
					if(sampledote1==1)	series_se(VX21);  //����VX21	
					if(sampledote1==2)	series_se(GND);   //����GND	
					if(sampledote1==3)	series_se(VR0_A); //������׼VR0_A
				}
				else
					sampledote1=0;
			break;

			default :
				break;
		}
		
	}
	
	else if(channel==2)
	{
		
		switch(RTU[channel].Type)       //ѡ��channel2
		{

		  case V_5:                     //��ѹ�ź�����
			case V_1_5:
			CLR_currentnum2;              //�ض�Q4 MOS��
			
		   if(sampledote2<=2)           
			{
				 if(sampledote2==0)	          //����VX31
				 {
					 series_se(VX31);
				 

				 }
				 if(sampledote2==1)           //����GND
				 {
					 series_se(GND);
				 }
				 if(sampledote2==2)           //����VR0_A
				 {
					 series_se(VR0_A);  
				 }
			}
			else 
			{
			     sampledote2 = 0;
			}
			
		    break;

			case mA0_10:	          //0-10mA �����ź�  
			case mA4_20:		        //4-20mA �����ź�
				
			SET_currentnum2;        //�� Q4 MOS��
				if(sampledote2<=3)
				{	
				   
					if(sampledote2==0)	series_se(VX32);  //����VX32
					if(sampledote2==1)	series_se(VX31);  //����VX31	
					if(sampledote2==2)	series_se(GND);   //����GND	
					if(sampledote2==3)	series_se(VR0_A); //������׼VR0_A
				}
				else
					sampledote2=0;
			break;

			default :
		  break;
		}
		
	}
	
}

/*********************************************************************************************************
** Function name:        void sw_select(unsigned char channel)        
** Modified by:          ѡ��ͬ���ź�ͨ��channel0-channel2
** Modified date:        2016-11-28
** intput value:         channelΪRTU��ͨ��ȡֵ��ΧΪ 0-2     
*********************************************************************************************************/

//static void BD_sw_select(unsigned char channel,unsigned signaltype)
//{
//	
//	if(channel==0)
//	{
//		switch(signaltype)              //ѡ��channel0�¶�Ӧ�������ź�����
//		{

//		  case V_5:                     //��ѹ�ź�����
//			case V_1_5:
//			CLR_currentnum0;              //�ض�Q1 MOS��
//			
//		   if(BD_sampledote<=2)           
//			{
//				 if(BD_sampledote==0)	       //����VX11
//				 {
//					 series_se(VX11);
//				 }
//				 if(BD_sampledote==1)        //����GND
//				 {
//					 series_se(GND);
//				 }
//				 if(BD_sampledote==2)        //����VR0_A
//				 {
//					 series_se(VR0_A);  
//				 }
//			}
//			else 
//			{
//			     BD_sampledote = 0;
//			}
//			
//		  break;

//			case mA0_10:	          //0-10mA �����ź�  
//			case mA4_20:		        //4-20mA �����ź�
//			SET_currentnum0;        //�� Q1 MOS
//				if(BD_sampledote<=3)
//				{	
//				   
//					if(BD_sampledote==0)	series_se(VX12);  //����VX12
//					if(BD_sampledote==1)	series_se(VX11);  //����VX11	
//					if(BD_sampledote==2)	series_se(GND);   //����GND	
//					if(BD_sampledote==3)	series_se(VR0_A); //������׼VR0
//				}
//				else
//					BD_sampledote=0;
//			break;

//			default :
//				break;
//		}	
//	}	
//	
//	else if(channel==1)                //ѡ��channel1��������ź�����
//	{	
//		switch(signaltype)            
//		{

//		  case V_5:                     //��ѹ�ź�����
//			case V_1_5:
//			CLR_currentnum1;              //�ض�Q3 MOS��
//			
//		   if(BD_sampledote<=2)           
//			{
//				 if(BD_sampledote==0)	          //����VX21
//				 {
//					 series_se(VX21);
//				 

//				 }
//				 if(BD_sampledote==1)           //����GND
//				 {
//					 series_se(GND);
//				 }
//				 if(BD_sampledote==2)           //����VR0_A
//				 {
//					 series_se(VR0_A);  
//				 }
//			}
//			else 
//			{
//			     BD_sampledote = 0;
//			}
//			
//		  break;

//			case mA0_10:	           //0-10mA �����ź�  
//			case mA4_20:		         //4-20mA �����ź�
//				
//			SET_currentnum1;         //�� Q3 MOS��
//			if(BD_sampledote<=3)
//			{	
//				 
//				if(BD_sampledote==0)	series_se(VX22);  //����VX22
//				if(BD_sampledote==1)	series_se(VX21);  //����VX21	
//				if(BD_sampledote==2)	series_se(GND);   //����GND	
//				if(BD_sampledote==3)	series_se(VR0_A); //������׼VR0_A
//			}
//			else
//				BD_sampledote=0;
//			break;

//			default :
//				break;
//		}
//		
//	}
//	
//	else if(channel==2)
//	{
//		
//		switch(signaltype)              //ѡ��channel2
//		{

//		  case V_5:                     //��ѹ�ź�����
//			case V_1_5:
//			CLR_currentnum2;              //�ض�Q4 MOS��
//			
//		   if(BD_sampledote<=2)           
//			{
//				 if(BD_sampledote==0)	          //����VX31
//				 {
//					 series_se(VX31);
//				 

//				 }
//				 if(BD_sampledote==1)           //����GND
//				 {
//					 series_se(GND);
//				 }
//				 if(BD_sampledote==2)           //����VR0_A
//				 {
//					 series_se(VR0_A);  
//				 }
//			}
//			else 
//			{
//			     BD_sampledote = 0;
//			}
//			
//		    break;

//			case mA0_10:	          //0-10mA �����ź�  
//			case mA4_20:		        //4-20mA �����ź�
//				
//			SET_currentnum2;        //�� Q4 MOS��
//				if(BD_sampledote<=3)
//				{	
//				   
//					if(BD_sampledote==0)	series_se(VX32);  //����VX32
//					if(BD_sampledote==1)	series_se(VX31);  //����VX31	
//					if(BD_sampledote==2)	series_se(GND);   //����GND	
//					if(BD_sampledote==3)	series_se(VR0_A); //������׼VR0_A
//				}
//				else
//					BD_sampledote=0;
//			break;

//			default :
//		  break;
//		}
//		
//	}
//	
//}

/*********************************************************************************************************
** Function name:        void Data_Deal(unsigned char ch_temp,unsigned char pred)        
** Modified by:          �Բ������������
** Modified date:        2016-12-01
** intput  value��       ch_temp ΪRTUͨ��(ȡֵ��ΧΪ0-2)  pred Ϊ��Ե�ǰRTUͨ��,�����ĵ��λ��(ȡֵ��Χ 0-3)
*********************************************************************************************************/

void Data_Deal(unsigned char ch_temp,unsigned char pred)
{
	unsigned char  i_temp,j_temp,m_temp;
	unsigned long	data_temp;
	data_temp=0;
	
	
	switch(ch_temp)
	{

        //ͨ��0

		case CHANNEL0:
			for(j_temp=0;j_temp<199;j_temp++) 	  //�����㷨
			{
				for(m_temp=0;m_temp<199-j_temp;m_temp++)
				{
					if(ADCConvertedValue[m_temp]>ADCConvertedValue[m_temp+1])
					{
						data_temp=ADCConvertedValue[m_temp];
						ADCConvertedValue[m_temp]=ADCConvertedValue[m_temp+1];
						ADCConvertedValue[m_temp+1]=data_temp;
					}
				}
			}
			data_temp=0;
			for(i_temp=88;i_temp<113;i_temp++)
			{
				data_temp=data_temp+ADCConvertedValue[i_temp];
			}
			data_temp=data_temp/25;
			ADCval0=(unsigned short) data_temp;
			
			switch(pred)             //�������λ��
			{
				case Samplepoint0:		                                                    //��Ϊ�����ź������VX12
					
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)  
					{
										
						 SAMPLE[ch_temp].sample_Vpoint_2=(unsigned short) data_temp;
								
				  }

				
					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)              //���ǵ�ѹ�ź�,�����VX11
					{
					  				  
					  SAMPLE[ch_temp].sample_Vpoint_1 = (unsigned short) data_temp; 
				
					}

				break;	 
					 
				case Samplepoint1:                                                          //��Ϊ�����źţ������VX11 
					
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
						
					{
						 SAMPLE[ch_temp].sample_Vpoint_1=(unsigned short) data_temp;
					}
				 
					
			    	else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)	            //����ǵ�ѹ�ź�(������׼GND)
					{
					   SAMPLE[ch_temp].sample_LOW=(unsigned short) data_temp;
					}
				break;

				case Samplepoint2:		                                                     //��Ϊ�����źŲ���GND
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
					{
						SAMPLE[ch_temp].sample_LOW=(unsigned short) data_temp;
					}
					
			    	else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)	             //��Ϊ��ѹ�ź�(������׼VR0)
					{
					    SAMPLE[ch_temp].sample_HIGH=(unsigned short) data_temp;

					    CHannel0finish = 1;
					}
				   
				break;

				case Samplepoint3:	                                                        //��Ϊ�����źŲ���VR0
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
					{
						SAMPLE[ch_temp].sample_HIGH=(unsigned short) data_temp;
						CHannel0finish = 1;
					}
					else
					{
             ;
					}
				    
				break;

				default :
					break;
			}
			
			break;

      //ͨ��1

			case CHANNEL1:

				for(j_temp=0;j_temp<199;j_temp++) 	  //�����㷨
				{
					for(m_temp=0;m_temp<199-j_temp;m_temp++)
					{
						if(ADCConvertedValue[m_temp]>ADCConvertedValue[m_temp+1])
						{
							data_temp=ADCConvertedValue[m_temp];
							ADCConvertedValue[m_temp]=ADCConvertedValue[m_temp+1];
							ADCConvertedValue[m_temp+1]=data_temp;
						}
					}
				}
				data_temp=0;
				for(i_temp=88;i_temp<113;i_temp++)
				{
					data_temp=data_temp+ADCConvertedValue[i_temp];
				}
				data_temp=data_temp/25;
				ADCval1=(unsigned short) data_temp;

				switch(pred)                                                              //�������λ��
				{
				case Samplepoint0:		                                                    //��Ϊ�����ź������VX22

					if(RTU[ch_temp].Type== mA0_10||RTU[ch_temp].Type== mA4_20)  
					{

						SAMPLE[ch_temp].sample_Vpoint_2=(unsigned short) data_temp;

					}


					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)              //���ǵ�ѹ�ź�,�����VX21
					{

						SAMPLE[ch_temp].sample_Vpoint_1 = (unsigned short) data_temp; 

					}

					break;	 

				case Samplepoint1:                                                       //��Ϊ�����źţ������VX21 

					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)

					{
						SAMPLE[ch_temp].sample_Vpoint_1=(unsigned short) data_temp;
					}


					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)	            //����ǵ�ѹ�ź�(������׼GND)
					{
						SAMPLE[ch_temp].sample_LOW=(unsigned short) data_temp;
					}
					break;

				case Samplepoint2:		                                                     //��Ϊ�����źŲ���GND
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
					{
						SAMPLE[ch_temp].sample_LOW=(unsigned short) data_temp;
					}

					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)	             //��Ϊ��ѹ�ź�(������׼VR0)
					{
						SAMPLE[ch_temp].sample_HIGH=(unsigned short) data_temp;

						CHannel1finish = 1;
					}

					break;

				case Samplepoint3:	                                                      //��Ϊ�����źŲ���VR0
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
					{
						SAMPLE[ch_temp].sample_HIGH=(unsigned short) data_temp;
						CHannel1finish = 1;
					}
					else
					{
						;
					}

					break;



				default :
					break;
				}
				break;




				//ͨ��2

			case CHANNEL2:

				for(j_temp=0;j_temp<199;j_temp++) 	  //�����㷨
				{
					for(m_temp=0;m_temp<199-j_temp;m_temp++)
					{
						if(ADCConvertedValue[m_temp]>ADCConvertedValue[m_temp+1])
						{
							data_temp=ADCConvertedValue[m_temp];
							ADCConvertedValue[m_temp]=ADCConvertedValue[m_temp+1];
							ADCConvertedValue[m_temp+1]=data_temp;
						}
					}
				}
				data_temp=0;
				for(i_temp=88;i_temp<113;i_temp++)
				{
					data_temp=data_temp+ADCConvertedValue[i_temp];
				}
				data_temp=data_temp/25;
				ADCval2=(unsigned short) data_temp;

				switch(pred)             //�������λ��
				{
				case Samplepoint0:		                                                    //��Ϊ�����ź������VX32

					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)  
					{

						SAMPLE[ch_temp].sample_Vpoint_2=(unsigned short) data_temp;

					}


					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)              //���ǵ�ѹ�ź�,�����VX31
					{

						SAMPLE[ch_temp].sample_Vpoint_1 = (unsigned short) data_temp; 

					}

					break;	 

				case Samplepoint1:                                                       //��Ϊ�����źţ������VX31 

					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)

					{
						SAMPLE[ch_temp].sample_Vpoint_1=(unsigned short) data_temp;
					}


					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)	            //����ǵ�ѹ�ź�(������׼GND)
					{
						SAMPLE[ch_temp].sample_LOW=(unsigned short) data_temp;
					}
					break;

				case Samplepoint2:		                                                     //��Ϊ�����źŲ���GND
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
					{
						SAMPLE[ch_temp].sample_LOW=(unsigned short) data_temp;
					}

					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)	             //��Ϊ��ѹ�ź�(������׼VR0)
					{
						SAMPLE[ch_temp].sample_HIGH=(unsigned short) data_temp;

						CHannel2finish = 1;
					}

					break;

				case Samplepoint3:	                                                      //��Ϊ�����źŲ���VR0
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
					{
						SAMPLE[ch_temp].sample_HIGH=(unsigned short) data_temp;
						CHannel2finish = 1;
					}
					else
					{
						;
					}

					break;



				default :
					break;
				}
				break;
	
		default :
			break;
		
	}
	
		
}
//��ʵ�ʲ���������ֵת����ʵ�ʵ�ѹֵ���ߵ���ֵ

void Calculate(unsigned char channel)
{
	
	
	float	projectval,bdhigh,bdlow,atemp1,atemp2,tdata1,tdata2,ax1,ax2;
//	float	temp_data1,temp_data2,temp_data3,temp_data4,temp_data5,temp_data6;

	switch(channel)
	{
		case CHANNEL0:

			switch(RTU[channel].Type)
			{

				case V_5:		                                    //��ѹ�ź�0-5V ����1-5V
				case V_1_5:
		       bdhigh=(float)RTU[channel].BD.BD_5_HIGH/1000.0;
				   bdlow=(float)RTU[channel].BD.BD_5_LOW/100000.0;
				   atemp1=(((float)SAMPLE[channel].sample_Vpoint_1)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   tdata2=atemp1*bdhigh+bdlow;                  //��ȡʵ�ʵĵ�ѹֵ
				   if(tdata2<0.0005)	tdata2=0;
				   if(AV_BUF0>10.0)     
				   {
					   AV_BUF0=0;	
				   }		
				   if((RTU[channel].Type==V_5)&&(AV_BUF0<0.0005))    //0-5V�ź�
				   {
						   AV_BUF0=0;	

				   }
				   if((RTU[channel].Type==V_1_5)&&(AV_BUF0<0.0005))  // 1-5V �ź�
				   {
						   AV_BUF0=1.0;
				   }				   
				   ax1=tdata2*1000;
				   ax2=AV_BUF0*1000;
				   if(ax1-ax2>=10)
				     AV_BUF0=tdata2;
				   if(ax2-ax1>=10)
				     AV_BUF0=tdata2;	
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;				   
				     tdata2=tdata2*0.1+AV_BUF0*0.9;    //�����˲� 0.9*��ֵ+0.1��ֵ  
				   if((tdata2<10)&&(tdata2>0.0005))
						 AV_BUF0=tdata2;

				   if(SAMPLE[channel].sample_Vpoint_1<(SAMPLE[channel].sample_LOW-0x200)) //�������ﲻ����
				   {
				     tdata2=0;
					   AV_BUF0=0;
					   RTU[channel].Alarm=0x01;         //���߱�־
				   }
				   //else
				   //{
				   //  RTU[channel].Alarm=0x00;
				   //}
				   if(RTU[channel].Type==V_1_5)       //�ź�Ϊ1-5V�ź�  
				   {
				     if(tdata2<1.0)
					 {
					    tdata2=1.0;
						  RTU[channel].Alarm=0x01;         //���߱�־
					 }
				   }
					 
					 #if 0 
				   tdata2=tdata2/5*30000;
				   if(tdata2>30000) tdata2=30000;
					 #endif
					 
					  projectval = tdata2/5*30000;
					  if(projectval>30000) projectval =30000;
					
					   if(tdata2>5.0)
							 tdata2 =5.0;
					 
					 if(RTU[channel].Type==V_5)                   //0-5V �ź�����
					 {
						 
						    RTU[channel].RSV = RTU[channel].RangeL+(projectval-0)*(RTU[channel].RangeH-RTU[channel].RangeL)/(30000-0); //ת����0-65535�Ĺ���ֵ
						   //RTU[channel].RSV = 0+(projectval-0)*(65535-0)/(30000-0); //ת����0-65535�Ĺ���ֵ
						   RTU[channel].value = tdata2;          //���ʵ��AVֵ

						   if(RTU[channel].RSV>=RTU[channel].HiAlm)
						   {
							   RTU[channel].Alarm = 0x02;	 
						   }
						   if(RTU[channel].RSV>=RTU[channel].RangeH)
						   {
							   RTU[channel].Alarm = 0x04;
						   }
						   if(RTU[channel].RSV<=RTU[channel].LowAlm)
						   {
                 RTU[channel].Alarm = 0x03;
						   }
						   if(RTU[channel].RSV<=RTU[channel].RangeL)
						   {
							   RTU[channel].Alarm = 0x05;
						   }

						   if(RTU[channel].RSV<(RTU[channel].HiAlm-Deadline))   //���������ޱ������
						   {
								 RTU[channel].Alarm = 0x00; 

						   }

//						 if(RTU[channel].RSV<(RTU[channel].RangeH-Deadline))  //���������ޱ������
//					   {
//						   RTU[channel].Alarm = 0x00;
//					   }

							 if(RTU[channel].RSV>(RTU[channel].LowAlm+Deadline))  //���ڱ������ޱ������
						   {
								 RTU[channel].Alarm = 0x00;
						   }
//						   if(RTU[channel].RSV>(RTU[channel].RangeL+Deadline))  //�����������ޱ������
//						   {
//								 RTU[channel].Alarm = 0x00;
//						   }

						  
						 
            }
					 else if (RTU[channel].Type==V_1_5)           // 1--5V �ź�����
					 {
						 if(tdata2<1.0)
						 {
							  tdata2 = 1.0; 
							  projectval = 6000;
							  
             }
						 else if(projectval>30000)
						 {
							 projectval = 30000; 
							 tdata2 = 5.0;
             }

						 RTU[channel].RSV = RTU[channel].RangeL+(projectval-6000)*(RTU[channel].RangeH-RTU[channel].RangeL)/(30000-6000);   //ת����0-65535�Ĺ���ֵ
						 //RTU[channel].RSV = 0+(projectval-6000)*(65535-0)/(30000-6000);   //ת����0-65535�Ĺ���ֵ
						 RTU[channel].value = tdata2;

						 if(RTU[channel].RSV>=RTU[channel].HiAlm)
						 {
							 RTU[channel].Alarm = 0x02;	 
						 }
						 if(RTU[channel].RSV>=RTU[channel].RangeH)
						 {
							 RTU[channel].Alarm = 0x04;
						 }
						 if(RTU[channel].RSV<=RTU[channel].LowAlm)
						 {
							 RTU[channel].Alarm = 0x03;
						 }
						 if(RTU[channel].RSV<=RTU[channel].RangeL)
						 {
							 RTU[channel].Alarm = 0x05;
						 }

						 if(RTU[channel].RSV<(RTU[channel].HiAlm-Deadline))   //���������ޱ������
						 {
							 RTU[channel].Alarm = 0x00; 

						 }

//						 if(RTU[channel].RSV<(RTU[channel].RangeH-Deadline))  //���������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }

						 if(RTU[channel].RSV>(RTU[channel].LowAlm+Deadline))  //���ڱ������ޱ������
						 {
							 RTU[channel].Alarm = 0x00;
						 }
//						 if(RTU[channel].RSV>(RTU[channel].RangeL+Deadline))  //�����������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }
 

						 
             }
					 
					break;

				case mA0_10:                                    //0-10mA
				case mA4_20:                                    //4-20mA
					
		       bdhigh=(float)RTU[channel].BD.BD_20mA_HIGH/100;
				   bdlow=(float)RTU[channel].BD.BD_20mA_LOW;
				   atemp1=(((float)SAMPLE[channel].sample_Vpoint_1)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   atemp2=(((float)SAMPLE[channel].sample_Vpoint_2)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   tdata1=atemp1*((float)RTU[channel].BD.BD_5_HIGH/1000)+((float)RTU[channel].BD.BD_5_LOW/100000);
				   tdata2=atemp2*((float)RTU[channel].BD.BD_5_HIGH/1000)+((float)RTU[channel].BD.BD_5_LOW/100000);
				   rang_f=(tdata1-tdata2)/ma_tie*1000;
				   tdata2=(tdata1-tdata2)/bdhigh*1000;
				   if(tdata2<0.0005)	tdata2=0;
				
				   if((RTU[channel].Type==mA0_10)&&(AV_BUF0<0.0005))
				   {
						   AV_BUF0=0;	
				   }
				   ax1=tdata2*1000;
				   ax2=AV_BUF0*1000;
				   if(ax1-ax2>=50)
				     AV_BUF0=tdata2;
				   if(ax2-ax1>=50)
				     AV_BUF0=tdata2;			
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;				   
				    tdata2=tdata2*0.1+AV_BUF0*0.9;
				   if((tdata2<30)&&(tdata2>0.0005))
						AV_BUF0=tdata2;
				   if((RTU[channel].Type==mA4_20)&&(AV_BUF0<3.960))
				   {
				     tdata2=4.0;
					   AV_BUF0=0;
					   RTU[channel].Alarm=0x01; //����
				   }

				   //else
				   //{
				   //  RTU[channel].Alarm=0x00;
				   //}

					 #if 0 
				   tdata2=tdata2/20*30000;
				   if(tdata2>30000) tdata2=30000;
					 # endif
					 
					  projectval = tdata2/20*30000;
					  if(projectval>30000) projectval =30000;
					  if(tdata2>20.0)
					    tdata2 =20.0;
					 
					 if(RTU[channel].Type==mA0_10)    //0-10mA �ź�����
					 {
						  if(projectval>15000) 
						 {
							  projectval = 15000; 
							  tdata2 =10.0;
                         }
						 if(tdata2>10.0)
						 {
							 tdata2 =10.0;
							 projectval =15000;
						 }
						 
						 RTU[channel].RSV =  RTU[channel].RangeL+(projectval-0)*(RTU[channel].RangeH-RTU[channel].RangeL)/(15000-0); //ת����0-65535�Ĺ���?
						 //RTU[channel].RSV =  0+(projectval-0)*(65535-0)/(15000-0); //ת����0-65535�Ĺ���?
						 RTU[channel].value = tdata2;

						 if(RTU[channel].RSV>=RTU[channel].HiAlm)
						 {
							 RTU[channel].Alarm = 0x02;	 
						 }
						 if(RTU[channel].RSV>=RTU[channel].RangeH)
						 {
							 RTU[channel].Alarm = 0x04;
						 }
						 if(RTU[channel].RSV<=RTU[channel].LowAlm)
						 {
							 RTU[channel].Alarm = 0x03;
						 }
						 if(RTU[channel].RSV<=RTU[channel].RangeL)
						 {
							 RTU[channel].Alarm = 0x05;
						 }

						 if(RTU[channel].RSV<(RTU[channel].HiAlm-Deadline))   //���������ޱ������
						 {
							 RTU[channel].Alarm = 0x00; 

						 }

//						 if(RTU[channel].RSV<(RTU[channel].RangeH-Deadline))  //���������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }

						 if(RTU[channel].RSV>(RTU[channel].LowAlm+Deadline))  //���ڱ������ޱ������
						 {
							 RTU[channel].Alarm = 0x00;
						 }
//						 if(RTU[channel].RSV>(RTU[channel].RangeL+Deadline))  //�����������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }


            }
					 else if (RTU[channel].Type==mA4_20) // 4--20mA �ź�����
					 {
						 if(projectval<6000) 
						 {
							  projectval = 6000; 
							  tdata2 = 4.0;
							 
             }
						 else if(projectval>30000)
						 {
							 projectval = 30000;
               tdata2 =20.0;							 
             }
						 if(tdata2<4.0)
						 {
							 tdata2 =4.0;
							 projectval = 6000;
						 }
						 else if(tdata2>20.0)
						 {
							 projectval = 30000;
						 }
						 RTU[channel].RSV = RTU[channel].RangeL+(projectval-6000)*(RTU[channel].RangeH-RTU[channel].RangeL)/(30000-6000);  //ת����0-65535֮���ֵ
						 //RTU[channel].RSV = 0+(projectval-6000)*(65535-0)/(30000-6000);  //ת����0-65535֮���ֵ
						 RTU[channel].value = tdata2;

						 if(RTU[channel].RSV>=RTU[channel].HiAlm)
						 {
							 RTU[channel].Alarm = 0x02;	 
						 }
						 if(RTU[channel].RSV>=RTU[channel].RangeH)
						 {
							 RTU[channel].Alarm = 0x04;
						 }
						 if(RTU[channel].RSV<=RTU[channel].LowAlm)
						 {
							 RTU[channel].Alarm = 0x03;
						 }
						 if(RTU[channel].RSV<=RTU[channel].RangeL)
						 {
							 RTU[channel].Alarm = 0x05;
						 }

						 if(RTU[channel].RSV<(RTU[channel].HiAlm-Deadline))   //���������ޱ������
						 {
							 RTU[channel].Alarm = 0x00; 

						 }

//						 if(RTU[channel].RSV<(RTU[channel].RangeH-Deadline))  //���������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }

						 if(RTU[channel].RSV>(RTU[channel].LowAlm+Deadline))  //���ڱ������ޱ������
						 {
							 RTU[channel].Alarm = 0x00;
						 }
//						 if(RTU[channel].RSV>(RTU[channel].RangeL+Deadline))  //�����������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }


            }
					 
					break;
				default:
					break;
				   
			}
			break;
			
			case CHANNEL1:
			switch(RTU[channel].Type)
			{
				
				  case V_5:		                                    //��ѹ�ź�0-5V ����1-5V
				  case V_1_5:
		       bdhigh=(float)RTU[channel].BD.BD_5_HIGH/1000;
				   bdlow=(float)RTU[channel].BD.BD_5_LOW/100000;
				   atemp1=(((float)SAMPLE[channel].sample_Vpoint_1)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   tdata2=atemp1*bdhigh+bdlow;                    //��ȡʵ�ʵĵ�ѹֵ
				   if(tdata2<0.0005)	tdata2=0;
				   if(AV_BUF1>10.0)     
				   {
					   AV_BUF1=0;	
				   }		
				   if((RTU[channel].Type==V_5)&&(AV_BUF1<0.0005))    //0-5V�ź�
				   {
						   AV_BUF1=0;	
				   }
				   if((RTU[channel].Type==V_1_5)&&(AV_BUF1<0.0005))  // 1-5V �ź�
				   {
						   AV_BUF1=1.0;
				   }				   
				   ax1=tdata2*1000;
				   ax2=AV_BUF1*1000;
				   if(ax1-ax2>=10)
				     AV_BUF1=tdata2;
				   if(ax2-ax1>=10)
				     AV_BUF1=tdata2;	
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF1=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF1=tdata2;				   
				     tdata2=tdata2*0.1+AV_BUF1*0.9;    //�����˲� 0.9*��ֵ+0.1��ֵ  
				   if((tdata2<10)&&(tdata2>0.0005))
						 AV_BUF1=tdata2;

				   if(SAMPLE[channel].sample_Vpoint_1<(SAMPLE[channel].sample_LOW-0x200)) //�������ﲻ����
				   {
				     tdata2=0;
					   AV_BUF1=0;
					   RTU[channel].Alarm=0x01;         //���߱�־
				   }
				   //else
				   //{
				   //  RTU[channel].Alarm=0x00;
				   //}
				   if(RTU[channel].Type==V_1_5)       //�ź�Ϊ1-5V�ź�  
				   {
				     if(tdata2<1.0)
					 {
					    tdata2=1.0;
						  RTU[channel].Alarm=0x01;         //���߱�־
					 }
				   }
					 
					 #if 0 
				   tdata2=tdata2/5*30000;
				   if(tdata2>30000) tdata2=30000;
					 #endif
					 
					  projectval = tdata2/5*30000;
					  if(projectval>30000) projectval =30000;
					
					   if(tdata2>5.0)
							 tdata2 =5.0;
					 
					 if(RTU[channel].Type==V_5)                   //0-5V �ź�����
					 {
						 
						   RTU[channel].RSV = RTU[channel].RangeL+(projectval-0)*(RTU[channel].RangeH-RTU[channel].RangeL)/(30000-0); //ת����0-65535�Ĺ���ֵ
						  // RTU[channel].RSV = 0+(projectval-0)*(65535-0)/(30000-0); //ת����0-65535�Ĺ���ֵ
						   RTU[channel].value = tdata2;             //���ʵ��AVֵ

						   if(RTU[channel].RSV>=RTU[channel].HiAlm)
						   {
							   RTU[channel].Alarm = 0x02;	 
						   }
						   if(RTU[channel].RSV>=RTU[channel].RangeH)
						   {
							   RTU[channel].Alarm = 0x04;
						   }
						   if(RTU[channel].RSV<=RTU[channel].LowAlm)
						   {
							   RTU[channel].Alarm = 0x03;
						   }
						   if(RTU[channel].RSV<=RTU[channel].RangeL)
						   {
							   RTU[channel].Alarm = 0x05;
						   }

						   if(RTU[channel].RSV<(RTU[channel].HiAlm-Deadline))   //���������ޱ������
						   {
							   RTU[channel].Alarm = 0x00; 

						   }

//						   if(RTU[channel].RSV<(RTU[channel].RangeH-Deadline))  //���������ޱ������
//						   {
//							   RTU[channel].Alarm = 0x00;
//						   }

						   if(RTU[channel].RSV>(RTU[channel].LowAlm+Deadline))  //���ڱ������ޱ������
						   {
							   RTU[channel].Alarm = 0x00;
						   }
//						   if(RTU[channel].RSV>(RTU[channel].RangeL+Deadline))  //�����������ޱ������
//						   {
//							   RTU[channel].Alarm = 0x00;
//						   }

						  
						 
           }
					 else if (RTU[channel].Type==V_1_5)           // 1--5V �ź�����
					 {
						 if(tdata2<1.0)
						 {
							  tdata2 = 1.0; 
							  projectval = 6000;
							  
             }
						 else if(projectval>30000)
						 {
							 projectval = 30000; 
							 tdata2 = 5.0;
             }
						 RTU[channel].RSV = RTU[channel].RangeL+(projectval-6000)*(RTU[channel].RangeH-RTU[channel].RangeL)/(30000-6000);   //ת����0-65535�Ĺ���ֵ
						 //RTU[channel].RSV = 0+(projectval-6000)*(65535-0)/(30000-6000);   //ת����0-65535�Ĺ���ֵ
						 RTU[channel].value = tdata2;
						 if(RTU[channel].RSV>=RTU[channel].HiAlm)
						 {
							 RTU[channel].Alarm = 0x02;	 
						 }
						 if(RTU[channel].RSV>=RTU[channel].RangeH)
						 {
							 RTU[channel].Alarm = 0x04;
						 }
						 if(RTU[channel].RSV<=RTU[channel].LowAlm)
						 {
							 RTU[channel].Alarm = 0x03;
						 }
						 if(RTU[channel].RSV<=RTU[channel].RangeL)
						 {
							 RTU[channel].Alarm = 0x05;
						 }

						 if(RTU[channel].RSV<(RTU[channel].HiAlm-Deadline))   //���������ޱ������
						 {
							 RTU[channel].Alarm = 0x00; 

						 }

//						 if(RTU[channel].RSV<(RTU[channel].RangeH-Deadline))  //���������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }

						 if(RTU[channel].RSV>(RTU[channel].LowAlm+Deadline))  //���ڱ������ޱ������
						 {
							 RTU[channel].Alarm = 0x00;
						 }
//						 if(RTU[channel].RSV>(RTU[channel].RangeL+Deadline))  //�����������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }
						 
           }
					 
					break;

				case mA0_10:                                    //0-10mA
				case mA4_20:                                    //4-20mA
					
		       bdhigh=(float)RTU[channel].BD.BD_20mA_HIGH/100;
				   bdlow=(float)RTU[channel].BD.BD_20mA_LOW;
				   atemp1=(((float)SAMPLE[channel].sample_Vpoint_1)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   atemp2=(((float)SAMPLE[channel].sample_Vpoint_2)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   tdata1=atemp1*((float)RTU[channel].BD.BD_5_HIGH/1000)+((float)RTU[channel].BD.BD_5_LOW/100000);
				   tdata2=atemp2*((float)RTU[channel].BD.BD_5_HIGH/1000)+((float)RTU[channel].BD.BD_5_LOW/100000);
				   rang_f=(tdata1-tdata2)/ma_tie*1000;
				   tdata2=(tdata1-tdata2)/bdhigh*1000;
				   if(tdata2<0.0005)	tdata2=0;
				   if((RTU[channel].Type==mA0_10)&&(AV_BUF1<0.0005))
				   {
						   AV_BUF1=0;	

				   }
				   ax1=tdata2*1000;
				   ax2=AV_BUF1*1000;
				   if(ax1-ax2>=50)
				     AV_BUF1=tdata2;
				   if(ax2-ax1>=50)
				     AV_BUF1=tdata2;			
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF1=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF1=tdata2;				   
				    tdata2=tdata2*0.1+AV_BUF1*0.9;
				   if((tdata2<30)&&(tdata2>0.0005))
						AV_BUF1=tdata2;
				   if((RTU[channel].Type==mA4_20)&&(AV_BUF1<3.960))
				   {
				     tdata2=4.0;
					   AV_BUF1=1;
					   RTU[channel].Alarm=0x01; //����
				   }
				   //else
				   //{
				   //  RTU[channel].Alarm=0x00;
				   //}

					 #if 0 
				   tdata2=tdata2/20*30000;
				   if(tdata2>30000) tdata2=30000;
					 # endif
					 
					  projectval = tdata2/20*30000;
					  if(projectval>30000) projectval =30000;
					  if(tdata2>20.0)
							 tdata2 =20.0;
					 
					 if(RTU[channel].Type==mA0_10)    //0-10mA �ź�����
					 {
						  if(projectval>15000) 
						 {
							  projectval = 15000; 
							  tdata2 =10.0;
             }
						 if(tdata2>10.0)
						 {
							 tdata2 =10.0;
							 projectval =15000;
						 }
						 
						 RTU[channel].RSV =  RTU[channel].RangeL+(projectval-0)*(RTU[channel].RangeH-RTU[channel].RangeL)/(15000-0); //ת����0-65535�Ĺ���?
						 //RTU[channel].RSV =  0+(projectval-0)*(65535-0)/(15000-0); //ת����0-65535�Ĺ���?
						 RTU[channel].value = tdata2;

						 if(RTU[channel].RSV>=RTU[channel].HiAlm)
						 {
							 RTU[channel].Alarm = 0x02;	 
						 }
						 if(RTU[channel].RSV>=RTU[channel].RangeH)
						 {
							 RTU[channel].Alarm = 0x04;
						 }
						 if(RTU[channel].RSV<=RTU[channel].LowAlm)
						 {
							 RTU[channel].Alarm = 0x03;
						 }
						 if(RTU[channel].RSV<=RTU[channel].RangeL)
						 {
							 RTU[channel].Alarm = 0x05;
						 }

						 if(RTU[channel].RSV<(RTU[channel].HiAlm-Deadline))   //���������ޱ������
						 {
							 RTU[channel].Alarm = 0x00; 

						 }

//						 if(RTU[channel].RSV<(RTU[channel].RangeH-Deadline))  //���������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }

						 if(RTU[channel].RSV>(RTU[channel].LowAlm+Deadline))  //���ڱ������ޱ������
						 {
							 RTU[channel].Alarm = 0x00;
						 }
//						 if(RTU[channel].RSV>(RTU[channel].RangeL+Deadline))  //�����������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }
           }
					 else if (RTU[channel].Type==mA4_20) // 4--20mA �ź�����
					 {
						 if(projectval<6000) 
						 {
							  projectval = 6000; 
							  tdata2 = 4.0;
							 
             }
						 else if(projectval>30000)
						 {
							 projectval = 30000;
               tdata2 =20.0;							 
             }
						 if(tdata2<4.0)
						 {
							 tdata2 =4.0;
							 projectval = 6000;
						 }
						 else if(tdata2>20.0)
						 {
							 projectval = 30000;
						 }
						 RTU[channel].RSV = RTU[channel].RangeL+(projectval-6000)*(RTU[channel].RangeH-RTU[channel].RangeL)/(30000-6000);  //ת����0-65535֮���ֵ
						 //RTU[channel].RSV = 0+(projectval-6000)*(65535-0)/(30000-6000);  //ת����0-65535֮���ֵ
						 RTU[channel].value = tdata2;

						 if(RTU[channel].RSV>=RTU[channel].HiAlm)
						 {
							 RTU[channel].Alarm = 0x02;	 
						 }
						 if(RTU[channel].RSV>=RTU[channel].RangeH)
						 {
							 RTU[channel].Alarm = 0x04;
						 }
						 if(RTU[channel].RSV<=RTU[channel].LowAlm)
						 {
							 RTU[channel].Alarm = 0x03;
						 }
						 if(RTU[channel].RSV<=RTU[channel].RangeL)
						 {
							 RTU[channel].Alarm = 0x05;
						 }

						 if(RTU[channel].RSV<(RTU[channel].HiAlm-Deadline))   //���������ޱ������
						 {
							 RTU[channel].Alarm = 0x00; 

						 }

//						 if(RTU[channel].RSV<(RTU[channel].RangeH-Deadline))  //���������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }

						 if(RTU[channel].RSV>(RTU[channel].LowAlm+Deadline))  //���ڱ������ޱ������
						 {
							 RTU[channel].Alarm = 0x00;
						 }
//						 if(RTU[channel].RSV>(RTU[channel].RangeL+Deadline))  //�����������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }

           }
					 
					break;
				default:
					break;
				
			}	
			break;
			
			case CHANNEL2:
		  switch(RTU[channel].Type)
			{
			 	case V_5:		                                    //��ѹ�ź�0-5V ����1-5V
				case V_1_5:
		       bdhigh=(float)RTU[channel].BD.BD_5_HIGH/1000;
				   bdlow=(float)RTU[channel].BD.BD_5_LOW/100000;
				   atemp1=(((float)SAMPLE[channel].sample_Vpoint_1)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   tdata2=atemp1*bdhigh+bdlow;                  //��ȡʵ�ʵĵ�ѹֵ
				   if(tdata2<0.0005)	tdata2=0;
				   if(AV_BUF2>10.0)     
				   {
					   AV_BUF2=0;	
				   }		
				   if((RTU[channel].Type==V_5)&&(AV_BUF2<0.0005))    //0-5V�ź�
				   {
						   AV_BUF2=0;	

				   }
				   if((RTU[channel].Type==V_1_5)&&(AV_BUF2<0.0005))  // 1-5V �ź�
				   {
						   AV_BUF2=1.0;
				   }				   
				   ax1=tdata2*1000;
				   ax2=AV_BUF2*1000;
				   if(ax1-ax2>=10)
				     AV_BUF2=tdata2;
				   if(ax2-ax1>=10)
				     AV_BUF2=tdata2;	
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF2=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF2=tdata2;				   
				     tdata2=tdata2*0.1+AV_BUF2*0.9;    //�����˲� 0.9*��ֵ+0.1��ֵ  
				   if((tdata2<10)&&(tdata2>0.0005))
						 AV_BUF2=tdata2;

				   if(SAMPLE[channel].sample_Vpoint_1<(SAMPLE[channel].sample_LOW-0x200)) //�������ﲻ����
				   {
				     tdata2=0;
					   AV_BUF2=0;
					   RTU[channel].Alarm=0x01;         //���߱�־
				   }
				   //else
				   //{
				   //  RTU[channel].Alarm=0x00;
				   //}
				   if(RTU[channel].Type==V_1_5)       //�ź�Ϊ1-5V�ź�  
				   {
				     if(tdata2<1.0)
					 {
					   tdata2=1.0;
					   RTU[channel].Alarm=0x01;         //���߱�־
					 }
				   }
					 
					 #if 0 
				   tdata2=tdata2/5*30000;
				   if(tdata2>30000) tdata2=30000;
					 #endif
					 
					  projectval = tdata2/5*30000;
					  if(projectval>30000) projectval =30000;
					
					   if(tdata2>5.0)
							 tdata2 =5.0;
					 
					 if(RTU[channel].Type==V_5)                   //0-5V �ź�����
					 {
						 
						   RTU[channel].RSV = RTU[channel].RangeL+(projectval-0)*(RTU[channel].RangeH-RTU[channel].RangeL)/(30000-0); //ת����0-65535�Ĺ���ֵ
						   //RTU[channel].RSV = 0+(projectval-0)*(65535-0)/(30000-0); //ת����0-65535�Ĺ���ֵ
						   RTU[channel].value = tdata2;             //���ʵ��AVֵ
						   if(RTU[channel].RSV>=RTU[channel].HiAlm)
						   {
							   RTU[channel].Alarm = 0x02;	 
						   }
						   if(RTU[channel].RSV>=RTU[channel].RangeH)
						   {
							   RTU[channel].Alarm = 0x04;
						   }
						   if(RTU[channel].RSV<=RTU[channel].LowAlm)
						   {
							   RTU[channel].Alarm = 0x03;
						   }
						   if(RTU[channel].RSV<=RTU[channel].RangeL)
						   {
							   RTU[channel].Alarm = 0x05;
						   }

						   if(RTU[channel].RSV<(RTU[channel].HiAlm-Deadline))   //���������ޱ������
						   {
							   RTU[channel].Alarm = 0x00; 

						   }

//						   if(RTU[channel].RSV<(RTU[channel].RangeH-Deadline))  //���������ޱ������
//						   {
//							   RTU[channel].Alarm = 0x00;
//						   }

						   if(RTU[channel].RSV>(RTU[channel].LowAlm+Deadline))  //���ڱ������ޱ������
						   {
							   RTU[channel].Alarm = 0x00;
						   }
//						   if(RTU[channel].RSV>(RTU[channel].RangeL+Deadline))  //�����������ޱ������
//						   {
//							   RTU[channel].Alarm = 0x00;
//						   }
						  	 
           }
					 else if (RTU[channel].Type==V_1_5)           // 1--5V �ź�����
					 {
						 if(tdata2<1.0)
						 {
							  tdata2 = 1.0; 
							  projectval = 6000;
							  
             }
						 else if(projectval>30000)
						 {
							 projectval = 30000; 
							 tdata2 = 5.0;
             }
						 RTU[channel].RSV = RTU[channel].RangeL+(projectval-6000)*(RTU[channel].RangeH-RTU[channel].RangeL)/(30000-6000);   //ת����0-65535�Ĺ���ֵ
						 //RTU[channel].RSV = 0+(projectval-6000)*(65535-0)/(30000-6000);   //ת����0-65535�Ĺ���ֵ
						 RTU[channel].value = tdata2;
						 if(RTU[channel].RSV>=RTU[channel].HiAlm)
						 {
							 RTU[channel].Alarm = 0x02;	 
						 }
						 if(RTU[channel].RSV>=RTU[channel].RangeH)
						 {
							 RTU[channel].Alarm = 0x04;
						 }
						 if(RTU[channel].RSV<=RTU[channel].LowAlm)
						 {
							 RTU[channel].Alarm = 0x03;
						 }
						 if(RTU[channel].RSV<=RTU[channel].RangeL)
						 {
							 RTU[channel].Alarm = 0x05;
						 }

						 if(RTU[channel].RSV<(RTU[channel].HiAlm-Deadline))   //���������ޱ������
						 {
							 RTU[channel].Alarm = 0x00; 

						 }

//						 if(RTU[channel].RSV<(RTU[channel].RangeH-Deadline))  //���������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }

						 if(RTU[channel].RSV>(RTU[channel].LowAlm+Deadline))  //���ڱ������ޱ������
						 {
							 RTU[channel].Alarm = 0x00;
						 }
//						 if(RTU[channel].RSV>(RTU[channel].RangeL+Deadline))  //�����������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }
						 
          }
					 
					break;

				case mA0_10:                                    //0-10mA
				case mA4_20:                                    //4-20mA
					
		       bdhigh=(float)RTU[channel].BD.BD_20mA_HIGH/100;
				   bdlow=(float)RTU[channel].BD.BD_20mA_LOW;
				   atemp1=(((float)SAMPLE[channel].sample_Vpoint_1)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   atemp2=(((float)SAMPLE[channel].sample_Vpoint_2)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   tdata1=atemp1*((float)RTU[channel].BD.BD_5_HIGH/1000)+((float)RTU[channel].BD.BD_5_LOW/100000);
				   tdata2=atemp2*((float)RTU[channel].BD.BD_5_HIGH/1000)+((float)RTU[channel].BD.BD_5_LOW/100000);
				   rang_f=(tdata1-tdata2)/ma_tie*1000;
				   tdata2=(tdata1-tdata2)/bdhigh*1000;
				   if(tdata2<0.0005)	tdata2=0;
				
				   if((RTU[channel].Type==mA0_10)&&(AV_BUF2<0.0005))
				   {
						   AV_BUF2=0;	
				   }
					 
				   ax1=tdata2*1000;
				   ax2=AV_BUF2*1000;
				   if(ax1-ax2>=50)
				     AV_BUF2=tdata2;
				   if(ax2-ax1>=50)
				     AV_BUF2=tdata2;			
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF2=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF2=tdata2;				   
				    tdata2=tdata2*0.1+AV_BUF2*0.9;
				   if((tdata2<30)&&(tdata2>0.0005))
						AV_BUF2=tdata2;
				   if((RTU[channel].Type==mA4_20)&&(AV_BUF2<3.960))
				   {
				     tdata2=4.0;
					   AV_BUF2=0;
					   RTU[channel].Alarm=0x01; //����
				   }
				   //else
				   //{
				   //  RTU[channel].Alarm=0x00;
				   //}
					 #if 0 
				   tdata2=tdata2/20*30000;
				   if(tdata2>30000) tdata2=30000;
					 # endif
					 
					  projectval = tdata2/20*30000;
					  if(projectval>30000) projectval =30000;
					  if(tdata2>20.0)
							 tdata2 =20.0;
					 
					 if(RTU[channel].Type==mA0_10)    //0-10mA �ź�����
					 {
						  if(projectval>15000) 
						 {
							  projectval = 15000; 
							  tdata2 =10.0;
             }
						 if(tdata2>10.0)
						 {
							 tdata2 =10.0;
							 projectval =15000;
						 }
						 
						 RTU[channel].RSV =  RTU[channel].RangeL+(projectval-0)*(RTU[channel].RangeH-RTU[channel].RangeL)/(15000-0); //ת����0-65535�Ĺ���?
						 //RTU[channel].RSV =  0+(projectval-0)*(65535-0)/(15000-0); //ת����0-65535�Ĺ���?
						 RTU[channel].value = tdata2;
						 if(RTU[channel].RSV>=RTU[channel].HiAlm)
						 {
							 RTU[channel].Alarm = 0x02;	 
						 }
						 if(RTU[channel].RSV>=RTU[channel].RangeH)
						 {
							 RTU[channel].Alarm = 0x04;
						 }
						 if(RTU[channel].RSV<=RTU[channel].LowAlm)
						 {
							 RTU[channel].Alarm = 0x03;
						 }
						 if(RTU[channel].RSV<=RTU[channel].RangeL)
						 {
							 RTU[channel].Alarm = 0x05;
						 }

						 if(RTU[channel].RSV<(RTU[channel].HiAlm-Deadline))   //���������ޱ������
						 {
							 RTU[channel].Alarm = 0x00; 

						 }

//						 if(RTU[channel].RSV<(RTU[channel].RangeH-Deadline))  //���������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }

						 if(RTU[channel].RSV>(RTU[channel].LowAlm+Deadline))  //���ڱ������ޱ������
						 {
							 RTU[channel].Alarm = 0x00;
						 }
//						 if(RTU[channel].RSV>(RTU[channel].RangeL+Deadline))  //�����������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }
           }
					 else if (RTU[channel].Type==mA4_20) // 4--20mA �ź�����
					 {
						 if(projectval<6000) 
						 {
							  projectval = 6000; 
							  tdata2 = 4.0;
							 
             }
						 else if(projectval>30000)
						 {
							 projectval = 30000;
               tdata2 =20.0;							 
             }
						 if(tdata2<4.0)
						 {
							 tdata2 =4.0;
							 projectval = 6000;
						 }
						 else if(tdata2>20.0)
						 {
							 projectval = 30000;
						 }
						 RTU[channel].RSV = RTU[channel].RangeL+(projectval-6000)*(RTU[channel].RangeH-RTU[channel].RangeL)/(30000-6000);  //ת����0-65535֮���ֵ
						 //RTU[channel].RSV = 0+(projectval-6000)*(65535-0)/(30000-6000);  //ת����0-65535֮���ֵ
						 RTU[channel].value = tdata2;
						 if(RTU[channel].RSV>=RTU[channel].HiAlm)
						 {
							 RTU[channel].Alarm = 0x02;	 
						 }
						 if(RTU[channel].RSV>=RTU[channel].RangeH)
						 {
							 RTU[channel].Alarm = 0x04;
						 }
						 if(RTU[channel].RSV<=RTU[channel].LowAlm)
						 {
							 RTU[channel].Alarm = 0x03;
						 }
						 if(RTU[channel].RSV<=RTU[channel].RangeL)
						 {
							 RTU[channel].Alarm = 0x05;
						 }

						 if(RTU[channel].RSV<(RTU[channel].HiAlm-Deadline))   //���������ޱ������
						 {
							 RTU[channel].Alarm = 0x00; 

						 }

//						 if(RTU[channel].RSV<(RTU[channel].RangeH-Deadline))  //���������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }

						 if(RTU[channel].RSV>(RTU[channel].LowAlm+Deadline))  //���ڱ������ޱ������
						 {
							 RTU[channel].Alarm = 0x00;
						 }
//						 if(RTU[channel].RSV>(RTU[channel].RangeL+Deadline))  //�����������ޱ������
//						 {
//							 RTU[channel].Alarm = 0x00;
//						 }
           }
				break;
					 
				default: 
					break;
	 				
			}
			break;
			
			default:
			break;
	}
	



}

static void system_para_init(void)
{

   /**********ADC������ر�����ʼ��*********/
	  AV_BUF0 = 0; 
	  AV_BUF1 = 0;
	  AV_BUF2 = 0;
    
   /****************************************/
	  memset(&RTU[0],0,sizeof(RTU[0]));
	  memset(&RTU[1],0,sizeof(RTU[1]));
	  memset(&RTU[2],0,sizeof(RTU[2]));
    sec_10 =0;
	  RUNorBD =0;
	  collectChannel =0;   //��ǰ�ɼ�ͨ����ʼ��Ϊ0 
	  BusCheckEN =0;
	  RTU[CHANNEL0].Type = V_5;
	  RTU[CHANNEL1].Type = V_5;
	  RTU[CHANNEL2].Type = V_5;
//	  RTU[CHANNEL0].BD.BD_5_HIGH = 5075;  
//	  RTU[CHANNEL0].BD.BD_5_LOW = 911; 
//	  RTU[CHANNEL1].BD.BD_5_HIGH = 5082;  
//	  RTU[CHANNEL1].BD.BD_5_LOW = 788; 	
//	  RTU[CHANNEL2].BD.BD_5_HIGH = 5082;  
//	  RTU[CHANNEL2].BD.BD_5_LOW = 788; 


  RTU[CHANNEL0].BD.BD_5_HIGH = 5082;  
	  RTU[CHANNEL0].BD.BD_5_LOW = 788; 
	  RTU[CHANNEL1].BD.BD_5_HIGH = 5082;  
	  RTU[CHANNEL1].BD.BD_5_LOW = 788; 	
	  RTU[CHANNEL2].BD.BD_5_HIGH = 5082;  
	  RTU[CHANNEL2].BD.BD_5_LOW = 788; 
		
		RTU[CHANNEL0].BD.BD_20mA_HIGH = 19949;  
	  RTU[CHANNEL0].BD.BD_20mA_LOW = 0; 
		RTU[CHANNEL1].BD.BD_20mA_HIGH = 19949;  
	  RTU[CHANNEL1].BD.BD_20mA_LOW = 0; 
		RTU[CHANNEL2].BD.BD_20mA_HIGH = 19949;  
	  RTU[CHANNEL2].BD.BD_20mA_LOW = 0; 

    RTU[CHANNEL0].RangeH = 65535;
	  RTU[CHANNEL0].RangeL =0;
	  RTU[CHANNEL0].HiAlm =  65535;
	  RTU[CHANNEL0].LowAlm =0; 
 
	  RTU[CHANNEL1].RangeH = 65535;
	  RTU[CHANNEL1].RangeL =0;
	  RTU[CHANNEL1].HiAlm =  65535;
	  RTU[CHANNEL1].LowAlm =0;

	  RTU[CHANNEL2].RangeH = 65535;
	  RTU[CHANNEL2].RangeL =0;
	  RTU[CHANNEL2].HiAlm =  65535;
	  RTU[CHANNEL2].LowAlm =0;

		RTU[CHANNEL0].Alarm = 0x00;
		RTU[CHANNEL1].Alarm = 0x00;
		RTU[CHANNEL2].Alarm = 0x00;

	  //��W25q64�ж�ȡ����ֵ 20170306
	  sampledote0 = 0;
    sampledote1 = 0;
    sampledote2 = 0;
	  sampleolddote0 =0;
	  sampleolddote1 =0;
   	sampleolddote2 =0;
	  CHannel0finish =0;
	  CHannel1finish =0; 
	  CHannel2finish =0;
	
	RTU[CHANNEL0].Alarm = 0x00;
	RTU[CHANNEL1].Alarm = 0x00;
	RTU[CHANNEL2].Alarm = 0x00;
	  
   /****************************************/
	 /****************************************/

}

//�����������л�����
static void DealwithCollect(unsigned char channel)
{
	if(BusCheckEN==1)
	 {
		  
		  BusCheckEN =0; 
		  switch(channel)
			{
				case CHANNEL0:
				sampleolddote0 = sampledote0;
				sampledote0++;   
				sw_select(CHANNEL0);  
        DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);		
				Data_Deal(CHANNEL0,sampleolddote0);
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 
				break;
			  
				case CHANNEL1:
				sampleolddote1 = sampledote1;
				sampledote1++;
				sw_select(CHANNEL1);
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);	
				Data_Deal(CHANNEL1,sampleolddote1);
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
				break;
				
				case CHANNEL2:
				sampleolddote2 = sampledote2;
				sampledote2++;
				sw_select(CHANNEL2);
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);	
				Data_Deal(CHANNEL2,sampleolddote2);
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
				break;
				
				default:
				break;
				
			}
		 	 
	 }
}



//�жϴ�����һ��ͨ�������ݲɼ�
static void FinishCollect(void)
{
	if((CHannel0finish==1)||(CHannel1finish==1)||(CHannel2finish==1))
	{
		 if(CHannel0finish==1)            //ͨ��0������ɣ�
		 {
			  CHannel0finish =0;
			  DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);
			  TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE); 
			  sampleolddote0 =0;
			  sampledote0 =0;
			  Calculate(CHANNEL0);          //����ת��ͨ��0
				ADCValUpdate( CHANNEL0);
			  if(collectChannel==0)         //�ɼ��굱ǰ������ͨ��,�л�����һ��ͨ��
				{
					collectChannel =1;
					sw_select(CHANNEL1);        //׼������ͨ��1
				}
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 
		 }
		 else if(CHannel1finish ==1)         //ͨ��1 �������?
		 {
			   CHannel1finish =0;
			   DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);
			   TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE); 
			   sampleolddote1 =0;
			   sampledote1 =0;
			   Calculate(CHANNEL1);          //����ת��ͨ��1
				ADCValUpdate( CHANNEL1);
			   if(collectChannel==1)         //�ɼ��굱ǰ������ͨ��,�л�����һ��ͨ��
				 {
					 collectChannel =2;
					 sw_select(CHANNEL2);        //׼���ɼ�ͨ��2 
				 }
				 
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 
		 }
		 
		 else if(CHannel2finish==1)          //ͨ��2 �������?
		 {
			 CHannel2finish=0;
			 DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);
			 TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
			sampleolddote2 =0;
			 sampledote2 =0;	
			Calculate(CHANNEL2);           //����ת��ͨ��2
			 ADCValUpdate( CHANNEL2);
			 if(collectChannel==2)
			 {
				 collectChannel =0;
				 sw_select(CHANNEL0);         //׼������ͨ��0
			 }
			 DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);
			 TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
       			 
		 }
		 
	}
				 
				
}

//int adc_start( void *base, int chn)
//{
//	
//	return ERR_OK;
//}

//��ȡԭʼ�������ݣ�����ֵ�����ݵĳ���
//������ֵ<0��ʱ���ʾʧ��
//int adc_getRawData( void *base, int chn, void *out_data)
//{
//	
//	return 0;
//}

//void digit_filtering( void *in_data, int in_len, void *out_data, int *out_len)
//{
//	
//	
//}

////����ʵʱֵ,��������ֵת���ɵ�ѹֵ
//int calc_voltage(void *in_data, int in_len, void *out_val)
//{
//	
//	
//}

////����ѹֵת���ɹ���ֵ
//int calc_engiVal( void *voltage, void *engival)
//{
//	
//}
int adc_test(void *buf, int size)
{
//	int len = 0;
//	int vlt = 0;
//	float engi = 00;
//	while(1)
//	{
//		//һ�����ݲɼ��������Ĵ������
//		adc_start( ADC_BASE, ADC_chn);
//		len = adc_getRawData( ADC_BASE, ADC_chn, buf);
//		digit_filtering( buf, len, buf, &len);
//		calc_voltage( buf, len, &vlt);
//		calc_engiVal( &vlt, &engi);
//		
//	
//		
//		
//		
//	}
	return ERR_OK;
}

//DMA1ͨ��1��ADC���ж�
//������64�����ݽ��ж�
void DMA1_Channel1_IRQHandler(void)
{
  	if(DMA_GetITStatus(DMA1_IT_GL1)!= RESET)
	{
		/* Disable DMA1 channel1 */
  		DMA_Cmd(DMA1_Channel1, DISABLE);

		ADC_SoftwareStartConvCmd(ADC1, DISABLE);  //��ֹADC�ж�

		ADC_Cmd(ADC1, DISABLE);	                   		 
		//����ת������1��ͨ��10,MCP6S21�����ͨ������ 
    	//ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5); 
		BusCheckEN=1;                            //ADC������ɱ�־
		// dote0++;
		// if(dote0>3)
		//  dote0 = 0;
		//  sw_select(0);
		DMA1_Configuration();
		DMA_ClearFlag(DMA1_IT_GL1);
	}

}




#if 0
void Data_Deal(unsigned char ch_temp,unsigned char pred)
{
	unsigned char  i_temp,j_temp,m_temp;
	unsigned int	data_temp;
	data_temp=0;
	switch(ch_temp)
	{
		case 0:
			for(j_temp=0;j_temp<200;j_temp++) 	  //�����㷨
			{
				for(m_temp=0;m_temp<200-j_temp;m_temp++)
				{
					if(ADCConvertedValue[m_temp]>ADCConvertedValue[m_temp+1])
					{
						data_temp=ADCConvertedValue[m_temp];
						ADCConvertedValue[m_temp]=ADCConvertedValue[m_temp+1];
						ADCConvertedValue[m_temp+1]=data_temp;
					}
				}
			}
			data_temp=0;
			for(i_temp=88;i_temp<113;i_temp++)
			{
				data_temp=data_temp+ADCConvertedValue[i_temp];
			}
			data_temp=data_temp/25;
			test_data0=(unsigned short) data_temp;
			
			switch(pred)
			{
				case 0:		       //����VX12(ʵ���ź�low)
					
					if(RTU.Type==8||RTU.Type==9)
					{
					  	if(signal_select==3)	  //��������ź�20mA��ʱ�򣬶�Ӧ��200R�����¶˵ĵ�ѹֵ
						{
						            BD20mA_tvh2_currentval = (unsigned short) data_temp;
									if(BD20mA_tvh2_currentval>=BD20mA_tvh2_lastval)
									{
										BUSADError20mA_tvh2 = BD20mA_tvh2_currentval- BD20mA_tvh2_lastval;
									} 
									else
									{
										BUSADError20mA_tvh2 = BD20mA_tvh2_lastval - BD20mA_tvh2_currentval; 
									}	
									if(BUSADError20mA_tvh2>10)
									{
										 BDADCnt20mA_tvh2 = 0;  
									}	
		                            else
		                            {  
											 BDADCnt20mA_tvh2++;
									   if(BDADCnt20mA_tvh2>100)
									   {
									      BDADCnt20mA_tvh2 = 0;
									      BD20mA_tvh2 = BD20mA_tvh2_currentval;
										  BD_restart = 3;
		                               }	
		 								 
								    }
								
						BD20mA_tvh2_lastval = BD20mA_tvh2_currentval;      
					   }
					
						else if( signal_select==4)     //�������0-20mA��׼�ǵ� 0mA
						{
								 BD20mA_tl = 0; 
                                 BD_finish = 2;							
						}
						else
						{
								sample_V12=(unsigned short) data_temp;
						}
					
						
				}

				
					else if(RTU.Type==10||RTU.Type==11)          //����ǵ�ѹ�ź� ����ֵVX11
					{
					  
                       if(signal_select==1)	                     //BD״̬ʱ�������ⲿ����5V ��׼�ź�
					  {
					        BD5V_th_currentval =  (unsigned short) data_temp;
							if(BD5V_th_currentval>=BD5V_th_lastval)
							{
							  	  BUSADError_5Vth =BD5V_th_currentval-BD5V_th_lastval;
							}
							else 
							{
							      BUSADError_5Vth =BD5V_th_lastval-BD5V_th_currentval;
							}

	
							if(BUSADError_5Vth>10)
							{
							   BDADCnt_5Vth = 0;
							}
							else 
							{
							     BDADCnt_5Vth++;
								 if(BDADCnt_5Vth>100)
								 {
								    BDADCnt_5Vth = 0;
								    BD5V_th = BD5V_th_currentval;
									  BD_restart = 2;                        //������0-5V �źŵ�  5V ��׼֮�󣬽�����̶�ȡ����0V��״̬ 
									 
								 }
							}
                         
						BD5V_th_lastval = BD5V_th_currentval;    

					  }

					  else if(signal_select==2)                 //BD״̬ʱ������0-5V�ⲿ���� 0V
					  {
						     BD5V_tl_currentval =  (unsigned short) data_temp;
							if(BD5V_tl_currentval>=BD5V_tl_lastval)
							{
							  	  BUSADError_5Vtl =BD5V_tl_currentval-BD5V_tl_lastval;
							}
							else 
							{
							      BUSADError_5Vtl = 	BD5V_tl_lastval - BD5V_tl_currentval;
							}


							if(BUSADError_5Vtl>10)
							{
							   BDADCnt_5Vtl = 0;
							}
							else 
							{
							    BDADCnt_5Vtl++;
								 if(BDADCnt_5Vtl>100)
								 {
								    BDADCnt_5Vtl = 0;
								    BD5V_tl = BD5V_tl_currentval;
									  BD_finish = 1;
									 
								 }
							}
                         
						  BD5V_tl_lastval = BD5V_tl_currentval;    

					  }

					  else
					  {
					    sample_V11 = (unsigned short) data_temp; 
					  }

					}

				break;	          //����VX11(ʵ���ź�high)
				case 1:
					if(RTU.Type==8||RTU.Type==9)
					{
						if(signal_select==3)       //��������ź�20mA��ʱ�򣬶�Ӧ��200R�����϶˵ĵ�ѹֵ
						{
							       BD20mA_tvh1_currentval = (unsigned short) data_temp;
									if(BD20mA_tvh1_currentval>=BD20mA_tvh1_lastval)
									{
										BUSADError20mA_tvh1 = BD20mA_tvh1_currentval- BD20mA_tvh1_lastval;
									} 
									else
									{
										BUSADError20mA_tvh1 = BD20mA_tvh1_lastval - BD20mA_tvh1_currentval; 
									}	
									if(BUSADError20mA_tvh1>10)
									{
										 BDADCnt20mA_tvh1 = 0;  
									}	
		                             else
		                            {  
											 BDADCnt20mA_tvh1++;
											 if(BDADCnt20mA_tvh1>100)
											 {
											      BDADCnt20mA_tvh1 = 0;
											      BD20mA_tvh1 = BD20mA_tvh1_currentval;
												  BD_restart = 4;
				                             }	
		 								 
								    }
								
						               BD20mA_tvh1_lastval = BD20mA_tvh1_currentval;   
                      }
					else if( signal_select==4)         //�������0-20mA��׼�ǵ� 0mA
					{
						 BD20mA_tl = 0; 
                         BD_finish = 2;									
					}
					else
					{
						 sample_V11=(unsigned short) data_temp;
					}


					}
				 
			    	else if(RTU.Type==10||RTU.Type==11)	      //����ǵ�ѹ�ź�(������׼GND)
					{
					   sample_LOW1=(unsigned short) data_temp;
					}
				break;

				case 2:		 //����GND
					if(RTU.Type==8||RTU.Type==9)
					{
						sample_LOW1=(unsigned short) data_temp;
					}
					
			    	else if(RTU.Type==10||RTU.Type==11)	     //����ǵ�ѹ�ź�(������׼VR0)
					{
					    sample_HIGH1=(unsigned short) data_temp;
					    ch0_over = 1;
					}
				   
				break;

				case 3:	 //����VR0
					if(RTU.Type==8||RTU.Type==9)
					{
						sample_HIGH1=(unsigned short) data_temp;
						ch0_over=1;
					}
					else
					{

					}
					 /*
					else if(RTU.Type==10||RTU.Type==11)	     //����ǵ�ѹ�ź�(������׼VR0)
					{
					    sample_NC=(unsigned short) data_temp;
					    ch0_over = 1;
					}
					*/
				    
				break;

				   /*
				case 4:
				      	if(RTU.Type==8||RTU.Type==9)
					{
						sample_NC=(unsigned short) data_temp;
						ch0_over=1;
					}

					else 
					{

					}

				  break;
				 */

				default :
					break;
			}
			break;
		
		
		default :
			break;
		
	}
	
	if( (BD_restart ==3)||(BD_restart == 4))   //������0-20mA�źŵ�20mA  �������0mA (��λ��Ӧ�ı�־)
	{
		   BD_restart =5;  
    }
		
}

void Calculate(unsigned char ch_temp)
{
	float	bdhigh,bdlow,atemp1,atemp2,tdata1,tdata2,ax1,ax2;
	
	float	temp_data1,temp_data2,temp_data3,temp_data4,temp_data5,temp_data6;
	switch(ch_temp)
	{
		case 0:

			switch(RTU.Type)
			{

				case 10:		                //��ѹ�ź�0-5V ����1-5V
				case 11:
		       bdhigh=(float)RTU.BD.BD_5_HIGH/1000;
				   bdlow=(float)RTU.BD.BD_5_LOW/100000;
				   atemp1=(((float)sample_V11)-(float)sample_LOW1)/((float)sample_HIGH1-(float)sample_LOW1);
				   tdata2=atemp1*bdhigh+bdlow;
				   if(tdata2<0.0005)	tdata2=0;
				   if(AV_BUF0>10)
				   {
					   AV_BUF0=0;	
				   }		
				   if((RTU.Type==V_5)&&(AV_BUF0<0.0005))
				   {
						   AV_BUF0=0;	

				   }
				   if((RTU.Type==V_1_5)&&(AV_BUF0<0.0005))
				   {
						   AV_BUF0=1.0;
				   }				   
				   ax1=tdata2*1000;
				   ax2=AV_BUF0*1000;
				   if(ax1-ax2>=10)
				     AV_BUF0=tdata2;
				   if(ax2-ax1>=10)
				     AV_BUF0=tdata2;	
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;				   
				   tdata2=tdata2*0.1+AV_BUF0*0.9;
				   
				   if((tdata2<10)&&(tdata2>0.0005))
						AV_BUF0=tdata2;

				   if(sample_V11<(sample_LOW1-0x200))
				   {
				     tdata2=0;
					 AV_BUF0=0;
					 RTU.Alarm=0x02; //����
				   }
				   else
				   {
				     RTU.Alarm=0x00;
				   }
				   if(RTU.Type==V_1_5)
				   {
				     if(tdata2<1.0)
					   tdata2=1;
				   }
				   tdata2=tdata2/5*30000;
				   if(tdata2>30000) tdata2=30000;
					 
					 if(RTU.Type==10)        //0-5V �ź�����
					 {
						 RTU.AV = tdata2*65535/30000;     //ת���ɹ���ֵ
           }
					 else if (RTU.Type==11) // 1--5V �ź�����
					 {
						 if(tdata2<6000) 
						 {
							  tdata2 = 6000; 
             }
						 else if(tdata2>30000)
						 {
							 tdata2 = 30000; 
             }
						 RTU.AV = ((tdata2-6000) *65535/24000 );  //ת���ɹ���ֵ
           }
				  // RTU.AV=tdata2;      //2014.11.27 llj
					break;

				case 8:  //0-10mA
				case 9:  //4-20mA
		       bdhigh=(float)RTU.BD.BD_20mA_HIGH/100;
				   bdlow=(float)RTU.BD.BD_20mA_LOW;
				   atemp1=(((float)sample_V11)-(float)sample_LOW1)/((float)sample_HIGH1-(float)sample_LOW1);
				   atemp2=(((float)sample_V12)-(float)sample_LOW1)/((float)sample_HIGH1-(float)sample_LOW1);
				   tdata1=atemp1*((float)RTU.BD.BD_5_HIGH/1000)+((float)RTU.BD.BD_5_LOW/100000);
				   tdata2=atemp2*((float)RTU.BD.BD_5_HIGH/1000)+((float)RTU.BD.BD_5_LOW/100000);
				   rang_f=(tdata1-tdata2)/ma_tie*1000;
				   tdata2=(tdata1-tdata2)/bdhigh*1000;
				   if(tdata2<0.0005)	tdata2=0;
				   if((RTU.Type==8)&&(AV_BUF0<0.0005))
				   {
						   AV_BUF0=0;	

				   }

				   ax1=tdata2*1000;
				   ax2=AV_BUF0*1000;
				   if(ax1-ax2>=50)
				     AV_BUF0=tdata2;
				   if(ax2-ax1>=50)
				     AV_BUF0=tdata2;			
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;				   
				   tdata2=tdata2*0.1+AV_BUF0*0.9;
				   
				   if((tdata2<30)&&(tdata2>0.0005))
						AV_BUF0=tdata2;

				   if((RTU.Type==mA4_20)&&(AV_BUF0<3.960))
				   {
				     tdata2=4;
					 AV_BUF0=0;
					 RTU.Alarm=0x02; //����
				   }
				   else
				   {
				     RTU.Alarm=0x00;
				   }
				   tdata2=tdata2/20*30000;
				   if(tdata2>30000) tdata2=30000;
					 
					 if(RTU.Type==8)        //0-10mA �ź�����
					 {
						  if(tdata2>15000) 
						 {
							  tdata2 = 15000; 
             }
						 RTU.AV = tdata2*65535/15000;     //ת���ɹ���ֵ
           }
					 else if (RTU.Type==9) // 4--20mA �ź�����
					 {
						 if(tdata2<6000) 
						 {
							  tdata2 = 6000; 
             }
						 else if(tdata2>30000)
						 {
							 tdata2 = 30000; 
                         }
						 RTU.AV = ((tdata2-6000) *65535/24000 );  //ת���ɹ���ֵ
           }
					 
			//	   RTU.AV=tdata2;
					break;
				default:
					break;
				   
			}
			break;

			default :
			break;
	}
	
}



//DMA1ͨ��1��ADC���ж�
//������64�����ݽ��ж�
void DMA1_Channel1_IRQHandler(void)
{
  	if(DMA_GetITStatus(DMA1_IT_GL1)!= RESET)
	{
		/* Disable DMA1 channel1 */
  		DMA_Cmd(DMA1_Channel1, DISABLE);

		ADC_SoftwareStartConvCmd(ADC1, DISABLE);  //��ֹADC�ж�

		ADC_Cmd(ADC1, DISABLE);	                   		 
		//����ת������1��ͨ��10,MCP6S21�����ͨ������ 
    	//ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5); 
		BusCheckEN=1;                            //ADC������ɱ�־
		// dote0++;
		// if(dote0>3)
		//  dote0 = 0;
		//  sw_select(0);
		DMA1_Configuration();
		DMA_ClearFlag(DMA1_IT_GL1);
	}

}
#endif


