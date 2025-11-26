// Configuration
const API_URL = 'http://192.168.31.222:5000/api';
const UPDATE_INTERVAL = 2000;
const CHART_MAX_POINTS = 100;

console.log('🚀 Dashboard JavaScript loaded!');

// Chart instances
let powerChart, currentChart, voltageChart, energyChart;

// Chart time range
let chartTimeRange = 'day'; // hour, day, week, month

// Initialize charts
function initCharts() {
    console.log('📊 Initializing charts...');
    
    const commonOptions = {
        responsive: true,
        maintainAspectRatio: false,
        interaction: {
            mode: 'index',
            intersect: false,
        },
        plugins: {
            legend: {
                display: true,
                position: 'top'
            },
            zoom: {
                pan: {
                    enabled: true,
                    mode: 'x',
                },
                zoom: {
                    wheel: {
                        enabled: true,
                    },
                    pinch: {
                        enabled: true
                    },
                    mode: 'x',
                }
            }
        },
        scales: {
            x: {
                display: true,
                title: {
                    display: true,
                    text: 'Time'
                }
            },
            y: {
                display: true,
                beginAtZero: true
            }
        }
    };

    // Power Chart
    const powerCtx = document.getElementById('powerChart').getContext('2d');
    powerChart = new Chart(powerCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Power 1 (W)',
                    data: [],
                    borderColor: 'rgb(255, 99, 132)',
                    backgroundColor: 'rgba(255, 99, 132, 0.1)',
                    tension: 0.4
                },
                {
                    label: 'Power 2 (W)',
                    data: [],
                    borderColor: 'rgb(54, 162, 235)',
                    backgroundColor: 'rgba(54, 162, 235, 0.1)',
                    tension: 0.4
                },
                {
                    label: 'Total Power (W)',
                    data: [],
                    borderColor: 'rgb(75, 192, 192)',
                    backgroundColor: 'rgba(75, 192, 192, 0.1)',
                    tension: 0.4
                }
            ]
        },
        options: commonOptions
    });

    // Current Chart
    const currentCtx = document.getElementById('currentChart').getContext('2d');
    currentChart = new Chart(currentCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Current 1 (A)',
                    data: [],
                    borderColor: 'rgb(255, 159, 64)',
                    backgroundColor: 'rgba(255, 159, 64, 0.1)',
                    tension: 0.4
                },
                {
                    label: 'Current 2 (A)',
                    data: [],
                    borderColor: 'rgb(153, 102, 255)',
                    backgroundColor: 'rgba(153, 102, 255, 0.1)',
                    tension: 0.4
                },
                {
                    label: 'Total Current (A)',
                    data: [],
                    borderColor: 'rgb(201, 203, 207)',
                    backgroundColor: 'rgba(201, 203, 207, 0.1)',
                    tension: 0.4
                }
            ]
        },
        options: commonOptions
    });

    // Voltage Chart
    const voltageCtx = document.getElementById('voltageChart').getContext('2d');
    voltageChart = new Chart(voltageCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Voltage (V)',
                    data: [],
                    borderColor: 'rgb(255, 205, 86)',
                    backgroundColor: 'rgba(255, 205, 86, 0.1)',
                    tension: 0.4
                }
            ]
        },
        options: commonOptions
    });

    // Energy Chart
    const energyCtx = document.getElementById('energyChart').getContext('2d');
    energyChart = new Chart(energyCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Total Energy (kWh)',
                    data: [],
                    borderColor: 'rgb(75, 192, 192)',
                    backgroundColor: 'rgba(75, 192, 192, 0.1)',
                    tension: 0.4,
                    fill: true
                }
            ]
        },
        options: commonOptions
    });
    
    console.log('✅ Charts initialized successfully');
}

// Update charts with new data
function updateCharts(timestamp, data) {
    const timeLabel = new Date(timestamp).toLocaleTimeString();

    powerChart.data.labels.push(timeLabel);
    powerChart.data.datasets[0].data.push(data.power1);
    powerChart.data.datasets[1].data.push(data.power2);
    powerChart.data.datasets[2].data.push(data.total_power);

    currentChart.data.labels.push(timeLabel);
    currentChart.data.datasets[0].data.push(data.current1);
    currentChart.data.datasets[1].data.push(data.current2);
    currentChart.data.datasets[2].data.push(data.total_current);

    voltageChart.data.labels.push(timeLabel);
    voltageChart.data.datasets[0].data.push(data.voltage);

    energyChart.data.labels.push(timeLabel);
    energyChart.data.datasets[0].data.push(data.total_energy || 0);

    // Limit chart data points for real-time view
    if (powerChart.data.labels.length > CHART_MAX_POINTS) {
        powerChart.data.labels.shift();
        powerChart.data.datasets.forEach(dataset => dataset.data.shift());
        
        currentChart.data.labels.shift();
        currentChart.data.datasets.forEach(dataset => dataset.data.shift());
        
        voltageChart.data.labels.shift();
        voltageChart.data.datasets[0].data.shift();

        energyChart.data.labels.shift();
        energyChart.data.datasets[0].data.shift();
    }

    powerChart.update('none');
    currentChart.update('none');
    voltageChart.update('none');
    energyChart.update('none');
}

