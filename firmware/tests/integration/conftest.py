"""Integration tests run against a live NMC (simulator container, k8s pod or a real card on the HIL rack).
Target is chosen with environment variables so the same suite runs everywhere."""
import os
import socket
import time

import pytest

HOST = os.getenv("NMC_HOST", "127.0.0.1")
HTTP_PORT = int(os.getenv("NMC_HTTP_PORT", "8080"))
SNMP_PORT = int(os.getenv("NMC_SNMP_PORT", "1161"))
MODBUS_PORT = int(os.getenv("NMC_MODBUS_PORT", "1502"))
COMMUNITY = os.getenv("NMC_SNMP_COMMUNITY", "lab-ro")
USER = os.getenv("NMC_ADMIN_USER", "admin")
PASSWORD = os.getenv("NMC_ADMIN_PASS", "Test#Pass1")
VARIANT = os.getenv("NMC_VARIANT", "")
PCNS_LISTEN_PORT = int(os.getenv("PCNS_LISTEN_PORT", "3052"))


@pytest.fixture(scope="session", autouse=True)
def wait_for_card():
    deadline = time.time() + 30
    while time.time() < deadline:
        try:
            with socket.create_connection((HOST, HTTP_PORT), timeout=1):
                return
        except OSError:
            time.sleep(0.5)
    pytest.exit(f"NMC at {HOST}:{HTTP_PORT} not reachable", returncode=2)
