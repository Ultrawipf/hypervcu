#ifndef pins_H_
#define pins_H_

#define VERSIONSTR "0.1.4"

#define DBG_SERIAL Serial

#define FP_TOUCH 6
#define FP_PWR_EN 7

// Info: V1.1 RX was 15, TX 16. Swapped  in V1.2+
#define FP_RX 16
#define FP_TX 15
#define FP_UARTNUM 2
#define FP_SERIAL Serial2

#define LED_IND_L_EN 21
#define LED_IND_R_EN 14
#define LED_IND_RR_EN 12
#define LED_IND_LR_EN 13
#define LED_FRONT_EN 45
#define LED_BRAKE_EN 47
#define LED_REAR_EN 48
#define LED_EN 46 // Power regulator enable (input)

#define VESC_TX 43 //uart0
#define VESC_RX 44
#define VESC_UARTNUM 0
#define VESC_SERIAL Serial0
#define VESC_EN 38
#define VESC_EN_MODE OUTPUT

#define DISP_TX 17
#define DISP_RX 18
#define DISP_UARTNUM 1
#define DISP_SERIAL Serial1

#define BRAKE_N 8 // input

#endif