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
0xFF200040  SW0     Bit 0           Display     Toggle between displaying current time (0) and lap time (1)
*/

#include "seven_segment_display.h"

#define TIMER_BASE          0xFFFEC600
#define SW_BASE             0xFF200040
#define BUTTON_BASE         0xFF200050

#define LOAD            0xFFFF // needs a load of at least 0xFFFF
#define CLOCK_RATE_HZ   200000000 
#define PRESCALER       255

// 100 in denominator is for scaling to get rate in hundredths of a second
const int PRESCALED_CLOCK_RATE_CHZ = CLOCK_RATE_HZ / (100 * (PRESCALER + 1));

// SW0 is the bit at the base, SW1 is the next, etc
volatile int* const switch_bank_ptr = (int *)(SW_BASE);
void write_switches_to_buffer(int* buffer) {
    *buffer = *switch_bank_ptr;
}

volatile int* const button_bank_ptr = (int *)(BUTTON_BASE);
void write_buttons_to_buffer(char* buffer) {
    *buffer = *button_bank_ptr & 0b1000;
    *(buffer + 0x1) = *button_bank_ptr & 0b0100;
    *(buffer + 0x2) = *button_bank_ptr & 0b0010;
    *(buffer + 0x3) = *button_bank_ptr & 0b0001;
}

volatile unsigned int* const current_value_ptr = (unsigned int *)(TIMER_BASE + 0x4);
unsigned int accumulated_clock_cycles = 0;
unsigned int overflows = 0; 
unsigned int last = 0;
void write_time_to_buffer(unsigned int* buffer) {
    unsigned int current = *current_value_ptr;
    if (current > last) { ++overflows; last = 0; }
    accumulated_clock_cycles += current - last;
    last = current; 

    *buffer = (LOAD - current) / PRESCALED_CLOCK_RATE_CHZ;
    hundredths_to_mm_ss_hh(buffer);
}

int timer_is_on = 0;
volatile unsigned int* const load_ptr = (unsigned int *)(TIMER_BASE);
volatile unsigned short* const control_ptr = (unsigned short *)(TIMER_BASE+0x8);

void start_timer(void) {
    if (timer_is_on) { return; }
    
    *load_ptr = LOAD; // load initial value
    *control_ptr = 0b1111111100000011; // set prescaler to 255, set auto bit to 1, set enable   
    
    timer_is_on = 1;
}

void stop_timer(void) {
    if (!timer_is_on) { return; }
    
    // shut off timer
    *control_ptr ^= 0b1; // sets the last 1 (the enable bit) to 0 

    timer_is_on = 0;
}

unsigned int stored_time = 0;
void lap_timer(void) {
    if (!timer_is_on) { return; }

    write_time_to_buffer(&stored_time);
}

void clear_timer(void) {
    if (!timer_is_on) { return; }
    stop_timer();

    // clear
    stored_time = 0;
    *load_ptr = LOAD;
    *current_value_ptr = 0x0; 
}

int last_hh = -1; 
void hundredths_to_mm_ss_hh(unsigned int* time) {
    int mm = (*time / 6000 + (overflows * (LOAD / PRESCALED_CLOCK_RATE_CHZ) / 6000)) % 100;
    int ss = (*time / 100 + (overflows * (LOAD / PRESCALED_CLOCK_RATE_CHZ) / 100)) % 60;
    int hh = (*time + (overflows * (LOAD / PRESCALED_CLOCK_RATE_CHZ))) % 100;
    if (hh == last_hh) { return; }
    printf("t: %u\n%d m : %d s ; %d hh\n\n", *time, mm, ss, hh);
    last_hh = hh;
    //return retval;
}

volatile int DELAY_LENGTH = 700000;
int main(void) {
    unsigned int unsigned_time_hundredths;
    int time_mm_ss_hh = 0;
    int current_time_or_lap_time = 0;
    volatile int delay = 0;

    display_hex(0);

    // bit 3, bit 2, bit 1, bit 0
    // clear, lap,   stop,  start
    char control_bits[4] = {0};
    while(1) {
        write_switches_to_buffer(&current_time_or_lap_time);
        write_buttons_to_buffer(&control_bits[0]);

        if (control_bits[0]) { start_timer(); }
        else if (control_bits[1]) { stop_timer(); }
        else if (control_bits[2]) { lap_timer(); }
        else if (control_bits[3]) { clear_timer(); }

        write_time_to_buffer(&unsigned_time_hundredths);

        //for (delay = DELAY_LENGTH; delay != 0; --delay) {}
    }
}
