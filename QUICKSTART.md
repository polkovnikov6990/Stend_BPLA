# ⚡ Быстрый старт

## 🎯 Цель
Запустить систему мониторинга квадрокоптера за 5 минут.

---

## ⚠️ Важно

Arduino использует UDP broadcast для отправки данных, поэтому:
- ❌ Прямое подключение браузера к Arduino невозможно
- ✅ Python сервер обязателен (получает UDP → предоставляет HTTP API)

---

## 📋 Что нужно

- ✅ Arduino Nano с датчиками
- ✅ Python 3.7+
- ✅ Библиотеки: `pip install pyserial matplotlib numpy netifaces`
- ✅ Компьютер в той же сети с Arduino

---

## 🚀 Запуск за 5 шагов

### Шаг 1: Установите зависимости
```bash
pip install pyserial matplotlib numpy netifaces
```

### Шаг 2: Настройте конфиг
```json
// config/config.json
{
  "connection": {
    "mode": "server"
  },
  "arduino": {
    "network": {
      "ip": "192.168.0.177",  // ← Ваш IP Arduino
      "broadcast_ip": "192.168.0.255",
      "udp_port": 8888
    }
  }
}
```

### Шаг 3: Загрузите код на Arduino
```bash
1. Откройте arduino/quadcopter_stand.ino в Arduino IDE
2. Установите библиотеки:
   - Ethernet
   - EthernetUdp
   - HX711
   - Wire
   - Adafruit_ADXL345
3. Загрузите на Arduino
```

### Шаг 4: Запустите Python сервер
```bash
cd backend
python monitor_with_web.py

# В GUI:
# 1. Нажмите "Запуск UDP" (порт 8888)
# 2. Нажмите "Запуск Web" (порт 8080)
```

### Шаг 5: Откройте веб-интерфейс
```bash
# В новом терминале:
python -m http.server 8000

# Откройте в браузере:
http://localhost:8000/frontend/
```

✅ **Готово!** Индикатор должен стать зеленым.

---

## 🔧 Настройка под ваш стенд

### 1. Сетевые параметры Arduino
```json
// config/config.json
"arduino": {
  "network": {
    "ip": "192.168.0.177",        // ← Ваш IP
    "broadcast_ip": "192.168.0.255",  // ← Ваш broadcast
    "udp_port": 8888
  }
}
```

### 2. Пины датчиков
```json
// config/config.json
"pins": {
  "rpm_sensors": {
    "motor_1": 6,  // ← Ваши пины (моторы 1-2 с прерываниями)
    "motor_2": 7,
    "motor_3": 8,  // ← Моторы 3-4 с polling
    "motor_4": 9
  },
  "load_cells": {
    "hx711_1_dout": 2,
    "hx711_1_sck": 3,
    "hx711_2_dout": 4,
    "hx711_2_sck": 5
  }
}
```

### 3. Калибровка
```json
// config/config.json
"calibration": {
  "load_cells": {
    "scale_1": 1000.0,  // ← Ваши коэффициенты
    "scale_2": 1000.0
  }
}
```

---

## ✅ Проверка работы

### Тест 1: Arduino отправляет UDP
```bash
# В Serial Monitor Arduino должны быть сообщения:
{"status":"READY"}
{"t":12345,"d":1,"m":[...],"r":[...],...}
```

### Тест 2: Python получает UDP
```bash
# В GUI Python должны появляться пакеты:
UDP: X пак, Y пак/с
```

### Тест 3: Веб-интерфейс работает
```
1. Откройте http://localhost:8000/frontend/
2. Индикатор подключения: 🟢 (зеленый)
3. Данные обновляются
```

### Тест 4: Графики работают
```
1. Кликните на любую карточку
2. Откроется график
3. График обновляется в реальном времени
```

---

## 🐛 Проблемы?

### "Нет связи" 🔴
```bash
1. Проверьте что Python сервер запущен
2. Проверьте что UDP слушатель активен (порт 8888)
3. Проверьте что Web сервер активен (порт 8080)
4. Проверьте Serial Monitor Arduino - должны быть JSON сообщения
5. Проверьте что Arduino и компьютер в одной сети
```

### Python не получает UDP
```bash
1. Проверьте IP в config.json
2. Проверьте что Arduino отправляет (Serial Monitor)
3. Проверьте firewall (может блокировать UDP)
4. Попробуйте подключить UART вместо UDP
```

### Неверные данные
```bash
1. Проверьте калибровку в config/config.json
2. Проверьте Serial Monitor Arduino
3. Проверьте подключение датчиков
```

---

## 📖 Подробная документация

- [README.md](README.md) - Общая информация
- [docs/PROJECT_STRUCTURE.md](docs/PROJECT_STRUCTURE.md) - Структура проекта
- [docs/SETUP_GUIDE.md](docs/SETUP_GUIDE.md) - Детальная установка
- [docs/INTEGRATION_GUIDE.md](docs/INTEGRATION_GUIDE.md) - Интеграция компонентов

---

## 🎨 Возможности

- ✅ Мониторинг в реальном времени (100 мс)
- ✅ Интерактивные графики
- ✅ Экспорт данных в CSV
- ✅ Цветовая индикация
- ✅ Адаптивный дизайн
- ✅ История 100 точек

---

## 👥 Авторы

- Полковников А.В.
- Глазырин К.С.

**Кафедра робототехники и противодействия робототехническим комплексам (системам)**

---

## 📞 Нужна помощь?

1. Проверьте документацию в `docs/`
2. Проверьте консоль браузера (F12)
3. Проверьте Serial Monitor Arduino
4. Проверьте `config/config.json`

---

**Версия:** 1.0.0  
**GitHub:** https://github.com/polkovnikov6990/Stend_BPLA
