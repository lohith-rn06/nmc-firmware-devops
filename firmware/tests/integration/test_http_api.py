from conftest import VARIANT
from nmc_client import http, http_json


def test_health():
    code, _, body = http("GET", "/healthz")
    assert code == 200 and body.strip() == "ok"


def test_version_reports_variant_and_features():
    code, data = http_json("GET", "/api/version")
    assert code == 200
    assert {"snmp", "modbus", "http", "pcns", "security"} <= set(data["features"])
    if VARIANT:
        assert data["model"] == VARIANT


def test_status_is_three_phase():
    code, s = http_json("GET", "/api/status")
    assert code == 200
    assert len(s["output_voltage"]) == 3 and all(v > 200 for v in s["output_voltage"])
    assert s["status"] in ("ONLINE", "ON_BATTERY", "BYPASS")
    assert 0 <= s["battery_pct"] <= 100


def test_nominal_frequency_matches_region():
    _, s = http_json("GET", "/api/status")
    expected = 60 if s["model"] == "GVXL-NAM" else 50
    assert abs(s["output_frequency"] - expected) < 0.5
