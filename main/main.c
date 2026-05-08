#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_pm.h"

// Drivers I2C Padrão e LIS3DH
#include "driver/i2c.h"
#include "lis3dh_reg.h"

// Drivers para o ULP
#include "driver/rtc_io.h"
#include "ulp_riscv.h"
#include "ulp_riscv_i2c.h"

// Header gerado automaticamente no build do ULP
#include "ulp_main.h" 

static const char *TAG = "HOST_MAIN";

// ================= CONFIG =================
#define I2C_MASTER_SDA_IO 3
#define I2C_MASTER_SCL_IO 2
#define I2C_MASTER_NUM    I2C_NUM_0
#define I2C_FREQ_HZ       100000
#define LIS3DH_ADDR       0x19
#define LIS3DH_INT1_GPIO  4
// =========================================

// ---------- I2C Normal Init ----------

// Importa as referências do binário do ULP geradas pelo linker
extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

void i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// ---------- I2C wrappers ----------
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
    uint8_t data[len + 1];
    data[0] = reg;
    memcpy(&data[1], bufp, len);
    return i2c_master_write_to_device(I2C_MASTER_NUM, LIS3DH_ADDR, data, len + 1, 100 / portTICK_PERIOD_MS);
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, LIS3DH_ADDR, &reg, 1, bufp, len, 100 / portTICK_PERIOD_MS);
}

static void platform_delay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

// ---------- LIS3DH CONFIG ----------
void lis3dh_config_low_power(stmdev_ctx_t *dev_ctx)
{
    uint8_t who;
    lis3dh_device_id_get(dev_ctx, &who);
    ESP_LOGI(TAG, "WHO_AM_I = 0x%02X", who);
    lis3dh_block_data_update_set(dev_ctx, PROPERTY_ENABLE);

    lis3dh_operating_mode_set(dev_ctx, LIS3DH_LP_8bit);
    lis3dh_data_rate_set(dev_ctx, LIS3DH_ODR_10Hz);
    
    // ========= CONFIGURAÇÃO DE INT1 PARA DETECÇÃO DE MOVIMENTO ==========
    // Configura INT1 para gerar interrupção quando há movimento
    lis3dh_int1_cfg_t int1_cfg = {
        .xlie = 1,   // X low interrupt
        .xhie = 1,   // X high interrupt
        .ylie = 1,   // Y low interrupt
        .yhie = 1,   // Y high interrupt
        .zlie = 1,   // Z low interrupt
        .zhie = 1,   // Z high interrupt
        .aoi = 0,    // 0 = OR (qualquer eixo), 1 = AND (todos os eixos)
        ._6d = 0     // 6D disabled
    };
    lis3dh_int1_gen_conf_set(dev_ctx, &int1_cfg);
    
    // Define threshold para INT1 (LSb = 16 mg em 2g range)
    // Valor 32 = 512 mg de threshold
    lis3dh_int1_gen_threshold_set(dev_ctx, 32);
    
    // Define duration (duração mínima da interrupção em unidades de 1/ODR)
    // Com 10 Hz ODR, duration=1 = 100ms
    lis3dh_int1_gen_duration_set(dev_ctx, 1);
    
    // Roteia INT1_GEN para o pino INT1 (via CTRL_REG3)
    lis3dh_ctrl_reg3_t ctrl_reg3 = {0};
    lis3dh_pin_int1_config_get(dev_ctx, &ctrl_reg3);
    ctrl_reg3.i1_ia1 = 1;  // Roteia Interrupt Generator 1 para INT1
    lis3dh_pin_int1_config_set(dev_ctx, &ctrl_reg3);
    
    // Define modo latched (interrupção permanece até ler INT1_SRC)
    lis3dh_int1_pin_notification_mode_set(dev_ctx, LIS3DH_INT1_LATCHED);
    
    ESP_LOGI(TAG, "LIS3DH INT1 configurado para detectar movimento");
    ESP_LOGI(TAG, "LIS3DH low-power configurado via main_core");
}

// ---------- RTC I2C CONFIG (PARA O ULP) ----------
static void init_ulp_rtc_i2c(void)
{
    printf("Inicializando RTC I2C ...\n");
    ulp_riscv_i2c_cfg_t i2c_cfg = ULP_RISCV_I2C_DEFAULT_CONFIG();
    
    // Configura pinos (Ajuste para GPIO 2 e 3 conforme exigência do hardware)
    i2c_cfg.i2c_pin_cfg.sda_io_num = 3; 
    i2c_cfg.i2c_pin_cfg.scl_io_num = 2;
    i2c_cfg.i2c_pin_cfg.scl_pullup_en = true;
    i2c_cfg.i2c_pin_cfg.sda_pullup_en = true;

    esp_err_t ret = ulp_riscv_i2c_master_init(&i2c_cfg);
    ESP_ERROR_CHECK(ret);
}

// ---------- MAIN ----------
void set_low_power(void)
{
    // No ESP-IDF v5, usamos esp_pm_config_t em vez de esp_pm_config_esp32_t
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 40,
        .min_freq_mhz = 40,
        .light_sleep_enable = false
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
}

void app_main(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        ESP_LOGI(TAG, "Cold boot: Iniciando configuracao do acelerometro...");

        // 1. Inicia I2C normal do ESP32 para configurar os registradores do LIS3DH
        i2c_init();
        
        stmdev_ctx_t dev_ctx;
        dev_ctx.write_reg = platform_write;
        dev_ctx.read_reg  = platform_read;
        dev_ctx.mdelay    = platform_delay;
        
        lis3dh_config_low_power(&dev_ctx);

        // IMPORTANTE: Liberar os pinos para que o RTC I2C possa assumi-los
        ESP_LOGI(TAG, "Deletando I2C normal...");
        i2c_driver_delete(I2C_MASTER_NUM);

        // 2. Inicia I2C do RTC (exclusivo para o ULP)
        ESP_LOGI(TAG, "Iniciando I2C do ULP...");
        init_ulp_rtc_i2c();

        // A MÁGICA ESTÁ AQUI: Ensine o RTC I2C qual é o escravo antes de ligar o ULP
        ulp_riscv_i2c_master_set_slave_addr(LIS3DH_ADDR); // LIS3DH_ADDR é 0x19

        // Carrega o firmware
        ESP_ERROR_CHECK(ulp_riscv_load_binary(ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start)));

        // Configura o timer para 100ms (100000 us = 10Hz)
        ulp_set_wakeup_period(0, 100000);

        // Roda o ULP
        ESP_ERROR_CHECK(ulp_riscv_run());

        // Vai dormir...
        esp_deep_sleep_start();
    }

    // Como você disse que NÃO precisa acordar o core principal nunca mais, 
    // ele simplesmente dorme para sempre aqui, e o ULP fica rodando sozinho
    ESP_LOGI(TAG, "Entrando em Deep Sleep. ULP ficara no controle do LIS3DH...");
    
    // Pequeno delay para os logs imprimirem antes de dormir
    vTaskDelay(pdMS_TO_TICKS(100)); 
    esp_deep_sleep_start();
}