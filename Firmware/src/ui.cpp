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
VCU* WEBGUI::vcu;


uint16_t WEBGUI::status;
uint16_t WEBGUI::batLabel;
bool WEBGUI::hiddenAp = false;

IPAddress apIP(10, 13, 37, 1);
// DNSServer dnsServer;
// const byte DNS_PORT = 53;

uint16_t WEBGUI::tuninglabel,WEBGUI::controlsLabel,WEBGUI::wifi_ssid_text,WEBGUI::batCellsLabel, WEBGUI::wifi_pass_text,WEBGUI::wifi_ap_pass_text,WEBGUI::wifi_ap_ssid_text;
uint16_t WEBGUI::fpStatLabel,WEBGUI::fpSlotSelect;
uint16_t WEBGUI::switchHiddenSSID;
uint16_t WEBGUI::saveWifiBtn;
uint8_t WEBGUI::clearFpCount = 0;

const String nvskey_SSID = "SSID";
const String nvskey_WIFIPASS = "WPASS";

const String nvskey_APPASS = "APASS";
const String nvskey_APSSID = "APSSID";
const String nvskey_APHIDDEN = "APHIDE";

// WEBGUI::WEBGUI(){
    

// void WEBGUI::taskFuncStatic(void * pvParameters){
//     if(!instance) return;
//     instance->task();
// }
void WEBGUI::run(VCU* vcu){
    
    setup(vcu);
    // task();
    xTaskCreate(
        WEBGUI::task, // Function that should be called
        "GUI",        // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE+8192,      // Stack size (bytes)
        NULL,      // Parameter to pass
        3,         // Task priority
        WEBGUI::taskHandle       // Task handle
    );
    // task();
}

void WEBGUI::update(){
    if(vcu){
        BMS* bms = vcu->controls.getBMS();
        if(bms && bms->getBmsConnected()){
            // Print bms info
            packBasicInfoStruct packBasicInfo = bms->getBmsBasicData();
            uint16_t buflen = 200;
            uint16_t curpos = 0;
            char buf[buflen];
            curpos += snprintf(buf+curpos,buflen-curpos,"Volt: %.2fV\n", (float)packBasicInfo.Volts / 1000);
            curpos += snprintf(buf+curpos,buflen-curpos,"Amps: %.2fA\n", (float)packBasicInfo.Amps / 1000);
            curpos += snprintf(buf+curpos,buflen-curpos,"Remaining: %.2f%%\n%.2fAh\n%.2fWh\n", (float)packBasicInfo.CapacityRemainPercent, (float)packBasicInfo.CapacityRemainAh / 100,(float)packBasicInfo.CapacityRemainWh / 10);
            curpos += snprintf(buf+curpos,buflen-curpos,"Temp1: %.2f°C\n", (float)packBasicInfo.Temp1 / 10);
            curpos += snprintf(buf+curpos,buflen-curpos,"ON?: %d\n", packBasicInfo.MosfetStatus);
            ESPUI.updateLabel(batLabel, buf);

            // Print cell info
            packCellInfoStruct packCellInfo = bms->getBmsCellData();
            if(packCellInfo.NumOfCells){
                uint16_t buflen = 200;
                uint16_t curpos = 0;
                char buf[buflen];
                for (byte i = 1; i <= packCellInfo.NumOfCells; i++){
                    curpos += snprintf(buf+curpos,buflen-curpos,"%u:   %.3fV\n", i,(float)packCellInfo.CellVolt[i - 1] / 1000);
                }

                ESPUI.updateLabel(batCellsLabel, buf);
            }
        }else{
            ESPUI.updateLabel(batLabel,"BMS not available");
        }
        // Finger
        FingerprintReader* fp = vcu->fingerprint;
        if(fp && fp->getConnected()){
            int32_t lastFinger = fp->getLastFingerState();
            String fingerstr;
            FingerprintReader::EnrollStep enrollstep = fp->getEnrollState();

            if(enrollstep != FingerprintReader::EnrollStep::none){ // Display enroll messages during enrollment
                // Maybe disable actions
                if(enrollstep == FingerprintReader::EnrollStep::waitfirst){fingerstr = "Place finger";}
                else if(enrollstep == FingerprintReader::EnrollStep::waitremove){fingerstr = "Remove finger";}
                else if(enrollstep == FingerprintReader::EnrollStep::waitsecond){fingerstr = "Place finger again";}
                else if(enrollstep == FingerprintReader::EnrollStep::processing){fingerstr = "Processing";}
                else if(enrollstep == FingerprintReader::EnrollStep::success){fingerstr = "Finger added successfully";}
                else if(enrollstep == FingerprintReader::EnrollStep::error){fingerstr = "Error during enroll process. Try again";}
                else if(enrollstep == FingerprintReader::EnrollStep::started){fingerstr = "Starting enrollment...";}
            }else if(lastFinger < 0){ // Show last finger status
                int32_t fingerErr = -lastFinger;
                if(fingerErr == FINGERPRINT_NOFINGER){fingerstr = "No finger";}
                else if(fingerErr == FINGERPRINT_PACKETRECIEVEERR){fingerstr = "Comm Error";}
                else if(fingerErr == FINGERPRINT_NOTFOUND){fingerstr = "Unknown finger";}
                else if(fingerErr == FINGERPRINT_IMAGEFAIL || fingerErr ==  FINGERPRINT_INVALIDIMAGE || fingerErr == FINGERPRINT_FEATUREFAIL || fingerErr == FINGERPRINT_IMAGEMESS)
                {
                    fingerstr = "Imaging error";
                }else{
                    fingerstr = "Unknown error:"+String(fingerErr);
                }
            }else{
                fingerstr = "Last finger: "+String(lastFinger);
            }

            ESPUI.updateLabel(fpStatLabel,fingerstr);
        }

        // Controls
        char controlStr[80];
        VehicleControls::ControlState controls = vcu->controls.getControls();
        snprintf(controlStr,80,"Connected: %d, Mode: %d, Ind: %d, Light: %d, Walk: %d, Brake: %d, Throttle: %f\n",controls.connected, controls.mode,controls.indicators,controls.light,controls.specialmode,controls.brake,controls.throttle);
        ESPUI.updateLabel(controlsLabel,controlStr);

        snprintf(controlStr,80,"PErr: %f, Ierr: %f, Derr: %f\n",vcu->speedPv,vcu->speedIv,vcu->speedDv);
        ESPUI.updateLabel(tuninglabel,controlStr);
        // Tuning

    }else{
        ESPUI.updateLabel(batLabel,"VCU+BMS not available");
    }
    if(clearFpCount){
        clearFpCount--;
    }
}

