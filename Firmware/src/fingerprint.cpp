

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
    10,         // Task priority
    FingerprintReader::taskHandle       // Task handle
    );


}

void FingerprintReader::task(){
    fingerSem = xSemaphoreCreateBinary();
    fingerBlockSem = xSemaphoreCreateBinary();

    fpSetPwr(false);

    vTaskDelay(200);
    
    FP_SERIAL.setPins(FP_RX,FP_TX);
    finger.begin(57600);
    fpSetPwr(true);
    vTaskDelay(500);
    if (finger.verifyPassword())
    {
        DBG_SERIAL.println("Found fingerprint sensor!");
        isConnected = true;
        
        // vTaskDelay(1000);
    }else{
        DBG_SERIAL.println("Fingerprint sensor missing?");
        isConnected = false;
        fpSetPwr(false);
    }
    xSemaphoreGive(fingerSem);

    bool fingerOn = false;
    while(1){
        vTaskDelay(150);
        if (digitalRead(FP_TOUCH) && !fingerOn && (enrollstep == EnrollStep::none || enrollstep == EnrollStep::error || enrollstep == EnrollStep::success)){
            fingerOn = true;
            fpSetPwr(true);
            vTaskDelay(100);
            xSemaphoreTake(fingerSem, portMAX_DELAY);
            lastFingerResult = checkCurFinger();
            xSemaphoreGive(fingerSem);
            xSemaphoreGive(fingerBlockSem);
            if(lastFingerResult >= 0){
                DBG_SERIAL.println("Finger OK");
                // DBG_SERIAL.println(lastFingerResult);
            }
            if(callback){
                callback(lastFingerResult);
            }
        }else{
            fingerOn = digitalRead(FP_TOUCH);
            // Not touching and not enrolling
            vTaskDelay(250);
            if(enrollstep == EnrollStep::started){
                addFinger(enrollNewFingerId);
            }else{
                enrollstep = EnrollStep::none;
                fpSetPwr(false);
            }
        }
    }
}

/**
 * Checks a currently touching finger for a match
 * Returns negative error code or positive finger ID on match
 */
int32_t FingerprintReader::checkCurFinger(){
    
    uint8_t p = finger.getImage();
    if(p != FINGERPRINT_OK){return -p;}
    p = finger.image2Tz();
    if(p != FINGERPRINT_OK){return -p;}
    p = finger.fingerFastSearch();
    if(p != FINGERPRINT_OK){return -p;}
    
    return finger.fingerID;
    
}
int32_t FingerprintReader::getLastFingerState(bool block){
    if(block){
        xSemaphoreTake(fingerBlockSem, portMAX_DELAY);
    }
    return lastFingerResult;
}

void FingerprintReader::fpSetPwr(bool enable,bool forceEnable){
    static bool forceState;
    if(forceEnable){
        forceState = enable;
    }else if(forceState) return;
    digitalWrite(enPin,!enable);
}

void FingerprintReader::fpSetLed(uint8_t mode,uint8_t speed,uint8_t color,uint8_t cycles){
    finger.LEDcontrol(mode, speed, color,cycles);
}

/**
 * Delete a finger with id. ID 0 deletes last seen finger
 * Returns deleted finger or negative error code
 */
int32_t FingerprintReader::clearAllFingers(){
    fpSetPwr(true,true);
    vTaskDelay(250);
    lastFingerResult = -finger.emptyDatabase();
    vTaskDelay(100);
    fpSetPwr(false,true);
    return lastFingerResult;
}

/**
 * Delete a finger with id. ID -1 deletes last seen finger
 * Returns deleted finger or negative error code
 */
int32_t FingerprintReader::deleteFinger(int16_t id){
    if(id == 0){
        if(lastFingerResult <= 0){
            return lastFingerResult;
        }
        id = lastFingerResult;
    }

    fpSetPwr(true,true);
    vTaskDelay(250);
    int32_t res = finger.deleteModel(id);
    lastFingerResult = 0;

    vTaskDelay(100);
    fpSetPwr(false,true);
    if(res){
        return -res;
    }else{
        return id;
    }
}

FingerprintReader::EnrollStep FingerprintReader::getEnrollState(){
    return enrollstep;
}

/**
 * Returns true if reader was detected at startup
 */
bool FingerprintReader::getConnected(){
    return isConnected;
}
/**
 * Starts process to add new finger into id slot in fingerprint thread
 */
void FingerprintReader::startEnrollment(uint16_t id){
    // Only start when no process running
    if((enrollstep == EnrollStep::none || enrollstep == EnrollStep::error || enrollstep == EnrollStep::success)){
        enrollNewFingerId = id;
        enrollstep = EnrollStep::started;
    }
    
}
/**
 * Starts process to add finger into id slot in same thread
 */
int32_t FingerprintReader::addFinger(uint16_t id){
    xSemaphoreTake(fingerSem, portMAX_DELAY);
    fpSetPwr(true,true);
    int32_t res = addFingerProcess(id);
    if(res == FINGERPRINT_OK){
        enrollstep = EnrollStep::success;
    }else{
        enrollstep = EnrollStep::error;
    }
    if(!res){
        lastFingerResult = finger.fingerID;
    }else{ 
        lastFingerResult = -res;
    }
    xSemaphoreGive(fingerSem);
    vTaskDelay(100);
    fpSetPwr(false,true);
    return lastFingerResult;
}
int32_t FingerprintReader::addFingerProcess(uint16_t id){
    // if(id == 0){
    //     Serial.println(finger.getTemplateCount());
    //     id = finger.templateCount;
    // }
    if(id > finger.capacity){
        return FINGERPRINT_BADLOCATION;
    }
    Serial.println(id);
    int16_t p = -1;
    int16_t timeout = 500;
    // First scan
    enrollstep = EnrollStep::waitfirst;
    do{
        p = finger.getImage();
        vTaskDelay(10);
    }while(p != FINGERPRINT_OK && timeout--);
    if(p != FINGERPRINT_OK){return p;}
    p = finger.image2Tz(1); // Set into slot 1
    if(p != FINGERPRINT_OK){return p;}
    // Wait for finger to be removed
    enrollstep = EnrollStep::waitremove;
    do{ 
        vTaskDelay(10);
    }while(finger.getImage() == FINGERPRINT_OK);
    vTaskDelay(500);
    enrollstep = EnrollStep::waitsecond;
    // Second scan
    timeout = 500;
    do{
        p = finger.getImage();
        vTaskDelay(10);
    }while(p != FINGERPRINT_OK && timeout--);
    if(p != FINGERPRINT_OK){return p;}
    p = finger.image2Tz(2); // Set into slot 2
    enrollstep = EnrollStep::processing;
    if(p != FINGERPRINT_OK){return p;}
    // Convert
    p = finger.createModel();
    if(p != FINGERPRINT_OK){return p;}
    p = finger.storeModel(id);
    if(p != FINGERPRINT_OK){return p;}
    return p;
}


void FingerprintReader::registerCallback(std::function<void(int)> cb){
    this->callback = cb;
}