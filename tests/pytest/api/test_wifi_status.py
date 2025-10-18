import pytest
import requests
from helper import device_ip
import logging
logger = logging.getLogger()


@pytest.mark.parametrize("uri", [
    ("/api/wifi/status"),
    ("/api/wifi/scan/results"),
    ("/api/wifi/config/ap"),
    ("/api/wifi/config/sta"),
    ("/api/device/info")
    ])
def test_status_code_200(uri):
    logger.info("Test case: status code 200")
    response = requests.get(f"http://{device_ip}{uri}", timeout=3)
    assert response.status_code == 200
    assert response.encoding == "utf-8"
    assert response.headers["Content-Type"] == "application/json"

def test_wifi_status():
    response = requests.get(f"http://{device_ip}/api/wifi/status", timeout=3)
    data = response.json()

    assert "ap_running" in data
    assert type(data["ap_running"]) is bool
    if data["ap_running"]:
        assert "ap_ssid" in data
        assert type(data["ap_ssid"]) is str
        assert "ap_password" in data
        assert type(data["ap_password"]) is str
        assert "ap_channel" in data
        assert type(data["ap_channel"]) is int
        assert "ap_ip" in data
        assert type(data["ap_ip"]) is str
        assert "ap_netmask" in data
        assert type(data["ap_netmask"]) is str
        assert "ap_gateway" in data
        assert type(data["ap_gateway"]) is str
        assert "ap_mac" in data
        assert type(data["ap_mac"]) is str
        assert "ap_hostname" in data
        assert type(data["ap_hostname"]) is str

    assert "sta_connected" in data
    assert type(data["sta_connected"]) is bool
    if data["sta_connected"]:
        assert "sta_ssid" in data
        assert type(data["sta_ssid"]) is str
        assert "sta_password" in data
        assert type(data["sta_password"]) is str
        assert "sta_has_ip" in data
        assert type(data["sta_has_ip"]) is bool
        assert "sta_mac" in data
        assert type(data["sta_mac"]) is str
        assert "sta_rssi" in data
        assert type(data["sta_rssi"]) is int
        assert "sta_ip" in data
        assert type(data["sta_ip"]) is str
        assert "sta_gateway" in data
        assert type(data["sta_gateway"]) is str
        assert "sta_netmask" in data
        assert type(data["sta_netmask"]) is str
        assert "sta_mac" in data
        assert type(data["sta_mac"]) is str
        assert "sta_dns" in data
        assert type(data["sta_dns"]) is str
        assert "sta_hostname" in data
        assert type(data["sta_hostname"]) is str
