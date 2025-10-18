import pytest
import time
import requests
from helper import device_ip

def test_wifi_scan():
    response = requests.get(f"http://{device_ip}/api/wifi/scan/results", timeout=3)
    data = response.json()
    
    assert "scanning" in data
    assert data["scanning"] == False

    assert "scan_done" in data
    assert data["scan_done"] == False

    assert "status" in data
    assert type(data["status"]) is str
    assert data["status"] == "no_scan_results_available"

    response = requests.get(f"http://{device_ip}/api/wifi/scan/start", timeout=3)
    data = response.json()

    assert "status" in data
    assert type(data["status"]) is str
    assert data["status"] == "scan_started"

    for i in range(10):
        response = requests.get(f"http://{device_ip}/api/wifi/scan/results", timeout=3)
        data = response.json()
        if data["scanning"] == False and data["scan_done"] == True:
            break
        time.sleep(1)

    assert "scanning" in data
    assert data["scanning"] == False
    assert "scan_done" in data
    assert data["scan_done"] == True
    assert "status" in data
    assert type(data["status"]) is str
    assert data["status"] == "scan_completed"

    assert "networks" in data
    assert type(data["networks"]) is list

    for network in data["networks"]:
        assert "ssid" in network
        assert type(network["ssid"]) is str
        assert "bssid" in network
        assert type(network["bssid"]) is str
        assert "channel" in network
        assert type(network["channel"]) is int
        assert "rssi" in network
        assert type(network["rssi"]) is int
        assert "authmode" in network
        assert type(network["authmode"]) is int