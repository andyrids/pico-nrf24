#ifndef ERROR_MANAGER_H
#define ERROR_MANAGER_H

typedef enum internal_status_e
{
  OK,
  PARAMETER_FAIL,
  REGISTER_R_FAIL,
  REGISTER_W_FAIL,
  TX_MODE_FAIL,
  RX_MODE_FAIL,
  RX_FIFO_EMPTY,
  SPI_FAIL
} internal_status_t;

typedef enum internal_status_irq_e
{
  NONE_ASSERTED,
  RX_DR_ASSERTED,
  TX_DS_ASSERTED,
  MAX_RT_ASSERTED
} internal_status_irq_t;

typedef enum external_status_e
{
  FAIL, PASS
} external_status_t;

#endif // ERROR_MANAGER_H