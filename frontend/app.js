// ============================================
// КОНФИГУРАЦИЯ ПОДКЛЮЧЕНИЯ
// ============================================

// Загрузка конфигурации из config.json
let CONFIG = null;

// Загружаем конфигурацию при старте
async function loadConfig() {
    try {
        const response = await fetch('../config/config.json');
        CONFIG = await response.json();
        console.log('✅ Конфигурация загружена:', CONFIG);
        return true;
    } catch (error) {
        console.warn('⚠️ Не удалось загрузить config.json, используем значения по умолчанию');
        console.error('Ошибка:', error);
        // Значения по умолчанию
        CONFIG = {
            connection: {
                mode: 'direct',
                direct: { arduino_ip: '192.168.0.177', arduino_port: 80, endpoint: '/data' },
                server: { server_ip: 'localhost', server_port: 8080, endpoint: '/data' }
            },
            frontend: {
                update_interval_ms: 100,
                max_chart_points: 100
            }
        };
        console.log('📝 Используется конфигурация по умолчанию:', CONFIG);
        return false;
    }
}

// Получение URL для подключения
function getConnectionURL() {
    if (!CONFIG) {
        console.error('❌ CONFIG не загружен!');
        return 'http://192.168.0.177/data';
    }
    
    const conn = CONFIG.connection;
    let url;
    
    if (conn.mode === 'direct') {
        url = `http://${conn.direct.arduino_ip}:${conn.direct.arduino_port}${conn.direct.endpoint}`;
    } else {
        url = `http://${conn.server.server_ip}:${conn.server.server_port}${conn.server.endpoint}`;
    }
    
    console.log(`🔗 URL подключения (${conn.mode}):`, url);
    return url;
}

// Константы (будут переопределены после загрузки конфига)
let UPDATE_INTERVAL = 100;
let MAX_DATA_POINTS = 100;

// Хранилище данных для графиков
const chartData = {
    rpm: { labels: [], datasets: [[], [], [], []] },
    temp: { labels: [], datasets: [[], [], [], []] },
    thrust: { labels: [], datasets: [[]] },
    vibration: { labels: [], datasets: [[], [], []] },
    noise: { labels: [], datasets: [[]] }
};

// Текущий открытый график
let currentChart = null;
let currentChartType = null;

// Элементы DOM
const elements = {
    // Обороты
    rpm1: document.getElementById('rpm1'),
    rpm2: document.getElementById('rpm2'),
    rpm3: document.getElementById('rpm3'),
    rpm4: document.getElementById('rpm4'),
    
    // Температура
    temp1: document.getElementById('temp1'),
    temp2: document.getElementById('temp2'),
    temp3: document.getElementById('temp3'),
    temp4: document.getElementById('temp4'),
    
    // Тяга
    thrust: document.getElementById('thrust'),
    
    // Вибрация
    vibX: document.getElementById('vibX'),
    vibY: document.getElementById('vibY'),
    vibZ: document.getElementById('vibZ'),
    
    // Шум
    noise: document.getElementById('noise'),
    noiseFill: document.getElementById('noiseFill'),
    
    // Статус
    statusText: document.getElementById('statusText'),
    connectionStatus: document.querySelector('.status-indicator'),
    lastUpdate: document.getElementById('lastUpdate')
};

// Состояние подключения
let isConnected = false;
let updateTimer = null;

// Функция получения данных от Arduino
async function fetchData() {
    try {
        const url = getConnectionURL();
        console.log('📡 Запрос данных:', url);
        
        const response = await fetch(url, {
            method: 'GET',
            headers: {
                'Content-Type': 'application/json'
            },
            cache: 'no-cache'
        });
        
        console.log('📥 Ответ получен:', response.status, response.statusText);
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        const data = await response.json();
        console.log('✅ Данные распарсены:', data);
        
        updateUI(data);
        setConnectionStatus(true);
        
    } catch (error) {
        console.error('❌ Ошибка получения данных:', error);
        console.error('Детали:', {
            message: error.message,
            name: error.name,
            stack: error.stack
        });
        setConnectionStatus(false);
    }
}

