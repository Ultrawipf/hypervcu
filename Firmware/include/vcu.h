#ifndef vcu_H_
#define vcu_H_
#include "Arduino.h"
#include "jbd_bms.h"
#include "VescUart.h"
#include "pins.h"
#include "fingerprint.h"


#define VCUERR_LOCKED 0x01
#define VCUERR_DISCONNECTED 0x06

// Interface for scooter/bike screens
class VehicleControls{
public:
    struct DisplayState{
        float speedkmh;
        float batterypct;
        bool light;
        bool brake;
        uint8_t indicators;
        bool warning;
        uint8_t errors;
    };

    struct ControlState{
        float throttle = 0; // Range 0 - 1.0
        uint8_t mode = 0;
        uint8_t specialmode = 0; // walk mode
        uint8_t indicators = 0;
        uint8_t light = 0;
        uint8_t brake = 0;
        bool connected = false;
    };

    VehicleControls();
    static TaskHandle_t* taskHandle;
    static VehicleControls* instance;
    
    static void run();
    static void taskFuncStatic(void * pvParameters);

    virtual bool setup();

    virtual ControlState getControls() = 0;
    virtual BMS* getBMS(){return nullptr;};

    DisplayState currentDisplay;
protected:
    virtual void task(void * pvParameters);
    ControlState currentControls;
    
};



class VCU{
    static constexpr TickType_t interval = 10;
    struct LightEffect{ // todo general effects
        uint8_t counter = 0;
        uint8_t period = 0;
    };
    struct LightState{
        bool overrideLights = false;

        uint8_t front = false;
        uint8_t rear = false;
        uint8_t brake = false;
        uint8_t indl = false;
        uint8_t indr = false;
        // fx counters?

        uint8_t indicatorPeriod = 500 / interval;
        uint8_t indicatorCounter = 0;
    };

    struct DriveMode{
        float maxCurrent;
        float maxSpeed;
        bool currentMode;
        // float maxAccel = 0;
        // float curveScale = 0;
    };
public:
    VCU(VehicleControls& controls,FingerprintReader* fingerprint = nullptr);
    // ~VCU();
    static TaskHandle_t* taskHandle;
    static VCU* instance;
    
    static void run();
    static void taskFuncStatic(void * pvParameters);

    void setupVesc();

    void updateLights();
    void updateDisplayState();
    // void setLights();
    void updateMotor();
    float calcSpeedScale(float wheelDiam,int poles);
    void updateBrakeCurrent(float targetCurrent);
    void updateCurrent(float current);

    void setLocked(bool locked);
    void changeDriveMode(DriveMode* mode);
    void updateDriveMode(VehicleControls::ControlState& newControls,uint8_t submode = 0);

    void saveSettings();
    void loadSettings();

    bool running = false;
    bool vescOk = false;
    bool locked = false;

    float brakeCurrent = 0;
    float offThrottleBrake = 0;
    float lockCurrent = 15;
    float minCurrent = 0.02f; // Minimum current when coasting with throttle >0

    float speedPIDscale = 0.01;
    float lastSpeed = 0;
    float lastCurrent = 0; // Previous requested motor current
    float speedP = 120, speedI = 3, speedD = 30;
    double speedPv = 0, speedIv = 0, speedDv = 0;
    double speedIlim = 500,speedIlimneg = -100;

    void fpCb(int finger);

    static constexpr uint8_t NUMDRIVEMODES = 3;
    static constexpr uint8_t NUMDRIVESUBMODES = 2;

    bool masterMode = false;
    bool zerostart = false;
    float minspeed = 2.0f; // Min non-zerostart speed. Also considered idle when below this speed. 
    float minspeedAlarm = 0.5f; // When locked flash indicators if pushed faster than this speed
    float curBrakeA = 0;
    float brakeRamp = 5; // in A/s
    float curCurrent = 0;

    const uint8_t currentSmoothing = 1; // Average this many old samples into the new current when ramping up only


    DriveMode driveModes[NUMDRIVESUBMODES][NUMDRIVEMODES] = { // Make define length. driveMode is mode -1. Mode 0 is off.
        {
            {
                .maxCurrent = 20,
                .maxSpeed = 10,
                .currentMode = false
            },
            {
                .maxCurrent = 30,
                .maxSpeed = 15,
                .currentMode = false
            },
            {
                .maxCurrent = 35,
                .maxSpeed = 21,
                .currentMode = false
            }
        },
        {
            {
                .maxCurrent = 20,
                .maxSpeed = 21,
                .currentMode = true
            },
            {
                .maxCurrent = 35,
                .maxSpeed = 21,
                .currentMode = true
            },
            {
                .maxCurrent = 40,
                .maxSpeed = 40,
                .currentMode = true
            }
        }
    };
    DriveMode* curDriveMode = nullptr;

    VehicleControls& controls;
    VescUart vesc;
    FingerprintReader* fingerprint = nullptr;
    // mc_configuration vescconf;

protected:
    void task(void * pvParameters);
    LightState curLightState;
    VehicleControls::ControlState curControlState;
    VehicleControls::ControlState lastControlState;

    packBasicInfoStruct curBmsdata;

    bool directMode = true;
    // float maxCurrent = 10;
    uint8_t poles = 30;
    float wheelDiamMM = 250;
    float rpmToKmh = 0;

    const uint8_t vescValUpdInterval = 4;
    uint8_t vescValUpdCnt = 0;
    float curSpeedKmh = 0;

    bool zerostartWait = false;
};


#endif
