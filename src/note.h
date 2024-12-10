#pragma once

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#define NOTE_ON 128
#define NOTE_OFF 255

#define REST 0
#define C3 131
#define C_SHARP3 139
#define D_FLAT3 139
#define D3 147
#define D_SHARP3 156
#define E_FLAT3 156
#define E3 165
#define F3 175
#define F_SHARP3 185
#define G_FLAT3 185
#define G3 196
#define G_SHARP3 208
#define A_FLAT3 208
#define A3 220
#define A_SHARP3 233
#define B_FLAT3 233
#define B3 247
#define C4 261
#define C_SHARP4 277
#define D_FLAT4 277
#define D4 293
#define D_SHARP4 311
#define E_FLAT4 311
#define E4 329
#define F4 349
#define F_SHARP4 370
#define G_FLAT4 370
#define G4 392
#define G_SHARP4 415
#define A_FLAT4 415
#define A4 440
#define A_SHARP4 466
#define B_FLAT4 466
#define B4 493
#define C5 523
#define C_SHARP5 554
#define D_FLAT5 554
#define D5 587
#define D_SHARP5 622
#define E_FLAT5 622
#define E5 659
#define F5 698
#define F_SHARP5 740
#define G_FLAT5 740
#define G5 784
#define G_SHARP5 830
#define A_FLAT5 830
#define A5 880
#define A_SHARP5 932
#define B_FLAT5 932
#define B5 987
#define C6 1047
#define C_SHARP6 1109
#define D_FLAT6 1109
#define D6 1175
#define D_SHARP6 1245
#define E_FLAT6 1245
#define E6 1319
#define F6 1397
#define F_SHARP6 1480
#define G_FLAT6 1480
#define G6 1568
#define G_SHARP6 1661
#define A_FLAT6 1661
#define A6 1760
#define A_SHARP6 1865
#define B_FLAT6 1865
#define B6 1976
#define C7 2093
#define C_SHARP7 2217
#define D_FLAT7 2217
#define D7 2349
#define D_SHARP7 2489
#define E_FLAT7 2489
#define E7 2637
#define F7 2794
#define F_SHARP7 2960
#define G_FLAT7 2960
#define G7 3136
#define G_SHARP7 3322
#define A_FLAT7 3322
#define A7 3520
#define A_SHARP7 3729
#define B_FLAT7 3729
#define B7 3951
#define C8 4186

#define QUARTER_NOTE 545 // 60,000 ms / 110 bpm
#define HALF_NOTE (QUARTER_NOTE * 2)
#define WHOLE_NOTE (QUARTER_NOTE * 4)
#define EIGHTH_NOTE (QUARTER_NOTE / 2)
#define SIXTEENTH_NOTE (QUARTER_NOTE / 4)
#define THIRTYSECOND_NOTE (SIXTEENTH_NOTE / 2)
#define SIXTYFOURTH_NOTE (THIRTYSECOND_NOTE / 2)
#define DOTTED_HALF_NOTE (HALF_NOTE + (HALF_NOTE / 2))
#define DOTTED_QUARTER_NOTE (QUARTER_NOTE + (QUARTER_NOTE / 2))
#define DOTTED_EIGHTH_NOTE (EIGHTH_NOTE + (EIGHTH_NOTE / 2))
#define DOUBLE_DOTTED_EIGHTH_NOTE (QUARTER_NOTE - THIRTYSECOND_NOTE)

unsigned int currentNote = 0;
unsigned int currentNoteDuration = 0;

typedef struct
{
  uint16_t value;
  uint16_t duration;
} Note;

// Jun Maeda - My Precious Treasure
const Note song[] PROGMEM = {
    {A5, QUARTER_NOTE},
    {G_SHARP5, EIGHTH_NOTE},
    {A5, QUARTER_NOTE},
    {E5, EIGHTH_NOTE},
    {D5, QUARTER_NOTE},
    {A5, QUARTER_NOTE},
    {B5, QUARTER_NOTE},
    {C_SHARP6, DOTTED_QUARTER_NOTE},
    {REST, EIGHTH_NOTE},
    {A5, QUARTER_NOTE},
    {G_SHARP5, EIGHTH_NOTE},
    {A5, QUARTER_NOTE},
    {E5, EIGHTH_NOTE},
    {D5, QUARTER_NOTE},
    {A5, QUARTER_NOTE},
    {B5, QUARTER_NOTE},
    {C_SHARP6, DOTTED_QUARTER_NOTE},
    {REST, EIGHTH_NOTE},
    {A5, QUARTER_NOTE},
    {G_SHARP5, EIGHTH_NOTE},
    {A5, QUARTER_NOTE},
    {E5, EIGHTH_NOTE},
    {D5, QUARTER_NOTE},
    {A5, QUARTER_NOTE},
    {B5, QUARTER_NOTE},
    {C_SHARP6, DOTTED_QUARTER_NOTE},
    {A5, EIGHTH_NOTE},
    {B5, QUARTER_NOTE},
    {A5, QUARTER_NOTE},
    {D5, QUARTER_NOTE},
    {C_SHARP5, QUARTER_NOTE},
    {B4, QUARTER_NOTE},
    {REST, SIXTYFOURTH_NOTE},
    {B4, QUARTER_NOTE},
    {REST, SIXTYFOURTH_NOTE},
    {A4, EIGHTH_NOTE},
    {REST, SIXTYFOURTH_NOTE},
    {A4, QUARTER_NOTE},
    {REST, EIGHTH_NOTE}};

#define NUM_NOTES (sizeof(song) / sizeof(song[0]))

void setBuzzerFrequency(uint16_t frequency)
{
  if (frequency == 0)
  {

    OCR1A = 0;
  }
  else
  {

    uint16_t top = (F_CPU / (2 * frequency)) - 1;

    ICR1 = top;

    OCR1A = top / 2;
  }
}

void buzzer_init(void)
{

  TCCR1A = (1 << COM1A1);
  TCCR1B = (1 << WGM13) | (1 << WGM12);
  TCCR1A |= (1 << WGM11);
  TCCR1B &= ~(1 << WGM10);
  OCR1A = 0;
  TCCR1B = (TCCR1B & 0xF8) | 0x02;
}

int NoteTick(int state)
{
  if (currentNoteDuration == 0)
  {
    Note current;
    memcpy_P(&current, &song[currentNote], sizeof(Note));

    currentNote++;

    if (currentNote >= NUM_NOTES)
    {
      currentNote = 0;
    }

    setBuzzerFrequency(current.value);
    currentNoteDuration = current.duration;
  }
  else
  {
    currentNoteDuration--;
  }

  return state;
}
