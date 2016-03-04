#include "sam.h"
#include "utils.h"
#include <stdlib.h>
#include "grbl.h"
#include "ra8875.h"
#include <stdio.h>
volatile char realtime=0;

char buffer[128];


OPTIMIZE_HIGH
//RAMFUNC
void portable_delay_cycles(unsigned long n)
{
	UNUSED(n);

	__asm (
		"loop: DMB	\n"
		"SUBS R0, R0, #1  \n"
		"BNE.N loop         "
	);
}



void delay_ms(int ms)
{
	while(ms--)
	{
		_delay_us(1000);
	}
}
void _delay_us(int us)
{
	volatile int x;
	while(us--)
	{
		x = 8;
		while(x--);
	}
}
void _delay_ms(int ms)
{
	delay_ms(ms);
}
uint32_t get_heap_free_size1( void )
{
	uint32_t high_mark= 600000;
	uint32_t low_mark = 0;
	uint32_t size ;
	void* p_mem;

	size = (high_mark + low_mark)/2;

	do
	{
		p_mem = malloc(size);
		if( p_mem != NULL)
		{ // Can allocate memory
			free(p_mem);
			low_mark = size;
		}
		else
		{ // Can not allocate memory
			high_mark = size;
		}

		size = (high_mark + low_mark)/2;
	}
	while( (high_mark-low_mark) >1 );

	return size;
}
void init_grbl(void)
{
    serial_reset_read_buffer(); // Clear serial read buffer
    gc_init(); // Set g-code parser to default state
    spindle_init();
    coolant_init();
    limits_init();
    probe_init();
    plan_reset(); // Clear block buffer and planner variables
    st_reset(); // Clear stepper subsystem variables.

    // Sync cleared gcode and planner positions to current system position.
    plan_sync_position();
    gc_sync_position();

    // Reset system variables.
    sys.abort = false;
    sys_rt_exec_state = 0;
    sys_rt_exec_alarm = 0;
    sys.suspend = false;
    // Start Grbl main loop. Processes program inputs and executes them.
}

void draw_pos()
{
	float x = sys.position[0] / DEFAULT_X_STEPS_PER_MM;
	float y = sys.position[1] / DEFAULT_Y_STEPS_PER_MM;
	float z = sys.position[2] / DEFAULT_Z_STEPS_PER_MM;
	sprintf(buffer," X=%10.3f Y=%10.3f Z=%10.3f",x,y,z);
	tft_textColor(RA8875_BLACK ,RA8875_LIME);
	tft_textWrite(1,1,0,0,buffer,ALINE_LEFT);
	tft_draw_press_unpress();
	
	
	
}