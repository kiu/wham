#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <util/delay.h>
#include <EEPROM.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN    10

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

#define BTN_PIN 2
#define LED_PIN 3
#define MP3_PIN 4

uint8_t ts = 0;
uint8_t tm = 0;
uint8_t th = 0;
uint8_t td = 0;

char text[7];
char text_old[7];

uint8_t play_mode = 1;
uint16_t play_count = 0;

uint8_t led_idx = 0;

void wham() {
  if (play_mode == 1) {
    return;
  }
  play_mode = 1;

  ts = 0;
  tm = 0;
  th = 0;
  td = 0;

  play_count++;
  P.displayText("X X X", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  EEPROM.put(0x42, play_count);

  digitalWrite(MP3_PIN, 0);
  _delay_ms(300);
  digitalWrite(MP3_PIN, 1);
}

void tick() {
  ts++;
  if (ts == 60) {
    ts = 0;
    tm++;
    if (tm == 60) {
      tm = 0;
      th++;
      if (th == 24) {
        th = 0;
        td++;
        if (td > 99) {
          td = 99;
        }
      }
    }
  }

  if (play_mode) {
    if (ts < 10) {
      return;
    } else {
      play_mode = 0;
    }
  }

  strcpy(text_old, text);

  if (td != 0) {
    sprintf(text, "%d day", td);
  } else {
    if (th != 0) {
      sprintf(text, "%d hrs", th);
    } else {
      if (tm != 0) {
        sprintf(text, "%d min", tm);
      } else {
        sprintf(text, "%d sec", ts);
      }
    }
  }

  if (strcmp(text, text_old)) {
    P.displayText(text, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  }
}

void tock() {
  if (play_mode) {
    digitalWrite(LED_PIN, 0);
    return;
  }

  if (led_idx < 4 || (led_idx >= 20 && led_idx < 24)) {
    digitalWrite(LED_PIN, 1);
  } else {
    digitalWrite(LED_PIN, 0);
  }


  led_idx++;
  if (led_idx >= 180) {
    led_idx = 0;
  }
}

void btn() {
  for (uint8_t i = 0; i < 50; i++) {
    if (digitalRead(BTN_PIN)) {
      return;
    }
    _delay_ms(1);
  }
  wham();
}

ISR(TIMER1_COMPA_vect) {
  tick();
}

ISR(TIMER2_COMPA_vect) {
  tock();
}

void setupTimer1() { // https://www.arduinoslovakia.eu/application/timer-calculator
  noInterrupts();
  // Clear registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // 1 Hz (16000000/((15624+1)*1024))
  OCR1A = 15624;
  // CTC
  TCCR1B |= (1 << WGM12);
  // Prescaler 1024
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // Output Compare Match A Interrupt Enable
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

void setupTimer2() { // https://www.arduinoslovakia.eu/application/timer-calculator
  noInterrupts();
  // Clear registers
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;

  // 100.16025641025641 Hz (16000000/((155+1)*1024))
  OCR2A = 155;
  // CTC
  TCCR2A |= (1 << WGM21);
  // Prescaler 1024
  TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);
  // Output Compare Match A Interrupt Enable
  TIMSK2 |= (1 << OCIE2A);
  interrupts();
}


void setup(void) {
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(MP3_PIN, OUTPUT);
  digitalWrite(MP3_PIN, 1);

  EEPROM.get(0x42, play_count);

  P.begin();

  setupTimer1();
  setupTimer2();

  P.displayText("kiu", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  while (ts < 5) {
    P.displayAnimate();
  }

  sprintf(text, "%dx", play_count);
  P.displayText(text, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  while (ts < 10) {
    P.displayAnimate();
  }
}

void loop(void) {
  P.displayAnimate();
  btn();
}
