from conftest import COMMUNITY
from nmc_client import snmp

SYS_DESCR = "1.3.6.1.2.1.1.1.0"
UPS_OUTPUT_SOURCE = "1.3.6.1.2.1.33.1.4.1.0"
UPS_OUTPUT_VOLTAGE_L1 = "1.3.6.1.2.1.33.1.4.4.1.2.1"


def test_sysdescr():
    oid, tag, val = snmp(SYS_DESCR, COMMUNITY)
    assert oid == SYS_DESCR and tag == 0x04 and val.startswith("NMC ")


def test_ups_mib_output_voltage():
    _, tag, val = snmp(UPS_OUTPUT_VOLTAGE_L1, COMMUNITY)
    assert tag == 0x02 and 200 < val < 300


def test_output_source_normal_or_battery():
    _, _, val = snmp(UPS_OUTPUT_SOURCE, COMMUNITY)
    assert val in (3, 4, 5)


def test_walk_ups_mib_returns_many_objects():
    oid, seen = "1.3.6.1.2.1.33", 0
    while True:
        r = snmp(oid, COMMUNITY, pdu=0xA1)
        if r is None or not r[0].startswith("1.3.6.1.2.1.33") or r[1] == 0x82:
            break
        oid, seen = r[0], seen + 1
    assert seen >= 20
