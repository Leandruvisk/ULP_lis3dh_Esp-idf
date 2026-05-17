#ifndef MAX30102_ULP_H
#define MAX30102_ULP_H

#include <stdint.h>

/* Read one sample from MAX30102 FIFO (red and IR) */
void max30102_read_fifo(int32_t *ir, int32_t *red);

#endif // MAX30102_ULP_H
