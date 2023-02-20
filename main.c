/*
Cortex-A9 Private Timer 
- one per core
- counts down
    * set load value = delta t
- 200 MHz
- starts at 0xFFFEC600
    + 0x0  = Load value data register
    + 0x4  = Current value data register 
    + 0x8  = Control register
        - bit 1 = Enable (1 for start, 0 for stop)
        - bit 2 = Auto-reload (1 for auto-reload, 0 for stop after one iteration)
        - bit 3 = Interrupt? (1 generates a processor interrupt when cv = 0)
        - bits 8 to 15, inclusive = prescaler (timer decrements each ps + 1 clock cycle)
    + 0x12 = Interrupt status register
        - bit 1 = Interrupt status, 1 if timer reaches 0. Write a 1 to it to clear it 

expected format on the 7SD-bank of 6 displays is MM:SS:HH where H is 1 s / 100 (a centisecond)

Address     Switch  Bit Position    Function    Description
0xFF200050  KEY0    Bit 0           Start       Start the stopwatch
            KEY1    Bit 1           Stop        Stop the stopwatch
            KEY2    Bit 2           Lap         Store current time
            KEY3    Bit 3           Clear       Resets the stopwatch back to zero
0xFF200040  SW0     Bit 0           Display     Toggle between displaying current time and lap time
*/

#include "seven_segment_display.h"

#define PRIVATE_TIMER_BASE 0xFFFEC600
#define SW_BASE         0xFF200040

// SW0 is the bit at the base, SW1 is the next, etc
volatile int* const switch_bank_ptr = (int *)(SW_BASE);
int read_switches(void) {
    return *switch_bank_ptr; 
}

volatile int DELAY_LENGTH = 700000;
int main(void) {
    //display_hex(-345434);
    int num;
    volatile int delay_count = 0;
    while(1) {
        // on 
        num = read_switches();
        display_hex(num);

        for (delay_count = DELAY_LENGTH; delay_count != 0; --delay_count) {}
        
        // off
        display_hex(1000000);
        for (delay_count = DELAY_LENGTH; delay_count != 0; --delay_count) {}
    }
}

