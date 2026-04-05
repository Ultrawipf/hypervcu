#ifndef ui_H_
#define ui_H_

#include "ESPUI.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include <string>

#include "vcu.h"

class WEBGUI{
public:
    static WEBGUI* instance;
    static TaskHandle_t* taskHandle;
    WEBGUI();
    // ~WEBGUI();
    static void run(VCU* vcu = 0);
    static void setup(VCU* vcu = 0);
    static void dummyCb(Control* sender, int type);
    static void enterWifiDetailsCallback(Control *sender, int type);
    static void switchCall(Control *sender, int type);
    static void update();

    static void fpDeleteCb(Control *sender, int type);
    static void fpClearCb(Control *sender, int type);
    static void fpAddCb(Control *sender, int type);

protected:
    static void task(void * pvParameters = 0);
    // static void taskFuncStatic(void * pvParameters);

    static bool hiddenAp;
    static uint16_t fpSlotSelect,fpStatLabel,batCellsLabel,wifi_ssid_text, wifi_pass_text,wifi_ap_ssid_text,wifi_ap_pass_text,switchHiddenSSID,saveWifiBtn,status,batLabel;
    static uint16_t tuninglabel,controlsLabel,saveControlBtn,vesclabel;
    static uint16_t drivemodeUi[VCU::NUMDRIVESUBMODES][VCU::NUMDRIVEMODES][3];
    static VCU* vcu;
    static uint8_t clearFpCount;
    static String drivemodeNames[VCU::NUMDRIVESUBMODES][VCU::NUMDRIVEMODES];
};


#endif