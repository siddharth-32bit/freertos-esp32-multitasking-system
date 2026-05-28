#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -------- Pins --------
#define YELLOW_LED 23
#define GREEN_LED  19
#define BLUE_LED   18

#define BUTTON_PIN 4        // State button
#define MODE_BUTTON_PIN 14  // Mode toggle button

#define BUZZER_PIN 5

#define DHTPIN 15
#define DHTTYPE DHT11

// -------- OLED --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -------- Objects --------
DHT dht(DHTPIN, DHTTYPE);

// -------- FreeRTOS Handles --------
SemaphoreHandle_t buttonSemaphore;
SemaphoreHandle_t modeSemaphore;
QueueHandle_t sensorQueue;

// -------- System State --------
int state = 0;
bool autoMode = true;

// -------- Sensor Struct --------
typedef struct {
  int temp;
  int hum;
} SensorData;

SensorData latestData;

// ================= LED + BUZZER TASK =================
void ledTask(void *pvParameters) {
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  while (1) {

    // Toggle AUTO / MANUAL
    if (xSemaphoreTake(modeSemaphore, 0) == pdTRUE) {
      autoMode = !autoMode;
    }

    // Manual control (only in MANUAL mode)
    if (!autoMode && xSemaphoreTake(buttonSemaphore, 0) == pdTRUE) {
      state = (state + 1) % 3;
    }

    // Automatic control (only in AUTO mode)
    if (autoMode) {
      if (latestData.temp > 35) state = 2;
      else if (latestData.temp > 30) state = 1;
      else state = 0;
    }

    // -------- STATE BEHAVIOR --------

    // 🟢 NORMAL
    if (state == 0) {
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(YELLOW_LED, HIGH);
      digitalWrite(BLUE_LED, HIGH);
      digitalWrite(BUZZER_PIN, LOW);

      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // 🟡 WARNING (slow blink + slow beep)
    else if (state == 1) {
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(BLUE_LED, HIGH);

      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(BUZZER_PIN, HIGH);
      vTaskDelay(400 / portTICK_PERIOD_MS);

      digitalWrite(YELLOW_LED, HIGH);
      digitalWrite(BUZZER_PIN, LOW);
      vTaskDelay(600 / portTICK_PERIOD_MS);
    }

    // 🔵 ALERT (fast blink + fast beep)
    else if (state == 2) {
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(YELLOW_LED, HIGH);

      digitalWrite(BLUE_LED, LOW);
      digitalWrite(BUZZER_PIN, HIGH);
      vTaskDelay(150 / portTICK_PERIOD_MS);

      digitalWrite(BLUE_LED, HIGH);
      digitalWrite(BUZZER_PIN, LOW);
      vTaskDelay(150 / portTICK_PERIOD_MS);
    }
  }
}

// ================= BUTTON TASK (STATE) =================
void buttonTask(void *pvParameters) {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  while (1) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      xSemaphoreGive(buttonSemaphore);
      vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// ================= MODE BUTTON TASK =================
void modeButtonTask(void *pvParameters) {
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);

  while (1) {
    if (digitalRead(MODE_BUTTON_PIN) == LOW) {
      xSemaphoreGive(modeSemaphore);
      vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// ================= SENSOR TASK =================
void sensorTask(void *pvParameters) {
  SensorData data;
  dht.begin();

  while (1) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      data.temp = (int)t;
      data.hum  = (int)h;

      latestData = data;
      xQueueSend(sensorQueue, &data, portMAX_DELAY);
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

// ================= SERIAL TASK =================
void serialTask(void *pvParameters) {
  SensorData received;

  while (1) {
    if (xQueueReceive(sensorQueue, &received, portMAX_DELAY)) {
      Serial.println("Mode: " + String(autoMode ? "AUTO" : "MANUAL") +
                     " | State: " + String(state) +
                     " | Temp: " + String(received.temp) +
                     " | Hum: " + String(received.hum));
    }
  }
}

// ================= OLED TASK =================
void oledTask(void *pvParameters) {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  while (1) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(0, 0);

    if (state == 0) display.println("STATE: NORMAL");
    else if (state == 1) display.println("STATE: WARNING");
    else if (state == 2) display.println("STATE: ALERT");

    display.println("--------------------");
    display.println("Temp: " + String(latestData.temp) + " C");
    display.println("Hum : " + String(latestData.hum) + " %");

    display.println("--------------------");
    display.println("Mode: " + String(autoMode ? "AUTO" : "MANUAL"));

    display.display();

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  buttonSemaphore = xSemaphoreCreateBinary();
  modeSemaphore   = xSemaphoreCreateBinary();
  sensorQueue     = xQueueCreate(5, sizeof(SensorData));

// All tasks currently run at same priority

  xTaskCreate(ledTask, "LED Task", 2048, NULL, 1, NULL);
  xTaskCreate(buttonTask, "Button Task", 2048, NULL, 1, NULL);
  xTaskCreate(modeButtonTask, "Mode Button Task", 2048, NULL, 1, NULL);
  xTaskCreate(sensorTask, "Sensor Task", 4096, NULL, 1, NULL);
  xTaskCreate(serialTask, "Serial Task", 2048, NULL, 1, NULL);
  xTaskCreate(oledTask, "OLED Task", 4096, NULL, 1, NULL);
}

// ================= LOOP =================
void loop() {}