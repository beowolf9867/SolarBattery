#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/pgmspace.h>

#define TOTAL_KEYS 25
#define TOUCH_THRESHOLD 450

const uint16_t note_freqs[TOTAL_KEYS] PROGMEM = {
    3822, 3608, 3405, 3214, 3034, 2863, 2703, 2551,
    2408, 2273, 2145, 2024, 1911, 1804, 1703, 1607,
    1517, 1432, 1351, 1275, 1204, 1136, 1073, 1012,
    956
};

void system_init(void) {
    // setup port c for mux addressing and enable lines
    DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | 
            (1 << PC3) | (1 << PC4) | (1 << PC5) | (1 << PC6);
    PORTC |= (1 << PC3) | (1 << PC4) | (1 << PC5) | (1 << PC6);
    DDRB |= (1 << PB4);
    DDRD |= (1 << PD5);
    DDRA &= ~(1 << PA0);
    PORTA &= ~(1 << PA0);
}

void adc_init(void) {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

uint16_t adc_read(void) {
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADCW;
}

void select_key(uint8_t key_index) {
    uint8_t mux_num = key_index / 8;
    uint8_t channel = key_index % 8;

    PORTC = (PORTC & 0xF8) | (channel & 0x07);
    PORTC |= (1 << PC3) | (1 << PC4) | (1 << PC5) | (1 << PC6);

    switch (mux_num) {
        case 0: PORTC &= ~(1 << PC3); break;
        case 1: PORTC &= ~(1 << PC4); break;
        case 2: PORTC &= ~(1 << PC5); break;
        case 3: PORTC &= ~(1 << PC6); break;
    }
}

uint16_t read_capacitive_pad(uint8_t key_index) {
    select_key(key_index);

    // discharge pad
    DDRA |= (1 << PA0);
    PORTA &= ~(1 << PA0);
    _delay_us(10);

    // charge pad and sample via adc
    DDRA &= ~(1 << PA0);
    _delay_us(15);

    return adc_read();
}

void timer1_init(void) {
    TCCR1A = (1 << COM1A0);
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);
    OCR1A = 0;
}

void play_note(uint16_t ocr_value) {
    OCR1A = ocr_value;
}

void stop_sound(void) {
    OCR1A = 0;
}

int main(void) {
    system_init();
    adc_init();
    timer1_init();

    while (1) {
        uint8_t pressed_key = 25;

        for (uint8_t i = 0; i < TOTAL_KEYS; i++) {
            uint16_t adc_val = read_capacitive_pad(i);
            if (adc_val < TOUCH_THRESHOLD) {
                pressed_key = i;
                break;
            }
        }

        if (pressed_key < TOTAL_KEYS) {
            uint16_t ocr_val = pgm_read_word(&note_freqs[pressed_key]);
            play_note(ocr_val);
            PORTB |= (1 << PB4);
        } else {
            stop_sound();
            PORTB &= ~(1 << PB4);
        }

        _delay_ms(5);
    }
}