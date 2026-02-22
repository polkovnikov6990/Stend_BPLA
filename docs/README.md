# Веб-интерфейс для стенда тестирования квадрокоптера

## Описание
Веб-интерфейс для мониторинга параметров квадрокоптера на испытательном стенде в реальном времени.

## Измеряемые параметры
- 🔄 Обороты 4 двигателей (RPM)
- 🌡️ Температура 4 двигателей (°C)
- ⚖️ Тяга от 2 тензодатчиков (г)
- 📊 Вибрация по 3 осям (м/с²)
- 🔊 Уровень шума (дБ)

## Установка и запуск

### 1. Интеграция с Arduino
Для связи с Arduino используйте файлы:
- `data-mapping.json` - структура данных и маппинг переменных
- `arduino-template.ino` - шаблон кода для Arduino
- `INTEGRATION_GUIDE.md` - подробная инструкция по интеграции

### 2. Настройка Frontend
В файле `app.js` измените IP-адрес Arduino:
```javascript
const ARDUINO_IP = '192.168.1.100'; // Твой IP Arduino
```

### 3. Формат данных от Arduino
Arduino должен отправлять JSON по HTTP GET запросу на `/data`:

```json
{
  "rpm": [1200, 1250, 1180, 1220],
  "temp": [45.5, 47.2, 44.8, 46.1],
  "thrust": 850.5,
  "vibration": {
    "x": 0.15,
    "y": 0.12,
    "z": 9.85
  },
  "noise": 75.3
}
```

### 3. Пример кода для Arduino (sketch)
```cpp
#include <Ethernet.h>
#include <ArduinoJson.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 100);
EthernetServer server(80);

void setup() {
  Ethernet.begin(mac, ip);
  server.begin();
}

void loop() {
  EthernetClient client = server.available();
  
  if (client) {
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        
        if (c == '\n' && request.endsWith("\r\n\r\n")) {
          if (request.indexOf("GET /data") >= 0) {
            sendSensorData(client);
          }
          break;
        }
      }
    }
    delay(1);
    client.stop();
  }
}

void sendSensorData(EthernetClient &client) {
  // Читай данные с датчиков
  int rpm[4] = {1200, 1250, 1180, 1220};
  float temp[4] = {45.5, 47.2, 44.8, 46.1};
  float thrust = 850.5;
  float vibX = 0.15, vibY = 0.12, vibZ = 9.85;
  float noise = 75.3;
  
  // Формируй JSON
  StaticJsonDocument<512> doc;
  JsonArray rpmArray = doc.createNestedArray("rpm");
  for(int i = 0; i < 4; i++) rpmArray.add(rpm[i]);
  
  JsonArray tempArray = doc.createNestedArray("temp");
  for(int i = 0; i < 4; i++) tempArray.add(temp[i]);
  
  doc["thrust"] = thrust;
  
  JsonObject vib = doc.createNestedObject("vibration");
  vib["x"] = vibX;
  vib["y"] = vibY;
  vib["z"] = vibZ;
  
  doc["noise"] = noise;
  
  // Отправь ответ
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  serializeJson(doc, client);
}
```

### 4. Запуск веб-интерфейса
Просто открой `index.html` в браузере или используй локальный сервер:

```bash
# Python 3
python -m http.server 8000

# Или Node.js
npx http-server
```

Затем открой: `http://localhost:8000`

## Особенности
- ✅ Автоматическое переподключение при потере связи
- ✅ Индикатор статуса подключения
- ✅ Цветовая индикация перегрева двигателей
- ✅ Визуальный индикатор уровня шума
- ✅ Адаптивный дизайн для разных экранов
- ✅ Обновление данных каждые 100 мс
- ✅ Интерактивные графики в реальном времени
- ✅ Экспорт данных в CSV
- ✅ История до 100 точек на каждом графике

## Работа с графиками
- Нажми на любую карточку с параметром для открытия графика
- График обновляется в реальном времени
- Кнопка "Очистить" - сброс истории данных
- Кнопка "Экспорт" - сохранение данных в CSV файл
- Закрытие графика - крестик, клик вне окна или Escape

## Настройка
В `app.js` можно изменить:
- `ARDUINO_IP` - IP-адрес Arduino
- `UPDATE_INTERVAL` - интервал обновления (мс)

## Требования
- Браузер с поддержкой ES6+
- Arduino с Ethernet модулем (W5500)
- Библиотека ArduinoJson для Arduino
