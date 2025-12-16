#ifndef fingerprint_H_
#define fingerprint_H_
#include <Adafruit_Fingerprint.h>
#include <Arduino.h>

class FingerprintReader{
public:
    static FingerprintReader* instance;
    static TaskHandle_t* taskHandle;

    FingerprintReader(HardwareSerial *hs,uint8_t enPin,uint8_t touchPin);
    static void run();

    void fpSetPwr(bool enable);
    void fpSetLed(uint8_t mode,uint8_t speed,uint8_t color,uint8_t cycles=0);


protected:
    void task();
    static void taskFuncStatic(void * pvParameters);
    Adafruit_Fingerprint finger;
    uint8_t enPin;
    uint8_t touchPin;
};



#endif