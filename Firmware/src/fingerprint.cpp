

#include "pins.h"
#include "fingerprint.h"
// Adafruit_Fingerprint finger = Adafruit_Fingerprint(&FP_SERIAL);
FingerprintReader* FingerprintReader::instance = 0;
TaskHandle_t* FingerprintReader::taskHandle;

FingerprintReader::FingerprintReader(HardwareSerial *hs,uint8_t enPin,uint8_t touchPin)
: finger(Adafruit_Fingerprint(hs)),enPin(enPin),touchPin(touchPin)

{
    instance = this;
}


void FingerprintReader::taskFuncStatic(void * pvParameters){
    if(!instance) return;
    instance->task();
}
void FingerprintReader::run(){
    

    xTaskCreate(
    FingerprintReader::taskFuncStatic, // Function that should be called
    "FP",        // Name of the task (for debugging)
    2048,      // Stack size (bytes)
    NULL,      // Parameter to pass
    20,         // Task priority
    FingerprintReader::taskHandle       // Task handle
    );


}

void FingerprintReader::task(){
    // Enable power for now...
    fpSetPwr(true);
    
    DBG_SERIAL.println("Starting finger!");
    
    FP_SERIAL.setPins(FP_RX,FP_TX);
    finger.begin(57600);
    vTaskDelay(1000);
    if (finger.verifyPassword())
    {
        DBG_SERIAL.println("Found fingerprint sensor!");
        
        // vTaskDelay(1000);
    }else{
        DBG_SERIAL.println("Fingerprint sensor missing?");
        fpSetPwr(false);
    }

        
    while(1){
        vTaskDelay(500);
        if (digitalRead(FP_TOUCH)){
            fpSetPwr(true);
            vTaskDelay(500);
            if (finger.verifyPassword())
            {
                DBG_SERIAL.println("Found fingerprint sensor!");
            }else{
                DBG_SERIAL.println("Fingerprint sensor missing?");
                fpSetPwr(false);
                continue;
            }
        }
        
        
    }
}

void FingerprintReader::fpSetPwr(bool enable){
    digitalWrite(enPin,!enable);
}

void FingerprintReader::fpSetLed(uint8_t mode,uint8_t speed,uint8_t color,uint8_t cycles){
    finger.LEDcontrol(mode, speed, color,cycles);
}