// Обновление интерфейса
function updateUI(data) {
    const timestamp = new Date().toLocaleTimeString('ru-RU');
    
    // Обороты двигателей
    if (data.rpm) {
        elements.rpm1.textContent = data.rpm[0] || 0;
        elements.rpm2.textContent = data.rpm[1] || 0;
        elements.rpm3.textContent = data.rpm[2] || 0;
        elements.rpm4.textContent = data.rpm[3] || 0;
        
        // Сохранение данных для графика
        updateChartData('rpm', timestamp, data.rpm);
    }
    
    // Температура
    if (data.temp) {
        elements.temp1.textContent = data.temp[0]?.toFixed(1) || '0.0';
        elements.temp2.textContent = data.temp[1]?.toFixed(1) || '0.0';
        elements.temp3.textContent = data.temp[2]?.toFixed(1) || '0.0';
        elements.temp4.textContent = data.temp[3]?.toFixed(1) || '0.0';
        
        // Подсветка при перегреве
        [elements.temp1, elements.temp2, elements.temp3, elements.temp4].forEach((el, i) => {
            const temp = data.temp[i];
            if (temp > 80) {
                el.style.color = '#f44336';
            } else if (temp > 60) {
                el.style.color = '#ffc107';
            } else {
                el.style.color = '#764ba2';
            }
        });
        
        // Сохранение данных для графика
        updateChartData('temp', timestamp, data.temp);
    }
    
    // Тяга
    if (data.thrust !== undefined) {
        elements.thrust.textContent = data.thrust.toFixed(0);
        updateChartData('thrust', timestamp, [data.thrust]);
    }
    
    // Вибрация
    if (data.vibration) {
        elements.vibX.textContent = data.vibration.x?.toFixed(2) || '0.00';
        elements.vibY.textContent = data.vibration.y?.toFixed(2) || '0.00';
        elements.vibZ.textContent = data.vibration.z?.toFixed(2) || '0.00';
        
        updateChartData('vibration', timestamp, [
            data.vibration.x,
            data.vibration.y,
            data.vibration.z
        ]);
    }
    
    // Шум
    if (data.noise !== undefined) {
        elements.noise.textContent = data.noise.toFixed(0);
        
        // Обновление индикатора шума (0-120 дБ)
        const noisePercent = Math.min((data.noise / 120) * 100, 100);
        elements.noiseFill.style.width = `${noisePercent}%`;
        
        updateChartData('noise', timestamp, [data.noise]);
    }
    
    // Обновление времени
    updateTimestamp();
}

// Установка статуса подключения
function setConnectionStatus(connected) {
    isConnected = connected;
    
    if (connected) {
        elements.statusText.textContent = 'Подключено';
        elements.connectionStatus.className = 'status-indicator connected';
    } else {
        elements.statusText.textContent = 'Нет связи';
        elements.connectionStatus.className = 'status-indicator disconnected';
    }
}

// Обновление времени последнего обновления
function updateTimestamp() {
    const now = new Date();
    const hours = String(now.getHours()).padStart(2, '0');
    const minutes = String(now.getMinutes()).padStart(2, '0');
    const seconds = String(now.getSeconds()).padStart(2, '0');
    elements.lastUpdate.textContent = `${hours}:${minutes}:${seconds}`;
}

// Запуск периодического обновления
function startUpdates() {
    fetchData(); // Первый запрос сразу
    updateTimer = setInterval(fetchData, UPDATE_INTERVAL);
}

// Остановка обновлений
function stopUpdates() {
    if (updateTimer) {
        clearInterval(updateTimer);
        updateTimer = null;
    }
}

