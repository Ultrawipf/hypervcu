#ifndef ui_H_
#define ui_H_

#include "ESPUI.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include <string>

class WEBGUI{
public:
    static WEBGUI* instance;
    static TaskHandle_t* taskHandle;
    WEBGUI();
    // ~WEBGUI();
    static void run();
    static void setup();
    static void stop();
    static void textCall(Control* sender, int type);
    static void enterWifiDetailsCallback(Control *sender, int type);
    static void switchCall(Control *sender, int type);

protected:
    static void task();
    static void taskFuncStatic(void * pvParameters);

    static bool hiddenAp;
    static uint16_t wifi_ssid_text, wifi_pass_text,wifi_ap_ssid_text,wifi_ap_pass_text,switchHiddenSSID,saveWifiBtn,status,batLabel;
    
};


#endif