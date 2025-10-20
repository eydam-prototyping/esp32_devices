// ESP32 Device Dashboard JavaScript

let deviceInfo = {};
let wifiInfo = {};
let memoryUpdateInterval = null;
let statusUpdateInterval = null;

// Initialize dashboard on page load
document.addEventListener('DOMContentLoaded', function() {
    initializeDashboard();
});

async function initializeDashboard() {
    console.log('🚀 Initializing ESP32 Dashboard...');
    
    // Load initial data
    await loadDeviceInfo();
    await loadWifiStatus();
    
    // Start real-time memory monitoring (every 1 second)
    memoryUpdateInterval = setInterval(updateMemoryInfo, 1000);
    
    // Update other status info every 5 seconds
    statusUpdateInterval = setInterval(() => {
        loadWifiStatus();
        updateSystemPerformance();
        updatePartitionsAndTasks();
    }, 5000);
    
    console.log('✅ Dashboard initialized successfully');
}

// Memory monitoring with 1-second updates
async function updateMemoryInfo() {
    try {
        // Get memory info and storage info in parallel
        const [memResponse, storageResponse] = await Promise.all([
            fetch('/api/device/meminfo'),
            fetch('/api/device/storageinfo')
        ]);
        
        const memData = await memResponse.json();
        const storageData = await storageResponse.json();
        
        updateMemoryDisplay(memData, storageData);
        
        // Update live indicator
        updateLastRefresh();
        
    } catch (error) {
        console.error('Error updating memory info:', error);
    }
}

function updateMemoryDisplay(memData, storageData) {
    // Heap memory from meminfo
    if (memData) {
        const heapUsed = memData.total_heap - memData.free_heap;
        const heapUsagePercent = (heapUsed / memData.total_heap * 100).toFixed(1);
        
        document.getElementById('free-heap').textContent = formatBytes(memData.free_heap);
        document.getElementById('used-heap').textContent = formatBytes(heapUsed);
        document.getElementById('total-heap').textContent = formatBytes(memData.total_heap);
        document.getElementById('min-free-heap').textContent = formatBytes(memData.largest_free_block);
        
        // Update memory bar
        const heapFill = document.getElementById('heap-fill');
        const heapPercentage = document.getElementById('heap-percentage');
        
        heapFill.style.width = heapUsagePercent + '%';
        heapPercentage.textContent = heapUsagePercent + '%';
        
        // Color coding for memory usage
        heapFill.className = 'memory-fill';
        if (heapUsagePercent > 80) {
            heapFill.classList.add('danger');
        } else if (heapUsagePercent > 60) {
            heapFill.classList.add('warning');
        }
    }
    
    // Flash storage from storageinfo
    if (storageData && storageData.flash_size) {
        // Calculate flash usage from partitions
        let totalPartitionSize = 0;
        if (storageData.partitions) {
            storageData.partitions.forEach(partition => {
                totalPartitionSize += partition.size;
            });
        }
        
        const flashUsed = totalPartitionSize;
        const flashFree = storageData.flash_size - flashUsed;
        const flashUsagePercent = (flashUsed / storageData.flash_size * 100).toFixed(1);
        
        document.getElementById('flash-total').textContent = formatBytes(storageData.flash_size);
        document.getElementById('flash-used').textContent = formatBytes(flashUsed);
        document.getElementById('flash-free').textContent = formatBytes(flashFree);
        
        const flashFill = document.getElementById('flash-fill');
        const flashPercentage = document.getElementById('flash-percentage');
        
        flashFill.style.width = flashUsagePercent + '%';
        flashPercentage.textContent = flashUsagePercent + '%';
        
        flashFill.className = 'storage-fill';
        if (flashUsagePercent > 80) {
            flashFill.classList.add('danger');
        } else if (flashUsagePercent > 60) {
            flashFill.classList.add('warning');
        }
    }
    
    // SPIFFS storage from storageinfo
    if (storageData && storageData.spiffs_total) {
        const spiffsUsed = storageData.spiffs_used;
        const spiffsFree = storageData.spiffs_total - storageData.spiffs_used;
        const spiffsUsagePercent = (spiffsUsed / storageData.spiffs_total * 100).toFixed(1);
        
        document.getElementById('spiffs-total').textContent = formatBytes(storageData.spiffs_total);
        document.getElementById('spiffs-used').textContent = formatBytes(spiffsUsed);
        document.getElementById('spiffs-free').textContent = formatBytes(spiffsFree);
        
        const spiffsFill = document.getElementById('spiffs-fill');
        const spiffsPercentage = document.getElementById('spiffs-percentage');
        
        spiffsFill.style.width = spiffsUsagePercent + '%';
        spiffsPercentage.textContent = spiffsUsagePercent + '%';
        
        spiffsFill.className = 'storage-fill';
        if (spiffsUsagePercent > 80) {
            spiffsFill.classList.add('danger');
        } else if (spiffsUsagePercent > 60) {
            spiffsFill.classList.add('warning');
        }
    } else if (storageData && storageData.spiffs_status === "not_mounted") {
        // SPIFFS not mounted
        document.getElementById('spiffs-total').textContent = 'N/A';
        document.getElementById('spiffs-used').textContent = 'N/A';
        document.getElementById('spiffs-free').textContent = 'Not mounted';
        
        const spiffsFill = document.getElementById('spiffs-fill');
        const spiffsPercentage = document.getElementById('spiffs-percentage');
        
        spiffsFill.style.width = '0%';
        spiffsPercentage.textContent = 'N/A';
    }
}

