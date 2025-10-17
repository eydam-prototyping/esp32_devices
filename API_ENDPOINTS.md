# ESP32 Device Manager - API Endpunkte

Diese Dokumentation beschreibt alle API-Endpunkte, die von der ESP32 Device Manager Webseite erwartet werden.

## WiFi Status & Management

### `GET /api/wifi/status`
**Beschreibung:** Gibt den aktuellen WiFi-Status zurück  
**Response Format:**
```json
{
  "ap_running": true,
  "ap_ssid": "ESP32_AP",
  "ap_password": "mypassword",
  "ap_ip": "192.168.4.1",
  "sta_connected": false,
  "sta_ssid": "MyNetwork",
  "sta_password": "networkpass",
  "sta_has_ip": false,
  "sta_ip": "192.168.1.100",
  "sta_mac": "aa:bb:cc:dd:ee:ff",
  "sta_gateway": "192.168.1.1",
  "sta_netmask": "255.255.255.0",
  "sta_dns": "8.8.8.8"
}
```
**Hinweise:**
- `ap_running`: Ob der Access Point aktiv ist
- `sta_connected`: Ob eine Station-Verbindung besteht
- `sta_has_ip`: Ob die Station eine IP-Adresse erhalten hat
- `sta_mac`: MAC-Adresse der Station
- Netzwerk-Details nur verfügbar wenn `sta_has_ip` true ist
- Einige Felder können Platzhalter-Werte enthalten (z.B. "Your_SSID")
- RSSI (Signalstärke) ist in diesem Endpunkt nicht verfügbar - siehe WiFi-Scan für Details

### `GET /api/wifi/scan/start`
**Beschreibung:** Startet einen WiFi-Netzwerk-Scan (asynchron)  
**Response Format:**
```json
{
  "success": true,
  "message": "Scan started"
}
```

### `GET /api/wifi/scan/results`
**Beschreibung:** Gibt die Ergebnisse des letzten WiFi-Scans zurück  
**Response Format:**
```json
{
  "success": true,
  "scanning": false,
  "scan_done": true,
  "networks": [
    {
      "ssid": "NetworkName",
      "bssid": "aa:bb:cc:dd:ee:ff",
      "rssi": -45,
      "channel": 6,
      "authmode": 3
    }
  ]
}
```
**Scan-Status:**
- `"scanning": true` - Scan läuft gerade
- `"scanning": false, "scan_done": true` - Scan abgeschlossen, Ergebnisse verfügbar
- `"scanning": false, "scan_done": false` - Kein Scan gestartet oder Ergebnisse abgeholt

**Hinweise:**
- `authmode`: 0 = Open, 1+ = Secured
- `rssi`: Signalstärke in dBm
- `bssid`: MAC-Adresse des Access Points
- Polling-Empfehlung: Alle 2 Sekunden nach `/api/wifi/scan/results` fragen
- Scan dauert typischerweise 5-10 Sekunden

### `POST /api/wifi/connect`
**Beschreibung:** Verbindet mit einem WiFi-Netzwerk  
**Request Body:**
```json
{
  "ssid": "NetworkName",
  "password": "password123",
  "bssid": "aa:bb:cc:dd:ee:ff"
}
```
**Response Format:**
```json
{
  "success": true,
  "message": "Connection initiated"
}
```
**Hinweise:**
- `bssid` ist optional. Wenn angegeben, verbindet nur zu diesem spezifischen AP
- Wenn `bssid` nicht angegeben, verbindet zu jedem AP mit der angegebenen SSID

### `POST /api/wifi/config/sta`
**Beschreibung:** Konfiguriert die Station (Client) WiFi-Einstellungen  
**Request Body:**
```json
{
  "ssid": "NetworkName",
  "password": "password123",
  "bssid": "aa:bb:cc:dd:ee:ff",
  "use_specific_bssid": true
}
```
**Response Format:**
```json
{
  "success": true,
  "status": "wifi_config_changed"
}
```
**Hinweise:**
- `ssid`: Erforderlich, darf nicht leer sein
- `password`: Erforderlich (kann leer sein für offene Netzwerke)
- `bssid`: Optional, MAC-Adresse des spezifischen Access Points
- `use_specific_bssid`: Optional, boolean (Standard: false)

### `POST /api/wifi/disconnect`
**Beschreibung:** Trennt die aktuelle WiFi-Verbindung  
**Response Format:**
```json
{
  "success": true,
  "message": "WiFi disconnected"
}
```

### `POST /api/wifi/reset`
**Beschreibung:** Setzt alle Netzwerk-Einstellungen zurück  
**Response Format:**
```json
{
  "success": true,
  "message": "Network settings reset"
}
```

## Access Point Konfiguration

### `POST /api/wifi/config/ap`
**Beschreibung:** Konfiguriert die Access Point Einstellungen  
**Request Body:**
```json
{
  "ssid": "ESP32_AP",
  "password": "mypassword",
  "channel": 6,
  "max_connections": 4
}
```
**Response Format:**
```json
{
  "success": true,
  "status": "wifi_config_changed"
}
```
**Hinweise:**
- `ssid`: Erforderlich, darf nicht leer sein
- `password`: Kann leer sein für offenes Netzwerk (min. 8 Zeichen wenn gesetzt)
- `channel`: Optional, 1-11 (Standard: 6)
- `max_connections`: Optional, 1-10 (Standard: 4)

### `GET /api/ap/config`
**Beschreibung:** Gibt die aktuelle AP-Konfiguration zurück  
**Response Format:**
```json
{
  "success": true,
  "ssid": "ESP32_AP",
  "password": "mypassword",
  "channel": 6,
  "max_connections": 4
}
```

## Device Information & Control

### `GET /api/device/info`
**Beschreibung:** Gibt Device-Informationen zurück  
**Response Format:**
```json
{
  "chip": "ESP32",
  "cores": 2,
  "flash_size": "4MB",
  "free_heap": 234567,
  "uptime": 12345,
  "mac_address": "aa:bb:cc:dd:ee:ff"
}
```

### `POST /api/led/toggle`
**Beschreibung:** Schaltet die LED um  
**Response Format:**
```json
{
  "success": true,
  "led_state": true
}
```

### `POST /api/device/restart`
**Beschreibung:** Startet das Device neu  
**Response Format:**
```json
{
  "success": true,
  "message": "Device restarting"
}
```

## Error Handling

Alle API-Endpunkte sollten bei Fehlern folgendes Format zurückgeben:
```json
{
  "success": false,
  "message": "Error description"
}
```

## Statische Dateien

Die folgenden statischen Dateien werden von der Webseite angefordert:
- `/` oder `/index.html` - Dashboard
- `/wifi.html` - WiFi Konfiguration
- `/style.css` - CSS Styles
- `/script.js` - Dashboard JavaScript
- `/wifi.js` - WiFi JavaScript

## HTTP Status Codes

- `200 OK` - Erfolgreiche Anfrage
- `404 Not Found` - Endpoint oder Datei nicht gefunden
- `400 Bad Request` - Ungültige Anfrage
- `500 Internal Server Error` - Server-Fehler

## Implementierungs-Hinweise

1. **CORS:** Für lokale Entwicklung eventuell CORS-Header setzen
2. **Content-Type:** JSON-Responses sollten `application/json` verwenden
3. **Timeouts:** WiFi-Scan kann 5-10 Sekunden dauern
4. **Buffering:** Netzwerk-Liste kann groß werden (bis zu 20+ Netzwerke)
5. **Persistence:** AP-Konfiguration sollte im NVS gespeichert werden