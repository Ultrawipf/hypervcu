#ifndef fingerprint_H_
#define fingerprint_H_
#include <Adafruit_Fingerprint.h>
#include <Arduino.h>
#include "semphr.h"
#include "functional"

class FingerprintReader{
public:
    enum class EnrollStep{none,started,waitfirst,waitremove,waitsecond,processing,success,error};
    static FingerprintReader* instance;
    static TaskHandle_t* taskHandle;

    FingerprintReader(HardwareSerial *hs,uint8_t enPin,uint8_t touchPin);
    static void run();

    void fpSetPwr(bool enable,bool forceEnable = false);
    void fpSetLed(uint8_t mode,uint8_t speed,uint8_t color,uint8_t cycles=0);
    bool getConnected();
    bool getReady(); // False during startup

    int32_t getLastFingerState(bool block = false);
    EnrollStep getEnrollState();

    void startEnrollment(uint16_t id = 0);
    int32_t addFinger(uint16_t id = 0);
    int32_t deleteFinger(int16_t id = -1);
    int32_t clearAllFingers();

    void registerCallback(std::function<void(int)> cb);

protected:
    void task();
    int32_t checkCurFinger();
    static void taskFuncStatic(void * pvParameters);
    int32_t addFingerProcess(uint16_t id = 0);
    Adafruit_Fingerprint finger;
    uint8_t enPin;
    uint8_t touchPin;
    bool isConnected = false;
    bool isReady = false;
    int32_t lastFingerResult = -FINGERPRINT_NOFINGER;

    uint16_t enrollNewFingerId = 0;
    std::function<void(int)> callback = nullptr;
    
    EnrollStep enrollstep = EnrollStep::none;

    SemaphoreHandle_t fingerSem = NULL;
    SemaphoreHandle_t fingerBlockSem = NULL;
};



#endif