#ifndef SEVEN_SEGMENT_DISPLAY_H
#define SEVEN_SEGMENT_DISPLAY_H

#define HEX3_HEX0_BASE  0xFF200020
#define HEX5_HEX4_BASE  0xFF200030

typedef unsigned int hex_digit_t;

hex_digit_t SEVEN_SEG_DISPLAY_PATTERN_LOOKUP[12] = {
    0b00111111 << 24, /* 0 */
    0b00000110 << 24, /* 1 */
    0b01011011 << 24, /* 2 */
    0b01001111 << 24, /* 3 */
    0b01100110 << 24, /* 4 */
    0b01101101 << 24, /* 5 */
    0b01111101 << 24, /* 6 */
    0b00000111 << 24, /* 7 */
    0b01111111 << 24, /* 8 */
    0b01101111 << 24, /* 9 */
    0b00000000 << 24, /*   */
    0b01000000 << 24  /* - */
};

unsigned int ERR_PATTERN[] = {
    0b01010000 << 24, /* r */
    0b01011100 << 24, /* o */
    0b01010000 << 24, /* r */
    0b01010000 << 24, /* r */
    0b01111001 << 24, /* E */
    0b00000000 << 24  /*   */
};

// volatile because hardware
volatile unsigned int* const hex_register_one_ptr = (unsigned int*)(HEX3_HEX0_BASE);
volatile unsigned int* const hex_register_two_ptr = (unsigned int*)(HEX5_HEX4_BASE);

void write_to_hex(unsigned int* to_write) {
    *hex_register_one_ptr = 0;
    *hex_register_two_ptr = 0;
    int idx;
    // register length - 1. Don't bitshift after last entry
    for (idx = 0; idx < 3; idx++) {
        *hex_register_one_ptr |= *(to_write + idx);
        *hex_register_one_ptr >>= 8;
    }

    *hex_register_one_ptr |= *(to_write + 3);

    *hex_register_two_ptr |= (*(to_write + 4) >> 24);
    *hex_register_two_ptr |= (*(to_write + 5) >> 16);
} 

// Input is a 32 bit value in decimal
void display_hex(int value)
{
    unsigned int digits[6] = {0};
    unsigned char idx = 0;

    // set all 7SDs to blank
    if (value > 999999) {
        for (idx = 0; idx < 4; idx++) {
            digits[idx] = 0b0;
        }
        write_to_hex(digits);
        return;
    }

    int value_cpy = value;

    // take the abs value 
    unsigned char is_negative = 0;
    if (value < 0) {
        value_cpy = 0 - value; 
        is_negative = 1;
    } 

    
    while (value_cpy > 0) {
        digits[idx] = SEVEN_SEG_DISPLAY_PATTERN_LOOKUP[value_cpy % 10];
        
        value_cpy /= 10; // floor of the number div 10 to advance to next place
        idx += 1;
    }

    if (is_negative && idx == 6) {
        write_to_hex(ERR_PATTERN);
        return;
    }

    // make leading places nothing, or add the negative sign just before
    // the first digit
    for (int i = 5; i > idx - 1; i--) {
        // silly 
        digits[i] = SEVEN_SEG_DISPLAY_PATTERN_LOOKUP[10 + (is_negative && i == idx)];
    }

    // this is to print on one 7SD hex digits instead of decimal across multiple
    // if (value > 9) {
    //     if (value == 10) {
    //         digits[0] = 0b01110111 << 24;
    //     } else if (value == 11) {
    //         digits[0] = SEVEN_SEG_DISPLAY_PATTERN_LOOKUP[8];
    //     } else if (value == 12) {
    //         digits[0] = 0b00111001 << 24;
    //     } else if (value == 13) {
    //         digits[0] = SEVEN_SEG_DISPLAY_PATTERN_LOOKUP[0];
    //     } else if (value == 14) {
    //         digits[0] = 0b01111001 << 24;
    //     } else if (value == 15) {
    //         digits[0] = 0b01110001 << 24;
    //     }
    // }

    if (value == 0 || value > 15) {
        digits[0] = SEVEN_SEG_DISPLAY_PATTERN_LOOKUP[0];   
    }

    write_to_hex(digits);
    return;
}

#endif