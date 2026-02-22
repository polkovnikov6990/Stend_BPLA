/*
 * ==============================================================================
 * ПРОШИВКА ДЛЯ ARDUINO NANO v2.3 - Стенд тестирования квадрокоптера
 * ==============================================================================
 * 
 * Авторы: Полковников А.В., Глазырин К.С.
 * Кафедра робототехники и противодействия робототехническим комплексам (системам)
 * 
 * Версия: 2.3
 * Дата: 2024
 * 
 * ВАЖНО: Настройки берутся из config/config.json
 * 
 * ==============================================================================
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Wire.h>
#include <HX711.h>

/*
 * ==============================================================================
 * ГЛОБАЛЬНЫЕ ОБЪЕКТЫ
 * ==============================================================================
 */
EthernetUDP Udp;

/*
 * ==============================================================================
 * PROGMEM КОНСТАНТЫ (экономия RAM)
 * ==============================================================================
 */
#include <avr/pgmspace.h>

// MAC адрес и сетевые настройки (из config.json)
const PROGMEM byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01};
IPAddress ip(192, 168, 0, 177);
IPAddress broadcastIp(192, 168, 0, 255);
const unsigned int localPort = 8888;

/*
 * ==============================================================================
 * СТРУКТУРА ДАННЫХ
 * ==============================================================================
 */
struct __attribute__((packed)) SensorData {
  uint16_t deviceId;
  uint32_t timestamp;
  int16_t motorTemp[4];      // Температура * 100
  int16_t loadCell1;         // Нагрузка * 100
  int16_t loadCell2;         // Нагрузка * 100
  uint16_t rpm[4];           // Обороты в минуту
  int16_t accel[3];          // Акселерометр (сырые данные)
  uint8_t soundLevel;        // Уровень звука (0-255)
} sensorData;

/*
 * ==============================================================================
 * ПИНЫ (из config.json)
 * ==============================================================================
 */
// Термисторы NTC
const int NTC_PINS[] = {A0, A1, A2, A3};

// Тензодатчики HX711
const int LOADCELL1_DT = 2;
const int LOADCELL1_SCK = 3;
const int LOADCELL2_DT = 4;
const int LOADCELL2_SCK = 5;

// Датчики оборотов TCRT5000
const int TCRT_PINS[] = {6, 7, 8, 9};

// Микрофон
const int MIC_PIN = A6;

// Акселерометр ADXL345
const int ADXL345_ADDRESS = 0x53;

/*
 * ==============================================================================
 * ОБЪЕКТЫ ДАТЧИКОВ
 * ==============================================================================
 */
HX711 loadCell1;
HX711 loadCell2;

/*
 * ==============================================================================
 * ПЕРЕМЕННЫЕ ДЛЯ ОБОРОТОВ
 * ==============================================================================
 */
// Счетчики импульсов (для прерываний)
volatile byte pulseCount[4] = {0, 0, 0, 0};

// Время последнего обновления RPM
unsigned long lastRpmTime = 0;

// Последнее состояние для моторов 3-4 (polling)
byte lastState[4] = {LOW, LOW, LOW, LOW};

/*
 * ==============================================================================
 * ФЛАГИ ДОСТУПНОСТИ ДАТЧИКОВ
 * ==============================================================================
 */
bool adxl345_available = false;
bool ethernet_available = false;
bool hx711_1_available = false;
bool hx711_2_available = false;

/*
 * ==============================================================================
 * ТАЙМИНГИ
 * ==============================================================================
 */
unsigned long lastUartTime = 0;
unsigned long lastUdpTime = 0;
const unsigned long UART_INTERVAL = 200;  // мс
const unsigned long UDP_INTERVAL = 200;   // мс

/*
 * ==============================================================================
 * БУФЕР ДЛЯ JSON
 * ==============================================================================
 */
char jsonBuffer[200];

/*
 * ==============================================================================
 * СТРОКОВЫЕ КОНСТАНТЫ В PROGMEM
 * ==============================================================================
 */
