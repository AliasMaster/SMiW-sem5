const tempEl = document.getElementById('temp');
const thresholdValEl = document.getElementById('threshold-val');
const thresholdSlider = document.getElementById('threshold-slider');
const setBtn = document.getElementById('set-btn');
const alarmBanner = document.getElementById('alarm-banner');
const statusBadge = document.getElementById('status-badge');
const ctx = document.getElementById('historyChart').getContext('2d');

const API_KEY = '4IJIIgnUBzG4QWdFDoSmJ1Lp4OLxLkjj';
let historyChart;
let currentThreshold = 30;
let currentRange = 'day';

function initChart() {
    historyChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Temperatura (°C)',
                data: [],
                borderColor: '#00d2ff',
                backgroundColor: 'rgba(0, 210, 255, 0.1)',
                borderWidth: 2,
                fill: true,
                tension: 0.4,
                pointRadius: 0
            }, {
                label: 'Próg (°C)',
                data: [],
                borderColor: '#ff4b2b',
                borderDash: [5, 5],
                borderWidth: 1,
                fill: false,
                pointRadius: 0
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                x: {
                    display: true, // Włączamy oś X
                    grid: {
                        color: 'rgba(255, 255, 255, 0.05)'
                    },
                    ticks: {
                        color: 'rgba(255, 255, 255, 0.3)',
                        font: { size: 10 },
                        maxRotation: 0,
                        autoSkip: true,
                        maxTicksLimit: 6
                    }
                },
                y: {
                    grid: {
                        color: 'rgba(255, 255, 255, 0.1)'
                    },
                    ticks: {
                        color: 'rgba(255, 255, 255, 0.5)'
                    }
                }
            },
            plugins: {
                legend: {
                    display: false
                }
            }
        }
    });
}

function updateUI() {
    fetch(`/api/history?range=${currentRange}`, {
        headers: { 'X-API-Key': API_KEY }
    })
        .then(response => response.json())
        .then(data => {
            const history = data.history;
            const threshold = data.currentThreshold;
            const latestTemp = data.latestTemp;

            // Zawsze pokazuj absolutnie ostatnią temperaturę (niezależnie od zakresu wykresu)
            if (latestTemp !== null) {
                tempEl.innerText = latestTemp.toFixed(1);
                
                if (latestTemp > threshold) {
                    alarmBanner.classList.remove('hidden');
                } else {
                    alarmBanner.classList.add('hidden');
                }
            }

            if (history.length > 0) {
                // Update Chart
                historyChart.data.labels = history.map(d => {
                    const date = new Date(d.timestamp);
                    if (currentRange === 'day') return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
                    if (currentRange === 'month') return date.toLocaleDateString([], { day: 'numeric', month: 'short' });
                    return date.toLocaleDateString([], { month: 'short', year: '2-digit' });
                });
                historyChart.data.datasets[0].data = history.map(d => d.temp);
                historyChart.data.datasets[1].data = history.map(d => d.threshold); 
                historyChart.update('none');
            }
            
            // Suwak zawsze używa aktualnego ustawienia z serwera (z tabeli settings)
            if (document.activeElement !== thresholdSlider) {
                thresholdValEl.innerText = threshold.toFixed(1);
                thresholdSlider.value = threshold;
            }
            currentThreshold = threshold;

            statusBadge.innerText = "Online";
            statusBadge.style.background = "rgba(0, 210, 255, 0.2)";
            statusBadge.style.color = "#00d2ff";
        })
        .catch(err => {
            console.error('Błąd:', err);
            statusBadge.innerText = "Błąd serwera";
            statusBadge.style.background = "rgba(255, 75, 43, 0.2)";
            statusBadge.style.color = "#ff4b2b";
        });
}

thresholdSlider.addEventListener('input', (e) => {
    thresholdValEl.innerText = parseFloat(e.target.value).toFixed(1);
});

setBtn.addEventListener('click', () => {
    const val = thresholdSlider.value;
    setBtn.disabled = true;
    setBtn.innerText = "Zapisywanie...";
    
    fetch('/api/threshold', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'X-API-Key': API_KEY
        },
        body: JSON.stringify({ value: val })
    })
    .then(response => {
        if (response.ok) {
            setBtn.innerText = "Zapisano!";
            setTimeout(() => {
                setBtn.innerText = "Zapisz ustawienie";
                setBtn.disabled = false;
            }, 2000);
            updateUI();
        } else {
            throw new Error("Błąd serwera");
        }
    })
    .catch(err => {
        alert("Nie udało się zapisać progu.");
        setBtn.innerText = "Zapisz ustawienie";
        setBtn.disabled = false;
    });
});

// Obsługa przycisków zakresu
document.querySelectorAll('.range-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        document.querySelectorAll('.range-btn').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        currentRange = btn.dataset.range;
        updateUI();
    });
});

initChart();
setInterval(updateUI, 5000);
updateUI();
