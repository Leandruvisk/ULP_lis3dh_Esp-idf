#ifndef     LIS3DH_H
#define     LIS3DH_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"
#include "../protocols/i2c_drivers.h"
#include "../lis3dh_reg.h"


#define LIS3DH_ADDR       0x19
#define LIS3DH_INT1_GPIO  4

void platform_delay(uint32_t ms);
int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);

void read_accel_once(stmdev_ctx_t *dev_ctx);
void lis3dh_config_low_power(stmdev_ctx_t *dev_ctx);


#endif /* LIS3DH_H */