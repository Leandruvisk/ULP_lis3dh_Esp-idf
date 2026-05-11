
#include <stdio.h>
#include <string.h>
#include "lis3dh.h"
#include "esp_log.h"

const char *TAG = "HOST_MAIN";


int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
    uint8_t data[len + 1];
    data[0] = reg;
    memcpy(&data[1], bufp, len);

    return i2c_master_write_to_device(I2C_MASTER_NUM, LIS3DH_ADDR, data, len + 1, 100 / portTICK_PERIOD_MS);
}

int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, LIS3DH_ADDR, &reg, 1, bufp, len, 100 / portTICK_PERIOD_MS);
}

void platform_delay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void lis3dh_config_low_power(stmdev_ctx_t *dev_ctx)
{
    uint8_t who;
    lis3dh_device_id_get(dev_ctx, &who);
    ESP_LOGI(TAG, "WHO_AM_I = 0x%02X", who);

    lis3dh_block_data_update_set(dev_ctx, PROPERTY_ENABLE);

    // low power
    lis3dh_operating_mode_set(dev_ctx, LIS3DH_LP_8bit);
    lis3dh_data_rate_set(dev_ctx, LIS3DH_ODR_10Hz);

    // 🔥 HIGH PASS FILTER (ESSENCIAL)
    lis3dh_high_pass_on_outputs_set(dev_ctx, PROPERTY_ENABLE);
    lis3dh_high_pass_int_conf_set(dev_ctx, LIS3DH_ON_INT1_GEN);

    // interrupt config
    lis3dh_int1_cfg_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.xhie = 1;
    cfg.yhie = 1;
    cfg.zhie = 1;

    lis3dh_int1_gen_conf_set(dev_ctx, &cfg);

    // threshold REAL
    lis3dh_int1_gen_threshold_set(dev_ctx, 15);

    // duração maior
    lis3dh_int1_gen_duration_set(dev_ctx, 2);

    // map INT1
    lis3dh_ctrl_reg3_t ctrl3 = {0};
    ctrl3.i1_ia1 = 1;
    lis3dh_pin_int1_config_set(dev_ctx, &ctrl3);

    // pulso
    lis3dh_int1_pin_notification_mode_set(dev_ctx, LIS3DH_INT1_PULSED);

    ESP_LOGI(TAG, "LIS3DH low-power wake configured");
}

// ---------- READ QUICK DATA ----------
void read_accel_once(stmdev_ctx_t *dev_ctx)
{
    int16_t raw[3];
    lis3dh_acceleration_raw_get(dev_ctx, raw);

    float x = lis3dh_from_fs2_hr_to_mg(raw[0]);
    float y = lis3dh_from_fs2_hr_to_mg(raw[1]);
    float z = lis3dh_from_fs2_hr_to_mg(raw[2]);

    ESP_LOGI(TAG, "ACC [mg]: %.2f %.2f %.2f", x, y, z);
}