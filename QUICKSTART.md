# ⚡ Быстрый старт

## 🎯 Цель
Запустить систему мониторинга квадрокоптера за 5 минут.

---

## 📋 Что нужно

### Минимум (прямое подключение):
- ✅ Arduino Nano с датчиками
- ✅ Компьютер в той же сети
- ✅ Браузер

### Опционально (через сервер):
- ✅ Python 3.7+
- ✅ Библиотеки: `pip install pyserial matplotlib numpy netifaces`

---

## 🚀 Запуск за 3 шага

### Вариант 1: Прямое подключение (рекомендуется)

#### Шаг 1: Настройте Arduino
```bash
1. Откройте arduino/quadcopter_stand.ino в Arduino IDE
2. Установите библиотеки:
   - ArduinoJson
   - HX711
   - Adafruit_ADXL345
3. Измените IP в config/config.json (если нужно)
4. Загрузите на Arduino
```

#### Шаг 2: Настройте конфиг
```json
// config/config.json
{
  "connection": {
    "mode": "direct",
    "direct": {
      "arduino_ip": "192.168.1.100"  // ← Ваш IP Arduino
    }
  }
}
```

#### Шаг 3: Откройте веб-интерфейс
```bash
# Вариант А: Прямо из файла
Откройте frontend/index.html в браузере

# Вариант Б: Через локальный сервер (рекомендуется)
python -m http.server 8000
# Откройте: http://localhost:8000/frontend/
```

✅ **Готово!** Индикатор должен стать зеленым.

---

### Вариант 2: Через Python сервер

#### Шаг 1: Установите зависимости
```bash
pip install pyserial matplotlib numpy netifaces
```

#### Шаг 2: Настройте конфиг
```json
// config/config.json
{
  "connection": {
    "mode": "server"
  }
}
```

#### Шаг 3: Запустите сервер
```bash
cd backend
python monitor_with_web.py

# В GUI:
# 1. Подключите UART или запустите UDP
# 2. Нажмите "Запуск Web"
```

#### Шаг 4: Откройте веб-интерфейс
```
http://localhost:8080/frontend/
```

---

## 🔧 Настройка под ваш стенд

### 1. IP адрес Arduino
```json
// config/config.json
"arduino": {
  "network": {
    "ip": "192.168.1.XXX"  // ← Ваш IP
  }
}
```

### 2. Пины датчиков
```json
// config/config.json
"pins": {
  "rpm_sensors": {
    "motor_1": 2,  // ← Ваши пины
    "motor_2": 3,
    ...
  }
}
```

### 3. Калибровка
```json
// config/config.json
"calibration": {
  "load_cells": {
    "scale_1": 420.0,  // ← Ваши коэффициенты
    "scale_2": 420.0
  }
}
```

---

## ✅ Проверка работы

### Тест 1: Arduino доступен
```bash
# В браузере откройте:
http://192.168.1.100/data

# Должен вернуть JSON с данными
```

### Тест 2: Веб-интерфейс работает
```
1. Откройте frontend/index.html
2. Индикатор подключения: 🟢 (зеленый)
3. Данные обновляются
```

### Тест 3: Графики работают
```
1. Кликните на любую карточку
2. Откроется график
3. График обновляется в реальном времени
```

---

## 🐛 Проблемы?

### "Нет связи" 🔴
```bash
1. Проверьте IP в config/config.json
2. Ping Arduino: ping 192.168.1.100
3. Откройте консоль браузера (F12)
4. Проверьте endpoint: http://ARDUINO_IP/data
```

### Неверные данные
```bash
1. Проверьте калибровку в config/config.json
2. Проверьте Serial Monitor Arduino
3. Проверьте подключение датчиков
```

### CORS ошибки
```bash
# Используйте локальный сервер:
python -m http.server 8000
# Не открывайте через file://
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