async function loadWifiStatus() {
    try {
        const response = await fetch('/api/wifi/status');
        const data = await response.json();
        wifiInfo = data;
        
        // Update AP status
        const apDot = document.getElementById('ap-dot');
        const apStatusText = document.getElementById('ap-status-text');
        const apSsid = document.getElementById('ap-ssid');
        const apIp = document.getElementById('ap-ip');
        
        if (data.ap_running) {
            apDot.className = 'status-dot connected';
            apStatusText.textContent = 'Running';
            apSsid.textContent = data.ap_ssid || 'ESP32_AP';
            apIp.textContent = data.ap_ip || '192.168.4.1';
        } else {
            apDot.className = 'status-dot';
            apStatusText.textContent = 'Stopped';
            apSsid.textContent = 'N/A';
            apIp.textContent = 'N/A';
        }
        
        // Update STA status
        const staDot = document.getElementById('sta-dot');
        const staStatusText = document.getElementById('sta-status-text');
        const staSsid = document.getElementById('sta-ssid');
        const staIp = document.getElementById('sta-ip');
        const staSignal = document.getElementById('sta-signal');
        
        if (data.sta_connected && data.sta_has_ip) {
            staDot.className = 'status-dot connected';
            staStatusText.textContent = 'Connected';
            staSsid.textContent = data.sta_ssid || 'Unknown';
            staIp.textContent = data.sta_ip || 'Unknown';
            staSignal.textContent = 'Connected'; // RSSI not available in status endpoint
        } else if (data.sta_connected) {
            staDot.className = 'status-dot warning';
            staStatusText.textContent = 'Connected (No IP)';
            staSsid.textContent = data.sta_ssid || 'Unknown';
            staIp.textContent = 'No IP';
            staSignal.textContent = 'No Signal';
        } else {
            staDot.className = 'status-dot';
            staStatusText.textContent = 'Disconnected';
            staSsid.textContent = 'N/A';
            staIp.textContent = 'N/A';
            staSignal.textContent = 'N/A';
        }
        
    } catch (error) {
        console.error('Error loading WiFi status:', error);
        updateErrorState('wifi');
    }
}

