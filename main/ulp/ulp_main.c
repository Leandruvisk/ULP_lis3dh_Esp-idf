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
    
    // Verifica se INT1 está ativo (interrupção de movimento)
    uint8_t int1_level = ulp_riscv_gpio_get_level(GPIO_INT1);
    
    if (int1_level) {
        // INT1 está ALTO - movimento detectado!
        // Pisca o LED
        blink_led();
        read_max30102 ();
        
        // Lê INT1_SRC do sensor para limpar a interrupção (latched)
        ulp_riscv_i2c_master_set_slave_reg_addr(LIS3DH_INT1_SRC);
        uint8_t int_src = 0;
        ulp_riscv_i2c_master_read_from_device(&int_src, 1);
        
        // Agora INT1 voltará a LOW após ler INT1_SRC
    }
    
    return 0; // Dorme até o próximo ciclo configurado no main (ulp_set_wakeup_period)
}