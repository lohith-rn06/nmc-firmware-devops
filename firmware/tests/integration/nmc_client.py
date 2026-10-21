"""Small dependency-free protocol clients (HTTP, SNMP v2c, Modbus TCP) for tests."""
import base64
import json
import socket
import struct
import urllib.error
import urllib.request

from conftest import HOST, HTTP_PORT, MODBUS_PORT, SNMP_PORT


# ---------------------------------------------------------------- HTTP
def http(method, path, user=None, password=None):
    req = urllib.request.Request(f"http://{HOST}:{HTTP_PORT}{path}", method=method, data=b"" if method == "POST" else None)
    if user is not None:
        req.add_header("Authorization", "Basic " + base64.b64encode(f"{user}:{password}".encode()).decode())
    try:
        with urllib.request.urlopen(req, timeout=5) as r:
            return r.status, dict(r.headers), r.read().decode()
    except urllib.error.HTTPError as e:
        return e.code, dict(e.headers), e.read().decode()


def http_json(method, path, **kw):
    code, headers, body = http(method, path, **kw)
    return code, json.loads(body) if body.strip().startswith(("{", "[")) else body


# ---------------------------------------------------------------- SNMP (BER)
def _tlv(tag, val):
    n = len(val)
    if n < 0x80:
        ln = bytes([n])
    elif n < 0x100:
        ln = bytes([0x81, n])
    else:
        ln = bytes([0x82, n >> 8, n & 0xFF])
    return bytes([tag]) + ln + val


def _int(v):
    return _tlv(0x02, v.to_bytes((v.bit_length() + 8) // 8 or 1, "big", signed=True))


def _oid(dotted):
    parts = [int(p) for p in dotted.strip(".").split(".")]
    out = bytearray([parts[0] * 40 + parts[1]])
    for p in parts[2:]:
        chunk = [p & 0x7F]
        p >>= 7
        while p:
            chunk.insert(0, (p & 0x7F) | 0x80)
            p >>= 7
        out += bytes(chunk)
    return _tlv(0x06, bytes(out))


def _read_tlv(buf, pos):
    tag, ln = buf[pos], buf[pos + 1]
    pos += 2
    if ln & 0x80:
        nb = ln & 0x7F
        ln = int.from_bytes(buf[pos:pos + nb], "big")
        pos += nb
    return tag, buf[pos:pos + ln], pos + ln


def _decode_oid(v):
    parts = [v[0] // 40, v[0] % 40]
    acc = 0
    for b in v[1:]:
        acc = (acc << 7) | (b & 0x7F)
        if not b & 0x80:
            parts.append(acc)
            acc = 0
    return ".".join(map(str, parts))


def snmp(oid, community, pdu=0xA0, timeout=2.0):
    """Returns (oid, tag, value) or None when the agent stays silent."""
    vb = _tlv(0x30, _tlv(0x30, _oid(oid) + b"\x05\x00"))
    body = _int(7) + _int(0) + _int(0) + vb
    msg = _tlv(0x30, _int(1) + _tlv(0x04, community.encode()) + _tlv(pdu, body))
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(timeout)
    try:
        s.sendto(msg, (HOST, SNMP_PORT))
        data, _ = s.recvfrom(2048)
    except socket.timeout:
        return None
    finally:
        s.close()
    _, seq, _ = _read_tlv(data, 0)
    p = 0
    _, _, p = _read_tlv(seq, p)          # version
    _, _, p = _read_tlv(seq, p)          # community
    _, pdu_body, _ = _read_tlv(seq, p)
    q = 0
    for _ in range(3):
        _, _, q = _read_tlv(pdu_body, q)
    _, vbs, _ = _read_tlv(pdu_body, q)
    _, one, _ = _read_tlv(vbs, 0)
    _, oidv, r = _read_tlv(one, 0)
    tag, val, _ = _read_tlv(one, r)
    if tag == 0x04:
        val = val.decode()
    elif tag in (0x02, 0x42, 0x43):
        val = int.from_bytes(val, "big", signed=(tag == 0x02))
    return _decode_oid(oidv), tag, val


# ---------------------------------------------------------------- Modbus TCP
def modbus(fc, addr, qty_or_value, tid=1):
    pdu = struct.pack(">BHH", fc, addr, qty_or_value)
    frame = struct.pack(">HHHB", tid, 0, len(pdu) + 1, 1) + pdu
    with socket.create_connection((HOST, MODBUS_PORT), timeout=3) as s:
        s.sendall(frame)
        hdr = s.recv(7)
        _, _, ln, _ = struct.unpack(">HHHB", hdr)
        body = b""
        while len(body) < ln - 1:
            body += s.recv(ln - 1 - len(body))
    if body[0] & 0x80:
        return {"exception": body[1], "fc": body[0]}
    if fc in (3, 4):
        n = body[1] // 2
        return {"registers": list(struct.unpack(f">{n}H", body[2:2 + 2 * n]))}
    return {"raw": body}
