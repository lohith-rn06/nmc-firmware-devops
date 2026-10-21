# MIBs supported by the NMC

| MIB | OID root | Notes |
|---|---|---|
| MIB-II system | 1.3.6.1.2.1.1 | sysDescr, sysUpTime, sysName |
| UPS-MIB (RFC 1628) | 1.3.6.1.2.1.33 | ident, battery, input, output groups |

Try it against a running simulator:
```bash
snmpwalk -v2c -c lab-ro 127.0.0.1:1161 1.3.6.1.2.1.33
snmpget  -v2c -c lab-ro 127.0.0.1:1161 1.3.6.1.2.1.33.1.4.4.1.2.1   # output voltage L1
```
Get the RFC 1628 MIB text with `apt install snmp-mibs-downloader`; it is not vendored here.
