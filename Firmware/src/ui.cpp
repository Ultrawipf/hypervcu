#include "ui.h"
#include <ArduinoNvs.h>
#include <AsyncOTA.h>

WEBGUI* WEBGUI::instance = 0;
TaskHandle_t* WEBGUI::taskHandle;

// const char* ssid = "Lokale Verwipfung";
// const char* password = "Local_Wipf!";
const char* hostname = "hypervcu";
const char* apssid = "Hyper VCU"; // AP Mode
const char* apPass = "hypervcu"; // AP Mode default



uint16_t WEBGUI::status;
uint16_t WEBGUI::batLabel;
bool WEBGUI::hiddenAp = false;

IPAddress apIP(10, 13, 37, 1);
// DNSServer dnsServer;
// const byte DNS_PORT = 53;

uint16_t WEBGUI::wifi_ssid_text, WEBGUI::wifi_pass_text,WEBGUI::wifi_ap_pass_text,WEBGUI::wifi_ap_ssid_text;
uint16_t WEBGUI::switchHiddenSSID;
uint16_t WEBGUI::saveWifiBtn;

const String nvskey_SSID = "SSID";
const String nvskey_WIFIPASS = "WPASS";

const String nvskey_APPASS = "APASS";
const String nvskey_APSSID = "APSSID";
const String nvskey_APHIDDEN = "APHIDE";

WEBGUI::WEBGUI(){
    
}
void WEBGUI::stop(){
    // dnsServer.stop();
    // WiFi.disconnect(true);
}
void WEBGUI::taskFuncStatic(void * pvParameters){
    if(!instance) return;
    instance->task();
}
void WEBGUI::run(){
    task();
    // configMAX_TASK_NAME_LEN
    // xTaskCreate(
    // WEBGUI::taskFuncStatic, // Function that should be called
    // "GUI",        // Name of the task (for debugging)
    // 2048,      // Stack size (bytes)
    // NULL,      // Parameter to pass
    // 21,         // Task priority
    // WEBGUI::taskHandle       // Task handle
    // );


}


void WEBGUI::task(){
    setup();

    while(1){
        // dnsServer.processNextRequest();

        vTaskDelay(100);
    }
}

