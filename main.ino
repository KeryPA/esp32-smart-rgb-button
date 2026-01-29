#include <Arduino.h>

#define LED_R      6
#define LED_G      2
#define LED_B      5
#define BTN_MODE   0

#define HOLD_INTERVAL 10
#define MIN_BRIGHT 0
#define MAX_BRIGHT 250

#define COLOR_CHANGE_INTERVAL 20000   // каждые 10 сек
#define COLOR_FADE_INTERVAL 100        // шаг плавности (мс)

#define FLOW_DURATION 3000
#define FLOW_STEP_INTERVAL 200

TaskHandle_t ledTaskHandle = NULL;

struct LedState {
  float r, g, b;
  float baseR, baseG, baseB;
  float targetR, targetG, targetB;

  int brightness;
  bool increasing;
  bool locked;

  unsigned long lastStep;
  unsigned long lastColorChange;
  unsigned long lastFadeStep;

  unsigned long flowStart;
  unsigned long lastFlowStep;
  bool inFlow;
};

LedState ledState = {
  0,0,0,
  0,0,0,
  0,0,0,
  MIN_BRIGHT,
  true,
  false,
  0,
  0,
  0,
  0,
  0,
  false
};

// ---------- функции ----------
void setColor(float r, float g, float b){
  analogWrite(LED_R, (int)r);
  analogWrite(LED_G, (int)g);
  analogWrite(LED_B, (int)b);
}

void updateBrightness(){
  ledState.r = ledState.baseR * ledState.brightness / 250.0;
  ledState.g = ledState.baseG * ledState.brightness / 250.0;
  ledState.b = ledState.baseB * ledState.brightness / 250.0;
  setColor(ledState.r, ledState.g, ledState.b);
}

void startFlow(){
  ledState.inFlow = true;
  ledState.flowStart = millis();
  ledState.lastFlowStep = 0;
  ledState.locked = true;
}

// ---------- задача ----------
void ledTask(void *pvParameters){
  while(true){
    bool pressed = (digitalRead(BTN_MODE) == LOW);
    unsigned long now = millis();

    // запуск плавной смены цвета раз в 10 секунд
    if(!ledState.inFlow && now - ledState.lastColorChange >= COLOR_CHANGE_INTERVAL){
      ledState.targetR = random(0,256);
      ledState.targetG = random(0,256);
      ledState.targetB = random(0,256);
      ledState.lastColorChange = now;
    }

    // плавное движение к целевому цвету
    if(!ledState.inFlow && now - ledState.lastFadeStep >= COLOR_FADE_INTERVAL){
      ledState.lastFadeStep = now;

      ledState.baseR += (ledState.targetR - ledState.baseR) * 0.05;
      ledState.baseG += (ledState.targetG - ledState.baseG) * 0.05;
      ledState.baseB += (ledState.targetB - ledState.baseB) * 0.05;

      updateBrightness();
    }

    // перелив
    if(ledState.inFlow){
      if(now - ledState.lastFlowStep >= FLOW_STEP_INTERVAL){
        ledState.lastFlowStep = now;
        ledState.baseR = random(0,256);
        ledState.baseG = random(0,256);
        ledState.baseB = random(0,256);
        updateBrightness();
      }

      if(now - ledState.flowStart >= FLOW_DURATION){
        ledState.inFlow = false;
        ledState.locked = false;
        ledState.increasing = !ledState.increasing;
      }
    }

    // кнопка
    else if(!ledState.locked && pressed){
      if(now - ledState.lastStep >= HOLD_INTERVAL){
        ledState.lastStep = now;

        if(ledState.increasing){
          ledState.brightness++;
          if(ledState.brightness >= MAX_BRIGHT){
            ledState.brightness = MAX_BRIGHT;
            startFlow();
          }
        } else {
          ledState.brightness--;
          if(ledState.brightness <= MIN_BRIGHT){
            ledState.brightness = MIN_BRIGHT;
            startFlow();
          }
        }
        updateBrightness();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// ---------- setup ----------
void setup(){
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  pinMode(BTN_MODE, INPUT_PULLUP);

  digitalWrite(LED_G, LOW); // гасим встроенный LED

  randomSeed(esp_random());

  ledState.baseR = random(0,256);
  ledState.baseG = random(0,256);
  ledState.baseB = random(0,256);

  ledState.targetR = ledState.baseR;
  ledState.targetG = ledState.baseG;
  ledState.targetB = ledState.baseB;

  ledState.brightness = MIN_BRIGHT;
  ledState.lastColorChange = millis();

  updateBrightness();

  xTaskCreate(ledTask, "LED Task", 4096, NULL, 1, &ledTaskHandle);
}

void loop(){
  // пусто
}
