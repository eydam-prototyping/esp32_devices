import requests

device_ip = "192.168.10.66"


def set_value_verify_and_restore(new_values, url):
    # Get current config
    response = requests.get(url, timeout=3)
    data = response.json()
    old_values = {key: data[key] for key in new_values.keys()}

    # Set new value
    response = requests.post(url, json=new_values, timeout=3)
    data = response.json()

    assert "success" in data
    assert data["success"] is True

    # Verify the change
    response = requests.get(url, timeout=3)
    data = response.json()
    for key in new_values.keys():
        assert data[key] == new_values[key]

    # Restore old value
    payload = {key: old_values[key] for key in new_values.keys()}
    response = requests.post(url, json=payload, timeout=3)
    data = response.json()

    assert "success" in data
    assert data["success"] is True
