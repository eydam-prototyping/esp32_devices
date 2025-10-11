// WiFi Configuration JavaScript

let availableNetworks = [];
let selectedNetwork = null;
let currentModal = null;

// Initialize page
document.addEventListener('DOMContentLoaded', function() {
    loadCurrentStatus();
    loadApConfig();
    loadStaConfig();
    checkForExistingScanResults();
    
    // Setup form handlers
    document.getElementById('wifi-form').addEventListener('submit', handleWifiConnection);
    document.getElementById('ap-form').addEventListener('submit', handleApConfiguration);
    
    // Refresh status every 10 seconds
    setInterval(loadCurrentStatus, 10000);
});

// Check if there are existing scan results when page loads
async function checkForExistingScanResults() {
    try {
        const response = await fetch('/api/wifi/scan/results');
        const data = await response.json();
        
        if (data.success && data.scan_done && data.networks && !data.scanning) {
            availableNetworks = data.networks;
            displayNetworks();
            
            document.getElementById('scan-status').textContent = 
                `Found ${availableNetworks.length} networks (previous scan)`;
                
            // Change button to "Refresh Scan" since we have results
            const scanBtn = document.getElementById('scan-btn');
            scanBtn.innerHTML = '<span class="btn-icon">🔄</span> Refresh Scan';
        }
    } catch (error) {
        console.log('No existing scan results available');
    }
}

// Load current WiFi status
async function loadCurrentStatus() {
    try {
        const response = await fetch('/api/wifi/status');
        const data = await response.json();
        
        updateStatusDisplay(data);
    } catch (error) {
        console.error('Error loading WiFi status:', error);
        showError('Failed to load WiFi status');
    }
}

function updateStatusDisplay(data) {
    const statusElement = document.getElementById('current-status');
    const ssidElement = document.getElementById('current-ssid');
    const rssiElement = document.getElementById('current-rssi');
    const ipElement = document.getElementById('current-ip');
    
    if (data.sta_connected && data.sta_has_ip) {
        statusElement.textContent = 'Connected';
        statusElement.className = 'status-badge connected';
        ssidElement.textContent = data.sta_ssid || 'Unknown';
        rssiElement.textContent = 'N/A'; // RSSI not in status, need separate endpoint
        ipElement.textContent = data.sta_ip || 'Unknown';
    } else if (data.sta_connected) {
        statusElement.textContent = 'Connected (No IP)';
        statusElement.className = 'status-badge disconnected';
        ssidElement.textContent = data.sta_ssid || 'Unknown';
        rssiElement.textContent = 'N/A';
        ipElement.textContent = 'Waiting for IP...';
    } else {
        statusElement.textContent = 'Disconnected';
        statusElement.className = 'status-badge disconnected';
        ssidElement.textContent = 'Not connected';
        rssiElement.textContent = 'N/A';
        
        // Show AP IP if AP is running
        if (data.ap_running) {
            ipElement.textContent = `AP: ${data.ap_ip || '192.168.4.1'}`;
        } else {
            ipElement.textContent = 'No IP';
        }
    }
}

// Network scanning
async function scanNetworks() {
    const scanBtn = document.getElementById('scan-btn');
    const scanStatus = document.getElementById('scan-status');
    const container = document.getElementById('networks-container');
    
    // Check if this is a refresh of existing results
    const isRefresh = scanBtn.textContent.includes('Refresh');
    
    // Update UI for scanning state
    scanBtn.disabled = true;
    scanBtn.innerHTML = '<span class="btn-icon">⏳</span> Starting Scan...';
    scanStatus.textContent = isRefresh ? 'Refreshing scan results...' : 'Initializing WiFi scan...';
    
    try {
        // Start the scan
        const startResponse = await fetch('/api/wifi/scan/start');
        const startData = await startResponse.json();
        
        if (!startData.success) {
            throw new Error(startData.message || 'Failed to start scan');
        }
        
        scanStatus.textContent = 'Scanning for networks...';
        scanBtn.innerHTML = '<span class="btn-icon">🔍</span> Scanning...';
        
        // Poll for results every 2 seconds
        const pollForResults = async () => {
            try {
                const resultsResponse = await fetch('/api/wifi/scan/results');
                const resultsData = await resultsResponse.json();
                
                if (resultsData.success) {
                    if (resultsData.scanning) {
                        // Still scanning, continue polling
                        scanStatus.textContent = 'Scanning in progress...';
                        setTimeout(pollForResults, 2000);
                        
                    } else if (resultsData.scan_done) {
                        // Scan completed
                        availableNetworks = resultsData.networks || [];
                        displayNetworks();
                        
                        scanStatus.textContent = `Found ${availableNetworks.length} networks`;
                        
                        // Change button to "Refresh Scan"
                        scanBtn.disabled = false;
                        scanBtn.innerHTML = '<span class="btn-icon">�</span> Refresh Scan';
                        
                    } else {
                        // No scan done yet, continue polling
                        setTimeout(pollForResults, 2000);
                    }
                } else {
                    throw new Error(resultsData.message || 'Failed to get scan results');
                }
                
            } catch (pollError) {
                throw pollError;
            }
        };
        
        // Start polling after a short delay
        setTimeout(pollForResults, 2000);
        
        // Timeout after 30 seconds
        setTimeout(() => {
            if (scanBtn.disabled) {
                scanBtn.disabled = false;
                scanBtn.innerHTML = '<span class="btn-icon">🔍</span> Scan for Networks';
                scanStatus.textContent = 'Scan timeout - please try again';
            }
        }, 30000);
        
    } catch (error) {
        console.error('Error scanning networks:', error);
        showError('Failed to scan networks: ' + error.message);
        scanStatus.textContent = 'Scan failed - ' + error.message;
        container.innerHTML = '<div class="networks-placeholder"><p>Failed to scan networks. Please try again.</p></div>';
        
        // Reset button state
        scanBtn.disabled = false;
        scanBtn.innerHTML = '<span class="btn-icon">🔍</span> Scan for Networks';
    }
}