// Инициализация при загрузке страницы
window.addEventListener('load', async () => {
    console.log('Инициализация интерфейса...');
    
    // Загружаем конфигурацию
    await loadConfig();
    
    // Применяем настройки из конфига
    if (CONFIG) {
        UPDATE_INTERVAL = CONFIG.frontend.update_interval_ms;
        MAX_DATA_POINTS = CONFIG.frontend.max_chart_points;
        
        console.log(`Режим подключения: ${CONFIG.connection.mode}`);
        console.log(`URL: ${getConnectionURL()}`);
    }
    
    startUpdates();
});

// Остановка при закрытии страницы
window.addEventListener('beforeunload', () => {
    stopUpdates();
});

// Обработка потери фокуса (опционально - экономия ресурсов)
document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
        stopUpdates();
    } else {
        startUpdates();
    }
});

// === ФУНКЦИИ ДЛЯ РАБОТЫ С ГРАФИКАМИ ===

// Обновление данных для графиков
function updateChartData(type, label, values) {
    const data = chartData[type];
    
    // Добавление метки времени
    data.labels.push(label);
    
    // Добавление значений для каждого датасета
    values.forEach((value, index) => {
        if (data.datasets[index]) {
            data.datasets[index].push(value);
        }
    });
    
    // Ограничение количества точек
    if (data.labels.length > MAX_DATA_POINTS) {
        data.labels.shift();
        data.datasets.forEach(dataset => dataset.shift());
    }
    
    // Обновление графика если он открыт
    if (currentChartType === type && currentChart) {
        updateCurrentChart();
    }
}

// Открытие графика
function openChart(type) {
    currentChartType = type;
    const modal = document.getElementById('chartModal');
    const canvas = document.getElementById('chartCanvas');
    const title = document.getElementById('chartTitle');
    
    // Установка заголовка
    const titles = {
        rpm: '⚙️ График оборотов двигателей',
        temp: '🌡️ График температуры двигателей',
        thrust: '⚖️ График тяги',
        vibration: '📊 График вибрации',
        noise: '🔊 График уровня шума'
    };
    title.textContent = titles[type];
    
    // Показ модального окна
    modal.classList.add('active');
    
    // Создание графика
    createChart(type, canvas);
}

// Создание графика
function createChart(type, canvas) {
    // Уничтожение предыдущего графика
    if (currentChart) {
        currentChart.destroy();
    }
    
    const data = chartData[type];
    const ctx = canvas.getContext('2d');
    
    // Конфигурация датасетов
    const datasetConfigs = {
        rpm: [
            { label: 'Двигатель 1', color: '#667eea' },
            { label: 'Двигатель 2', color: '#764ba2' },
            { label: 'Двигатель 3', color: '#f093fb' },
            { label: 'Двигатель 4', color: '#4facfe' }
        ],
        temp: [
            { label: 'Двигатель 1', color: '#f44336' },
            { label: 'Двигатель 2', color: '#ff9800' },
            { label: 'Двигатель 3', color: '#ffc107' },
            { label: 'Двигатель 4', color: '#ff5722' }
        ],
        thrust: [
            { label: 'Тяга', color: '#4caf50' }
        ],
        vibration: [
            { label: 'Ось X', color: '#2196f3' },
            { label: 'Ось Y', color: '#9c27b0' },
            { label: 'Ось Z', color: '#ff9800' }
        ],
        noise: [
            { label: 'Уровень шума', color: '#e91e63' }
        ]
    };
    
    const configs = datasetConfigs[type];
    const datasets = configs.map((config, index) => ({
        label: config.label,
        data: data.datasets[index] || [],
        borderColor: config.color,
        backgroundColor: config.color + '20',
        borderWidth: 2,
        tension: 0.4,
        fill: true,
        pointRadius: 0,
        pointHoverRadius: 5
    }));
    
    currentChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: data.labels,
            datasets: datasets
        },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            interaction: {
                mode: 'index',
                intersect: false
            },
            plugins: {
                legend: {
                    display: true,
                    position: 'top',
                    labels: {
                        usePointStyle: true,
                        padding: 15,
                        font: {
                            size: 12,
                            family: "'Segoe UI', Tahoma, Geneva, Verdana, sans-serif"
                        }
                    }
                },
                tooltip: {
                    backgroundColor: 'rgba(0, 0, 0, 0.8)',
                    padding: 12,
                    titleFont: {
                        size: 14
                    },
                    bodyFont: {
                        size: 13
                    },
                    cornerRadius: 8
                }
            },
            scales: {
                x: {
                    display: true,
                    grid: {
                        display: false
                    },
                    ticks: {
                        maxRotation: 45,
                        minRotation: 45,
                        maxTicksLimit: 10
                    }
                },
                y: {
                    display: true,
                    grid: {
                        color: 'rgba(0, 0, 0, 0.05)'
                    },
                    beginAtZero: true
                }
            },
            animation: {
                duration: 300
            }
        }
    });
}

