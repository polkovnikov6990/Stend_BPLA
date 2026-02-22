#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
HTTP сервер для веб-интерфейса мониторинга квадрокоптера
Получает данные от Arduino Monitor и отдает их через REST API
"""

from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import threading
import time
from datetime import datetime


class WebAPIHandler(BaseHTTPRequestHandler):
    """Обработчик HTTP запросов"""
    
    # Ссылка на монитор (устанавливается извне)
    monitor = None
    
    def do_GET(self):
        """Обработка GET запросов"""
        
        # Endpoint для получения данных
        if self.path == '/data':
            self.send_data_response()
        
        # Endpoint для статистики
        elif self.path == '/stats':
            self.send_stats_response()
        
        # Endpoint для проверки связи
        elif self.path == '/ping':
            self.send_ping_response()
        
        else:
            self.send_error(404, "Endpoint not found")
    
    def do_OPTIONS(self):
        """Обработка OPTIONS для CORS"""
        self.send_response(200)
        self.send_cors_headers()
        self.end_headers()
    
    def send_cors_headers(self):
        """Отправка CORS заголовков"""
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
    
    def send_data_response(self):
        """Отправка данных датчиков"""
        try:
            if not self.monitor:
                self.send_error(500, "Monitor not initialized")
                return
            
            # Получаем последние данные
            last_data = self.monitor.last_data
            
            # Формируем ответ в формате для веб-интерфейса
            response = {
                # Обороты двигателей
                "rpm": [
                    int(last_data['rpms'][0]) if len(last_data['rpms']) > 0 else 0,
                    int(last_data['rpms'][1]) if len(last_data['rpms']) > 1 else 0,
                    int(last_data['rpms'][2]) if len(last_data['rpms']) > 2 else 0,
                    int(last_data['rpms'][3]) if len(last_data['rpms']) > 3 else 0
                ],
                
                # Температура двигателей
                "temp": [
                    float(last_data['motors'][0]) if last_data['motors'][0] is not None else 0.0,
                    float(last_data['motors'][1]) if last_data['motors'][1] is not None else 0.0,
                    float(last_data['motors'][2]) if last_data['motors'][2] is not None else 0.0,
                    float(last_data['motors'][3]) if last_data['motors'][3] is not None else 0.0
                ],
                
                # Тяга (сумма двух тензодатчиков в граммах)
                "thrust": float(
                    (last_data['loadCells'][0] + last_data['loadCells'][1]) * 1000
                ) if len(last_data['loadCells']) >= 2 else 0.0,
                
                # Вибрация
                "vibration": {
                    "x": float(last_data['accel'][0]) if len(last_data['accel']) > 0 else 0.0,
                    "y": float(last_data['accel'][1]) if len(last_data['accel']) > 1 else 0.0,
                    "z": float(last_data['accel'][2]) if len(last_data['accel']) > 2 else 0.0
                },
                
                # Уровень шума (преобразуем 0-255 в дБ, примерно 40-100 дБ)
                "noise": float(40 + (last_data['sound'] / 255.0) * 60)
            }
            
            # Отправляем ответ
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_cors_headers()
            self.end_headers()
            
            json_response = json.dumps(response, ensure_ascii=False)
            self.wfile.write(json_response.encode('utf-8'))
            
        except Exception as e:
            print(f"❌ Ошибка отправки данных: {e}")
            self.send_error(500, str(e))
    
    def send_stats_response(self):
        """Отправка статистики"""
        try:
            if not self.monitor:
                self.send_error(500, "Monitor not initialized")
                return
            
            stats = self.monitor.get_statistics()
            
            response = {
                "uptime": stats['uptime'],
                "uart_packets": stats['uart_packets'],
                "udp_packets": stats['udp_packets'],
                "total_packets": stats['total_packets'],
                "uart_rate": stats['uart_rate'],
                "udp_rate": stats['udp_rate'],
                "connected": self.monitor.uart_connected or self.monitor.udp_listening
            }
            
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_cors_headers()
            self.end_headers()
            
            json_response = json.dumps(response, ensure_ascii=False)
            self.wfile.write(json_response.encode('utf-8'))
            
        except Exception as e:
            print(f"❌ Ошибка отправки статистики: {e}")
            self.send_error(500, str(e))
    
    def send_ping_response(self):
        """Простой ping для проверки связи"""
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_cors_headers()
        self.end_headers()
        
        response = {
            "status": "ok",
            "timestamp": time.time()
        }
        
        json_response = json.dumps(response)
        self.wfile.write(json_response.encode('utf-8'))
    
    def log_message(self, format, *args):
        """Переопределяем логирование (опционально)"""
        # Можно закомментировать для отключения логов запросов
        timestamp = datetime.now().strftime("%H:%M:%S")
        print(f"[{timestamp}] [WEB] {format % args}")


class WebServer:
    """HTTP сервер для веб-интерфейса"""
    
    def __init__(self, monitor, host='0.0.0.0', port=8080):
        """
        Инициализация сервера
        
        Args:
            monitor: Экземпляр ArduinoMonitor
            host: IP адрес для прослушивания (0.0.0.0 = все интерфейсы)
            port: Порт сервера
        """
        self.monitor = monitor
        self.host = host
        self.port = port
        self.server = None
        self.thread = None
        self.running = False
        
        # Устанавливаем ссылку на монитор в обработчик
        WebAPIHandler.monitor = monitor
    
    def start(self):
        """Запуск сервера"""
        try:
            self.server = HTTPServer((self.host, self.port), WebAPIHandler)
            self.running = True
            
            # Запускаем в отдельном потоке
            self.thread = threading.Thread(target=self._run_server)
            self.thread.daemon = True
            self.thread.start()
            
            print(f"✅ HTTP сервер запущен на http://{self.host}:{self.port}")
            print(f"   Endpoints:")
            print(f"   - http://{self.host}:{self.port}/data   (данные датчиков)")
            print(f"   - http://{self.host}:{self.port}/stats  (статистика)")
            print(f"   - http://{self.host}:{self.port}/ping   (проверка связи)")
            
            # Показываем локальный IP
            import socket
            hostname = socket.gethostname()
            local_ip = socket.gethostbyname(hostname)
            print(f"   Локальный IP: {local_ip}")
            print(f"   Используйте в app.js: const ARDUINO_IP = '{local_ip}:{self.port}';")
            
            return True
            
        except Exception as e:
            print(f"❌ Ошибка запуска HTTP сервера: {e}")
            return False
    
    def _run_server(self):
        """Запуск сервера (внутренний метод)"""
        try:
            self.server.serve_forever()
        except Exception as e:
            if self.running:
                print(f"❌ Ошибка HTTP сервера: {e}")
    
    def stop(self):
        """Остановка сервера"""
        self.running = False
        if self.server:
            self.server.shutdown()
            self.server.server_close()
            print("🔌 HTTP сервер остановлен")


# Пример использования
if __name__ == "__main__":
    print("Этот модуль должен импортироваться из основного скрипта")
    print("Используйте: python monitor_with_web.py")