function displayNetworks() {
    const container = document.getElementById('networks-container');
    
    if (availableNetworks.length === 0) {
        container.innerHTML = '<div class="networks-placeholder"><p>No networks found. Try scanning again.</p></div>';
        return;
    }
    
    // Sort networks by signal strength
    const sortedNetworks = [...availableNetworks].sort((a, b) => b.rssi - a.rssi);
    
    const networksHtml = sortedNetworks.map((network, index) => {
        const signalBars = getSignalBars(network.rssi);
        const securityIcon = network.authmode === 0 ? '🔓' : '🔒';
        const authModeText = getAuthModeText(network.authmode);
        const channelText = `Ch ${network.channel}`;
        
        return `
            <div class="network-item" onclick="selectNetwork(${index}, '${network.ssid}', '${network.bssid}')">
                <div class="network-info">
                    <div class="network-ssid">${escapeHtml(network.ssid)}</div>
                    <div class="network-details">
                        <span>BSSID: ${network.bssid}</span>
                        <span>${channelText}</span>
                        <span class="signal-strength">
                            Signal: ${network.rssi} dBm
                            <div class="signal-bars">${signalBars}</div>
                        </span>
                        <span class="security-info">
                            ${securityIcon} ${authModeText}
                        </span>
                    </div>
                </div>
            </div>
        `;
    }).join('');
    
    container.innerHTML = networksHtml;
}

function getSignalBars(rssi) {
    const bars = [];
    const strength = Math.max(0, Math.min(4, Math.floor((rssi + 100) / 12.5)));
    
    for (let i = 0; i < 4; i++) {
        const height = (i + 1) * 3;
        const active = i < strength ? 'active' : '';
        bars.push(`<div class="signal-bar ${active}" style="height: ${height}px;"></div>`);
    }
    
    return bars.join('');
}

function getAuthModeText(authmode) {
    const authModes = {
        0: 'Open',
        1: 'WEP',
        2: 'WPA PSK',
        3: 'WPA2 PSK',
        4: 'WPA/WPA2 PSK',
        5: 'WPA2 Enterprise',
        6: 'WPA3 PSK',
        7: 'WPA2/WPA3 PSK'
    };
    return authModes[authmode] || `Unknown (${authmode})`;
}

function selectNetwork(index, ssid, bssid) {
    // Remove previous selection
    document.querySelectorAll('.network-item').forEach(item => {
        item.classList.remove('selected');
    });
    
    // Add selection to clicked item
    event.currentTarget.classList.add('selected');
    
    // Store selected network
    selectedNetwork = availableNetworks.find(n => n.ssid === ssid && n.bssid === bssid);
    
    // Show connection form
    document.getElementById('selected-ssid').value = ssid;
    document.getElementById('selected-bssid').value = bssid;
    document.getElementById('wifi-password').value = '';
    document.getElementById('specific-bssid').checked = false;
    document.getElementById('connection-form').style.display = 'block';
    
    // Scroll to form
    document.getElementById('connection-form').scrollIntoView({ behavior: 'smooth' });
}

function cancelConnection() {
    document.getElementById('connection-form').style.display = 'none';
    selectedNetwork = null;
    
    // Remove selection
    document.querySelectorAll('.network-item').forEach(item => {
        item.classList.remove('selected');
    });
}

