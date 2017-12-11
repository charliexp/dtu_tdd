#ifndef __ADC_H__
#define __ADC_H__
#include <stdint.h>
#define		Time500mS		       	100
#define     Time5000mS			   	500
#define		Time1000mS		     	100



/*===Ai�ڵ��ź����Ͷ���===*/
#define B_o         	 0  /*B���ȵ�ż*/
#define E_o        		 1  /*E���ȵ�ż*/
#define J_o        		 2  /*J���ȵ�ż*/
#define K_o         	 3  /*K���ȵ�ż*/
#define S_o         	 4  /*S���ȵ�ż*/
#define T_o        		 5  /*T���ȵ�ż*/
#define Pt100          6  /*Pt100�ȵ���*/
#define Cu50           7  /*Cu50�ȵ���*/
#define mA0_10			   8	/*0~10���������ź�*/
#define mA4_20			   9	/*4~20���������ź�*/
#define V_5			 	     10	/*0~5�����ź�*/
#define V_1_5			     11	/*1��5�����ź�*/
#define mV_20      	 	 12	/*0~20������ѹ�ź�*/
#define mV_100     	 	 13	/*0��100������ѹ�ź�*/

//=====================================//
#define VX32           0
#define VX22           1
#define VX21           2
#define VX31           3
#define VX12           4
#define GND            5
#define VX11           6
#define VR0_A          7
//=====================================//

#define CHANNEL0       0
#define CHANNEL1       1
#define CHANNEL2       2
//=====================================//

#define Samplepoint0   0 
#define Samplepoint1   1
#define Samplepoint2   2
#define Samplepoint3   3
//=====================================//


//***********ѡ���������ͨ���л�����**********//

#define   SelectSET4051_A               GPIO_ResetBits(GPIOB,GPIO_Pin_5)
#define   SelectCLR4051_A               GPIO_SetBits(GPIOB,GPIO_Pin_5) 

#define   SelectSET4051_B               GPIO_ResetBits(GPIOB,GPIO_Pin_6)
#define   SelectCLR4051_B               GPIO_SetBits(GPIOB,GPIO_Pin_6) 

#define   SelectSET4051_C               GPIO_ResetBits(GPIOB,GPIO_Pin_7)
#define   SelectCLR4051_C               GPIO_SetBits(GPIOB,GPIO_Pin_7) 

//---------------------------------------------------------------------//

//***********�����������ֵʱ���л�����*************//
#define   SET_currentnum0         GPIO_SetBits(GPIOC,GPIO_Pin_1) 
#define   CLR_currentnum0 			  GPIO_ResetBits(GPIOC,GPIO_Pin_1)

#define   SET_currentnum1         GPIO_SetBits(GPIOC,GPIO_Pin_2) 
#define   CLR_currentnum1 			  GPIO_ResetBits(GPIOC,GPIO_Pin_2)

#define   SET_currentnum2         GPIO_SetBits(GPIOC,GPIO_Pin_3) 
#define   CLR_currentnum2 			  GPIO_ResetBits(GPIOC,GPIO_Pin_3)


//---------------------------------------------------------------------//

#define Deadline  655                  //�����ز�ֵ0.05V    20170303
typedef struct{
  	 unsigned short  BD_20mV_HIGH;	       //20mV�궨Highֵ	     
	   short           BD_20mV_LOW;				   //20mV�궨Lowֵ
	   unsigned short  BD_100_HIGH;	         //100mV�궨Highֵ  
	   short           BD_100_LOW;				   //100mV�궨Lowֵ	   
	   unsigned short  BD_5_HIGH;		         //5V�궨Highֵ	  
	   short           BD_5_LOW;				     //5V�궨LOWֵ		  
	   unsigned short  BD_20mA_HIGH;	       //20mA�궨Highֵ	 
	   short           BD_20mA_LOW;			     //20mA�궨Lowֵ	   
	   unsigned short  BD_Pt100_HIGH;	       //PT100�궨Highֵ
     short           BD_Pt100_LOW;			   //PT100�궨Lowֵ
	   unsigned short  BD_Cu50_HIGH;	       //Cu50�궨Highֵ
	   short           BD_Cu50_LOW;			     //Cu50�궨Lowֵ	
}BD_para;


extern unsigned short ConfigCheckCnt;
extern unsigned short ADC_timcount;        //ADC����ʱ��
extern unsigned short comout_count;
extern unsigned char  HalfSecCnt500mS;
extern unsigned short HalfSecCnt5000mS;
extern unsigned char  Falg_HalfSec;
extern unsigned char  RunStat;
extern unsigned char  sec_10;
extern unsigned char  BusCheckEN;		
extern unsigned char  collectChannel;                    //��ǰ�Ĳɼ�ͨ��
extern unsigned char  RUNorBD;                           //�궨�������б�־(RUNorBD =1��ʾΪ�궨״̬  RUNorBD ==0��ʾΪ����״̬)
extern unsigned char  sampledote0,sampleolddote0;        //ͨ��0������
extern unsigned char  sampledote1,sampleolddote1;        //ͨ��1������
extern unsigned char  sampledote2,sampleolddote2;        //ͨ��2������

