# wakeuplight
__Working title:__ _Morning has broken_  
__Production title:__ _Waking on sunshine_

Arduino project, LCD for time, joystick for setting time, PWM output (LED).  
As of today, Arduino IDE does not like Pro Micro, patch instructions in folder arduino_addon.  
Starts PWM LED (linear to the eye!) at a hard coded time. 30 minutes, then shuts off.

# HW
- Arduino Pro Micro
- Arduino joystick. Anuthing should work, currently only uses 1 axis + button.
- Arduino display, apparently it is some Toshiba thingy, identifiable by having 16 legs. Only use half of data signals (4-7).

# Usage
- One long click to enable display cursor and clock adjustment mode.
- Move cursor with short clicks
- Change time with joystick.
- Leave mode with long click or not touching joystick for like 6 seconds.
- Potentioeter must be modified for text on display to be visible.
- Display is hotplugg-ish. If removed, just activate cursor on display and it is re-initiated.

# Schematics
MCU_GND - MCU GND  
MCU_GND - GND  
MCU_2 - 1MOhm - GND  
MCU_2 (clk in) - MCU_10 (clk out)  
MCU_3 - MOSFET_1_Gate  
MCU_4 - Disaply_D7  
MCU_5 - Display_D6  
MCU_6 - Display_D5  
MCU_7 - Joy_SW  
MCU_8 - Display_E  
MCU_9 - Display_RS  
MCU_16 - Display_D4  
MCU_A0 - Joy_VRx  
MCU_A1 - Joy_VRy  
MCU_VCC - 5V  
Joy_GND - GND  
Joy_5V - 5V  
MOSFET_3_Drain - GND  
MOSFET_2_Source - LED  
MOSFET_1_Gate - 47kOhm - GND  
LED - 12V  
Display_K - GND  
Display_RW - GND  
Display_VSS - GND  
Display_VDD - 5V  
Display_A - 5V  
Pot_2kOhm_GND - GND  
Pot_2kOhm_5V - 5V  
Pot_2kOhm_o - Display_V0  
Vreg_in - 12V  
Vreg_gnd - GND  
Vreg_out - 5V  
Vreg_out - power_led - ohm - GND  

# TODO
- No alarm on weekends.
- LED on until it is turned off or timeout.
- If LED not turned off, smal motor pops up and starts strobing.
- Configurable alarm time.
- Something w. backlight dimming.