async function loadDeviceInfo() {
    try {
        // Load device info and memory info
        const [deviceResponse, memResponse] = await Promise.all([
            fetch('/api/device/sysinfo'),
            fetch('/api/device/meminfo')
        ]);
        
        const deviceData = await deviceResponse.json();
        const memData = await memResponse.json();
        
        deviceInfo = deviceData;
        
        // Update device model and hardware info
        document.getElementById('device-model').textContent = deviceData.device_model || 'Unknown';
        document.getElementById('cpu-cores').textContent = deviceData.chip_info?.cores || 'Unknown';
        document.getElementById('cpu-freq').textContent = deviceData.cpu_frequency_mhz ? `${deviceData.cpu_frequency_mhz} MHz` : 'Unknown';
        document.getElementById('chip-revision').textContent = deviceData.chip_info?.revision || 'Unknown';
        document.getElementById('reset-reason').textContent = formatResetReason(deviceData.reset_reason);
        
        // Features
        if (deviceData.features && Array.isArray(deviceData.features)) {
            document.getElementById('chip-features').textContent = deviceData.features.join(', ') || 'None';
        }
        
        // Software info
        document.getElementById('firmware-version').textContent = deviceData.firmware_version || 'Unknown';
        document.getElementById('sdk-version').textContent = deviceData.sdk_version || 'Unknown';
        
        // Git info
        if (deviceData.git_info) {
            document.getElementById('git-branch').textContent = deviceData.git_info.branch || 'Unknown';
            document.getElementById('git-commit').textContent = deviceData.git_info.commit ? deviceData.git_info.commit.substring(0, 8) : 'Unknown';
            document.getElementById('build-time').textContent = deviceData.git_info.build_timestamp || 'Unknown';
        }
        
        // MAC addresses
        if (deviceData.mac_addresses) {
            document.getElementById('mac-wifi-sta').textContent = deviceData.mac_addresses.wifi_sta || 'Unknown';
            document.getElementById('mac-wifi-ap').textContent = deviceData.mac_addresses.wifi_ap || 'Unknown';
            document.getElementById('mac-bluetooth').textContent = deviceData.mac_addresses.bluetooth || 'N/A';
            document.getElementById('mac-base').textContent = deviceData.mac_addresses.base_mac || 'Unknown';
        }
        
        // System performance from memory info
        if (memData.uptime_seconds) {
            document.getElementById('uptime').textContent = formatUptime(memData.uptime_seconds);
        }
        
        updateSystemPerformance();
        
        // Load partitions and tasks initially
        await updatePartitionsAndTasks();
        
    } catch (error) {
        console.error('Error loading device info:', error);
        updateErrorState('device');
    }
}

function updateSystemPerformance() {
    // CPU temperature (not available in ESP-IDF v5.5)
    document.getElementById('cpu-temp').textContent = 'N/A (ESP-IDF v5.5)';
}

// Update partitions and tasks information
async function updatePartitionsAndTasks() {
    try {
        // Get storage info and memory info in parallel
        const [storageResponse, memResponse] = await Promise.all([
            fetch('/api/device/storageinfo'),
            fetch('/api/device/meminfo')
        ]);
        
        const storageData = await storageResponse.json();
        const memData = await memResponse.json();
        
        updatePartitionsDisplay(storageData);
        updateTasksDisplay(memData);
        
    } catch (error) {
        console.error('Error updating partitions and tasks:', error);
    }
}

function updatePartitionsDisplay(storageData) {
    // Update summary info
    document.getElementById('running-partition').textContent = storageData.running_partition || 'Unknown';
    document.getElementById('flash-mode').textContent = storageData.flash_mode || 'Unknown';
    document.getElementById('flash-speed').textContent = storageData.flash_speed || 'Unknown';
    
    // Update partitions table
    const partitionsList = document.getElementById('partitions-list');
    partitionsList.innerHTML = '';
    
    if (storageData.partitions && storageData.partitions.length > 0) {
        storageData.partitions.forEach(partition => {
            const row = document.createElement('div');
            row.className = 'table-row';
            
            row.innerHTML = `
                <div class="table-cell partition-name">${partition.label}</div>
                <div class="table-cell partition-type">${partition.type}</div>
                <div class="table-cell partition-size">${formatBytes(partition.size)}</div>
                <div class="table-cell partition-address">0x${partition.address.toString(16).toUpperCase()}</div>
            `;
            
            partitionsList.appendChild(row);
        });
    } else {
        partitionsList.innerHTML = '<div class="loading-row">No partitions found</div>';
    }
}

function updateTasksDisplay(memData) {
    // Update task summary
    if (memData.num_tasks !== undefined) {
        document.getElementById('total-tasks').textContent = memData.num_tasks;
    }
    
    // Count tasks by state and update summary
    let runningCount = 0, blockedCount = 0, readyCount = 0;
    
    const tasksList = document.getElementById('tasks-list');
    tasksList.innerHTML = '';
    
    if (memData.tasks && memData.tasks.length > 0) {
        memData.tasks.forEach(task => {
            // Count task states
            switch(task.state) {
                case 'running': runningCount++; break;
                case 'blocked': blockedCount++; break;
                case 'ready': readyCount++; break;
            }
            
            const row = document.createElement('div');
            row.className = 'table-row';
            
            const stackWatermark = task.highwatermark ? `${task.highwatermark} bytes` : 'N/A';
            
            row.innerHTML = `
                <div class="table-cell task-name">${task.name}</div>
                <div class="table-cell task-state ${task.state}">${task.state}</div>
                <div class="table-cell task-priority">${task.priority}</div>
                <div class="table-cell task-stack">${stackWatermark}</div>
            `;
            
            tasksList.appendChild(row);
        });
        
        // Update task state counters
        document.getElementById('running-tasks').textContent = runningCount;
        document.getElementById('blocked-tasks').textContent = blockedCount;
        document.getElementById('ready-tasks').textContent = readyCount;
        
    } else {
        tasksList.innerHTML = '<div class="loading-row">Task info not available (FreeRTOS trace disabled)</div>';
        document.getElementById('running-tasks').textContent = 'N/A';
        document.getElementById('blocked-tasks').textContent = 'N/A';
        document.getElementById('ready-tasks').textContent = 'N/A';
    }
}

