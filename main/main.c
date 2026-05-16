#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_pm.h"

#include "driver/i2c.h"
#include "lis3dh_reg.h"

#include "driver/rtc_io.h"
#include "ulp_riscv.h"
#include "ulp_riscv_i2c.h"

#include "ulp_main.h" 

#include "protocols/i2c_drivers.h"
#include "sensors/lis3dh.h"

static const char *TAG = "HOST_MAIN";

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

void app_main(void)
{
    i2c_init();
    max30102_init();

    stmdev_ctx_t dev_ctx;
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg  = platform_read;
    dev_ctx.mdelay    = platform_delay;

    lis3dh_config_low_power(&dev_ctx);

    // limpa interrupção pendente
    lis3dh_int1_src_t dummy;
    lis3dh_int1_gen_source_get(&dev_ctx, &dummy);

    // carrega ULP
    ESP_ERROR_CHECK(ulp_riscv_load_binary(
        ulp_main_bin_start,
        (ulp_main_bin_end - ulp_main_bin_start)));

    // acorda ULP a cada 200 ms
    ulp_set_wakeup_period(0, 200000);

    // inicia ULP
    ESP_ERROR_CHECK(ulp_riscv_run());

    ESP_LOGI(TAG, "Entering deep sleep");

    esp_sleep_enable_ulp_wakeup();

    esp_deep_sleep_start();
}
