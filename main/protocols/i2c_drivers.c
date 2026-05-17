#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "i2c_drivers.h"

void i2c_init(void)
{
    i2c_config_t conf_lis3 = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO_LSI3,
        .scl_io_num = I2C_MASTER_SCL_IO_LSI3,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };

    i2c_config_t conf_max30102 = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = I2C_MASTER_SDA_IO_MAX30102,
    .scl_io_num = I2C_MASTER_SCL_IO_MAX30102,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = I2C_FREQ_HZ,
    };

    i2c_param_config(I2C_MASTER_NUM_LIS3, &conf_lis3);
    i2c_driver_install(I2C_MASTER_NUM_LIS3, conf_lis3.mode, 0, 0, 0);

    i2c_param_config(I2C_MASTER_NUM_MAX30102, &conf_max30102);
    i2c_driver_install(I2C_MASTER_NUM_MAX30102, conf_max30102.mode, 0, 0, 0);
}

esp_err_t i2c_write(uint8_t addr, uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM_MAX30102, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

// Read byte data from a specific I2C address and register
esp_err_t i2c_read(uint8_t addr, uint8_t reg, uint8_t* data, size_t data_len) {
    if (data_len == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    if (data_len > 1) {
        i2c_master_read(cmd, data, data_len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + data_len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM_MAX30102, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}