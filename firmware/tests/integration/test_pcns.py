"""PCNS: simulate a mains failure and check the registered shutdown client is notified."""
import json
import socket
import threading

from conftest import PASSWORD, PCNS_LISTEN_PORT, USER
from nmc_client import http_json


def test_on_battery_notifies_pcns_client():
    received = []
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("0.0.0.0", PCNS_LISTEN_PORT))
    srv.listen(1)
    srv.settimeout(10)

    def accept():
        try:
            c, _ = srv.accept()
            received.append(c.recv(1024).decode())
            c.close()
        except socket.timeout:
            pass

    t = threading.Thread(target=accept)
    t.start()
    try:
        code, body = http_json("POST", "/api/simulate/onbattery", user=USER, password=PASSWORD)
        assert code == 200, body
        t.join(12)
        assert received, "PCNS client was not notified"
        event = json.loads(received[0])
        assert event["event"] == "ON_BATTERY"
        assert event["action"] in ("PREPARE_SHUTDOWN", "SHUTDOWN_NOW")
    finally:
        http_json("POST", "/api/simulate/online", user=USER, password=PASSWORD)
        srv.close()
