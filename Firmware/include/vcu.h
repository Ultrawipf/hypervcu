#ifndef vcu_H_
#define vcu_H_
#include "Arduino.h"
#include "jbd_bms.h"
#include "VescUart.h"
#include "pins.h"
#include "fingerprint.h"


#define VCUERR_LOCKED 0x01
#define VCUERR_DISCONNECTED 0x06
#define VCUERR_VESCNOK 0x10
#define VCUERR_VESCTEMP 0x20

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
        float throttle = 0;    // Range 0 - 1.0, interpolated
        float throttleRaw = 0; // Range 0 - 1.0, last parsed value
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
public:
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

        uint8_t indicatorPeriod = 333 / interval;
        uint32_t indicatorCounter = 0;
    };

    struct DriveMode{
        float maxCurrent;
        float maxSpeed;
        bool currentMode;
        bool zeroStart; // Zerostart active immediately
        // float maxAccel = 0;
        // float curveScale = 0;
    };

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
    float updateSpeedScale(float wheelDiam,int poles);
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
    float minCurrent = 0.0f; // Minimum current when coasting with throttle >0

    float speedPIDscale = 0.01;
    float lastSpeed = 0;
    float lastCurrent = 0; // Previous requested motor current
    float speedP = 130, speedI = 2, speedD = 50;
    double speedPv = 0, speedIv = 0, speedDv = 0;
    double speedIlim = 500,speedIlimneg = -100;

    void fpCb(int finger);

    static constexpr uint8_t NUMDRIVEMODES = 3;
    static constexpr uint8_t NUMDRIVESUBMODES = 2;

    bool masterMode = false;
    bool zerostart = false; // Zerostart override
    float minspeed = 2.0f; // Min non-zerostart speed. Also considered idle when below this speed. 
    float minspeedAlarm = 0.5f; // When locked flash indicators if pushed faster than this speed
    float curBrakeA = 0;
    float brakeRamp = 15; // in A/s
    float curCurrent = 0;

    const uint8_t currentSmoothing = 1; // Average this many old samples into the new current when ramping up only


    DriveMode driveModes[NUMDRIVESUBMODES][NUMDRIVEMODES] = { // Make define length. driveMode is mode -1. Mode 0 is off.
        {
            {
                .maxCurrent = 20,
                .maxSpeed = 10,
                .currentMode = false,
                .zeroStart = false
            },
            {
                .maxCurrent = 30,
                .maxSpeed = 15,
                .currentMode = false,
                .zeroStart = false
            },
            {
                .maxCurrent = 35,
                .maxSpeed = 21,
                .currentMode = false,
                .zeroStart = false
            }
        },
        {
            {
                .maxCurrent = 20,
                .maxSpeed = 21,
                .currentMode = true,
                .zeroStart = true
            },
            {
                .maxCurrent = 35,
                .maxSpeed = 21,
                .currentMode = true,
                .zeroStart = true
            },
            {
                .maxCurrent = 40,
                .maxSpeed = 40,
                .currentMode = true,
                .zeroStart = true
            }
        }
    };
    DriveMode* curDriveMode = nullptr;

    VehicleControls& controls;
    VescUart vesc;
    FingerprintReader* fingerprint = nullptr;
    // mc_configuration vescconf;

    uint8_t getMotorPoles();
    float getWheelDiam();

protected:
    void task(void * pvParameters);
    LightState curLightState;
    VehicleControls::ControlState curControlState;
    VehicleControls::ControlState lastControlState;

    packBasicInfoStruct curBmsdata;

    bool directMode = true;
    // float maxCurrent = 10;
    uint8_t poles = 30;
    float wheelDiamMM = 250; // Make configurable. 225 or 250?
    float rpmToKmh = 0;

    const uint8_t vescValUpdInterval = 0; // 0 always update, 1, half rate...
    const uint8_t vescKeepaliveInterval = 100;
    uint8_t vescValUpdCnt = 0, vescKeepaliveCnt = 0;
    float curSpeedKmh = 0;

    bool zerostartWait = false;

    const float speedLimBegin = 0.80;
    const float speedLimEnd = 0.95;
};


#endif