// Utility functions
function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    const size = (bytes / Math.pow(k, i)).toFixed(1);
    return `${size} ${sizes[i]}`;
}

function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    
    if (days > 0) {
        return `${days}d ${hours}h ${minutes}m`;
    } else if (hours > 0) {
        return `${hours}h ${minutes}m`;
    } else {
        return `${minutes}m ${seconds % 60}s`;
    }
}

function formatResetReason(reason) {
    const reasonMap = {
        'power_on': 'Power On',
        'external_reset': 'External Reset',
        'software_reset': 'Software Reset',
        'panic_reset': 'Panic Reset',
        'interrupt_watchdog': 'Interrupt WDT',
        'task_watchdog': 'Task WDT',
        'other_watchdog': 'Watchdog',
        'deep_sleep': 'Deep Sleep',
        'brownout': 'Brownout',
        'sdio_reset': 'SDIO Reset'
    };
    return reasonMap[reason] || reason || 'Unknown';
}

function updateLastRefresh() {
    document.getElementById('last-updated').textContent = new Date().toLocaleTimeString();
}

function updateErrorState(component) {
    const errorText = 'Connection Error';
    switch(component) {
        case 'wifi':
            document.getElementById('ap-status-text').textContent = errorText;
            document.getElementById('sta-status-text').textContent = errorText;
            break;
        case 'device':
            document.getElementById('device-model').textContent = errorText;
            document.getElementById('firmware-version').textContent = errorText;
            break;
    }
}

// Action functions
async function toggleLed() {
    try {
        const response = await fetch('/api/led/toggle', { method: 'POST' });
        const data = await response.json();
        console.log('LED toggled:', data);
        
        // Show success message
        showNotification('LED toggled successfully', 'success');
    } catch (error) {
        console.error('Error toggling LED:', error);
        showNotification('Error toggling LED', 'error');
    }
}

async function restartDevice() {
    if (confirm('⚠️ Are you sure you want to restart the device? This will interrupt the connection temporarily.')) {
        try {
            await fetch('/api/device/restart', { method: 'POST' });
            showNotification('Device restarting... Please wait for reconnection.', 'warning');
            
            // Stop intervals during restart
            clearInterval(memoryUpdateInterval);
            clearInterval(statusUpdateInterval);
            
            // Show reconnection message
            setTimeout(() => {
                showNotification('Attempting to reconnect...', 'info');
                // Try to reinitialize after restart
                setTimeout(() => {
                    location.reload();
                }, 10000);
            }, 5000);
            
        } catch (error) {
            console.error('Error restarting device:', error);
            showNotification('Error restarting device', 'error');
        }
    }
}

function refreshStatus() {
    loadWifiStatus();
    loadDeviceInfo();
    showNotification('Status refreshed', 'success');
}

// Simple notification system
function showNotification(message, type = 'info') {
    // Create notification element
    const notification = document.createElement('div');
    notification.className = `notification notification-${type}`;
    notification.textContent = message;
    
    // Style the notification
    Object.assign(notification.style, {
        position: 'fixed',
        top: '20px',
        right: '20px',
        padding: '15px 20px',
        borderRadius: '8px',
        color: 'white',
        fontWeight: '500',
        zIndex: '9999',
        maxWidth: '300px',
        boxShadow: '0 4px 20px rgba(0,0,0,0.3)'
    });
    
    // Color based on type
    const colors = {
        'success': '#28a745',
        'error': '#dc3545',
        'warning': '#ffc107',
        'info': '#17a2b8'
    };
    notification.style.background = colors[type] || colors.info;
    
    // Add to page
    document.body.appendChild(notification);
    
    // Remove after 3 seconds
    setTimeout(() => {
        if (notification.parentNode) {
            notification.parentNode.removeChild(notification);
        }
    }, 3000);
}

// Cleanup on page unload
window.addEventListener('beforeunload', function() {
    if (memoryUpdateInterval) clearInterval(memoryUpdateInterval);
    if (statusUpdateInterval) clearInterval(statusUpdateInterval);
});