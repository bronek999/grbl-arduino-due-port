/*
  report.c - reporting and messaging methods
  Part of Grbl

  Copyright (c) 2012-2015 Sungeun K. Jeon  

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

/* 
  This file functions as the primary feedback interface for Grbl. Any outgoing data, such 
  as the protocol status messages, feedback messages, and status reports, are stored here.
  For the most part, these functions primarily are called from protocol.c methods. If a 
  different style feedback is desired (i.e. JSON), then a user can change these following 
  methods to accomodate their needs.
*/

#include "grbl.h"


// Handles the primary confirmation protocol response for streaming interfaces and human-feedback.
// For every incoming line, this method responds with an 'ok' for a successful command or an 
// 'error:'  to indicate some error event with the line or some critical system error during 
// operation. Errors events can originate from the g-code parser, settings module, or asynchronously
// from a critical error, such as a triggered hard limit. Interface should always monitor for these
// responses.
// NOTE: In silent mode, all error codes are greater than zero.
// TODO: Install silent mode to return only numeric values, primarily for GUIs.
void report_status_message(uint8_t status_code) 
{
  if (status_code == 0) { // STATUS_OK
    printString(("ok\r\n"));
  } else {
    printString(("error: "));
    #ifdef REPORT_GUI_MODE
      print_uint8_base10(status_code);
    #else
      switch(status_code) {          
        case STATUS_EXPECTED_COMMAND_LETTER:
        printString(("Expected command letter")); break;
        case STATUS_BAD_NUMBER_FORMAT:
        printString(("Bad number format")); break;
        case STATUS_INVALID_STATEMENT:
        printString(("Invalid statement")); break;
        case STATUS_NEGATIVE_VALUE:
        printString(("Value < 0")); break;
        case STATUS_SETTING_DISABLED:
        printString(("Setting disabled")); break;
        case STATUS_SETTING_STEP_PULSE_MIN:
        printString(("Value < 3 usec")); break;
        case STATUS_SETTING_READ_FAIL:
        printString(("EEPROM read fail. Using defaults")); break;
        case STATUS_IDLE_ERROR:
        printString(("Not idle")); break;
        case STATUS_ALARM_LOCK:
        printString(("Alarm lock")); break;
        case STATUS_SOFT_LIMIT_ERROR:
        printString(("Homing not enabled")); break;
        case STATUS_OVERFLOW:
        printString(("Line overflow")); break;
        #ifdef MAX_STEP_RATE_HZ
          case STATUS_MAX_STEP_RATE_EXCEEDED: 
          printString(("Step rate > 30kHz")); break;
        #endif      
        // Common g-code parser errors.
        case STATUS_GCODE_MODAL_GROUP_VIOLATION:
        printString(("Modal group violation")); break;
        case STATUS_GCODE_UNSUPPORTED_COMMAND:
        printString(("Unsupported command")); break;
        case STATUS_GCODE_UNDEFINED_FEED_RATE:
        printString(("Undefined feed rate")); break;
        default:
          // Remaining g-code parser errors with error codes
          printString(("Invalid gcode ID:"));
          print_uint8_base10(status_code); // Print error code for user reference
      }
    #endif  
    printString(("\r\n"));
  }
}

// Prints alarm messages.
void report_alarm_message(int8_t alarm_code)
{
  printString(("ALARM: "));
  #ifdef REPORT_GUI_MODE
    print_uint8_base10(alarm_code);
  #else
    switch (alarm_code) {
      case ALARM_HARD_LIMIT_ERROR: 
      printString(("Hard limit")); break;
      case ALARM_SOFT_LIMIT_ERROR:
      printString(("Soft limit")); break;
      case ALARM_ABORT_CYCLE: 
      printString(("Abort during cycle")); break;
      case ALARM_PROBE_FAIL:
      printString(("Probe fail")); break;
      case ALARM_HOMING_FAIL:
      printString(("Homing fail")); break;
    }
  #endif
  printString(("\r\n"));
  delay_ms(500); // Force delay to ensure message clears serial write buffer.
}