void WEBGUI::task(void * pvParameters){
    

    while(1){
        // if(ESPUI.WebSocket()->count() == 0){
        //     vTaskDelay(2000);
        // }else{
        //     vTaskDelay(500);
        // }
        vTaskDelay(1000);
        update();
    }
}

void WEBGUI::dummyCb(Control* sender, int type)
{
    // Serial.print("Text: ID: ");
    // Serial.print(sender->id);
    // Serial.print(", Value: ");
    // Serial.println(sender->value);
}



void WEBGUI::switchCall(Control* sender, int value)
{
    if(sender == ESPUI.getControl(switchHiddenSSID)){
        hiddenAp = value == S_ACTIVE;
    }
}

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
	}
}


void WEBGUI::fpDeleteCb(Control *sender, int type){
    if(vcu && vcu->fingerprint){
        int32_t res = vcu->fingerprint->deleteFinger(ESPUI.getControl(fpSlotSelect)->value.toInt());
        String fingerstr;
        if(res > 0) fingerstr = "Deleted finger: " + String(res);
        else fingerstr = "Error deleting. Code " + String(-res);
        ESPUI.updateLabel(fpStatLabel,fingerstr);
    }
}
void WEBGUI::fpClearCb(Control *sender, int type){
    if(clearFpCount++ > 1){
        if(vcu && vcu->fingerprint){
            clearFpCount = 0;
            int32_t res = vcu->fingerprint->clearAllFingers();
            if(!res)
                ESPUI.updateLabel(fpStatLabel,"Cleared database");
        }
    }else{
        ESPUI.updateLabel(fpStatLabel,"Click again to clear");
    }
}

void WEBGUI::fpAddCb(Control *sender, int type){
    if(vcu && vcu->fingerprint)
        vcu->fingerprint->startEnrollment(ESPUI.getControl(fpSlotSelect)->value.toInt()); // Start in fingerprint thread
}