// WiFi connection form handler
async function handleWifiConnection(event) {
    event.preventDefault();
    
    if (!selectedNetwork) {
        showError('No network selected');
        return;
    }
    
    const password = document.getElementById('wifi-password').value;
    const specificBssid = document.getElementById('specific-bssid').checked;
    
    // Validate password for secured networks
    if (selectedNetwork.authmode !== 0 && !password.trim()) {
        showError('Password is required for secured networks');
        return;
    }
    
    const connectionData = {
        ssid: selectedNetwork.ssid,
        password: password,
        bssid: specificBssid ? selectedNetwork.bssid : null
    };
    
    showModal(
        'Connect to WiFi',
        `Connect to "${selectedNetwork.ssid}"${specificBssid ? ` (${selectedNetwork.bssid})` : ''}?${selectedNetwork.authmode === 0 ? ' (Open Network)' : ''}`,
        () => performWifiConnection(connectionData)
    );
}

async function performWifiConnection(connectionData) {
    try {
        // Prepare data for the new STA config endpoint
        const staConfigData = {
            ssid: connectionData.ssid,
            password: connectionData.password,
            bssid: connectionData.bssid || null,
            use_specific_bssid: connectionData.bssid ? true : false
        };
        
        const response = await fetch('/api/wifi/config/sta', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(staConfigData)
        });
        
        const result = await response.json();
        
        if (result.success) {
            showSuccess('WiFi configuration saved. Device will attempt to connect...');
            cancelConnection();
            
            // Update stored STA config and display
            window.currentStaConfig = staConfigData;
            showStaConfigDetails();
            
            // Refresh status after a delay
            setTimeout(loadCurrentStatus, 5000);
        } else {
            throw new Error(result.status || 'Configuration failed');
        }
    } catch (error) {
        console.error('Error configuring WiFi:', error);
        showError('Failed to configure WiFi: ' + error.message);
    }
}

// AP Configuration
async function loadApConfig() {
    try {
        const response = await fetch('/api/wifi/config/ap');
        const data = await response.json();
        
        if (data.success) {
            document.getElementById('ap-ssid').value = data.ssid || 'ESP32_AP';
            document.getElementById('ap-password').value = data.password || '';
            document.getElementById('ap-channel').value = data.channel || 6;
            document.getElementById('max-connections').value = data.max_connections || 4;
            
            // Store AP config for display
            window.currentApConfig = {
                ssid: data.ssid || 'ESP32_AP',
                password: data.password || '',
                channel: data.channel || 6,
                max_connections: data.max_connections || 4
            };
            
            // Update config display
            showApConfigDetails();
        } else {
            console.warn('Failed to load AP config:', data.status);
            setDefaultApConfig();
        }
    } catch (error) {
        console.error('Error loading AP config:', error);
        setDefaultApConfig();
    }
}

function setDefaultApConfig() {
    // Set defaults if loading fails
    document.getElementById('ap-ssid').value = 'ESP32_AP';
    document.getElementById('ap-password').value = '';
    document.getElementById('ap-channel').value = 6;
    document.getElementById('max-connections').value = 4;
    
    // Store defaults
    window.currentApConfig = {
        ssid: 'ESP32_AP',
        password: '',
        channel: 6,
        max_connections: 4
    };
    
    showApConfigDetails();
}

// STA Configuration
async function loadStaConfig() {
    try {
        const response = await fetch('/api/wifi/config/sta');
        const data = await response.json();
        
        if (data.success) {
            // Store STA config for display or future use
            window.currentStaConfig = {
                ssid: data.ssid || '',
                password: data.password || '',
                bssid: data.bssid || '',
                use_specific_bssid: data.use_specific_bssid || false
            };
            
            // Update status display if connected to this network
            showStaConfigDetails();
        } else {
            console.warn('Failed to load STA config:', data.status);
            // Set empty config
            window.currentStaConfig = {
                ssid: '',
                password: '',
                bssid: '',
                use_specific_bssid: false
            };
            showStaConfigDetails();
        }
    } catch (error) {
        console.error('Error loading STA config:', error);
        // Set empty config
        window.currentStaConfig = {
            ssid: '',
            password: '',
            bssid: '',
            use_specific_bssid: false
        };
        showStaConfigDetails();
    }
}

