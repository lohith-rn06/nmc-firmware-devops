# Runs the protocol-level integration suite (HTTP, SNMP, Modbus, PCNS, security) against any NMC.
FROM python:3.12-slim
RUN useradd -m -u 1000 runner
WORKDIR /tests
COPY firmware/tests/integration/requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY firmware/tests/integration/ .
RUN mkdir -p /tests/reports && chown runner /tests/reports
USER runner
EXPOSE 3052
ENTRYPOINT ["pytest"]