const char STR_STARTUP[] PROGMEM = "{\"status\":\"STARTUP\"}";
const char STR_CHECK_ADXL[] PROGMEM = "{\"status\":\"CHECK\",\"sensor\":\"ADXL345\"}";
const char STR_OK_ADXL[] PROGMEM = "{\"status\":\"OK\",\"sensor\":\"ADXL345\"}";
const char STR_ERROR_ADXL[] PROGMEM = "{\"status\":\"ERROR\",\"sensor\":\"ADXL345\"}";
const char STR_CHECK_HX711_1[] PROGMEM = "{\"status\":\"CHECK\",\"sensor\":\"HX711_1\"}";
const char STR_CHECK_HX711_2[] PROGMEM = "{\"status\":\"CHECK\",\"sensor\":\"HX711_2\"}";
const char STR_OK_HX711[] PROGMEM = "{\"status\":\"OK\",\"sensor\":\"HX711\"}";
const char STR_ERROR_HX711[] PROGMEM = "{\"status\":\"ERROR\",\"sensor\":\"HX711\"}";
const char STR_CHECK_ETH[] PROGMEM = "{\"status\":\"CHECK\",\"sensor\":\"Ethernet\"}";
const char STR_OK_ETH[] PROGMEM = "{\"status\":\"OK\",\"sensor\":\"Ethernet\"}";
const char STR_ERROR_ETH[] PROGMEM = "{\"status\":\"ERROR\",\"sensor\":\"Ethernet\"}";
const char STR_READY[] PROGMEM = "{\"status\":\"READY\"}";

/*
 * ==============================================================================
 * ПРЕРЫВАНИЯ ДЛЯ ПОДСЧЕТА ОБОРОТОВ (моторы 1-2)
 * ==============================================================================
 */
void countPulse0() { 
  if (pulseCount[0] < 255) pulseCount[0]++; 
}

void countPulse1() { 
  if (pulseCount[1] < 255) pulseCount[1]++; 
}

/*
 * ==============================================================================
 * ИНИЦИАЛИЗАЦИЯ ADXL345
 * ==============================================================================
 */
void initADXL345() {
  Wire.begin();
  Wire.setClock(100000);
  delay(10);
  
  // Проверка наличия датчика
  Wire.beginTransmission(ADXL345_ADDRESS);
  uint8_t error = Wire.endTransmission();
  
  if (error != 0) {
    adxl345_available = false;
    return;
  }
  
  // Включение измерений
  Wire.beginTransmission(ADXL345_ADDRESS);
  Wire.write(0x2D);  // POWER_CTL register
  Wire.write(0x08);  // Measure mode
  if (Wire.endTransmission() != 0) {
    adxl345_available = false;
    return;
  }
  
  delay(10);
  
  // Установка диапазона ±2g
  Wire.beginTransmission(ADXL345_ADDRESS);
  Wire.write(0x31);  // DATA_FORMAT register
  Wire.write(0x00);  // ±2g range
  if (Wire.endTransmission() != 0) {
    adxl345_available = false;
    return;
  }
  
  adxl345_available = true;
}

/*
 * ==============================================================================
 * ЧТЕНИЕ ADXL345
 * ==============================================================================
 */
void readADXL345() {
  if (!adxl345_available) {
    sensorData.accel[0] = 0;
    sensorData.accel[1] = 0;
    sensorData.accel[2] = 0;
    return;
  }
  
  Wire.beginTransmission(ADXL345_ADDRESS);
  Wire.write(0x32);  // Начало регистров данных
  if (Wire.endTransmission(false) != 0) {
    adxl345_available = false;
    return;
  }
  
  Wire.requestFrom(ADXL345_ADDRESS, 6);
  
  // Таймаут на чтение
  unsigned long timeout = millis() + 5;
  while (Wire.available() < 6) {
    if (millis() > timeout) {
      adxl345_available = false;
      return;
    }
  }
  
  if (Wire.available() >= 6) {
    int16_t X = Wire.read() | (Wire.read() << 8);
    int16_t Y = Wire.read() | (Wire.read() << 8);
    int16_t Z = Wire.read() | (Wire.read() << 8);
    
    sensorData.accel[0] = X;
    sensorData.accel[1] = Y;
    sensorData.accel[2] = Z;
  }
}

/*
 * ==============================================================================
 * ЧТЕНИЕ ТЕРМИСТОРОВ NTC
 * ==============================================================================
 * Использует уравнение Стейнхарта-Харта для расчета температуры
 */
void readNTC() {
  for (int i = 0; i < 4; i++) {
    int raw = analogRead(NTC_PINS[i]);
    
    // Проверка на валидность показаний
    if (raw > 10 && raw < 1013) {
      // Расчет сопротивления термистора
      float resistance = 100000.0 / ((1023.0 / raw) - 1.0);
      
      if (resistance > 1000 && resistance < 1000000) {
        // Уравнение Стейнхарта-Харта
        float logR = log(resistance);
        float temp = (1.0 / (0.001129148 + (0.000234125 * logR) + 
                     (0.0000000876741 * logR * logR * logR))) - 273.15;
        
        // Сохраняем температуру * 100
        sensorData.motorTemp[i] = (int16_t)(temp * 100);
      } else {
        sensorData.motorTemp[i] = -9999;  // Ошибка датчика
      }
    } else {
      sensorData.motorTemp[i] = -9999;  // Ошибка датчика
    }
  }
}

