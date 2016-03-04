#include "sam.h"
#include "ra8875.h"
#include <stdio.h>
#include <string.h>
#include "flash_storage.h"
#include "utils.h"
#include "grbl.h"
#include "protocol.h"


system_t sys; 


int main(void)
{

	char buffer[128];
	int speed = 5000;
	int x = 150;
	uint32_t ram;
	int i = 0;
	int accel = 150;
	uint8_t flash;
    SystemInit();
	WDT->WDT_MR = WDT_MR_WDDIS;	// Disable watchdog
	
	tft_init(); 
	settings_init(); // Load Grbl settings from EEPROM
	stepper_init();  // Configure stepper pins and interrupt timers
	system_init();   // Configure pinout pins and pin-change interrupt		
	memset(&sys, 0, sizeof(sys));  // Clear all system variables
	sys.abort = true;   // Set abort to complete initialization		
	init_grbl();
	beep(50);
	tft_draw_press_unpress();
	my_flash_init();
	flash = flash_read_8(0);
	if(flash != 0x55) 
	{
		tft_calibrate_touch();
		flash = 0x55;
		flash_write_buffer(0,&flash,1);
	} else 
		tft_calibrate_load();

znovu:
	settings.acceleration[0] = accel * 60 * 60;
	settings.acceleration[1] = accel * 60 * 60;
	settings.acceleration[2] = accel * 60 * 60;
	tft_delete_all_objects();
	tft_fillScreen(RA8875_LIME);
	tft_textColor(RA8875_BLACK ,RA8875_LIME);
	ram = get_heap_free_size1();
	sprintf(buffer,"HEAP=%8ld",ram);
	tft_textWrite(10,200,0,0,buffer,ALINE_LEFT);
	sprintf(buffer,"G1X%d",x);
	tft_button(10,CalcY(10,4,4),200,CalcHeight(10,4),buffer,1,RA8875_CYAN,5);
	sprintf(buffer,"G1X%d",0);
	tft_button(220,CalcY(10,4,4),200,CalcHeight(10,4),buffer,2,RA8875_CYAN,5);
	tft_button(640,CalcY(10,4,1),150,CalcHeight(10,4),"while(1)",3,RA8875_CYAN,5);
	tft_button(640,CalcY(10,4,2),150,CalcHeight(10,4),"set speed",4,RA8875_CYAN,5);
	tft_button(640,CalcY(10,4,3),150,CalcHeight(10,4),"set X size",5,RA8875_CYAN,5);
	tft_button(640,CalcY(10,4,4),150,CalcHeight(10,4),"set accel",6,RA8875_CYAN,5);
    while (1) 
    {
		draw_pos();
		tft_draw_press_unpress();
		i++;
		if(Action1 == 1)
		{
			Action1 = 0;
			sprintf(buffer,"G1X%dF%d",x,speed);
			protocol_execute_line(buffer);
			bit_true_atomic(sys_rt_exec_state, EXEC_CYCLE_START);
			while(sys.state == STATE_CYCLE)
			{
				protocol_execute_realtime();	//	CNC
				draw_pos();
				i++;
			}
			beep(10);
		}
		if(Action1 == 2)
		{
			Action1 = 0;
			sprintf(buffer,"G1X00F%d",speed);
			protocol_execute_line(buffer);
			bit_true_atomic(sys_rt_exec_state, EXEC_CYCLE_START);
			while(sys.state == STATE_CYCLE)
			{
				protocol_execute_realtime();	//	CNC
				draw_pos();
				i++;
			}
			beep(10);
		}
		if(Action == 3)
		{
			Action = 0;
			while(1)	
			{
				sprintf(buffer,"G1X%dF%d",x,speed);
				protocol_execute_line(buffer);
				bit_true_atomic(sys_rt_exec_state, EXEC_CYCLE_START);
				while(sys.state == STATE_CYCLE)
				{
					protocol_execute_realtime();	//	CNC
					tft_textWrite(10,300,0,0,buffer,ALINE_LEFT);
					if(needtouch)
					{
						needtouch = false;
						draw_pos();
					}
					tft_draw_press_unpress();
					i++;
				}
				if(Action1 == 4) break;
				beep(10);
				sprintf(buffer,"G1X0F%d",speed);
				protocol_execute_line(buffer);
				bit_true_atomic(sys_rt_exec_state, EXEC_CYCLE_START);
				while(sys.state == STATE_CYCLE)
				{
					protocol_execute_realtime();	//	CNC
					if(needtouch)
					{
						needtouch = false;
						draw_pos();
					}
					tft_draw_press_unpress();
					i++;
				}
				if(Action1 == 4) break;
				beep(10);
			}
		}
		if(Action1 == 4)
		{
			Action1 = 0;
			speed = tft_GetInt("speed",speed,0,100000);
			goto znovu;
		}
		if(Action1 == 5)
		{
			Action1 = 0;
			x = tft_GetInt("X",x,0,10000);
			goto znovu;
		}
		if(Action1 == 6)
		{
			Action1 = 0;
			accel = tft_GetInt("accel",accel,0,10000);
			goto znovu;
		}
    }
}
