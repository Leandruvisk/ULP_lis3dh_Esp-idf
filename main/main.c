#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include "ulp_riscv.h"
#include "ulp_riscv_i2c.h"

#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static const char *TAG = "MAX30102";

static esp_err_t max30102_write_reg(uint8_t reg, uint8_t value)
{
    ulp_riscv_i2c_master_set_slave_reg_addr(reg);
    return ulp_riscv_i2c_master_write_to_device(&value, 1);
}

static esp_err_t max30102_read_reg(uint8_t reg, uint8_t *value)
{
    ulp_riscv_i2c_master_set_slave_reg_addr(reg);
    return ulp_riscv_i2c_master_read_from_device(value, 1);
}

static void init_i2c(void)
{
    ulp_riscv_i2c_cfg_t cfg = ULP_RISCV_I2C_DEFAULT_CONFIG();

    cfg.i2c_pin_cfg.sda_io_num = GPIO_NUM_3;
    cfg.i2c_pin_cfg.scl_io_num = GPIO_NUM_2;

    ESP_ERROR_CHECK(ulp_riscv_i2c_master_init(&cfg));
    ulp_riscv_i2c_master_set_slave_addr(0x57);

    ESP_LOGI(TAG, "RTC I2C initialized (SDA=GPIO3, SCL=GPIO2)");
}

static void max30102_init(void)
{
    ESP_ERROR_CHECK(max30102_write_reg(0x08, (2 << 5)));
    ESP_ERROR_CHECK(max30102_write_reg(0x09, 0x03));
    ESP_ERROR_CHECK(max30102_write_reg(0x0A, (3 << 5) | (3 << 2) | 3));
    ESP_ERROR_CHECK(max30102_write_reg(0x0C, 0xD0));
    ESP_ERROR_CHECK(max30102_write_reg(0x0D, 0xA0));

    ESP_LOGI(TAG, "MAX30102 initialized");
}

static void init_ulp(void)
{
    ESP_ERROR_CHECK(
        ulp_riscv_load_binary(
            ulp_main_bin_start,
            ulp_main_bin_end - ulp_main_bin_start
        )
    );

    /*
     * 10ms = 100Hz
     */
    ulp_set_wakeup_period(0, 10000);

    ESP_ERROR_CHECK(
        ulp_riscv_run()
    );
}

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    init_i2c();

    max30102_init();

    {
        uint8_t part_id = 0;
        esp_err_t err = max30102_read_reg(0xFF, &part_id);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "MAX30102 part ID = 0x%02X", part_id);
        } else {
            ESP_LOGE(TAG, "MAX30102 probe failed: %s", esp_err_to_name(err));
        }
    }

    init_ulp();

    while (1) {

        if (ulp_ulp_new_data) {

            ulp_ulp_new_data = 0;

            ESP_LOGI(
                TAG,
                "IR=%ld RED=%ld BPM=%ld",
                ulp_ulp_ir_value,
                ulp_ulp_red_value,
                ulp_ulp_bpm
            );
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}