// Prints feedback messages. This serves as a centralized method to provide additional
// user feedback for things that are not of the status/alarm message protocol. These are
// messages such as setup warnings, switch toggling, and how to exit alarms.
// NOTE: For interfaces, messages are always placed within brackets. And if silent mode
// is installed, the message number codes are less than zero.
// TODO: Install silence feedback messages option in settings
void report_feedback_message(uint8_t message_code)
{
  printString(("["));
  switch(message_code) {
    case MESSAGE_CRITICAL_EVENT:
    printString(("Reset to continue")); break;
    case MESSAGE_ALARM_LOCK:
    printString(("'$H'|'$X' to unlock")); break;
    case MESSAGE_ALARM_UNLOCK:
    printString(("Caution: Unlocked")); break;
    case MESSAGE_ENABLED:
    printString(("Enabled")); break;
    case MESSAGE_DISABLED:
    printString(("Disabled")); break; 
    case MESSAGE_SAFETY_DOOR_AJAR:
    printString(("Check Door")); break;
    case MESSAGE_PROGRAM_END:
    printString(("Pgm End")); break;
    case MESSAGE_RESTORE_DEFAULTS:
    printString(("Restoring defaults")); break;
  }
  printString(("]\r\n"));
}


// Welcome message
void report_init_message()
{
  printString(("\r\nGrbl " GRBL_VERSION " ['$' for help]\r\n"));
}

// Grbl help message
void report_grbl_help() {
  #ifndef REPORT_GUI_MODE
    printString(("$$ (view Grbl settings)\r\n"
                        "$# (view # parameters)\r\n"
                        "$G (view parser state)\r\n"
                        "$I (view build info)\r\n"
                        "$N (view startup blocks)\r\n"
                        "$x=value (save Grbl setting)\r\n"
                        "$Nx=line (save startup block)\r\n"
                        "$C (check gcode mode)\r\n"
                        "$X (kill alarm lock)\r\n"
                        "$H (run homing cycle)\r\n"
                        "~ (cycle start)\r\n"
                        "! (feed hold)\r\n"
                        "? (current status)\r\n"
                        "ctrl-x (reset Grbl)\r\n"));
  #endif
}


