#include "seven_segment_display.h"
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

