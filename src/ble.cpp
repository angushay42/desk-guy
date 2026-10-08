#include <ble.hpp>

const char *TAG = "BLE";


static void on_reset(int reason);
static void on_sync(void);
static void on_gatt_register(struct ble_gatt_register_ctxt *ctxt, void *arg);
static int on_store_status(struct ble_store_status_event *ctxt, void *arg);


BLE::BLE() :
    m_gatt_server{GATT()}
{ }

BLE::~BLE() 
{ }

int BLE::init() 
{
    int rc;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nimble_port_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to init NimBLE %d ", ret);
        return (rc = 1);
    }

    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb  = on_sync;
    ble_hs_cfg.gatts_register_cb = on_gatt_register;
    ble_hs_cfg.store_status_cb = on_store_status;

    ble_hs_cfg.sm_io_cap = 3;   // https://github.com/espressif/esp-idf/blob/v6.1/examples/bluetooth/nimble/bleprph/tutorial/bleprph_walkthrough.md#security-manager-configuration:~:text=for%20example%20configurations.-,I/O%20CAPABILITIES,-ble_hs_cfg.sm_io_cap

    rc = GATT_init();
    assert(rc == 0);

    return rc;
}

int GATT::init(void)
{
    ble_svc_gap_init();
    ble_svc_gatt_init();

    //unsure if this is needed: Alert Notification Server. Can't find much info on it either
    // ble_svc_ans_init();
    int rc;
    if ((rc = ble_gatts_count_cfg(m_gatt_services)) != 0)
        return rc;

    if ((rc = ble_gatts_add_svcs(m_gatt_services)) != 0)
        return rc;


    
    return 0;
}

static void on_reset(int reason)
{

}

static void on_sync(void)
{

}

static void on_gatt_register(struct ble_gatt_register_ctxt *ctxt, void *arg)
{


}

static int on_store_status(struct ble_store_status_event *ctxt, void *arg)
{
    return 0;
}