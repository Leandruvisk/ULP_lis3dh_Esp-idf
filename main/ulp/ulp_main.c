/* uLp/ulp_main.c */
#include <stdint.h>
#include "ulp_riscv.h"
#include "ulp_riscv_utils.h"
#include "ulp_riscv_i2c_ulp_core.h"
#include "soc/rtc_cntl_reg.h"
#include "ulp_riscv_gpio.h"

#define GPIO_LED GPIO_NUM_21
#define GPIO_INT1 GPIO_NUM_4

// Endereços LIS3DH
#define LIS3DH_OUT_X_L      0x28
#define AUTO_INCR_OUT_X_L   (LIS3DH_OUT_X_L | 0x80)
#define LIS3DH_INT1_SRC     0x31

#define MOVEMENT_THRESHOLD  2000

/* Variáveis globais (sem volatile). */
int is_calibrated = 0;
int16_t base_x = 0;
int16_t base_y = 0;
int16_t base_z = 0;

int16_t accel_x = 0;
int16_t accel_y = 0;
int16_t accel_z = 0;

void blink_led(void)
{
    ulp_riscv_gpio_init(GPIO_LED);
    ulp_riscv_gpio_output_enable(GPIO_LED);
    ulp_riscv_gpio_set_output_mode(GPIO_LED, RTCIO_MODE_OUTPUT);

    for(int i = 0; i < 3; i++) {
        ulp_riscv_gpio_output_level(GPIO_LED, 1);
        ulp_riscv_delay_cycles(ULP_RISCV_CYCLES_PER_MS * 100); // 100ms on
        ulp_riscv_gpio_output_level(GPIO_LED, 0);
        ulp_riscv_delay_cycles(ULP_RISCV_CYCLES_PER_MS * 100); // 100ms off
    }
}

int main(void)
{
    // Inicializa GPIO 4 (INT1) como entrada
    ulp_riscv_gpio_init(GPIO_INT1);
    ulp_riscv_gpio_input_enable(GPIO_INT1);
    ulp_riscv_gpio_pulldown_disable(GPIO_INT1);  // Sem pull-down

    // Inicializa LED como saída (para indicação simples)
    ulp_riscv_gpio_init(GPIO_LED);
    ulp_riscv_gpio_output_enable(GPIO_LED);
    ulp_riscv_gpio_set_output_mode(GPIO_LED, RTCIO_MODE_OUTPUT);

    // Endereços I2C
    #define I2C_ADDR_MAX30102 0x57
    #define LIS3DH_ADDR 0x19

    uint8_t buf[192];

    // Loop infinito: o ULP ficará aqui lendo os sensores continuamente
    while (1) {
        // Verifica nível do pino INT1 (LIS3 interrupt)
        uint8_t int1_level = ulp_riscv_gpio_get_level(GPIO_INT1);

        if (int1_level) {
            // Movimento detectado: pequeno blink
            for (int i = 0; i < 2; i++) {
                ulp_riscv_gpio_output_level(GPIO_LED, 1);
                ulp_riscv_delay_cycles(ULP_RISCV_CYCLES_PER_MS * 50);
                ulp_riscv_gpio_output_level(GPIO_LED, 0);
                ulp_riscv_delay_cycles(ULP_RISCV_CYCLES_PER_MS * 50);
            }

            // Lê INT1_SRC do LIS3 para limpar a interrupção (latched)
            ulp_riscv_i2c_master_set_slave_addr(LIS3DH_ADDR);
            ulp_riscv_i2c_master_set_slave_reg_addr(LIS3DH_INT1_SRC);
            uint8_t int_src = 0;
            ulp_riscv_i2c_master_read_from_device(&int_src, 1);

            // Lê acelerômetro X/Y/Z (6 bytes, auto-increment)
            ulp_riscv_i2c_master_set_slave_reg_addr(AUTO_INCR_OUT_X_L);
            ulp_riscv_i2c_master_read_from_device(buf, 6);
            accel_x = (int16_t)((buf[1] << 8) | buf[0]);
            accel_y = (int16_t)((buf[3] << 8) | buf[2]);
            accel_z = (int16_t)((buf[5] << 8) | buf[4]);
        }

        // Leitura básica do MAX30102: verifica ponteiros FIFO e lê alguns samples
        ulp_riscv_i2c_master_set_slave_addr(I2C_ADDR_MAX30102);
        ulp_riscv_i2c_master_set_slave_reg_addr(0x04); // FIFO_WR_PTR
        uint8_t wptr = 0;
        ulp_riscv_i2c_master_read_from_device(&wptr, 1);

        ulp_riscv_i2c_master_set_slave_reg_addr(0x06); // FIFO_RD_PTR
        uint8_t rptr = 0;
        ulp_riscv_i2c_master_read_from_device(&rptr, 1);

        uint8_t samp = (uint8_t)(((32 + wptr) - rptr) % 32);
        if (samp) {
            // Limita leitura para evitar grande uso de memória no ULP
            uint8_t to_read = samp;
            if (to_read > 4) to_read = 4;

            ulp_riscv_i2c_master_set_slave_reg_addr(0x07); // FIFO_DATA
            ulp_riscv_i2c_master_read_from_device(buf, 6 * to_read);

            // Processa primeiro sample lido (exemplo simples)
            uint32_t red = ((buf[0] & 0x03) << 16) | (buf[1] << 8) | buf[2];
            uint32_t ir  = ((buf[3] & 0x03) << 16) | (buf[4] << 8) | buf[5];

            // Indica presença de sinal com o LED
            if (ir > 1000) {
                ulp_riscv_gpio_output_level(GPIO_LED, 1);
            } else {
                ulp_riscv_gpio_output_level(GPIO_LED, 0);
            }
        }

        // Pequeno delay antes da próxima iteração (200 ms)
        ulp_riscv_delay_cycles(ULP_RISCV_CYCLES_PER_MS * 200);
    }

    // Nunca retorna
}
