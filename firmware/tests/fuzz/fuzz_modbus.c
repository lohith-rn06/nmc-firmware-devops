/* libFuzzer harness for the Modbus TCP frame parser. */
#include "modbus_tcp.h"
#include "nmc.h"
#include "ups_model.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static int init;
    if (!init) {
        ups_init();
        init = 1;
    }
    uint8_t out[300];
    modbus_process(data, size, out, sizeof out);
    return 0;
}