void WEBGUI::textCall(Control* sender, int type)
{
    Serial.print("Text: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);
}



// void WEBGUI::buttonExample(Control* sender, int type, void* param)
// {
//     Serial.print("param: ");
//     Serial.println((long)param);
//     switch (type)
//     {
//     case B_DOWN:
//         Serial.println("Status: Start");
//         ESPUI.updateControlValue(status, "Start");

//         ESPUI.getControl(button1)->color = ControlColor::Carrot;
//         ESPUI.updateControl(button1);
//         break;

//     case B_UP:
//         Serial.println("Status: Stop");
//         ESPUI.updateControlValue(status, "Stop");

//         ESPUI.getControl(button1)->color = ControlColor::Peterriver;
//         ESPUI.updateControl(button1);
//         break;
//     }
// }


void WEBGUI::switchCall(Control* sender, int value)
{
    if(sender == ESPUI.getControl(switchHiddenSSID)){
        hiddenAp = value == S_ACTIVE;
    }
}

// void WEBGUI::selectExample(Control* sender, int value)
// {
//     Serial.print("Select: ID: ");
//     Serial.print(sender->id);
//     Serial.print(", Value: ");
//     Serial.println(sender->value);
// }

void WEBGUI::enterWifiDetailsCallback(Control *sender, int type) {
	if(type == B_UP) {
		// Serial.println("Saving credentials to NVS...");
		// Serial.println(ESPUI.getControl(wifi_ssid_text)->value);
		// Serial.println(ESPUI.getControl(wifi_pass_text)->value);
		bool nvsok;
        nvsok = NVS.setString(nvskey_SSID,ESPUI.getControl(wifi_ssid_text)->value);
        nvsok = NVS.setString(nvskey_WIFIPASS,ESPUI.getControl(wifi_pass_text)->value);
        nvsok = NVS.setString(nvskey_APPASS,ESPUI.getControl(wifi_ap_pass_text)->value);
        nvsok = NVS.setString(nvskey_APSSID,ESPUI.getControl(wifi_ap_ssid_text)->value);
        nvsok = NVS.setInt(nvskey_APHIDDEN,hiddenAp ? 1:0);
        nvsok = NVS.commit();
        if(nvsok){
            ESPUI.getControl(saveWifiBtn)->color = ControlColor::Emerald;
            // ESPUI.getControl(status)->label = "Error writing to NVS";
        }else{
            ESPUI.getControl(saveWifiBtn)->color = ControlColor::Alizarin;
            // ESPUI.getControl(status)->label = "Saved successfully";
        }
		// EEPROM.begin(100);
		// for(i = 0; i < ESPUI.getControl(wifi_ssid_text)->value.length(); i++) {
		// 	EEPROM.write(i, ESPUI.getControl(wifi_ssid_text)->value.charAt(i));
		// 	if(i==30) break; //Even though we provided a max length, user input should never be trusted
		// }
		// EEPROM.write(i, '\0');

		// for(i = 0; i < ESPUI.getControl(wifi_pass_text)->value.length(); i++) {
		// 	EEPROM.write(i + 32, ESPUI.getControl(wifi_pass_text)->value.charAt(i));
		// 	if(i==94) break; //Even though we provided a max length, user input should never be trusted
		// }
		// EEPROM.write(i + 32, '\0');
		// EEPROM.end();
	}
}



void WEBGUI::setup(void)
{
    // Serial.begin(115200);

    WiFi.setHostname(hostname);


    // try to connect to existing network
    String wifissid = NVS.getString(nvskey_SSID);
    String wifipass = NVS.getString(nvskey_WIFIPASS);
    bool wifistored = (!wifissid.isEmpty());
    if(wifistored){
        WiFi.begin(wifissid, wifipass);
        Serial.print("\n\nTry to connect to existing network");

        uint8_t timeout = 10;

        // Wait for connection, 5s timeout
        do
        {
            vTaskDelay(500);
            Serial.print(".");
            timeout--;
        } while (timeout && WiFi.status() != WL_CONNECTED);
    }
    // not connected -> create hotspot
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("\n\nCreating hotspot");

        WiFi.mode(WIFI_AP);
        vTaskDelay(100);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        String appassnvs = NVS.getString(nvskey_APPASS,apPass);
        String apssidnvs = NVS.getString(nvskey_APPASS,apssid);
        int apHidden = NVS.getInt(nvskey_APHIDDEN,0);
        WiFi.softAP(apssidnvs,appassnvs,11,apHidden);

        uint8_t timeout = 5;

        do
        {
            vTaskDelay(500);
            Serial.print(".");
            timeout--;
        } while (timeout);
    }else{
        // Connected
        if (!MDNS.begin(hostname)) {
            Serial.println("Error setting up MDNS responder!");
        }
    }



    Serial.println("\n\nWiFi parameters:");
    Serial.print("Mode: ");
    Serial.println(WiFi.getMode() == WIFI_AP ? "Station" : "Client");
    Serial.print("IP address: ");
    Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());


    uint16_t tabstatus = ESPUI.addControl(ControlType::Tab, "tabstatus", "Status");
    uint16_t tabModes = ESPUI.addControl(ControlType::Tab, "drivemodes", "Drive modes");
    uint16_t tabWifi = ESPUI.addControl(ControlType::Tab, "tabsysconf", "System config");
    
    // status = ESPUI.addControl(ControlType::Label, "Status:", "Stop", ControlColor::Turquoise);
    


    batLabel = ESPUI.addControl(ControlType::Label, "BMS", "None", ControlColor::Turquoise,tabstatus);

    
    // Wifi settings
    ESPUI.addControl(ControlType::Separator, "External AP mode", "", ControlColor::None, tabWifi);
    wifi_ssid_text = ESPUI.addControl(Text, "Connect to external AP SSID\n(Leave empty to disable)", NVS.getString(nvskey_SSID,""), ControlColor::Wetasphalt, tabWifi, textCall);
	//Note that adding a "Max" control to a text control sets the max length
	ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
	wifi_pass_text = ESPUI.addControl(Text, "External AP Password", NVS.getString(nvskey_WIFIPASS,""), ControlColor::Wetasphalt, tabWifi, textCall);
	ESPUI.addControl(Max, "", "64", None, wifi_pass_text);

	ESPUI.addControl(ControlType::Separator, "Direct AP mode (Enabled when no AP found)", "", ControlColor::None, tabWifi);
    wifi_ap_ssid_text = ESPUI.addControl(Text, "Direct AP SSID", NVS.getString(nvskey_APSSID,apssid), Turquoise, tabWifi, textCall);
	ESPUI.addControl(Max, "", "32", None, wifi_ap_ssid_text);
    wifi_ap_pass_text = ESPUI.addControl(Text, "Direct AP Password", NVS.getString(nvskey_APPASS,apPass), Turquoise, tabWifi, textCall);
    ESPUI.addControl(Max, "", "64", None, wifi_ap_pass_text);

    switchHiddenSSID = ESPUI.addControl(  ControlType::Switcher, "Hidden SSID", NVS.getInt(nvskey_APHIDDEN,0) ? "1" : "0", ControlColor::Turquoise, tabWifi, &switchCall);
    
    ESPUI.addControl(ControlType::Separator, "System", "", ControlColor::None, tabWifi);
    saveWifiBtn = ESPUI.addControl(Button, "Actions", "Save", Peterriver, tabWifi, enterWifiDetailsCallback);
    ESPUI.addControl(Button, "Actions", "Reboot", Peterriver, saveWifiBtn, [](Control* target, int val){esp_restart();});

    uint16_t otaframe = ESPUI.addControl( ControlType::Label, "OTA Update", "<iframe src=/ota width=100%></iframe>", ControlColor::Carrot, tabWifi );
    ESPUI.setElementStyle(otaframe,"background-color: transparent;");

    // Start Webserver
    ESPUI.begin("Hyper VCU");
    AsyncOTA.begin(ESPUI.WebServer());
}

