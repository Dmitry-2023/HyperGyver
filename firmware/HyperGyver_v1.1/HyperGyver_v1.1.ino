интервал  getSoundLength () {
  интервал thisMax = 0 ;
  интервал этот сигнал = 0 ;
  для ( int я = 0 ; я < 300 ; я ++) {
    этот сигнал = аналоговое чтение (ADC_PIN);
    если (этот сигнал > этот макс) этот макс = этот сигнал;
  }

  статический  float filMax = 1000 , filMin = 1000 ;
  статическая  плавающая длина filLength;
  статический  интервал maxMax = 0 , minMin = 1023 ;
  статический счетчик байтов;

  счетчик++;
  если (счетчик == 50 ) {
    счетчик = 0 ;
    максМакс = 0 ;
    минМин = 1023 ;
  }
  если (thisMax > maxMax) maxMax = thisMax;
  если (thisMax < minMin) minMin = thisMax;

  filMax += ( float )(maxMax - filMax) * 0,01 ;
  filMin += ( float )(minMin - filMin) * 0,01 ;

  int thisLenght = карта (thisMax - filMin, VOL_THR, filMax - filMin, 0 , 100 );
  thisLenght = ограничение (thisLenght, 0 , 100 );

  filLength += ( float )(thisLenght - filLength) * 0,2 ;
  если (thisMax > filMax) filLength = 100 ;
  
  если (filMax - filMin > VOL_THR) вернуть filLength;
  иначе  вернуть  0 ;
}/*
  Скетч к проекту "Гиперкуб"
  Страница проекта (схемы, описания): https://alexgyver.ru/hypergyver/
  Исходники на GitHub: https://github.com/AlexGyver/hypergyver/
  Нравится, как написан и закомментирован код? Поддержи автора! https://alexgyver.ru/support_alex/
  Автор: AlexGyver Technologies, 2020
  http://AlexGyver.ru/
*/

// Версия 1.1: исправлен случайный глюк. Виноваты random функции из либы FastLED!!!

// ===== НАСТРОЙКИ =====
#define EDGE_LEDS 12    // кол-во диодов на ребре куба
#define LED_DI 2        // пин подключения
#define BRIGHT 255      // яркость
#define CHANGE_PRD 10   // смена режима, секунд
#define CUR_LIMIT 2000  // лимит тока в мА (0 - выкл)

// ===== ДЛЯ РАЗРАБОВ =====
const int NUM_LEDS = (EDGE_LEDS * 24);
#define USE_MICROLED 0
#include <FastLED.h>
CRGBPalette16 currentPalette;
const int FACE_SIZE = EDGE_LEDS * 4;
const int LINE_SIZE = EDGE_LEDS;
#define PAL_STEP 30
CRGB leds[NUM_LEDS];

int perlinPoint;
int curBright = BRIGHT;
bool fadeFlag = false;
bool mode = true;
bool colorMode = true;
uint16_t counter = 0;
byte speed = 15;
uint32_t tmrDraw, tmrColor, tmrFade;

#define DEBUG(x) //Serial.println(x)

uint32_t getPixColor(CRGB color) {
  return (((uint32_t)color.r << 16) | (color.g << 8) | color.b);
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2812, LED_DI, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  if (CUR_LIMIT > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, CUR_LIMIT);
  FastLED.setBrightness(BRIGHT);
  FastLED.clear();
   
  randomSeed(getEntropy(A0));   // My system's in decline, EMBRACING ENTROPY!
  perlinPoint = random(0, 32768);
  fadeFlag = true;  // сразу флаг на смену режима
}

void loop() {
  // отрисовка
  if (millis() - tmrDraw >= 40) {
    tmrDraw = millis();
    for (int i = 0; i < FACE_SIZE; i++) {
      if (mode) fillSimple(i, ColorFromPalette(currentPalette, getMaxNoise(i * PAL_STEP + counter, counter), 255, LINEARBLEND));
      else fillVertex(i, ColorFromPalette(currentPalette, getMaxNoise(i * PAL_STEP / 4 + counter, counter), 255, LINEARBLEND));
    }
    FastLED.show();
    counter += speed;
  }

  // смена режима и цвета
  if (millis() - tmrColor >= CHANGE_PRD * 1000L) {
    tmrColor = millis();
    fadeFlag = true;
  }

  // фейдер для смены через чёрный
  if (fadeFlag && millis() - tmrFade >= 30) {
    static int8_t fadeDir = -1;
    tmrFade = millis();
    if (fadeFlag) {
      curBright += 5 * fadeDir;

      if (curBright < 5) {
        curBright = 5;
        fadeDir = 1;
        changeMode();
      }
      if (curBright > BRIGHT) {
        curBright = BRIGHT;
        fadeDir = -1;
        fadeFlag = false;
      }
      FastLED.setBrightness(curBright);
    }
  }
} // луп

void changeMode() {
  if (!random(3)) mode = !mode;
  speed = random(4, 12);
  colorMode = !colorMode;
  int thisDebth = random(0, 32768);
  byte thisStep = random(2, 7) * 5;
  bool sparkles = !random(5);

  if (colorMode) {
    for (int i = 0; i < 16; i++) {
      currentPalette[i] = CHSV(getMaxNoise(thisDebth + i * thisStep, thisDebth),
                               sparkles ? (!random(9) ? 30 : 255) : 255,
                               constrain((i + 7) * (i + 7), 0, 255));
    }
  } else {
    for (int i = 0; i < 4; i++) {
      CHSV color = CHSV(random(0, 256), random(0, 256), (uint8_t)(i + 1) * 64 - 1);
      for (byte j = 0; j < 4; j++) {
        currentPalette[i * 4 + j] = color;
      }
    }
  }
}

// масштабируем шум
byte getMaxNoise(uint16_t x, uint16_t y) {
  return constrain(map(inoise8(x, y), 50, 200, 0, 255), 0, 255);
}

// заливка всех 6 граней в одинаковом порядке
void fillSimple(int num, CRGB color) {  // num 0-NUM_LEDS / 6
  for (byte i = 0; i < 6; i++) {
    leds[i * FACE_SIZE + num] = color;
  }
}

// заливка из четырёх вершин
void fillVertex(int num, CRGB color) { // num 0-NUM_LEDS / 6
  num /= 4;
  byte thisRow = 0;
  for (byte i = 0; i < 3; i++) {
    leds[LINE_SIZE * thisRow + num] = color;
    thisRow += 2;
    leds[LINE_SIZE * thisRow - num - 1] = color;
    leds[LINE_SIZE * thisRow + num] = color;
    thisRow += 2;
    leds[LINE_SIZE * thisRow - num - 1] = color;
  }
  thisRow = 13;
  for (byte i = 0; i < 3; i++) {
    leds[LINE_SIZE * thisRow - num - 1] = color;
    leds[LINE_SIZE * thisRow + num] = color;
    thisRow += 2;
    leds[LINE_SIZE * thisRow - num - 1] = color;
    leds[LINE_SIZE * thisRow + num] = color;
    thisRow += 2;
  }
}

// рандом сид из сырого аналога
uint32_t getEntropy(byte pin) {
  unsigned long seed = 0;
  for (int i = 0; i < 400; i++) {
    seed = 1;
    for (byte j = 0; j < 16; j++) {
      seed *= 4;
      seed += analogRead(pin) & 3;
    }
  }
  return seed;
}
