/*
 * Стенд тестирования квадрокоптера
 * Версия: 1.0
 * 
 * Авторы: Полковников А.В., Глазырин К.С.
 * Кафедра робототехники и противодействия робототехническим комплексам (системам)
 * 
 * ВАЖНО: Настройки берутся из config/config.json
 * Измените параметры в конфигурационном файле перед компиляцией
 */

#include <Ethernet.h>
#include <ArduinoJson.h>
#include <HX711.h>
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>

// ============================================
// КОНФИГУРАЦИЯ (из config.json)
// ============================================

// Сетевые настройки
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(80);

// Пины для датчиков оборотов (TCRT5000)
const int RPM_PIN_1 = 2;
const int RPM_PIN_2 = 3;
const int RPM_PIN_3 = 4;
const int RPM_PIN_4 = 5;

// Пины для термисторов (NTC 100k)
const int TEMP_PIN_1 = A0;
const int TEMP_PIN_2 = A1;
const int TEMP_PIN_3 = A2;
const int TEMP_PIN_4 = A3;

// Пины для тензодатчиков (HX711)
const int HX711_1_DOUT = 6;
const int HX711_1_SCK = 7;
const int HX711_2_DOUT = 8;
const int HX711_2_SCK = 9;

// Пин для микрофона (MAX9814)
const int MIC_PIN = A6;

// Калибровочные параметры термисторов
const float R_SERIES = 10000.0;
const float R_NOMINAL = 100000.0;
const float T_NOMINAL = 25.0;
const float B_COEFFICIENT = 3950.0;

// Калибровочные параметры тензодатчиков
const float SCALE_1 = 420.0;
const float SCALE_2 = 420.0;

// ============================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================

// Объекты датчиков
HX711 loadCell1;
HX711 loadCell2;
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Данные датчиков
volatile unsigned long rpmPulseCount[4] = {0, 0, 0, 0};
unsigned long lastRpmTime[4] = {0, 0, 0, 0};
int currentRPM[4] = {0, 0, 0, 0};

float motorTemps[4] = {0, 0, 0, 0};
float loadCellValues[2] = {0, 0};
float accelValues[3] = {0, 0, 0};
int noiseLevel = 0;

unsigned long lastUpdateTime = 0;
const unsigned long UPDATE_INTERVAL = 100; // мс

// ============================================
// ПРЕРЫВАНИЯ ДЛЯ ПОДСЧЕТА ОБОРОТОВ
// ============================================

void rpmISR1() { rpmPulseCount[0]++; }
void rpmISR2() { rpmPulseCount[1]++; }
void rpmISR3() { rpmPulseCount[2]++; }
void rpmISR4() { rpmPulseCount[3]++; }

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  Serial.println(F("==========================================="));
  Serial.println(F("Стенд тестирования квадрокоптера v1.0"));
  Serial.println(F("==========================================="));
  
  // Инициализация Ethernet
  Serial.print(F("Инициализация Ethernet..."));
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  Serial.println(F(" OK"));
  Serial.print(F("IP адрес: "));
  Serial.println(Ethernet.localIP());
  
  // Настройка пинов для датчиков оборотов
  pinMode(RPM_PIN_1, INPUT_PULLUP);
  pinMode(RPM_PIN_2, INPUT_PULLUP);
  pinMode(RPM_PIN_3, INPUT_PULLUP);
  pinMode(RPM_PIN_4, INPUT_PULLUP);
  
  // Прерывания для подсчета оборотов
  attachInterrupt(digitalPinToInterrupt(RPM_PIN_1), rpmISR1, FALLING);
  attachInterrupt(digitalPinToInterrupt(RPM_PIN_2), rpmISR2, FALLING);
  attachInterrupt(digitalPinToInterrupt(RPM_PIN_3), rpmISR3, FALLING);
  attachInterrupt(digitalPinToInterrupt(RPM_PIN_4), rpmISR4, FALLING);
  
  // Инициализация тензодатчиков
  Serial.print(F("Инициализация HX711..."));
  loadCell1.begin(HX711_1_DOUT, HX711_1_SCK);
  loadCell2.begin(HX711_2_DOUT, HX711_2_SCK);
  loadCell1.set_scale(SCALE_1);
  loadCell2.set_scale(SCALE_2);
  loadCell1.tare();
  loadCell2.tare();
  Serial.println(F(" OK"));
  
  // Инициализация акселерометра
  Serial.print(F("Инициализация ADXL345..."));
  if (accel.begin()) {
    accel.setRange(ADXL345_RANGE_2_G);
    Serial.println(F(" OK"));
  } else {
    Serial.println(F(" ОШИБКА!"));
  }
  
  Serial.println(F("Инициализация завершена"));
  Serial.println(F("==========================================="));
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  unsigned long currentTime = millis();
  
  // Обновление данных датчиков
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
    lastUpdateTime = currentTime;
    readAllSensors();
  }
  
  // Обработка HTTP запросов
  handleHTTPRequests();
}

