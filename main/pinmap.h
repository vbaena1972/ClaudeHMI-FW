#pragma once
#include "driver/gpio.h"

//#define LCD_BACKLIGHT_GPIO    GPIO_NUM_2
//#define LCD_SPI_HOST          SPI2_HOST
//#define LCD_PIN_CS            GPIO_NUM_3
//#define LCD_PIN_DC            GPIO_NUM_4
//#define LCD_PIN_RST           GPIO_NUM_5
//#define LCD_PIN_SCK           GPIO_NUM_6
//#define LCD_PIN_MOSI          GPIO_NUM_7
//#define LCD_PIN_MISO          GPIO_NUM_8

//#define TOUCH_I2C_PORT        I2C_NUM_0
//#define TOUCH_PIN_SCL         GPIO_NUM_15
//#define TOUCH_PIN_SDA         GPIO_NUM_16
//#define TOUCH_PIN_INT         GPIO_NUM_39
//#define TOUCH_PIN_RST         GPIO_NUM_40

#define ETH_SPI_HOST          SPI3_HOST
#define ETH_PIN_INT           GPIO_NUM_14
#define ETH_PIN_MISO          GPIO_NUM_13
#define ETH_PIN_SCK           GPIO_NUM_12
#define ETH_PIN_MOSI          GPIO_NUM_11
#define ETH_PIN_CS            GPIO_NUM_10
#define ETH_PIN_RST           GPIO_NUM_9

#define RS485_UART_NUM        UART_NUM_1
#define RS485_PIN_TX          GPIO_NUM_17
#define RS485_PIN_RX          GPIO_NUM_18
#define RS485_PIN_DE          GPIO_NUM_46

#define RS232_UART_NUM        UART_NUM_2
#define RS232_PIN_TX          GPIO_NUM_43
#define RS232_PIN_RX          GPIO_NUM_44

#define RELAY_GPIO            GPIO_NUM_35
#define BUZZER_PWM_GPIO       GPIO_NUM_36

#define I2C_SHARED_PORT       I2C_NUM_0
#define I2C_SHARED_SCL        GPIO_NUM_15
#define I2C_SHARED_SDA        GPIO_NUM_16

#define SD_CD_PIN             GPIO_NUM_42