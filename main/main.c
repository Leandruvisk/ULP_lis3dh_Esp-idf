#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "ulp_riscv.h"
#include "ulp_riscv_i2c.h"

#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static const char *TAG = "MAX30102";

static void init_i2c(void)
{
    /* Use RTC I2C pins valid for ESP32-S3 ULP:
     * SDA must be GPIO1 or GPIO3
     * SCL must be GPIO0 or GPIO2
     * Here we choose SDA=GPIO3 and SCL=GPIO2.
     * Do not wire SDA to GPIO2 and SCL to GPIO3; isso é invertido e inválido para ULP I2C.
     */
    ulp_riscv_i2c_cfg_t cfg = {
        .i2c_pin_cfg = {
            .sda_io_num = GPIO_NUM_3,
            .scl_io_num = GPIO_NUM_2,
            .sda_pullup_en = true,
            .scl_pullup_en = true,
        },
        .i2c_timing_cfg = {
            .scl_low_period = 1.4,
            .scl_high_period = 0.3,
            .sda_duty_period = 1,
            .scl_start_period = 2,
            .scl_stop_period = 1.3,
            .i2c_trans_timeout = 20,
        },
    };

    ESP_ERROR_CHECK(
        ulp_riscv_i2c_master_init(&cfg)
    );

    ulp_riscv_i2c_master_set_slave_addr(0x57);
}

static void max30102_init(void)
{
    uint8_t data;

    ulp_riscv_i2c_master_set_slave_reg_addr(0x08);
    data = (2 << 5);
    ulp_riscv_i2c_master_write_to_device(&data, 1);

    ulp_riscv_i2c_master_set_slave_reg_addr(0x09);
    data = 0x03;
    ulp_riscv_i2c_master_write_to_device(&data, 1);

    ulp_riscv_i2c_master_set_slave_reg_addr(0x0A);
    data = (3 << 5) | (3 << 2) | 3;
    ulp_riscv_i2c_master_write_to_device(&data, 1);

    ulp_riscv_i2c_master_set_slave_reg_addr(0x0C);
    data = 0xD0;
    ulp_riscv_i2c_master_write_to_device(&data, 1);

    ulp_riscv_i2c_master_set_slave_reg_addr(0x0D);
    data = 0xA0;
    ulp_riscv_i2c_master_write_to_device(&data, 1);
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