extern unsigned char  BD_sampledote,BD_sampleolddote;    //�궨��ǰͨ���Ĳ����źŵ�
extern unsigned short BUSADdelta; 
extern unsigned char  BUSADdelataCnt;                     
extern unsigned char  CHannel0finish, CHannel1finish, CHannel2finish; //һ���źŲ�����ɱ�־
extern unsigned short ADCval0,ADCval1,ADCval2;
extern float AV_BUF0,AV_BUF1,AV_BUF2;
extern float rang_f;
extern float ma_tie;
extern unsigned short ADCConvertedValue[200];






struct _RemoteRTU_Module_
{
	uint8_t		Type;			 //�ڵ�����			 	
	uint8_t		Status;		 //�ڵ�״̬			          
	uint8_t		Alarm;		 //�ڵ㱨��			 
	uint8_t    none;      //�����ֽ�  
	float value;     //ʵ�ʲ����ĵ���ֵ���ߵ�ѹֵ
	uint16_t   RSV;       //0-65535�Ĺ���ת��ֵ
	uint16_t		AV;			   //��ǰ����ֵ	
	uint16_t   AlarmDead; //�����ز�
	uint16_t   RangeH;    //��������
	uint16_t   RangeL;    //��������
	uint16_t   HiAlm;     //��������
	uint16_t   LowAlm;    //��������		
	uint16_t   DeadBard;  //ǰ�����β���ֵ����
	uint32_t   Nouse;     //�ֽڶ���

	BD_para		BD;			
	uint8_t    SPACE[2];  //�ֽڶ�����	
	uint8_t		LRC_NUM[2];				
};


struct _SampleValue_
{
	
 unsigned short	sample_Vpoint_1;
 unsigned short	sample_Vpoint_2;
 unsigned short	sample_Vpoint_3;
 unsigned short	sample_HIGH;
 unsigned short	sample_LOW;
 unsigned short sample_NC;
};


struct _BDSampleVal_
{		  
	
	
 unsigned short BD5V_th;
 unsigned short BD5V_tl;
 unsigned short BD5V_th_currentval;
 unsigned short BD5V_tl_currentval;
 unsigned short BD5V_th_lastval;
 unsigned short BD5V_tl_lastval; 
 unsigned char  BD5V_thDelta;
 unsigned char  BD5V_tlDelta;
 unsigned char  BD5V_thDeltaCnt;
 unsigned char  BD5V_tlDeltaCnt;

	
 unsigned short BD20mA_tvh2;
 unsigned short BD20mA_tvh1;
 unsigned short BD20mA_tvl;
 unsigned char  BD20mA_tvh2Delta;
 unsigned char  BD20mA_tvh1Delta;
 unsigned char  BD20mA_tvh2DeltaCnt;
 unsigned char  BD20mA_tvh1DeltaCnt;

 unsigned char  BDSignalHorL;            //1:��ʾ������ź��Ǳ궨�ź�0-20mA������(20mA)  2:��ʾ������źű궨�ź�0-20mA�Ǳ궨�źŵ�����(0mA)                                          
                                         //3:��ʾ������ź��Ǳ궨�ź�0-5V������(5V)      4:��ʾ������źű궨�ź�0-5V�Ǳ궨�źŵ�����(0V)
 unsigned char  SignalCollectFinish;     //�궨�ź��Ƿ��Ѿ���������
 unsigned short BD20mA_tvh2_currentval;
 unsigned short BD20mA_tvh2_lastval;
 unsigned short BD20mA_tvh1_currentval;
 unsigned short BD20mA_tvh1_lastval;

 unsigned short BD20mA_tl_currentval;
 unsigned short BD20mA_tl_lastval;
 unsigned short SampleLOW;
 unsigned short SampleHIGH;
	
};

extern struct  _RemoteRTU_Module_  RTU[3]; 
extern struct  _SampleValue_     SAMPLE[3];
extern struct  _BDSampleVal_     BDSAMPLE[3];


/*****************��ͨ������PI�ṹ�嶨��******************/
typedef enum { FAILED = 0, PASSED = !FAILED} TestStatus;

int adc_test(void *buf, int size);



void create_adc(void);
void Collect_job(void);
typedef void (*get_realVAl)( char chn, float realval);
typedef void (*get_rsv)( char chn, uint16_t sv);
typedef void (*get_alarm)( char chn, uint16_t alarm);

void Regist_get_val( get_realVAl get_val);
void Regist_get_rsv( get_rsv get_rsv);
void Regist_get_alarm( get_alarm get_alarm);
void Set_chnType( char chn, uint8_t type);
void Set_rangH( char chn, uint16_t rang);
void Set_rangL( char chn, uint16_t rang);
void Set_alarmH( char chn, uint16_t alarm);
void Set_alarmL( char chn, uint16_t alarm);
void ADC_50ms();
#endif
