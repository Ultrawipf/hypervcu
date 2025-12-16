#ifndef vcu_H_
#define vcu_H_
#include "Arduino.h"



// Interface for scooter/bike screens
class VehicleControls{
public:
    struct DisplayState{
        float speedkmh;
        float batterypct;
        bool light;
        bool brake;
        uint8_t indicators;
        bool errors;
    };

    struct ControlState{
        float throttle = 0;
        uint8_t mode = 0;
        uint8_t indicators = 0;
    };

    VehicleControls();

    // void setSpeed(float kmh);
    // void setBattery(uint8_t pct);
    // void setLightState();
    virtual void sendState() = 0;
    virtual ControlState getControls() = 0;

    ControlState currentControls;
    DisplayState currentDisplay;
};

class T6E : public VehicleControls{
    T6E();

    void sendState() override;
    ControlState getControls() override;
};

class VCU{
public:
    VCU(VehicleControls& controls);
    // ~VCU();
    TaskHandle_t* taskHandle = 0;

    void task(void * pvParameters);
    // void start();
    // void setLights();
    // void updateMotor();
    bool running = false;
    const TickType_t interval = 50;

    VehicleControls& controls;
};


#endif
