from nmc_client import modbus

MODEL_IDS = {1: "GVS", 2: "GVL", 3: "GVXL", 4: "GVXL-NAM"}


def test_read_all_holding_registers():
    r = modbus(3, 0, 20)["registers"]
    assert r[0] in (2, 3, 4)               # status
    assert all(2000 < v < 3000 for v in r[4:7])   # output V x10
    assert r[19] in MODEL_IDS


def test_input_registers_same_map():
    assert modbus(4, 18, 2)["registers"][1] in MODEL_IDS


def test_illegal_address_exception():
    assert modbus(3, 19, 5) == {"exception": 2, "fc": 0x83}
