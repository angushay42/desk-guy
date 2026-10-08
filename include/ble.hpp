#pragma once

extern "C" {
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nvs_flash.h"
#include "esp_err.h"

#include "modlog/modlog.h"
#include "nimble/nimble_port.h"
#include "host/ble_hs_id.h"
#include "host/ble_hs.h"

#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/ans/ble_svc_ans.h"

#include "assert.h"
}

// forward declare
class BLE;
class GATT;


// chr = characteristic

class BLE {
public:
    BLE();
    ~BLE();

    int init();

private:
    GATT m_gatt_server;
};


class GATT {
public:
    GATT();
    ~GATT();

    int init();

    const ble_uuid128_t m_gatt_service_uuid;
    uint8_t m_gatt_chr_val;
    uint16_t m_gatt_svr_chr_val_handle;
    

    struct ble_gatt_svc_def m_gatt_services[];

private:
    void createUUID128(uint64_t first, uint64_t second, ble_uuid128_t &dest) 
    {
        uint64_t *temp;

        for (size_t i = 0; i < 16; i++)
        {
            temp = &((i > 7) ? first : second);
            dest.value[i] = *temp & ((1U << 8) - 1);
            *temp <<= 8;
        }
    }
};