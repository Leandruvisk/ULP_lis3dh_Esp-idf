#include <stdint.h>
#include "ulp_riscv.h"
#include "ulp_riscv_utils.h"
#include "ulp_riscv_i2c_ulp_core.h"
#include "sensors/max30102_ulp.h"

#define MAX30102_ADDR 0x57

/*
 * Variáveis compartilhadas
 */
int32_t ulp_ir_value = 0;
int32_t ulp_red_value = 0;
int32_t ulp_bpm = 0;
int32_t ulp_new_data = 0;

/*
 * Estado interno do ULP
 */
static int32_t prev_sample = 0;
static int32_t prev_diff = 0;

static uint32_t sample_counter = 0;
static uint32_t last_peak = 0;

/* max30102_read_fifo() is implemented in sensors/max30102_ulp.c */

int main(void)
{
    int32_t ir;
    int32_t red;

    max30102_read_fifo(&ir, &red);

    ulp_ir_value = ir;
    ulp_red_value = red;

    /*
     * Filtro passa alta simples
     */
    int32_t filtered = ir - prev_sample;

    /*
     * Detecção de pico
     */
    if ((prev_diff > 0) && (filtered < 0)) {

        uint32_t delta = sample_counter - last_peak;

        /*
         * faixa válida:
         * 40 bpm -> 150 bpm
         */
        if (delta > 30 && delta < 150) {

            ulp_bpm = 6000 / delta;
        }

        last_peak = sample_counter;
    }

    prev_diff = filtered;
    prev_sample = ir;

    sample_counter++;

    /*
     * sinaliza dado novo
     */
    ulp_new_data = 1;
    ulp_riscv_wakeup_main_processor();

    return 0;
}