/*
 * ==============================================================================
 * ЧТЕНИЕ ТЕНЗОДАТЧИКОВ HX711
 * ==============================================================================
 */
void readHX711() {
  if (hx711_1_available && loadCell1.is_ready()) {
    sensorData.loadCell1 = (int16_t)(loadCell1.get_units(1) * 100);
  }
  
  if (hx711_2_available && loadCell2.is_ready()) {
    sensorData.loadCell2 = (int16_t)(loadCell2.get_units(1) * 100);
  }
}

/*
 * ==============================================================================
 * ОБНОВЛЕНИЕ ОБОРОТОВ
 * ==============================================================================
 * Моторы 1-2: используют прерывания
 * Моторы 3-4: используют polling (опрос состояния)
 */
void updateRPM() {
  unsigned long now = millis();
  
  if (now - lastRpmTime >= 1000) {
    // Моторы 1-2: подсчет по прерываниям
    for (int i = 0; i < 2; i++) {
      sensorData.rpm[i] = pulseCount[i] * 60;
      pulseCount[i] = 0;
    }
    
    // Моторы 3-4: подсчет по изменению состояния
    for (int i = 2; i < 4; i++) {
      int currentState = digitalRead(TCRT_PINS[i]);
      
      if (lastState[i] == HIGH && currentState == LOW) {
        pulseCount[i]++;
      }
      
      lastState[i] = currentState;
      sensorData.rpm[i] = pulseCount[i] * 60;
      pulseCount[i] = 0;
    }
    
    lastRpmTime = now;
  }
}

/*
 * ==============================================================================
 * ПОСТРОЕНИЕ JSON
 * ==============================================================================
 * Формат: {"t":time, "d":deviceId, "m":[...], "r":[...], "l":[...], "a":[...], "s":value}
 */
void buildJSON() {
  char* p = jsonBuffer;
  
  // Начало JSON
  p += sprintf_P(p, PSTR("{\"t\":%lu,\"d\":%u,\"m\":["), 
                 millis(), sensorData.deviceId);
  
  // Температуры моторов
  for (int i = 0; i < 4; i++) {
    if (i > 0) *p++ = ',';
    
    if (sensorData.motorTemp[i] < -9000) {
      p += sprintf_P(p, PSTR("null"));
    } else {
      p += sprintf(p, "%d", sensorData.motorTemp[i]);
    }
  }
  
  // Обороты
  p += sprintf_P(p, PSTR("],\"r\":["));
  for (int i = 0; i < 4; i++) {
    if (i > 0) *p++ = ',';
    p += sprintf(p, "%d", sensorData.rpm[i]);
  }
  
  // Тензодатчики, акселерометр, звук
  p += sprintf_P(p, PSTR("],\"l\":[%d,%d],\"a\":[%d,%d,%d],\"s\":%d}"),
                 sensorData.loadCell1,
                 sensorData.loadCell2,
                 sensorData.accel[0],
                 sensorData.accel[1],
                 sensorData.accel[2],
                 sensorData.soundLevel);
}

/*
 * ==============================================================================
 * ОТПРАВКА ДАННЫХ ЧЕРЕЗ UART
 * ==============================================================================
 */
void sendUARTData() {
  buildJSON();
  Serial.println(jsonBuffer);
}

/*
 * ==============================================================================
 * ОТПРАВКА ДАННЫХ ЧЕРЕЗ UDP BROADCAST
 * ==============================================================================
 */
void sendUDPBroadcast() {
  if (!ethernet_available) return;
  
  buildJSON();
  
  Udp.beginPacket(broadcastIp, localPort);
  Udp.print(jsonBuffer);
  Udp.endPacket();
}

/*
 * ==============================================================================
 * ИНИЦИАЛИЗАЦИЯ ETHERNET
 * ==============================================================================
 */
void initEthernet() {
  byte macBuffer[6];
  memcpy_P(macBuffer, mac, 6);
  
  Ethernet.init(10);  // CS pin
  
  // Инициализация Ethernet
  Ethernet.begin(macBuffer, ip);
  
  // Даем время на инициализацию
  delay(1000);
  
  // Проверка наличия шилда
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    strcpy_P(jsonBuffer, STR_ERROR_ETH);
    Serial.println(jsonBuffer);
    ethernet_available = false;
    return;
  }
  
  // Проверка статуса линка
  EthernetLinkStatus link = Ethernet.linkStatus();
  
  if (link == LinkON) {
    ethernet_available = true;
    Udp.begin(localPort);
    strcpy_P(jsonBuffer, STR_OK_ETH);
    Serial.println(jsonBuffer);
  } else {
    // Даже если кабель не подключен, UDP всё равно может работать
    ethernet_available = true;
    Udp.begin(localPort);
    strcpy_P(jsonBuffer, STR_OK_ETH);
    Serial.println(jsonBuffer);
    Serial.println("{\"warning\":\"Ethernet cable not connected\"}");
  }
}

