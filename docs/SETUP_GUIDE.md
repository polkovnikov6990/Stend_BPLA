# 🚀 Инструкция по запуску системы

## Архитектура системы

```
Arduino Nano (датчики)
    ↓ UART/Ethernet
Python Backend (arduino_monitor.py)
    ↓ HTTP API
Веб-интерфейс (index.html)
```

## Вариант 1: Через Python Backend (Рекомендуется)

### Шаг 1: Подготовка Python окружения

```bash
# Установка зависимостей
pip install pyserial matplotlib numpy netifaces
```

### Шаг 2: Интеграция веб-сервера

Есть два способа:

#### Способ А: Использовать готовый скрипт

1. Положите файлы рядом:
   - Ваш файл с Arduino Monitor (назовите `arduino_monitor.py`)
   - `web_server.py`
   - `monitor_with_web.py`

2. Запустите:
```bash
python monitor_with_web.py
```

#### Способ Б: Добавить в существующий код

Добавьте в конец вашего файла с Arduino Monitor:

```python
# В конце файла, перед if __name__ == "__main__":
from web_server import WebServer

# В функции main(), после создания monitor:
def main():
    monitor = ArduinoMonitor()
    
    # Добавьте эти строки:
    web_server = WebServer(monitor, host='0.0.0.0', port=8080)
    web_server.start()
    
    gui = MonitorGUI(monitor)
    # ... остальной код
```

### Шаг 3: Настройка веб-интерфейса

В файле `app.js` измените:

```javascript
const ARDUINO_IP = 'localhost:8080'; // Если на том же компьютере
// или
const ARDUINO_IP = '192.168.0.XXX:8080'; // IP компьютера с Python
```

### Шаг 4: Запуск

1. Запустите Python скрипт
2. Подключите UART или запустите UDP слушатель
3. Запустите веб-сервер (кнопка в GUI или автоматически)
4. Откройте `index.html` в браузере

---

## Вариант 2: Прямое подключение к Arduino

Если хотите подключаться напрямую к Arduino без Python:

### Шаг 1: Загрузите код на Arduino

Используйте `arduino-template.ino` и адаптируйте под свои датчики.

### Шаг 2: Настройте веб-интерфейс

В `app.js`:
```javascript
const ARDUINO_IP = '192.168.0.177'; // IP вашего Arduino
```

### Шаг 3: Откройте веб-интерфейс

Просто откройте `index.html` в браузере.

---

## Проверка работы

### Тест 1: Проверка Python сервера

Откройте в браузере:
```
http://localhost:8080/ping
```

Должен вернуть:
```json
{"status": "ok", "timestamp": 1234567890.123}
```

### Тест 2: Проверка данных

```
http://localhost:8080/data
```

Должен вернуть JSON с данными датчиков.

### Тест 3: Проверка веб-интерфейса

1. Откройте `index.html`
2. Индикатор подключения должен стать зеленым
3. Данные должны обновляться

---

## Маппинг данных

### Из Python в веб-интерфейс:

| Python (last_data) | Web API | Описание |
|-------------------|---------|----------|
| `rpms[0-3]` | `rpm[0-3]` | Обороты двигателей |
| `motors[0-3]` | `temp[0-3]` | Температура (°C) |
| `loadCells[0] + loadCells[1]` | `thrust` | Тяга (г) |
| `accel[0-2]` | `vibration.x/y/z` | Вибрация (g) |
| `sound` | `noise` | Шум (дБ) |

### Преобразования в web_server.py:

```python
# Обороты - прямая передача
"rpm": [int(rpms[0]), int(rpms[1]), int(rpms[2]), int(rpms[3])]

# Температура - прямая передача (уже в °C)
"temp": [float(motors[0]), float(motors[1]), float(motors[2]), float(motors[3])]

# Тяга - сумма тензодатчиков, кг → граммы
"thrust": (loadCells[0] + loadCells[1]) * 1000

# Вибрация - прямая передача (уже в g)
"vibration": {"x": accel[0], "y": accel[1], "z": accel[2]}

# Шум - преобразование 0-255 → 40-100 дБ
"noise": 40 + (sound / 255.0) * 60
```

---

## Отладка

### Проблема: "Нет связи" в веб-интерфейсе

1. Проверьте что Python сервер запущен
2. Проверьте IP адрес в `app.js`
3. Откройте консоль браузера (F12) - проверьте ошибки
4. Проверьте что порт 8080 не занят

### Проблема: Python сервер не запускается

1. Проверьте что все библиотеки установлены
2. Проверьте что порт 8080 свободен
3. Попробуйте другой порт (измените в коде)

### Проблема: Данные не обновляются

1. Проверьте что Arduino подключена (UART или UDP)
2. Проверьте что данные приходят в Python GUI
3. Проверьте endpoint `/data` в браузере

### Проблема: CORS ошибки

Если открываете `index.html` через `file://`:
- Используйте локальный сервер:
```bash
python -m http.server 8000
```
- Откройте: `http://localhost:8000`

---

## Структура файлов

```
project/
├── index.html              # Веб-интерфейс
├── style.css               # Стили
├── app.js                  # JavaScript логика
├── arduino_monitor.py      # Ваш Python скрипт
├── web_server.py           # HTTP API сервер
├── monitor_with_web.py     # Интегрированная версия
├── arduino-template.ino    # Шаблон для Arduino
├── data-mapping.json       # Описание структуры данных
├── INTEGRATION_GUIDE.md    # Руководство по интеграции
└── SETUP_GUIDE.md          # Эта инструкция
```

---

## Быстрый старт (TL;DR)

```bash
# 1. Установка зависимостей
pip install pyserial matplotlib numpy netifaces

# 2. Запуск (выберите один вариант)

# Вариант А: Интегрированная версия
python monitor_with_web.py

# Вариант Б: Ваш оригинальный скрипт + добавьте web_server

# 3. В GUI:
# - Подключите UART или запустите UDP
# - Запустите Web Server (кнопка или автоматически)

# 4. Настройте app.js:
# const ARDUINO_IP = 'localhost:8080';

# 5. Откройте index.html в браузере
```

---

## Endpoints API

### GET /data
Возвращает текущие данные датчиков

**Ответ:**
```json
{
  "rpm": [1200, 1250, 1180, 1220],
  "temp": [45.5, 47.2, 44.8, 46.1],
  "thrust": 850.5,
  "vibration": {"x": 0.15, "y": 0.12, "z": 9.85},
  "noise": 75.3
}
```

### GET /stats
Возвращает статистику работы

**Ответ:**
```json
{
  "uptime": 123.45,
  "uart_packets": 1234,
  "udp_packets": 567,
  "total_packets": 1801,
  "uart_rate": 10.0,
  "udp_rate": 4.6,
  "connected": true
}
```

### GET /ping
Проверка связи

**Ответ:**
```json
{
  "status": "ok",
  "timestamp": 1234567890.123
}
```

---

## Контакты и поддержка

При возникновении проблем проверьте:
1. Логи Python скрипта
2. Консоль браузера (F12)
3. Доступность endpoints через curl или браузер