// Grbl global settings print out.
// NOTE: The numbering scheme here must correlate to storing in settings.c
void report_grbl_settings() {
  // Print Grbl settings.
  #ifdef REPORT_GUI_MODE
    printString(("$0=")); print_uint8_base10(settings.pulse_microseconds);
    printString(("\r\n$1=")); print_uint8_base10(settings.stepper_idle_lock_time);
    printString(("\r\n$2=")); print_uint8_base10(settings.step_invert_mask); 
    printString(("\r\n$3=")); print_uint8_base10(settings.dir_invert_mask); 
    printString(("\r\n$4=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_INVERT_ST_ENABLE));
    printString(("\r\n$5=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_INVERT_LIMIT_PINS));
    printString(("\r\n$6=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_INVERT_PROBE_PIN));
    printString(("\r\n$10=")); print_uint8_base10(settings.status_report_mask);
    printString(("\r\n$11=")); printFloat_SettingValue(settings.junction_deviation);
    printString(("\r\n$12=")); printFloat_SettingValue(settings.arc_tolerance);
    printString(("\r\n$13=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_REPORT_INCHES));
    printString(("\r\n$20=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_SOFT_LIMIT_ENABLE));
    printString(("\r\n$21=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_HARD_LIMIT_ENABLE));
    printString(("\r\n$22=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_HOMING_ENABLE));
    printString(("\r\n$23=")); print_uint8_base10(settings.homing_dir_mask);
    printString(("\r\n$24=")); printFloat_SettingValue(settings.homing_feed_rate);
    printString(("\r\n$25=")); printFloat_SettingValue(settings.homing_seek_rate);
    printString(("\r\n$26=")); print_uint8_base10(settings.homing_debounce_delay);
    printString(("\r\n$27=")); printFloat_SettingValue(settings.homing_pulloff);
    printString(("\r\n"));
  #else      
    printString(("$0=")); print_uint8_base10(settings.pulse_microseconds);
    printString((" (step pulse, usec)\r\n$1=")); print_uint8_base10(settings.stepper_idle_lock_time);
    printString((" (step idle delay, msec)\r\n$2=")); print_uint8_base10(settings.step_invert_mask); 
    printString((" (step port invert mask:")); print_uint8_base2(settings.step_invert_mask);  
    printString((")\r\n$3=")); print_uint8_base10(settings.dir_invert_mask); 
    printString((" (dir port invert mask:")); print_uint8_base2(settings.dir_invert_mask);  
    printString((")\r\n$4=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_INVERT_ST_ENABLE));
    printString((" (step enable invert, bool)\r\n$5=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_INVERT_LIMIT_PINS));
    printString((" (limit pins invert, bool)\r\n$6=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_INVERT_PROBE_PIN));
    printString((" (probe pin invert, bool)\r\n$10=")); print_uint8_base10(settings.status_report_mask);
    printString((" (status report mask:")); print_uint8_base2(settings.status_report_mask);
    printString((")\r\n$11=")); printFloat_SettingValue(settings.junction_deviation);
    printString((" (junction deviation, mm)\r\n$12=")); printFloat_SettingValue(settings.arc_tolerance);
    printString((" (arc tolerance, mm)\r\n$13=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_REPORT_INCHES));
    printString((" (report inches, bool)\r\n$20=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_SOFT_LIMIT_ENABLE));
    printString((" (soft limits, bool)\r\n$21=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_HARD_LIMIT_ENABLE));
    printString((" (hard limits, bool)\r\n$22=")); print_uint8_base10(bit_istrue(settings.flags,BITFLAG_HOMING_ENABLE));
    printString((" (homing cycle, bool)\r\n$23=")); print_uint8_base10(settings.homing_dir_mask);
    printString((" (homing dir invert mask:")); print_uint8_base2(settings.homing_dir_mask);  
    printString((")\r\n$24=")); printFloat_SettingValue(settings.homing_feed_rate);
    printString((" (homing feed, mm/min)\r\n$25=")); printFloat_SettingValue(settings.homing_seek_rate);
    printString((" (homing seek, mm/min)\r\n$26=")); print_uint8_base10(settings.homing_debounce_delay);
    printString((" (homing debounce, msec)\r\n$27=")); printFloat_SettingValue(settings.homing_pulloff);
    printString((" (homing pull-off, mm)\r\n"));
  #endif
  
  // Print axis settings
  uint8_t idx, set_idx;
  uint8_t val = AXIS_SETTINGS_START_VAL;
  for (set_idx=0; set_idx<AXIS_N_SETTINGS; set_idx++) {
    for (idx=0; idx<N_AXIS; idx++) {
      printString(("$"));
      print_uint8_base10(val+idx);
      printString(("="));
      switch (set_idx) {
        case 0: printFloat_SettingValue(settings.steps_per_mm[idx]); break;
        case 1: printFloat_SettingValue(settings.max_rate[idx]); break;
        case 2: printFloat_SettingValue(settings.acceleration[idx]/(60*60)); break;
        case 3: printFloat_SettingValue(-settings.max_travel[idx]); break;
      }
      #ifdef REPORT_GUI_MODE
        printString(("\r\n"));
      #else
        printString((" ("));
        switch (idx) {
          case X_AXIS: printString(("x")); break;
          case Y_AXIS: printString(("y")); break;
          case Z_AXIS: printString(("z")); break;
        }
        switch (set_idx) {
          case 0: printString((", step/mm")); break;
          case 1: printString((" max rate, mm/min")); break;
          case 2: printString((" accel, mm/sec^2")); break;
          case 3: printString((" max travel, mm")); break;
        }      
        printString((")\r\n"));
      #endif
    }
    val += AXIS_SETTINGS_INCREMENT;
  }  
}


// Prints current probe parameters. Upon a probe command, these parameters are updated upon a
// successful probe or upon a failed probe with the G38.3 without errors command (if supported). 
// These values are retained until Grbl is power-cycled, whereby they will be re-zeroed.
void report_probe_parameters()
{
  uint8_t i;
  float print_position[N_AXIS];
 
  // Report in terms of machine position.
  printString(("[PRB:"));
  for (i=0; i< N_AXIS; i++) {
    print_position[i] = system_convert_axis_steps_to_mpos(sys.probe_position,i);
    printFloat_CoordValue(print_position[i]);
    if (i < (N_AXIS-1)) { printString((",")); }
  }
  printString((":"));
  print_uint8_base10(sys.probe_succeeded);
  printString(("]\r\n"));
}


// Prints Grbl NGC parameters (coordinate offsets, probing)
void report_ngc_parameters()
{
  float coord_data[N_AXIS];
  uint8_t coord_select, i;
  for (coord_select = 0; coord_select <= SETTING_INDEX_NCOORD; coord_select++) { 
    if (!(settings_read_coord_data(coord_select,coord_data))) { 
      report_status_message(STATUS_SETTING_READ_FAIL); 
      return;
    } 
    printString(("[G"));
    switch (coord_select) {
      case 6: printString(("28")); break;
      case 7: printString(("30")); break;
      default: print_uint8_base10(coord_select+54); break; // G54-G59
    }  
    printString((":"));         
    for (i=0; i<N_AXIS; i++) {
      printFloat_CoordValue(coord_data[i]);
      if (i < (N_AXIS-1)) { printString((",")); }
      else { printString(("]\r\n")); }
    } 
  }
  printString(("[G92:")); // Print G92,G92.1 which are not persistent in memory
  for (i=0; i<N_AXIS; i++) {
    printFloat_CoordValue(gc_state.coord_offset[i]);
    if (i < (N_AXIS-1)) { printString((",")); }
    else { printString(("]\r\n")); }
  } 
  printString(("[TLO:")); // Print tool length offset value
  printFloat_CoordValue(gc_state.tool_length_offset);
  printString(("]\r\n"));
  report_probe_parameters(); // Print probe parameters. Not persistent in memory.
}


// Print current gcode parser mode state
void report_gcode_modes()
{
  printString(("["));
  
  switch (gc_state.modal.motion) {
    case MOTION_MODE_SEEK : printString(("G0")); break;
    case MOTION_MODE_LINEAR : printString(("G1")); break;
    case MOTION_MODE_CW_ARC : printString(("G2")); break;
    case MOTION_MODE_CCW_ARC : printString(("G3")); break;
    case MOTION_MODE_NONE : printString(("G80")); break;
    default: 
      printString(("G38."));
      print_uint8_base10(gc_state.modal.motion - (MOTION_MODE_PROBE_TOWARD-2));
  }

  printString((" G"));
  print_uint8_base10(gc_state.modal.coord_select+54);
  
  switch (gc_state.modal.plane_select) {
    case PLANE_SELECT_XY : printString((" G17")); break;
    case PLANE_SELECT_ZX : printString((" G18")); break;
    case PLANE_SELECT_YZ : printString((" G19")); break;
  }
  
  if (gc_state.modal.units == UNITS_MODE_MM) { printString((" G21")); }
  else { printString((" G20")); }
  
  if (gc_state.modal.distance == DISTANCE_MODE_ABSOLUTE) { printString((" G90")); }
  else { printString((" G91")); }
  
  if (gc_state.modal.feed_rate == FEED_RATE_MODE_INVERSE_TIME) { printString((" G93")); }
  else { printString((" G94")); }
    
  switch (gc_state.modal.program_flow) {
    case PROGRAM_FLOW_RUNNING : printString((" M0")); break;
    case PROGRAM_FLOW_PAUSED : printString((" M1")); break;
    case PROGRAM_FLOW_COMPLETED : printString((" M2")); break;
  }

  switch (gc_state.modal.spindle) {
    case SPINDLE_ENABLE_CW : printString((" M3")); break;
    case SPINDLE_ENABLE_CCW : printString((" M4")); break;
    case SPINDLE_DISABLE : printString((" M5")); break;
  }
  
  switch (gc_state.modal.coolant) {
    case COOLANT_DISABLE : printString((" M9")); break;
    case COOLANT_FLOOD_ENABLE : printString((" M8")); break;
    #ifdef ENABLE_M7
      case COOLANT_MIST_ENABLE : printString((" M7")); break;
    #endif
  }
  
  printString((" T"));
  print_uint8_base10(gc_state.tool);
  
  printString((" F"));
  printFloat_RateValue(gc_state.feed_rate);
  
  #ifdef VARIABLE_SPINDLE
    printString((" S"));
    printFloat_RateValue(gc_state.spindle_speed);
  #endif

  printString(("]\r\n"));
}

// Prints specified startup line
void report_startup_line(uint8_t n, char *line)
{
  printString(("$N")); print_uint8_base10(n);
  printString(("=")); printString(line);
  printString(("\r\n"));
}


// Prints build info line
void report_build_info(char *line)
{
  printString(("[" GRBL_VERSION "." GRBL_VERSION_BUILD ":"));
  printString(line);
  printString(("]\r\n"));
}


// Prints the character string line Grbl has received from the user, which has been pre-parsed,
// and has been sent into protocol_execute_line() routine to be executed by Grbl.
void report_echo_line_received(char *line)
{
  printString(("[echo: ")); printString(line);
  printString(("]\r\n"));
}


 // Prints real-time data. This function grabs a real-time snapshot of the stepper subprogram 
 // and the actual location of the CNC machine. Users may change the following function to their
 // specific needs, but the desired real-time data report must be as short as possible. This is
 // requires as it minimizes the computational overhead and allows grbl to keep running smoothly, 
 // especially during g-code programs with fast, short line segments and high frequency reports (5-20Hz).
void report_realtime_status()
{
  // **Under construction** Bare-bones status report. Provides real-time machine position relative to 
  // the system power on location (0,0,0) and work coordinate position (G54 and G92 applied). Eventually
  // to be added are distance to go on block, processed block id, and feed rate. Also a settings bitmask
  // for a user to select the desired real-time data.
  uint8_t idx;
  int32_t current_position[N_AXIS]; // Copy current state of the system position variable
  memcpy(current_position,sys.position,sizeof(sys.position));
  float print_position[N_AXIS];
 
  // Report current machine state
  switch (sys.state) {
    case STATE_IDLE: printString(("<Idle")); break;
    case STATE_MOTION_CANCEL: // Report run state.
    case STATE_CYCLE: printString(("<Run")); break;
    case STATE_HOLD: printString(("<Hold")); break;
    case STATE_HOMING: printString(("<Home")); break;
    case STATE_ALARM: printString(("<Alarm")); break;
    case STATE_CHECK_MODE: printString(("<Check")); break;
    case STATE_SAFETY_DOOR: printString(("<Door")); break;
  }
 
  // If reporting a position, convert the current step count (current_position) to millimeters.
  if (bit_istrue(settings.status_report_mask,(BITFLAG_RT_STATUS_MACHINE_POSITION | BITFLAG_RT_STATUS_WORK_POSITION))) {
    system_convert_array_steps_to_mpos(print_position,current_position);
  }
  
  // Report machine position
  if (bit_istrue(settings.status_report_mask,BITFLAG_RT_STATUS_MACHINE_POSITION)) {
    printString((",MPos:")); 
    for (idx=0; idx< N_AXIS; idx++) {
      printFloat_CoordValue(print_position[idx]);
      if (idx < (N_AXIS-1)) { printString((",")); }
    }
  }
  
  // Report work position
  if (bit_istrue(settings.status_report_mask,BITFLAG_RT_STATUS_WORK_POSITION)) {
    printString((",WPos:")); 
    for (idx=0; idx< N_AXIS; idx++) {
      // Apply work coordinate offsets and tool length offset to current position.
      print_position[idx] -= gc_state.coord_system[idx]+gc_state.coord_offset[idx];
      if (idx == TOOL_LENGTH_OFFSET_AXIS) { print_position[idx] -= gc_state.tool_length_offset; }    
      printFloat_CoordValue(print_position[idx]);
      if (idx < (N_AXIS-1)) { printString((",")); }
    }
  }
        
  // Returns the number of active blocks are in the planner buffer.
  if (bit_istrue(settings.status_report_mask,BITFLAG_RT_STATUS_PLANNER_BUFFER)) {
    printString((",Buf:"));
    print_uint8_base10(plan_get_block_buffer_count());
  }

  // Report serial read buffer status
  if (bit_istrue(settings.status_report_mask,BITFLAG_RT_STATUS_SERIAL_RX)) {
    printString((",RX:"));
    print_uint8_base10(serial_get_rx_buffer_count());
  }
    
  #ifdef USE_LINE_NUMBERS
    // Report current line number
    printString((",Ln:")); 
    int32_t ln=0;
    plan_block_t * pb = plan_get_current_block();
    if(pb != NULL) {
      ln = pb->line_number;
    } 
    printInteger(ln);
  #endif
    
  #ifdef REPORT_REALTIME_RATE
    // Report realtime rate 
    printString((",F:")); 
    printFloat_RateValue(st_get_realtime_rate());
  #endif    
  
  if (bit_istrue(settings.status_report_mask,BITFLAG_RT_STATUS_LIMIT_PINS)) {
    printString((",Lim:"));
    print_unsigned_int8(limits_get_state(),2,N_AXIS);
  }
  
  #ifdef REPORT_CONTROL_PIN_STATE 
    printString((",Ctl:"));
    print_uint8_base2(CONTROL_PIN & CONTROL_MASK);
  #endif
  
  printString((">\r\n"));
}