// ============================================
// ЧТЕНИЕ ДАТЧИКОВ
// ============================================

void readAllSensors() {
  // Обороты двигателей
  for (int i = 0; i < 4; i++) {
    unsigned long currentTime = millis();
    unsigned long deltaTime = currentTime - lastRpmTime[i];
    
    if (deltaTime >= 1000) { // Обновляем каждую секунду
      noInterrupts();
      unsigned long pulses = rpmPulseCount[i];
      rpmPulseCount[i] = 0;
      interrupts();
      
      currentRPM[i] = (pulses * 60000) / deltaTime;
      lastRpmTime[i] = currentTime;
    }
  }
  
  // Температура двигателей
  motorTemps[0] = readTemperature(TEMP_PIN_1);
  motorTemps[1] = readTemperature(TEMP_PIN_2);
  motorTemps[2] = readTemperature(TEMP_PIN_3);
  motorTemps[3] = readTemperature(TEMP_PIN_4);
  
  // Тензодатчики
  if (loadCell1.is_ready()) {
    loadCellValues[0] = loadCell1.get_units(1);
  }
  if (loadCell2.is_ready()) {
    loadCellValues[1] = loadCell2.get_units(1);
  }
  
  // Акселерометр
  sensors_event_t event;
  accel.getEvent(&event);
  accelValues[0] = event.acceleration.x;
  accelValues[1] = event.acceleration.y;
  accelValues[2] = event.acceleration.z;
  
  // Уровень шума
  noiseLevel = analogRead(MIC_PIN);
}

// Чтение температуры с термистора
float readTemperature(int pin) {
  int rawValue = analogRead(pin);
  
  if (rawValue == 0 || rawValue >= 1023) {
    return -999.0; // Ошибка датчика
  }
  
  // Преобразование в сопротивление
  float resistance = R_SERIES / ((1023.0 / rawValue) - 1.0);
  
  // Уравнение Стейнхарта-Харта (упрощенное)
  float steinhart;
  steinhart = resistance / R_NOMINAL;
  steinhart = log(steinhart);
  steinhart /= B_COEFFICIENT;
  steinhart += 1.0 / (T_NOMINAL + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;
  
  return steinhart;
}

// ============================================
// ОБРАБОТКА HTTP ЗАПРОСОВ
// ============================================

void handleHTTPRequests() {
  EthernetClient client = server.available();
  
  if (client) {
    String request = "";
    boolean currentLineIsBlank = true;
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        
        if (c == '\n' && currentLineIsBlank) {
          if (request.indexOf("GET /data") >= 0) {
            sendSensorData(client);
          } else if (request.indexOf("GET /") >= 0) {
            sendHomePage(client);
          } else {
            client.println("HTTP/1.1 404 Not Found");
            client.println("Connection: close");
            client.println();
          }
          break;
        }
        
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    
    delay(1);
    client.stop();
  }
}

// Отправка данных в JSON формате
void sendSensorData(EthernetClient &client) {
  StaticJsonDocument<512> doc;
  
  // Обороты
  JsonArray rpm = doc.createNestedArray("rpm");
  for (int i = 0; i < 4; i++) {
    rpm.add(currentRPM[i]);
  }
  
  // Температура
  JsonArray temp = doc.createNestedArray("temp");
  for (int i = 0; i < 4; i++) {
    temp.add(motorTemps[i]);
  }
  
  // Тяга (сумма в граммах)
  float totalThrust = (loadCellValues[0] + loadCellValues[1]) * 1000;
  doc["thrust"] = totalThrust;
  
  // Вибрация
  JsonObject vibration = doc.createNestedObject("vibration");
  vibration["x"] = accelValues[0];
  vibration["y"] = accelValues[1];
  vibration["z"] = accelValues[2];
  
  // Шум (преобразуем в дБ)
  float noiseDB = 40.0 + (noiseLevel / 1023.0) * 60.0;
  doc["noise"] = noiseDB;
  
  // HTTP заголовки
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  
  // JSON данные
  serializeJson(doc, client);
  
  // Отладка
  Serial.print(F("["));
  Serial.print(millis());
  Serial.print(F("] Data sent: "));
  serializeJson(doc, Serial);
  Serial.println();
}

// Отправка домашней страницы
void sendHomePage(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  
  client.println("<!DOCTYPE html>");
  client.println("<html><head><meta charset='utf-8'>");
  client.println("<title>Стенд БПЛА</title></head><body>");
  client.println("<h1>Стенд тестирования квадрокоптера</h1>");
  client.println("<p>Система работает</p>");
  client.println("<p><a href='/data'>Получить данные (JSON)</a></p>");
  client.println("</body></html>");
}
