import pytest
import requests
from helper import device_ip, set_value_verify_and_restore

def test_wifi_config_sta():
    response = requests.get(f"http://{device_ip}/api/wifi/config/sta", timeout=3)
    data = response.json()

    assert "ssid" in data
    assert type(data["ssid"]) is str
    assert "password" in data
    assert type(data["password"]) is str
    assert "hostname" in data
    assert type(data["hostname"]) is str
    assert "bssid" in data
    assert type(data["bssid"]) is str
    assert "use_specific_bssid" in data
    assert type(data["use_specific_bssid"]) is bool


@pytest.mark.parametrize("data", [
    ({"ssid": "TestSTA_SSID"}),
    ({"password": "TestSTA_Password"}),
    ({"hostname": "TestSTA_Hostname"}),
    ({"bssid": "12:34:56:78:9A:BC"}),
    ({"use_specific_bssid": True})
    ])
def test_wifi_config_set_sta_data(data):
    url = f"http://{device_ip}/api/wifi/config/sta"
    set_value_verify_and_restore(data, url)


@pytest.mark.parametrize("data, error_message", [
    ({"ssid": "A"*33},"SSID too long"),
    ({"ssid": ""},"SSID empty"),
    ({"ssid": None},"SSID null"),
    ({"ssid": 123},"SSID not a string")
    ])
def test_wifi_config_set_sta_ssid_faulty(data, error_message):
    response = requests.post(f"http://{device_ip}/api/wifi/config/sta", json=data, timeout=3)
    data = response.json()

    assert "success" in data
    assert data["success"] is False


@pytest.mark.parametrize("data, error_message", [
    ({"password": "B"*65},"Password too long"),
    ({"password": None},"Password null"),
    ({"password": 123},"Password not a string")
    ])
def test_wifi_config_set_sta_password_faulty(data, error_message):
    response = requests.post(f"http://{device_ip}/api/wifi/config/sta", json=data, timeout=3)
    data = response.json()

    assert "success" in data
    assert data["success"] is False


@pytest.mark.parametrize("data, error_message", [
    ({"hostname": "B"*65},"Hostname too long"),
    ({"hostname": None},"Hostname null"),
    ({"hostname": 123},"Hostname not a string")
    ])
def test_wifi_config_set_sta_hostname_faulty(data, error_message):
    response = requests.post(f"http://{device_ip}/api/wifi/config/sta", json=data, timeout=3)
    data = response.json()

    assert "success" in data
    assert data["success"] is False


@pytest.mark.parametrize("data, error_message", [
    ({"bssid": "invalid_bssid"},"BSSID not a valid MAC address"),
    ({"bssid": 123},"BSSID not a string")
    ])
def test_wifi_config_set_sta_bssid_faulty(data, error_message):
    response = requests.post(f"http://{device_ip}/api/wifi/config/sta", json=data, timeout=3)
    data = response.json()

    assert "success" in data
    assert data["success"] is False

@pytest.mark.parametrize("data, error_message", [
    ({"use_specific_bssid": None},"Use specific BSSID null"),
    ({"use_specific_bssid": "not_a_bool"},"Use specific BSSID not a boolean"),
    ])
def test_wifi_config_set_sta_use_specific_bssid_faulty(data, error_message):
    response = requests.post(f"http://{device_ip}/api/wifi/config/sta", json=data, timeout=3)
    data = response.json()

    assert "success" in data
    assert data["success"] is False
