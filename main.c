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

references:
[1] DE10 Standard Datasheet - https://ftp.intel.com/Public/Pub/fpgaup/pub/Intel_Material/18.1/Computer_Systems/DE10-Standard/DE10-Standard_Computer_ARM.pdf 
*/

#include "math.h"

#define TIMER_BASE          0xFFFEC600
#define SW_BASE             0xFF200040
#define BUTTON_BASE         0xFF200050

#define LOAD            2000000 // this is one hundredth of a second
#define CLOCK_RATE_HZ   200000000 

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

volatile unsigned int* const load_ptr = (unsigned int *)(TIMER_BASE);
volatile unsigned int* const current_value_ptr = (unsigned int *)(TIMER_BASE + 0x4);
volatile unsigned int* const control_ptr = (unsigned int *)(TIMER_BASE+0x8);
volatile unsigned int* const interrupt_status_ptr = (unsigned int *)(TIMER_BASE+0xC);


char timer_is_on = 0;
void start_timer(void) {
    *interrupt_status_ptr = 1; // flag when a hundredth of a second has elapsed. Write a 1 to clear 
    *load_ptr = LOAD; // load initial value
    *control_ptr = 1; // set enable   
    timer_is_on = 1;
}

void stop_timer(void) {
    *control_ptr = 0; 
    timer_is_on = 0;
}

unsigned int elapsed_hundreds = 0;
void hundredths_to_mm_ss_hh(int* buffer) {
    //printf("overflows: %d\n", overflows);
    int mm = (elapsed_hundreds / 6000) % 100;
    int ss = (elapsed_hundreds / 100) % 60;
    int hh = (elapsed_hundreds) % 100;
    //printf("buffer: %u\n%d m : %d s ; %d hh\n\n", *buffer, mm, ss, hh);
    
    // mm:ss:hh
    *buffer = mm * pow(10, 4) + ss * pow(10, 2) + hh; 
}

void get_time(int* buffer) {
    if (*interrupt_status_ptr) {
        // a hundredth of a second has elapsed
        elapsed_hundreds++;
        start_timer();
        hundredths_to_mm_ss_hh(buffer);
    } 

    // do nothing if a hundredth has not elapsed
}


void clear_timer(int* lap_time) {
    stop_timer();
    *interrupt_status_ptr = 1;
    *load_ptr = 0;
    *current_value_ptr = 0;
    elapsed_hundreds = 0;
    *lap_time = 0;
}

int main(void) {
    int time_buffer = 0;
    int lap_time = 0;
    int current_time_or_lap_time = 0;
    *interrupt_status_ptr = 1;
    char start_is_only_button_pressed = 0;

    // bit 3, bit 2, bit 1, bit 0
    // clear, lap,   stop,  start
    char control_bits[4] = {0};

    while(1) {
        get_time(&time_buffer);

        if (control_bits[2]) { lap_time = time_buffer; }

        if (current_time_or_lap_time) {
            display_time(lap_time);
        } else {
            display_time(time_buffer);
        }

        write_switches_to_buffer(&current_time_or_lap_time);
        write_buttons_to_buffer(&control_bits[0]);

        start_is_only_button_pressed = (control_bits[0] && !control_bits[1] && !control_bits[2] && !control_bits[3]) ? 1 : 0;

        //printf("control_bits[0]: %d\ncontrol_bits[1]: %d\n", control_bits[0], control_bits[1]);
        if (timer_is_on && control_bits[1]) { stop_timer(); }
        else if (!timer_is_on && start_is_only_button_pressed) { start_timer(); }
        else if (control_bits[3]) { 
            clear_timer(&lap_time); 
            time_buffer = 0;    
        }
    }
}