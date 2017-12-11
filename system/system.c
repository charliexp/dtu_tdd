//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//
#include "system.h"
//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------
dtu_system 	dsys;

//------------------------------------------------------------------------------
// global function prototypes
//------------------------------------------------------------------------------

//============================================================================//
//            P R I V A T E   D E F I N I T I O N S                           //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------



//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//
void Led_level(int lv)
{
	uint16_t	c_ms[] = {200, 600, 1200, 2000, 0xffff};
	if(lv > 4 || lv < 0)
			lv = 0;
	dsys.led.led_cycle_ms = c_ms[lv];
	
}

bool check_bit(uint8_t *data, int bit)
{
	int i, j ;
	i = bit/8;
	j = bit % 8;
	return ( data[i] & ( 1 << j));
	
	
}

void clear_bit(uint8_t *data, int bit)
{
	int i, j ;
	i = bit/8;
	j = bit % 8;
	data[i] &= ~( 1 << j);
	
	
	
	
}

void set_bit(uint8_t *data, int bit)
{
	int i, j ;
	i = bit/8;
	j = bit % 8;
	data[i] |=  1 << j;
	
	
	
	
}
//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//
/// \name Private Functions
/// \{
