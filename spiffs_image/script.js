// ESP32 Device Manager JavaScript

let deviceInfo = {};

// Load device information on page load
document.addEventListener('DOMContentLoaded', function() {
    loadWifiStatus();
    loadDeviceInfo();
    
    // Refresh every 5 seconds
    setInterval(loadWifiStatus, 5000);
});

async function loadWifiStatus() {
    try {
        const response = await fetch('/api/wifi/status');
        const data = await response.json();
        
        const statusDiv = document.getElementById('wifi-status');
        if (data.connected) {
            statusDiv.innerHTML = `
                <div class="connected">
                    <strong>Connected to WiFi</strong><br>
                    IP Address: ${data.ip}<br>
                    SSID: ${data.ssid}
                </div>
            `;
            statusDiv.className = 'connected';
        } else {
            statusDiv.innerHTML = `
                <div class="disconnected">
                    <strong>WiFi Disconnected</strong><br>
                    Access Point Mode Active<br>
                    Connect to: ESP32_AP
                </div>
            `;
            statusDiv.className = 'disconnected';
        }
    } catch (error) {
        console.error('Error loading WiFi status:', error);
        document.getElementById('wifi-status').innerHTML = 'Error loading status';
    }
}

async function loadDeviceInfo() {
    try {
        const response = await fetch('/api/device/info');
        const data = await response.json();
        deviceInfo = data;
        
        document.getElementById('device-info').innerHTML = `
            <strong>Chip:</strong> ${data.chip}<br>
            <strong>Cores:</strong> ${data.cores}<br>
            <strong>Flash Size:</strong> ${data.flash_size}<br>
            <strong>Free Heap:</strong> ${data.free_heap}<br>
            <strong>Uptime:</strong> ${data.uptime} seconds
        `;
    } catch (error) {
        console.error('Error loading device info:', error);
        document.getElementById('device-info').innerHTML = 'Error loading device information';
    }
}

async function toggleLed() {
    try {
        const response = await fetch('/api/led/toggle', { method: 'POST' });
        const data = await response.json();
        console.log('LED toggled:', data);
    } catch (error) {
        console.error('Error toggling LED:', error);
        alert('Error toggling LED');
    }
}

async function restart() {
    if (confirm('Are you sure you want to restart the device?')) {
        try {
            await fetch('/api/device/restart', { method: 'POST' });
            alert('Device restarting...');
        } catch (error) {
            console.error('Error restarting device:', error);
        }
    }
}