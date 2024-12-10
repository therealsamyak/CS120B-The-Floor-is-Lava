#pragma once

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#define NOTE_ON 128
#define NOTE_OFF 255

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

unsigned int currentNote = 0;
unsigned int currentNoteDuration = 0;

typedef struct
{
  uint16_t value;
  uint16_t duration;
} Note;

const Note song[] PROGMEM = {
    {A4, 500},
    {B4, 500},
    {C_SHARP5, 500},
    {D5, 500},
    {E5, 500},
    {F_SHARP5, 500},
    {G_SHARP5, 500},
    {A5, 500},
};

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