void WEBGUI::setup(VCU* vcu)
{
    // Serial.begin(115200);
    WEBGUI::vcu = vcu;

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
    uint16_t tabFingerprint = ESPUI.addControl(ControlType::Tab, "fingerprint", "Fingerprint");
    uint16_t tabWifi = ESPUI.addControl(ControlType::Tab, "tabsysconf", "System config");
    
    // status = ESPUI.addControl(ControlType::Label, "Status:", "Stop", ControlColor::Turquoise);
    
    // BMS
    ESPUI.addControl(ControlType::Separator, "BMS", "", ControlColor::None, tabstatus);
    batLabel = ESPUI.addControl(ControlType::Label, "BMS Status", "None", ControlColor::Peterriver,tabstatus);
    batCellsLabel = ESPUI.addControl(ControlType::Label, "BMS Cells", "None", ControlColor::Peterriver,tabstatus);

    // Controls
    ESPUI.addControl(ControlType::Separator, "Controls", "", ControlColor::None, tabstatus);
    controlsLabel = ESPUI.addControl(ControlType::Label, "Control state", "None", ControlColor::Turquoise,tabstatus);


    // Drive modes
    ESPUI.addControl(ControlType::Separator, "Tuning", "", ControlColor::None, tabModes);
    uint16_t speedControlP = ESPUI.addControl(Number, "P", String(vcu->speedP), Peterriver, tabModes, [vcu](Control *sender, int t){if(t==N_VALUE) vcu->speedP = sender->value.toFloat();});
    uint16_t speedControlI = ESPUI.addControl(Number, "I", String(vcu->speedI), Peterriver, tabModes, [vcu](Control *sender, int t){if(t==N_VALUE) vcu->speedI = sender->value.toFloat();});
    uint16_t speedControlD = ESPUI.addControl(Number, "D", String(vcu->speedD), Peterriver, tabModes, [vcu](Control *sender, int t){if(t==N_VALUE) vcu->speedD = sender->value.toFloat();});
    uint16_t speedControlIlim = ESPUI.addControl(Number, "I limit", String(vcu->speedIlim), Peterriver, tabModes, [vcu](Control *sender, int t){if(t==N_VALUE) vcu->speedIlim = sender->value.toFloat();});
    uint16_t speedControlIlimneg = ESPUI.addControl(Number, "I limit neg", String(vcu->speedIlimneg), Peterriver, speedControlIlim, [vcu](Control *sender, int t){if(t==N_VALUE) vcu->speedIlimneg = sender->value.toFloat();});

    tuninglabel = ESPUI.addControl(ControlType::Label, "Tune info", "None", ControlColor::Turquoise,tabModes);
    ESPUI.addControl(Min, "", "1", None, speedControlP);
	ESPUI.addControl(Max, "", "300", None, speedControlP);


    // Fingerprint
    
    fpStatLabel = ESPUI.addControl(ControlType::Label, "Fingerprint status", "None", ControlColor::Turquoise,tabFingerprint);
    uint16_t slotLabel = ESPUI.addControl(ControlType::Label, "Actions", "Modify slot: ", ControlColor::Peterriver,tabFingerprint);
    fpSlotSelect = ESPUI.addControl(Number, "Actions", "0", Peterriver, slotLabel, dummyCb);
    ESPUI.setElementStyle(slotLabel,"height: auto;width: auto");
    ESPUI.addControl(Min, "", "0", None, fpSlotSelect);
	ESPUI.addControl(Max, "", "300", None, fpSlotSelect);
    uint16_t fpEnroll = ESPUI.addControl(Button, "Actions", "Add finger", Peterriver, slotLabel, fpAddCb);
    uint16_t fpDelete = ESPUI.addControl(Button, "Actions", "Delete finger", Peterriver, slotLabel, fpDeleteCb);
    uint16_t fpClear = ESPUI.addControl(Button, "Actions", "Clear database (Click 2x)", Peterriver, slotLabel, fpClearCb);
    ESPUI.addControl(ControlType::Label, "Actions", "Info: Slots 0-9 have master permissions.\nAll other slots have regular user permissions", ControlColor::Peterriver,slotLabel);

    
    // Wifi settings
    ESPUI.addControl(ControlType::Separator, "External AP mode", "", ControlColor::None, tabWifi);
    wifi_ssid_text = ESPUI.addControl(Text, "Connect to external AP SSID\n(Leave empty to disable)", NVS.getString(nvskey_SSID,""), ControlColor::Wetasphalt, tabWifi, dummyCb);
	//Note that adding a "Max" control to a text control sets the max length
	ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
	wifi_pass_text = ESPUI.addControl(Text, "External AP Password", NVS.getString(nvskey_WIFIPASS,""), ControlColor::Wetasphalt, tabWifi, dummyCb);
	ESPUI.addControl(Max, "", "64", None, wifi_pass_text);

	ESPUI.addControl(ControlType::Separator, "Direct AP mode (Enabled when no AP found)", "", ControlColor::None, tabWifi);
    wifi_ap_ssid_text = ESPUI.addControl(Text, "Direct AP SSID", NVS.getString(nvskey_APSSID,apssid), Turquoise, tabWifi, dummyCb);
	ESPUI.addControl(Max, "", "32", None, wifi_ap_ssid_text);
    wifi_ap_pass_text = ESPUI.addControl(Text, "Direct AP Password", NVS.getString(nvskey_APPASS,apPass), Turquoise, tabWifi, dummyCb);
    ESPUI.addControl(Max, "", "64", None, wifi_ap_pass_text);

    switchHiddenSSID = ESPUI.addControl(  ControlType::Switcher, "Hidden SSID", NVS.getInt(nvskey_APHIDDEN,0) ? "1" : "0", ControlColor::Turquoise, tabWifi, &switchCall);
    
    ESPUI.addControl(ControlType::Separator, "System", "", ControlColor::None, tabWifi);
    saveWifiBtn = ESPUI.addControl(Button, "Actions", "Save", Peterriver, tabWifi, enterWifiDetailsCallback);
    ESPUI.addControl(Button, "Actions", "Reboot", Peterriver, saveWifiBtn, [](Control* target, int val){esp_restart();});
    // OTA
    uint16_t otaframe = ESPUI.addControl( ControlType::Label, "OTA Update", "<iframe src=/ota width=100%></iframe>", ControlColor::Carrot, tabWifi );
    ESPUI.setElementStyle(otaframe,"background-color: transparent;");

    // Start Webserver
    ESPUI.begin("Hyper VCU");
    AsyncOTA.begin(ESPUI.WebServer());
}

