#include <Arduino.h>
#include "jbd_bms.h"
#include "pins.h"
#include <tuple>
#include <array>
#include "fingerprint.h"
#include "ui.h"
#include <ArduinoNvs.h>
#include "t6e.h"

SET_LOOP_TASK_STACK_SIZE(4096)

std::array<std::pair<uint8_t,bool>,7> lights{{{LED_FRONT_EN,false},{LED_REAR_EN,false},{LED_BRAKE_EN,false},{LED_IND_L_EN,false},{LED_IND_R_EN,false},{LED_IND_LR_EN,false},{LED_IND_RR_EN,false}}};

void setupPins(){
    // Leds
    pinMode(LED_EN,INPUT);
    pinMode(LED_BRAKE_EN,OUTPUT);
    pinMode(LED_FRONT_EN,OUTPUT);
    pinMode(LED_IND_LR_EN,OUTPUT);
    pinMode(LED_IND_RR_EN,OUTPUT);
    pinMode(LED_REAR_EN,OUTPUT);
    pinMode(LED_IND_R_EN,OUTPUT);
    pinMode(LED_IND_L_EN,OUTPUT);

    pinMode(FP_PWR_EN,OUTPUT);
    pinMode(FP_TOUCH,INPUT);

    pinMode(BRAKE_N,INPUT);

    pinMode(VESC_EN,VESC_EN_MODE); // Output if not directly connected to display.

    // pinMode(FP_RX,INPUT);
    // pinMode(FP_TX,OUTPUT);

    // Fingerprint
    FP_SERIAL.setPins(FP_RX,FP_TX);

    DISP_SERIAL.setPins(DISP_RX,DISP_TX);

    VESC_SERIAL.setPins(VESC_RX,VESC_TX);
}


TaskHandle_t* ledtask;
TaskHandle_t* bletask;
// TaskHandle_t* fingerprinttask;
FingerprintReader fp(&FP_SERIAL,FP_PWR_EN,FP_TOUCH);
BMS bms; // JBD bms interface helper
T6E vehicleControls = T6E(bms);
VCU vcu(vehicleControls,&fp);


void setup()
{
    DBG_SERIAL.begin(115200);
    setupPins();

    NVS.begin();
    
    setupBms();
    xTaskCreate(
        jbdBmsTask, // Function that should be called
        "BMS_BLE",        // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE+2048,      // Stack size (bytes)
        NULL,      // Parameter to pass
        5,         // Task priority
        bletask       // Task handle
    );

    
    // xTaskCreate(
    //     ledTask, // Function that should be called
    //     "LEDS",        // Name of the task (for debugging)
    //     configMINIMAL_STACK_SIZE+100,      // Stack size (bytes)
    //     NULL,      // Parameter to pass
    //     10,         // Task priority
    //     ledtask       // Task handle
    // );

    
    fp.run(); // Start fingerprint task
    vcu.run(); // Start VCU task
    
    // WEBGUI::setup(&vcu);
    WEBGUI::run(&vcu);
}

void loop()
{
    // WEBGUI::update();
    vTaskDelay(1000);

}
