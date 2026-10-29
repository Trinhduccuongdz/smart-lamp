/* [MÔ PHỎNG] bản giả của i2c.h */
#ifndef HAL_I2C_H
#define HAL_I2C_H
#include <stdint.h>
typedef enum { I2C_OK = 0, I2C_ERR_BUSY, I2C_ERR_START, I2C_ERR_NACK, I2C_ERR_TIMEOUT } i2c_status_t;
void i2c1_init(void);
void i2c1_get_idle_levels(int *scl, int *sda);
i2c_status_t i2c1_write(uint8_t addr7, uint8_t prefix, const uint8_t *data, uint16_t len);
#endif