// Fetch latest data
async function fetchLatestData() {
    try {
        const response = await fetch(`${API_URL}/latest`);
        
        if (response.ok) {
            const data = await response.json();
            updateDashboard(data);
            updateConnectionStatus(true);
        } else {
            updateConnectionStatus(false);
        }
    } catch (error) {
        console.error('❌ Error fetching data:', error);
        updateConnectionStatus(false);
    }
}

// Fetch statistics
async function fetchStatistics() {
    try {
        const response = await fetch(`${API_URL}/stats?period=${chartTimeRange}`);
        
        if (response.ok) {
            const data = await response.json();
            updateStatistics(data);
        }
    } catch (error) {
        console.error('❌ Error fetching statistics:', error);
    }
}

// Update dashboard
function updateDashboard(data) {
    // Real-time values
    document.getElementById('voltage').textContent = data.voltage.toFixed(2) + ' V';
    document.getElementById('current1').textContent = data.current1.toFixed(3) + ' A';
    document.getElementById('current2').textContent = data.current2.toFixed(3) + ' A';
    document.getElementById('current3').textContent = data.current3.toFixed(3) + ' A';
    document.getElementById('totalCurrent').textContent = data.total_current.toFixed(3) + ' A';
    
    document.getElementById('power1').textContent = data.power1.toFixed(2) + ' W';
    document.getElementById('power2').textContent = data.power2.toFixed(2) + ' W';
    document.getElementById('totalPower').textContent = data.total_power.toFixed(2) + ' W';
    document.getElementById('totalPowerHeader').textContent = data.total_power.toFixed(1) + ' W';
    
    // Energy values
    document.getElementById('energyL1').textContent = (data.energy_l1 || 0).toFixed(3) + ' kWh';
    document.getElementById('energyL2').textContent = (data.energy_l2 || 0).toFixed(3) + ' kWh';
    document.getElementById('totalEnergyDisplay').textContent = (data.total_energy || 0).toFixed(3) + ' kWh';
    
    // Cost values
    document.getElementById('costL1').textContent = '₹' + (data.cost_l1 || 0).toFixed(2);
    document.getElementById('costL2').textContent = '₹' + (data.cost_l2 || 0).toFixed(2);
    document.getElementById('totalCost').textContent = '₹' + (data.total_cost || 0).toFixed(2);
    
    // Relay states
    document.getElementById('relay1').checked = data.relay1_state;
    document.getElementById('relay2').checked = data.relay2_state;
    document.getElementById('relay3').checked = data.relay3_state;
    
    // Theft alert
    const theftAlert = document.getElementById('theftAlert');
    if (data.theft_detected || (data.theft_status && data.theft_status.detected)) {
        theftAlert.style.display = 'block';
        theftAlert.textContent = '🚨 THEFT DETECTED! Main relay disabled. Click to reset.';
        theftAlert.onclick = () => toggleRelay(3, true);
    } else {
        theftAlert.style.display = 'none';
    }
    
    // Last update
    const lastUpdate = new Date(data.timestamp);
    document.getElementById('lastUpdate').textContent = lastUpdate.toLocaleTimeString();
    
    // Update charts
    updateCharts(data.timestamp, data);
}

// Update statistics
function updateStatistics(data) {
    if (data.avg_voltage) {
        document.getElementById('avgVoltage').textContent = data.avg_voltage.toFixed(1) + 'V';
    }
    if (data.avg_current) {
        document.getElementById('avgCurrent').textContent = data.avg_current.toFixed(2) + 'A';
    }
    if (data.max_power) {
        document.getElementById('maxPower').textContent = data.max_power.toFixed(1) + 'W';
    }
    if (data.total_energy_kwh !== null) {
        document.getElementById('totalEnergy').textContent = data.total_energy_kwh.toFixed(3) + 'kWh';
    }
}