/*
 * ==============================================================================
 * SETUP
 * ==============================================================================
 */
void setup() {
  // Инициализация Serial
  Serial.begin(115200);
  delay(100);
  
  strcpy_P(jsonBuffer, STR_STARTUP);
  Serial.println(jsonBuffer);
  
  // Настройка пинов для датчиков оборотов
  for (int i = 0; i < 4; i++) {
    pinMode(TCRT_PINS[i], INPUT_PULLUP);
  }
  
  // ID устройства
  sensorData.deviceId = 1;
  
  // ============================================================================
  // ИНИЦИАЛИЗАЦИЯ ADXL345
  // ============================================================================
  strcpy_P(jsonBuffer, STR_CHECK_ADXL);
  Serial.println(jsonBuffer);
  
  initADXL345();
  
  if (adxl345_available) {
    strcpy_P(jsonBuffer, STR_OK_ADXL);
  } else {
    strcpy_P(jsonBuffer, STR_ERROR_ADXL);
  }
  Serial.println(jsonBuffer);
  
  // ============================================================================
  // ИНИЦИАЛИЗАЦИЯ HX711 #1
  // ============================================================================
  strcpy_P(jsonBuffer, STR_CHECK_HX711_1);
  Serial.println(jsonBuffer);
  
  loadCell1.begin(LOADCELL1_DT, LOADCELL1_SCK);
  delay(10);
  
  if (loadCell1.is_ready()) {
    hx711_1_available = true;
    loadCell1.set_scale(1000.0);
    loadCell1.tare();
    strcpy_P(jsonBuffer, STR_OK_HX711);
  } else {
    strcpy_P(jsonBuffer, STR_ERROR_HX711);
  }
  Serial.println(jsonBuffer);
  
  // ============================================================================
  // ИНИЦИАЛИЗАЦИЯ HX711 #2
  // ============================================================================
  strcpy_P(jsonBuffer, STR_CHECK_HX711_2);
  Serial.println(jsonBuffer);
  
  loadCell2.begin(LOADCELL2_DT, LOADCELL2_SCK);
  delay(10);
  
  if (loadCell2.is_ready()) {
    hx711_2_available = true;
    loadCell2.set_scale(1000.0);
    loadCell2.tare();
    strcpy_P(jsonBuffer, STR_OK_HX711);
  } else {
    strcpy_P(jsonBuffer, STR_ERROR_HX711);
  }
  Serial.println(jsonBuffer);
  
  // ============================================================================
  // ИНИЦИАЛИЗАЦИЯ ETHERNET
  // ============================================================================
  strcpy_P(jsonBuffer, STR_CHECK_ETH);
  Serial.println(jsonBuffer);
  
  initEthernet();
  
  // ============================================================================
  // НАСТРОЙКА ПРЕРЫВАНИЙ (моторы 1-2)
  // ============================================================================
  attachInterrupt(digitalPinToInterrupt(TCRT_PINS[0]), countPulse0, FALLING);
  attachInterrupt(digitalPinToInterrupt(TCRT_PINS[1]), countPulse1, FALLING);
  
  // Готово
  strcpy_P(jsonBuffer, STR_READY);
  Serial.println(jsonBuffer);
}

/*
 * ==============================================================================
 * MAIN LOOP
 * ==============================================================================
 */
void loop() {
  unsigned long now = millis();
  static unsigned long lastRead = 0;
  
  // Чтение датчиков каждые 50 мс
  if (now - lastRead > 50) {
    lastRead = now;
    
    readNTC();
    readADXL345();
    sensorData.soundLevel = analogRead(MIC_PIN) >> 2;  // Сдвиг на 2 бита (0-255)
    readHX711();
  }
  
  // Обновление оборотов
  updateRPM();
  
  // Отправка через UART
  if (now - lastUartTime >= UART_INTERVAL) {
    sendUARTData();
    lastUartTime = now;
  }
  
  // Отправка через UDP
  if (now - lastUdpTime >= UDP_INTERVAL) {
    sendUDPBroadcast();
    lastUdpTime = now;
  }
  
  delay(10);
}