function showStaConfigDetails() {
    const config = window.currentStaConfig || { ssid: '', password: '', bssid: '', use_specific_bssid: false };
    
    document.getElementById('saved-sta-ssid').textContent = config.ssid || 'Not configured';
    document.getElementById('saved-sta-password').textContent = 
        config.password ? '••••••••' : 'No password';
    
    let bssidInfo = 'Any access point';
    if (config.use_specific_bssid && config.bssid) {
        bssidInfo = `Specific BSSID: ${config.bssid}`;
    }
    document.getElementById('saved-sta-bssid-info').textContent = bssidInfo;
}

function showApConfigDetails() {
    const config = window.currentApConfig || { ssid: 'ESP32_AP', password: '', channel: 6, max_connections: 4 };
    
    document.getElementById('saved-ap-ssid').textContent = config.ssid;
    document.getElementById('saved-ap-security').textContent = 
        config.password ? `WPA2 PSK (Password protected)` : 'Open (No password)';
    
    // Calculate correct WiFi frequency for 2.4GHz channels
    const frequency = 2412 + (config.channel - 1) * 5;
    document.getElementById('saved-ap-channel').textContent = `${config.channel} (${frequency} MHz)`;
    document.getElementById('saved-ap-max-conn').textContent = `${config.max_connections} devices`;
}

async function handleApConfiguration(event) {
    event.preventDefault();
    
    const apConfig = {
        ssid: document.getElementById('ap-ssid').value,
        password: document.getElementById('ap-password').value,
        channel: parseInt(document.getElementById('ap-channel').value),
        max_connections: parseInt(document.getElementById('max-connections').value)
    };
    
    // Validate
    if (!apConfig.ssid.trim()) {
        showError('AP SSID cannot be empty');
        return;
    }
    
    if (apConfig.password && apConfig.password.length < 8) {
        showError('AP password must be at least 8 characters (or empty for open network)');
        return;
    }
    
    showModal(
        'Save AP Configuration',
        `Save AP configuration with SSID "${apConfig.ssid}"?`,
        () => performApConfiguration(apConfig)
    );
}

async function performApConfiguration(apConfig) {
    try {
        const response = await fetch('/api/wifi/config/ap', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(apConfig)
        });
        
        const result = await response.json();
        
        if (result.success) {
            showSuccess('AP configuration saved successfully');
            
            // Update stored config and display
            window.currentApConfig = apConfig;
            showApConfigDetails();
        } else {
            throw new Error(result.status || 'Configuration failed');
        }
    } catch (error) {
        console.error('Error saving AP config:', error);
        showError('Failed to save AP configuration: ' + error.message);
    }
}

// Advanced functions
async function disconnectWifi() {
    showModal(
        'Disconnect WiFi',
        'Disconnect from current WiFi network?',
        async () => {
            try {
                const response = await fetch('/api/wifi/disconnect', { method: 'POST' });
                const result = await response.json();
                
                if (result.success) {
                    showSuccess('WiFi disconnected');
                    loadCurrentStatus();
                } else {
                    throw new Error(result.message || 'Disconnect failed');
                }
            } catch (error) {
                console.error('Error disconnecting WiFi:', error);
                showError('Failed to disconnect: ' + error.message);
            }
        }
    );
}

async function resetNetworkSettings() {
    showModal(
        'Reset Network Settings',
        'This will reset all WiFi and AP settings to defaults. Are you sure?',
        async () => {
            try {
                const response = await fetch('/api/wifi/reset', { method: 'POST' });
                const result = await response.json();
                
                if (result.success) {
                    showSuccess('Network settings reset. Device will restart...');
                    setTimeout(() => {
                        window.location.reload();
                    }, 3000);
                } else {
                    throw new Error(result.message || 'Reset failed');
                }
            } catch (error) {
                console.error('Error resetting network settings:', error);
                showError('Failed to reset: ' + error.message);
            }
        }
    );
}

// Utility functions
function togglePasswordVisibility(inputId) {
    const input = document.getElementById(inputId);
    const button = input.nextElementSibling;
    
    if (input.type === 'password') {
        input.type = 'text';
        button.textContent = '🙈';
    } else {
        input.type = 'password';
        button.textContent = '👁️';
    }
}

function showModal(title, message, confirmCallback) {
    document.getElementById('modal-title').textContent = title;
    document.getElementById('modal-message').textContent = message;
    document.getElementById('modal').style.display = 'flex';
    
    currentModal = confirmCallback;
}

function closeModal() {
    document.getElementById('modal').style.display = 'none';
    currentModal = null;
}

function confirmAction() {
    if (currentModal) {
        currentModal();
    }
    closeModal();
}

function showError(message) {
    // You can implement a toast notification system here
    alert('Error: ' + message);
}

function showSuccess(message) {
    // You can implement a toast notification system here
    alert('Success: ' + message);
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}