// Обновление текущего графика
function updateCurrentChart() {
    if (!currentChart || !currentChartType) return;
    
    const data = chartData[currentChartType];
    
    currentChart.data.labels = data.labels;
    currentChart.data.datasets.forEach((dataset, index) => {
        dataset.data = data.datasets[index] || [];
    });
    
    currentChart.update('none'); // Обновление без анимации для плавности
}

// Закрытие графика
function closeChart() {
    const modal = document.getElementById('chartModal');
    modal.classList.remove('active');
    
    if (currentChart) {
        currentChart.destroy();
        currentChart = null;
    }
    
    currentChartType = null;
}

// Очистка данных графика
function clearChartData() {
    if (!currentChartType) return;
    
    const data = chartData[currentChartType];
    data.labels = [];
    data.datasets.forEach(dataset => dataset.length = 0);
    
    if (currentChart) {
        updateCurrentChart();
    }
}

// Экспорт данных графика
function exportChartData() {
    if (!currentChartType) return;
    
    const data = chartData[currentChartType];
    const csvContent = generateCSV(data, currentChartType);
    
    // Создание и скачивание файла
    const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
    const link = document.createElement('a');
    const url = URL.createObjectURL(blob);
    
    link.setAttribute('href', url);
    link.setAttribute('download', `${currentChartType}_${Date.now()}.csv`);
    link.style.visibility = 'hidden';
    
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}

// Генерация CSV
function generateCSV(data, type) {
    const headers = ['Время'];
    
    const datasetNames = {
        rpm: ['Двигатель 1', 'Двигатель 2', 'Двигатель 3', 'Двигатель 4'],
        temp: ['Двигатель 1', 'Двигатель 2', 'Двигатель 3', 'Двигатель 4'],
        thrust: ['Тяга'],
        vibration: ['X', 'Y', 'Z'],
        noise: ['Уровень шума']
    };
    
    headers.push(...datasetNames[type]);
    
    let csv = headers.join(',') + '\n';
    
    for (let i = 0; i < data.labels.length; i++) {
        const row = [data.labels[i]];
        data.datasets.forEach(dataset => {
            row.push(dataset[i] || 0);
        });
        csv += row.join(',') + '\n';
    }
    
    return csv;
}

// Закрытие модального окна по клику вне его
window.onclick = function(event) {
    const modal = document.getElementById('chartModal');
    if (event.target === modal) {
        closeChart();
    }
}

// Закрытие по Escape
document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
        closeChart();
        closeAuthors();
    }
});

// === ФУНКЦИИ ДЛЯ МОДАЛЬНОГО ОКНА АВТОРОВ ===

function openAuthors() {
    const modal = document.getElementById('authorsModal');
    modal.classList.add('active');
}

function closeAuthors() {
    const modal = document.getElementById('authorsModal');
    modal.classList.remove('active');
}

// Закрытие модального окна авторов по клику вне его
window.addEventListener('click', function(event) {
    const chartModal = document.getElementById('chartModal');
    const authorsModal = document.getElementById('authorsModal');
    
    if (event.target === chartModal) {
        closeChart();
    }
    
    if (event.target === authorsModal) {
        closeAuthors();
    }
});
