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

// chr = characteristic

static constexpr uint8_t to_hex(const char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'a') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

class GATT {
public:
    GATT();
    ~GATT();

    int init();

    const ble_uuid128_t m_gatt_service_uuid;
    const ble_uuid128_t m_gatt_chr1_uuid;
    const ble_uuid128_t m_gatt_dsc_uuid;

    uint8_t m_gatt_chr_val;
    uint16_t m_gatt_svr_chr_val_handle;
    

    struct ble_gatt_svc_def m_gatt_services[];

private:
    constexpr ble_uuid128_t createUUID128(const char *uuid_abnf) 
    {
        ble_uuid128_t ret = {};
        ret.u.type = BLE_UUID_TYPE_128;

        size_t count = 0, i = 36;
        // string is 32 hex + 4 hyphen + nul, 37
        while (i-- > 0)
        {
            const char c = uuid_abnf[i];
            if (c == '-') continue;
            const uint8_t dig = to_hex(c);
            ret.value[count / 2] |= static_cast<uint8_t>((count % 2 == 0) ? dig : (dig << 4));
            count++;    // character found
        }
        return ret;
    }
};


class BLE {
public:
    BLE();
    ~BLE();

    int init();

private:
    GATT m_gatt_server;
};



