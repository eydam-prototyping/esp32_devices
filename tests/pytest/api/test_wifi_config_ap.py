import pytest
import requests
from helper import device_ip, set_value_verify_and_restore


def test_wifi_config_ap():
    response = requests.get(f"http://{device_ip}/api/wifi/config/ap", timeout=3)
    data = response.json()

    assert "ssid" in data
    assert type(data["ssid"]) is str
    assert "password" in data
    assert type(data["password"]) is str
    assert "channel" in data
    assert type(data["channel"]) is int
    assert "max_connections" in data
    assert type(data["max_connections"]) is int
    assert "hostname" in data
    assert type(data["hostname"]) is str


@pytest.mark.parametrize("data", [
    ({"ssid": "TestAP_SSID"}),
    ({"password": "TestAP_Password"}),
    ({"channel": 6}),
    ({"max_connections": 3}),
    ({"hostname": "TestAP_Hostname"})
    ])
def test_wifi_config_set_ap_data(data):
    url = f"http://{device_ip}/api/wifi/config/ap"
    set_value_verify_and_restore(data, url)


@pytest.mark.parametrize("data, error_message", [
    ({"ssid": "A"*33},"SSID too long"),
    ({"ssid": ""},"SSID empty"),
    ({"ssid": None},"SSID null"),
    ({"ssid": 123},"SSID not a string")
    ])
def test_wifi_config_set_ap_ssid_faulty(data, error_message):
    response = requests.post(f"http://{device_ip}/api/wifi/config/ap", json=data, timeout=3)
    data = response.json()

    assert "success" in data
    assert data["success"] is False


@pytest.mark.parametrize("data, error_message", [
    ({"password": "B"*65},"Password too long"),
    ({"password": None},"Password null"),
    ({"password": 123},"Password not a string")
    ])
def test_wifi_config_set_ap_password_faulty(data, error_message):
    response = requests.post(f"http://{device_ip}/api/wifi/config/ap", json=data, timeout=3)
    data = response.json()

    assert "success" in data
    assert data["success"] is False


@pytest.mark.parametrize("data, error_message", [
    ({"hostname": "B"*65},"Hostname too long"),
    ({"hostname": None},"Hostname null"),
    ({"hostname": 123},"Hostname not a string")
    ])
def test_wifi_config_set_ap_hostname_faulty(data, error_message):
    response = requests.post(f"http://{device_ip}/api/wifi/config/ap", json=data, timeout=3)
    data = response.json()

    assert "success" in data
    assert data["success"] is False


@pytest.mark.parametrize("data, error_message", [
    ({"channel": None},"Channel null"),
    ({"channel": "1"},"Channel not a number"),
    ({"channel": -1},"Channel less than 0"),
    ({"channel": 12},"Channel greater than 11")
    ])
def test_wifi_config_set_ap_channel_faulty(data, error_message):
    response = requests.post(f"http://{device_ip}/api/wifi/config/ap", json=data, timeout=3)
    data = response.json()

    assert "success" in data
    assert data["success"] is False

@pytest.mark.parametrize("data, error_message", [
    ({"max_connections": None},"Max connections null"),
    ({"max_connections": "1"},"Max connections not a number"),
    ({"max_connections": 0},"Max connections less than 1"),
    ({"max_connections": 11},"Max connections greater than 10")
    ])
def test_wifi_config_set_ap_max_connections_faulty(data, error_message):
    response = requests.post(f"http://{device_ip}/api/wifi/config/ap", json=data, timeout=3)
    data = response.json()

    assert "success" in data
    assert data["success"] is False
