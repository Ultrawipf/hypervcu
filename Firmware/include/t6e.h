#ifndef t6e_H_
#define t6e_H_
#include "vcu.h"

class T6E : public VehicleControls{
public:
    T6E(BMS& bms);
    void sendState();
    ControlState getControls() override;
    BMS* getBMS() override;
    bool setup() override;
    BMS& bms;

    void parseControls(char* buf);


    void updateControls();
    char calcChecksum(char* buf,size_t len);

protected:
    void task(void * pvParameters) override;

private:
    char rxbuf[40];
    char txbuf[20];
    uint8_t timeoutcount = 0;
};

#endif