#ifndef __DTU_H__
#define __DTU_H__
#include "lw_oopc.h"
#include "stdint.h"
#include "gprs.h"

#define DTUTMP_BUF_LEN		64
extern char	DtuTempBuf[DTUTMP_BUF_LEN];

typedef int ( *hookFunc)( char *data, int len, void *arg);
ABS_CLASS( StateContext);
INTERFACE( BusinessProcess);
ABS_CLASS( WorkState);
INTERFACE( Builder);

enum WakeStateSeq {
	STATE_SelfTest,
	STATE_Connect,
	STATE_EventHandle,
	STATE_HeatBeatHandle,
	STATE_CnntManager,
	STATE_SMSHandle,
	STATE_Total
	
	
};

CLASS( DtuContextFactory)
{
	StateContext *( *createContext)( int mode);
	
};
CLASS( RtuContextFactory)
{
	StateContext *( *createContext)( int mode);
	
};

DtuContextFactory* DCFctGetInstance(void);
RtuContextFactory* RCFctGetInstance(void);

ABS_CLASS( StateContext)
{
//	Builder *stateBuilder;
	
	char 	*dataBuf;
	int 	bufLen;
	
//	WorkState *gprsSelfTestState;
//	WorkState *gprsConnectState;
//	WorkState *gprsEventHandleState;
//	WorkState *gprsDealSMSState;
////	WorkState *gprsSer485ProcessState;
//	WorkState *gprsHeatBeatState;
//	WorkState *gprsCnntManagerState;
	WorkState *curState;
	
	int (*init)( StateContext *this, char *buf, int bufLen);
	void ( *setCurState)( StateContext *this, int targetState);
	void	(*nextState)( StateContext *this, int myState); 
	int ( *construct)( StateContext *this , Builder *state_builder);
	void (*switchToSmsMode)( StateContext *this);
	//abs
	int (*initState)( StateContext *this);
};

//state

ABS_CLASS( WorkState)
{
	char 	*dataBuf;
	short 	bufLen;
//	short		stateNum;		//����״̬�����
	

	//abs
	int (*run)( WorkState *this, StateContext *context);
	
	//impl
	int ( *init)( WorkState *this, char *buf, int bufLen);
	void (*print)( WorkState *this, char *str);
};

INTERFACE( Builder)
{
	WorkState* ( *buildGprsSelfTestState)(StateContext *this );
	WorkState* ( *buildGprsConnectState)(StateContext *this );
	WorkState* ( *buildGprsEventHandleState)(StateContext *this );
	WorkState* ( *buildGprsDealSMSState)(StateContext *this );
//	WorkState* ( *builderSer485ProcessState)(StateContext *this );
	WorkState* ( *builderGprsHeatBeatState)(StateContext *this );
	WorkState* ( *builderGprsCnntManagerState)(StateContext *this );
};


CLASS( GprsSelfTestState)
{
	
	EXTENDS( WorkState);
	
	
	
};

CLASS( GprsConnectState)
{
	EXTENDS( WorkState);
	
};

CLASS( GprsEventHandleState)
{
	EXTENDS( WorkState);
	BusinessProcess *modbusProcess;
	BusinessProcess *configSystem;
	BusinessProcess *forwardSer485;
	BusinessProcess *forwardSMS;
	BusinessProcess *forwardNet;
	
};

CLASS( GprsDealSMSState)
{
	uint8_t			dvs_type;
	uint8_t			res[3];
	
	EXTENDS(WorkState);
	
	BusinessProcess *configSystem;
	BusinessProcess *forwardSer485;
	
};
//CLASS( Ser485ProcessState)
//{
//	IMPLEMENTS( WorkState);
//	BusinessProcess *modbusProcess;
//	BusinessProcess *forwardSMS;
//	BusinessProcess *forwardNet;
//	
//};

CLASS( GprsHeatBeatState)
{
	EXTENDS( WorkState);
	
};

CLASS( GprsCnntManagerState)
{
	EXTENDS( WorkState);
	
};













CLASS(LocalRTUModeContext)
{
	EXTENDS( StateContext);
	
};
CLASS(SMSModeContext)
{
	EXTENDS( StateContext);
	
};
CLASS(RemoteRTUModeContext)
{
	EXTENDS( StateContext);
	
};
CLASS(PassThroughModeContext)
{
	EXTENDS( StateContext);
	
};




CLASS( LocalRTUModeBuilder)
{
	IMPLEMENTS( Builder);
};
CLASS( SMSModeBuilder)
{
	IMPLEMENTS( Builder);
};
CLASS( RemoteRTUModeBuilder)
{
	IMPLEMENTS( Builder);
};
CLASS( PassThroughModeBuilder)
{
	IMPLEMENTS( Builder);
};

//ҵ����

INTERFACE( BusinessProcess)
{
	
//	int (*forwardSer485)( BusinessProcess *this);
//	int (*forwardNet)( BusinessProcess *this);
//	int (*forwardSMS)( BusinessProcess *this);
//	int (*modbusProcess)( BusinessProcess *this);
//	int (*configSystem)( BusinessProcess *this);
	int ( *process)(  char *data, int len, hookFunc cb, void *arg);

};

CLASS( EmptyProcess)
{
	IMPLEMENTS( BusinessProcess);
	
};

CLASS( ModbusBusiness)
{
	IMPLEMENTS( BusinessProcess);
	
};

CLASS( ConfigSystem)
{
	
	IMPLEMENTS( BusinessProcess);
};

CLASS( ForwardSer485)
{
	IMPLEMENTS( BusinessProcess);
	
};

CLASS( ForwardNet)
{
	IMPLEMENTS( BusinessProcess);
	
	
};
CLASS( ForwardSMS)
{
	IMPLEMENTS( BusinessProcess);
	
	
};

BusinessProcess *GetEmptyProcess(void);
BusinessProcess *GetModbusBusiness(void);
BusinessProcess *GetConfigSystem(void);
BusinessProcess *GetForwardSer485(void);
BusinessProcess *GetForwardNet(void);
BusinessProcess *GetForwardSMS(void);

int Ser485ModbusAckCB( char *data, int len, void *arg);



#endif