// Update connection status
function updateConnectionStatus(isConnected) {
    const statusElement = document.getElementById('connectionStatus');
    
    if (isConnected) {
        statusElement.textContent = 'Online';
        statusElement.className = 'status-value online';
    } else {
        statusElement.textContent = 'Offline';
        statusElement.className = 'status-value offline';
    }
}

// Toggle relay
async function toggleRelay(relay, state) {
    try {
        console.log(`🔌 Toggling Relay ${relay} to ${state ? 'ON' : 'OFF'}`);
        
        const response = await fetch(`${API_URL}/relay/control`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                relay: relay,
                state: state
            })
        });
        
        if (response.ok) {
            console.log('✅ Relay command sent');
        } else {
            const checkbox = document.getElementById(`relay${relay}`);
            checkbox.checked = !state;
        }
    } catch (error) {
        console.error('❌ Error toggling relay:', error);
        const checkbox = document.getElementById(`relay${relay}`);
        checkbox.checked = !state;
    }
}

// Update price
async function updatePrice() {
    const newPrice = parseFloat(document.getElementById('priceInput').value);
    
    if (isNaN(newPrice) || newPrice <= 0) {
        alert('Please enter a valid price');
        return;
    }
    
    try {
        const response = await fetch(`${API_URL}/settings/price`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ price: newPrice })
        });
        
        if (response.ok) {
            alert(`Price updated to ₹${newPrice} per kWh`);
        }
    } catch (error) {
        console.error('❌ Error updating price:', error);
        alert('Failed to update price');
    }
}

// Change chart time range
function changeTimeRange(range) {
    chartTimeRange = range;
    
    // Update button states
    document.querySelectorAll('.time-btn').forEach(btn => {
        btn.classList.remove('active');
    });
    event.target.classList.add('active');
    
    loadHistoricalData();
}

// Switch tabs
function switchTab(tabName) {
    document.querySelectorAll('.tab').forEach(tab => tab.classList.remove('active'));
    document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
    
    event.target.classList.add('active');
    document.getElementById(tabName + 'Tab').classList.add('active');
}

// Load historical data
async function loadHistoricalData() {
    try {
        let url;
        
        switch(chartTimeRange) {
            case 'hour':
                url = `${API_URL}/history?hours=1`;
                break;
            case 'day':
                url = `${API_URL}/history?hours=24`;
                break;
            case 'week':
                url = `${API_URL}/history?days=7`;
                break;
            case 'month':
                url = `${API_URL}/history?days=30`;
                break;
            default:
                url = `${API_URL}/history?hours=24`;
        }
        
        const response = await fetch(url);
        
        if (response.ok) {
            const data = await response.json();
            console.log(`✅ Loaded ${data.length} historical records`);
            
            // Clear charts
            powerChart.data.labels = [];
            powerChart.data.datasets.forEach(dataset => dataset.data = []);
            currentChart.data.labels = [];
            currentChart.data.datasets.forEach(dataset => dataset.data = []);
            voltageChart.data.labels = [];
            voltageChart.data.datasets[0].data = [];
            energyChart.data.labels = [];
            energyChart.data.datasets[0].data = [];
            
            // Add data
            data.forEach(reading => {
                const timeLabel = new Date(reading.timestamp).toLocaleString();
                
                powerChart.data.labels.push(timeLabel);
                powerChart.data.datasets[0].data.push(reading.power1);
                powerChart.data.datasets[1].data.push(reading.power2);
                powerChart.data.datasets[2].data.push(reading.total_power);
                
                currentChart.data.labels.push(timeLabel);
                currentChart.data.datasets[0].data.push(reading.current1);
                currentChart.data.datasets[1].data.push(reading.current2);
                currentChart.data.datasets[2].data.push(reading.total_current);
                
                voltageChart.data.labels.push(timeLabel);
                voltageChart.data.datasets[0].data.push(reading.voltage);

                energyChart.data.labels.push(timeLabel);
                energyChart.data.datasets[0].data.push(reading.total_energy || 0);
            });
            
            powerChart.update();
            currentChart.update();
            voltageChart.update();
            energyChart.update();
            
            fetchStatistics();
        }
    } catch (error) {
        console.error('❌ Error loading historical data:', error);
    }
}

// Initialize application
function init() {
    console.log('⚡ Initializing Energy Meter Dashboard v2.0...');
    
    initCharts();
    loadHistoricalData();
    fetchLatestData();
    
    setInterval(fetchLatestData, UPDATE_INTERVAL);
    setInterval(fetchStatistics, 60000);
    
    console.log('✅ Dashboard initialized successfully');
}

document.addEventListener('DOMContentLoaded', init);