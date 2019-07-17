#ifndef CONFIG_H
#define CONFIG_H

/* Software I2C */
#define I2C_TIMEOUT 100
#define SDA_PORT PORTD
#define SDA_PIN 6
#define SCL_PORT PORTD
#define SCL_PIN 7
/* Software I2C */

#define BMS_ALERT_PIN 17 // 17 = ALERT, PC3, PCINT11
#define BMS_BOOT_PIN 14 // 14 = BOOT, PC0
#define BMS_I2C_FET_PIN 8 // 8 = I2C Pull-Up FET Gate, PB0
#define BMS_K1_PIN 13 // 13 = PB5 = K1 connector
#define BMS_VDD_EN_PIN 15 // 15 = PC1 = VDD Enable

#define M365BMS_RADDR 0x22
#define M365BMS_WADDR 0x25

#endif
