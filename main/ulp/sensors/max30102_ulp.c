#include "max30102_ulp.h"
#include "ulp_riscv.h"
#include "ulp_riscv_i2c_ulp_core.h"

#define REG_FIFO_DATA 0x07

void max30102_read_fifo(int32_t *ir, int32_t *red)
{
	uint8_t data[6];

	ulp_riscv_i2c_master_set_slave_reg_addr(REG_FIFO_DATA);

	ulp_riscv_i2c_master_read_from_device(data, 6);

	*red =
		((data[0] & 0x03) << 16) |
		(data[1] << 8) |
		data[2];

	*ir =
		((data[3] & 0x03) << 16) |
		(data[4] << 8) |
		data[5];
}

