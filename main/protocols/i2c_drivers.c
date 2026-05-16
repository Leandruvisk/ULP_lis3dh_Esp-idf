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