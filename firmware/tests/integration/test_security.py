"""Security acceptance tests - these map to IEC 62443-4-2 component requirements."""
from conftest import PASSWORD, USER
from nmc_client import http, http_json, modbus, snmp


def test_config_requires_auth():                     # CR 1.1 human user identification
    assert http("GET", "/api/config")[0] == 401


def test_config_never_leaks_secrets():               # CR 4.1 information confidentiality
    code, body = http_json("GET", "/api/config", user=USER, password=PASSWORD)
    assert code == 200
    text = str(body).lower()
    assert PASSWORD.lower() not in text and "hash" not in text and "community" not in text


def test_security_headers_present():
    _, headers, _ = http("GET", "/api/status")
    h = {k.lower(): v for k, v in headers.items()}
    assert h["x-frame-options"] == "DENY"
    assert h["x-content-type-options"] == "nosniff"
    assert "content-security-policy" in h


def test_default_public_community_rejected():        # no well-known defaults
    assert snmp("1.3.6.1.2.1.1.1.0", "public", timeout=1.0) is None


def test_modbus_writes_disabled_by_default():        # CR 2.1 authorization enforcement
    assert modbus(6, 100, 1) == {"exception": 1, "fc": 0x86}


def test_failed_logins_are_audited():                # CR 2.8 auditable events
    http("GET", "/api/config", user=USER, password="definitely-wrong")
    code, events = http_json("GET", "/api/audit", user=USER, password=PASSWORD)
    assert code == 200
    assert any(e["action"] == "login" and e["result"] == "failure" for e in events)
