#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Интегрированная версия монитора с веб-сервером
Запускает Arduino Monitor + HTTP API для веб-интерфейса
"""

import sys

# Импортируем оригинальный монитор (предполагается что файл называется arduino_monitor.py)
# Если у вас другое название, измените здесь
try:
    # Попробуем импортировать из того же файла
    from __main__ import ArduinoMonitor, MonitorGUI
except:
    print("⚠️ Не удалось импортировать ArduinoMonitor")
    print("   Убедитесь что этот файл находится рядом с основным скриптом")
    print("   Или скопируйте класс ArduinoMonitor сюда")
    sys.exit(1)

# Импортируем веб-сервер
from web_server import WebServer


class MonitorGUIWithWeb(MonitorGUI):
    """Расширенная версия GUI с веб-сервером"""
    
    def __init__(self, monitor):
        super().__init__(monitor)
        
        # Создаем веб-сервер
        self.web_server = WebServer(monitor, host='0.0.0.0', port=8080)
        
        # Добавляем кнопку управления веб-сервером
        self.add_web_controls()
    
    def add_web_controls(self):
        """Добавление элементов управления веб-сервером"""
        import tkinter as tk
        from tkinter import ttk
        
        # Находим control_frame (должен быть создан в setup_ui)
        # Добавляем секцию для веб-сервера
        for widget in self.root.winfo_children():
            if isinstance(widget, ttk.Frame):
                # Это наш control_frame
                web_frame = ttk.LabelFrame(widget, text="Web Server", padding="5")
                web_frame.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
                
                ttk.Label(web_frame, text="Порт:").pack(side=tk.LEFT, padx=2)
                
                self.web_port_entry = ttk.Entry(web_frame, width=8)
                self.web_port_entry.insert(0, "8080")
                self.web_port_entry.pack(side=tk.LEFT, padx=2)
                
                self.web_connect_btn = ttk.Button(web_frame, text="Запуск Web",
                                                   command=self.toggle_web)
                self.web_connect_btn.pack(side=tk.LEFT, padx=5)
                
                self.web_status = ttk.Label(web_frame, text="🔴", font=('Arial', 14))
                self.web_status.pack(side=tk.LEFT, padx=5)
                
                break
    
    def toggle_web(self):
        """Запуск/остановка веб-сервера"""
        if not self.web_server.running:
            try:
                port = int(self.web_port_entry.get())
                self.web_server.port = port
                
                if self.web_server.start():
                    self.web_connect_btn.config(text="Остановить Web")
                    self.web_status.config(text="🟢")
                    self.log(f"✅ Web сервер запущен на порту {port}")
            except ValueError:
                from tkinter import messagebox
                messagebox.showerror("Ошибка", "Неверный номер порта")
        else:
            self.web_server.stop()
            self.web_connect_btn.config(text="Запуск Web")
            self.web_status.config(text="🔴")
            self.log("🔌 Web сервер остановлен")
    
    def on_closing(self):
        """Обработка закрытия окна"""
        self.web_server.stop()
        super().on_closing()


def main():
    """Главная функция"""
    print("=" * 60)
    print("Arduino Nano Монитор v2.0 + Web Server")
    print("Поддержка UART, Ethernet (UDP) и HTTP API")
    print("=" * 60)
    
    # Создаем монитор
    monitor = ArduinoMonitor()
    
    # Запускаем GUI с веб-сервером
    gui = MonitorGUIWithWeb(monitor)
    
    try:
        gui.run()
    except KeyboardInterrupt:
        print("\n⏹️ Остановка...")
    finally:
        monitor.stop()
        print("👋 Программа завершена")


if __name__ == "__main__":
    main()
