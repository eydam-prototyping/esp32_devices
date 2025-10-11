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
        
        const wifiApStatusDiv = document.getElementById('wifi-ap-status');
        const wifiStaStatusDiv = document.getElementById('wifi-sta-status');

        // Update WiFi Status
        if (data.ap_running) {
            wifiApStatusDiv.innerHTML = `
                <div class="connected">
                    <strong>Access Point running</strong><br>
                    Network: ${data.ap_ssid}<br>
                    Password: ${data.ap_password}<br>
                    ESP32 IP: ${data.ap_ip}
                </div>
            `;
            wifiApStatusDiv.className = 'status-value connected';
        } else {
            wifiApStatusDiv.innerHTML = `
                <div class="disconnected">
                    <strong>Access Point not running</strong><br>
                </div>
            `;
            wifiApStatusDiv.className = 'status-value disconnected';
        } 

        if (data.sta_connected) {
            wifiStaStatusDiv.innerHTML = `
                <div class="connected">
                    <strong>Connected to WiFi</strong><br>
                    Network: ${data.sta_ssid}<br>
                    IP Address: ${data.sta_ip}<br>
                    MAC Address: ${data.sta_mac}<br>
                    Gateway: ${data.sta_gateway}<br>
                    DNS: ${data.sta_dns}
                </div>
            `;
            wifiStaStatusDiv.className = 'status-value connected';
        } else {
            wifiStaStatusDiv.innerHTML = `
                <div class="disconnected">
                    <strong>Not connected</strong><br>
                </div>
            `;
            wifiStaStatusDiv.className = 'status-value disconnected';
        } 
                
    } catch (error) {
        console.error('Error loading WiFi status:', error);
        document.getElementById('wifi-status').innerHTML = 'Error loading status';
    }
}

async function loadDeviceInfo() {
    try {
        // Load device info and WiFi status in parallel
        const [deviceResponse, wifiResponse] = await Promise.all([
            fetch('/api/device/info'),
            fetch('/api/wifi/status')
        ]);
        
        const deviceData = await deviceResponse.json();
        const wifiData = await wifiResponse.json();
        
        deviceInfo = deviceData;
        
        // Update device information display
        document.getElementById('chip-type').textContent = deviceData.chip || 'Unknown';
        document.getElementById('cpu-cores').textContent = deviceData.cores || 'Unknown';
        document.getElementById('flash-size').textContent = deviceData.flash_size || 'Unknown';
        document.getElementById('free-heap').textContent = `${deviceData.free_heap || 'Unknown'} bytes`;
        document.getElementById('mac-address').textContent = wifiData.sta_mac || 'Unknown';
        
        // Update uptime in both device info and status
        if (deviceData.uptime) {
            const uptimeHours = Math.floor(deviceData.uptime / 3600);
            const uptimeMinutes = Math.floor((deviceData.uptime % 3600) / 60);
            const uptimeSeconds = deviceData.uptime % 60;
            const uptimeText = `${uptimeHours}h ${uptimeMinutes}m ${uptimeSeconds}s`;
            
            document.getElementById('uptime').textContent = uptimeText;
        }
        
    } catch (error) {
        console.error('Error loading device info:', error);
        document.getElementById('chip-type').textContent = 'Error loading';
        document.getElementById('cpu-cores').textContent = 'Error loading';
        document.getElementById('flash-size').textContent = 'Error loading';
        document.getElementById('free-heap').textContent = 'Error loading';
        document.getElementById('mac-address').textContent = 'Error loading';
    }
}

async function toggleLed() {
    try {
        const response = await fetch('/api/led/toggle', { method: 'POST' });
        const data = await response.json();
        console.log('LED toggled:', data);
        alert('LED toggled successfully');
    } catch (error) {
        console.error('Error toggling LED:', error);
        alert('Error toggling LED');
    }
}

async function restartDevice() {
    if (confirm('Are you sure you want to restart the device?')) {
        try {
            await fetch('/api/device/restart', { method: 'POST' });
            alert('Device restarting...');
        } catch (error) {
            console.error('Error restarting device:', error);
            alert('Error restarting device');
        }
    }
}

function refreshStatus() {
    loadWifiStatus();
    loadDeviceInfo();
    alert('Status refreshed');
}