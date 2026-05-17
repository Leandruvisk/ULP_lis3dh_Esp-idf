#ifndef I2C_DRIVERS_H
#define I2C_DRIVERS_H

#include "esp_err.h"


#define I2C_MASTER_SDA_IO_LSI3 3
#define I2C_MASTER_SCL_IO_LSI3 2

#define I2C_MASTER_SDA_IO_MAX30102 5
#define I2C_MASTER_SCL_IO_MAX30102 6

#define I2C_MASTER_NUM_LIS3             I2C_NUM_0
#define I2C_MASTER_NUM_MAX30102         I2C_NUM_1

#define I2C_FREQ_HZ       100000


void i2c_init(void);
esp_err_t i2c_write(uint8_t addr, uint8_t reg, uint8_t data);
esp_err_t i2c_read(uint8_t addr, uint8_t reg, uint8_t* data, size_t data_len);





#endif /* I2C_DRIVERS_H */