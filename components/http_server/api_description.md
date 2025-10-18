# API Description

## Wifi

### Status

Request: `GET /api/wifi/status`
Headers: 

    * don't care

Response: JSON Object
Headers: 

    * `Content-Type: application/json`

Body:
```json
{
  "ap_running": true,
  "ap_ssid": "<AP-SSID>",               // only if AP is running
  "ap_password": "<AP-Password>",       // only if AP is running
  "ap_ip": "<ESP32_IP>",                // only if AP is running
  "ap_netmask": "<Netmask>",            // only if AP is running
  "ap_gateway": "<Gateway-IP>",         // only if AP is running
  "ap_mac": "<STA-MAC>",                // only if AP is running
  "ap_dns": "<STA-DNS>",                // only if AP is running
  "ap_hostname": "<STA-HOSTNAME>",      // only if AP is running
  "sta_connected": true,
  "sta_ssid": "<STA-SSID>",             // only if STA is connected
  "sta_password": "<STA-Password>",     // only if STA is connected and BUILD_TYPE == DEVELOP
  "sta_has_ip": true,                   // only if STA is connected
  "sta_rssi": -60,                      // only if STA is connected
  "sta_ip": "<ESP32_IP>",               // only if STA is connected
  "sta_netmask": "<Netmask>",           // only if STA is connected
  "sta_gateway": "<Gateway-IP>",        // only if STA is connected
  "sta_mac": "<STA-MAC>",               // only if STA is connected
  "sta_dns": "<STA-DNS>",               // only if STA is connected
  "sta_hostname": "<STA-HOSTNAME>"      // only if STA is connected
}
```

### AP-Config

#### Request: `GET /api/wifi/config/ap`

Headers: 

    * don't care

Response: JSON Object
Headers: 

    * `Content-Type: application/json`

Body:
```json
{
  "error": "<Error message>",           // only if error occured
  "ssid": "<AP-SSID>",
  "password": "<AP-Password>",
  "channel": 6,
  "max_connections": 4
}
```

#### Request: `POST /api/wifi/config/ap`

Headers: 

    * don't care

Body: 
```json
{
  "ssid": "<AP-SSID>",
  "password": "<AP-Password>",
  "channel": 6,                         // optional
  "max_connections": 4                  // optional
}
```

Response: JSON Object
Headers: 

    * `Content-Type: application/json`

Body:

```json
{
  "success": true,
  "error": "<Error message>",           // only if error occured
}
```


#### STA-Config

Request: `GET /api/wifi/config/sta`
Headers: 

    * don't care

Response: JSON Object
Headers: 

    * `Content-Type: application/json`

Body:
```json
{
  "error": "<Error message>",           // only if error occured
  "ssid": "<STA-SSID>",
  "password": "<STA-Password>",         // only if BUILD_TYPE == DEVELOP
  "channel": 6,
  "max_connections": 4
}
```