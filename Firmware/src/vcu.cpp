#include "vcu.h"
#include <ArduinoNvs.h>

TaskHandle_t* VCU::taskHandle;
VCU* VCU::instance = 0;

TaskHandle_t* VehicleControls::taskHandle;
VehicleControls* VehicleControls::instance = 0;

const String nvsKeys_Modes[VCU::NUMDRIVESUBMODES][VCU::NUMDRIVEMODES][3] = {
    {{"M1CUR","M1SPD","M1MOD"},{"M2CUR","M2SPD","M2MOD"},{"M3CUR","M3SPD","M3MOD"}},
    {{"M1BCUR","M1BSPD","M1BMOD"},{"M2BCUR","M2BSPD","M2BMOD"},{"M3BCUR","M3BSPD","M3BMOD"}}
};

const String nvsKey_P = "PID_P";
const String nvsKey_I = "PID_I";
const String nvsKey_D = "PID_D";
const String nvsKey_brakecurrent = "BRK_A";
const String nvsKey_offthcurrent = "OFT_BRK_A";

VCU::VCU(VehicleControls& controls,FingerprintReader* fingerprint)
: controls(controls),fingerprint(fingerprint),vesc(25)
{
    instance = this;
    if(fingerprint){
        fingerprint->registerCallback([](int finger){instance->fpCb(finger);});
    }
    // loadSettings();
}

void VCU::saveSettings(){
    for(uint8_t m = 0;m< NUMDRIVESUBMODES;m++){
        for(uint8_t i = 0;i<NUMDRIVEMODES;i++){
            NVS.setFloat(nvsKeys_Modes[m][i][0],driveModes[m][i].maxCurrent,false);
            NVS.setFloat(nvsKeys_Modes[m][i][1],driveModes[m][i].maxSpeed,false);
            uint16_t modeSetting = driveModes[m][i].currentMode ? 1 : 0;
            NVS.setInt(nvsKeys_Modes[m][i][2],modeSetting,false);
        }
    }
    NVS.setFloat(nvsKey_P,speedP,false);
    NVS.setFloat(nvsKey_I,speedI,false);
    NVS.setFloat(nvsKey_D,speedD,false);
    NVS.setFloat(nvsKey_offthcurrent,offThrottleBrake,false);
    NVS.setFloat(nvsKey_brakecurrent,brakeCurrent,false);
    NVS.commit();
}

void VCU::loadSettings(){
    for(uint8_t m = 0;m< NUMDRIVESUBMODES;m++){
        for(uint8_t i = 0;i<NUMDRIVEMODES;i++){
            driveModes[m][i].maxCurrent = NVS.getFloat(nvsKeys_Modes[m][i][0],driveModes[m][i].maxCurrent);
            driveModes[m][i].maxSpeed = NVS.getFloat(nvsKeys_Modes[m][i][1],driveModes[m][i].maxSpeed);
            uint16_t modeSetting = driveModes[m][i].currentMode ? 1 : 0;
            modeSetting = NVS.getInt(nvsKeys_Modes[m][i][2],modeSetting);
        }
    }
    speedP = NVS.getFloat(nvsKey_P,speedP);
    speedI = NVS.getFloat(nvsKey_I,speedI);
    speedD = NVS.getFloat(nvsKey_D,speedD);
    offThrottleBrake = NVS.getFloat(nvsKey_offthcurrent,offThrottleBrake);
    brakeCurrent = NVS.getFloat(nvsKey_brakecurrent,brakeCurrent);
}

void VCU::task(void * pvParameters){
    // loadSettings();
    controls.setup();
    controls.run();
    setupVesc();
    
    setLocked(fingerprint->getConnected()); // Only set locked if fingerprint reader available. Should probably have a UI setting....
    rpmToKmh = calcSpeedScale(wheelDiamMM,poles);

    running = true;

    // Maybe run vesc/speed control and display in different tasks?
    while(running){
        vTaskDelay(interval);

        VehicleControls::ControlState newControls = controls.getControls();
        bool vescOn = digitalRead(VESC_EN);
        if(vescOn){
            
            if(vescValUpdCnt++ >= vescValUpdInterval){
                if(newControls.connected){
                    // vesc.sendKeepalive();
                }
                vescValUpdCnt = 0;
                vescOk = vesc.getVescValues(); // Update values
                lastSpeed = curSpeedKmh;
                curSpeedKmh = vesc.data.rpm * rpmToKmh;
            }

        }else{
            if(vescOk){
                vescOk = false;
                Serial.println("Vesc disconnected");
            }
        }
    
        // Control changes
        if(newControls.indicators != curControlState.indicators && curControlState.indicators == 0){
            curLightState.indicatorCounter = 0; // Reset indicator counter
        }else{
            curLightState.indicatorCounter += 1;
        }

        if(newControls.mode != curControlState.mode){
            if(newControls.mode == 0){ // Reset temporary modes when switching to 0
                masterMode = false;
                zerostart = false;
            }
            updateDriveMode(newControls,masterMode ? 1 : 0);
        }

        // If exiting pedestrian mode and no fingerprint unlock
        if(newControls.brake && curControlState.specialmode && !newControls.specialmode){
            if(!fingerprint->getConnected() && this->locked){
                fpCb(1); // Force master finger
            }
        }

        if(newControls.brake && curSpeedKmh < minspeed){
            // Quick Full throttle pull while standing and holding brake
            if(curControlState.throttle < 0.9 && newControls.throttle == 1.0){
                zerostartWait = true;
            }else if(curControlState.throttle > 0.1 && newControls.throttle == 0 && zerostartWait){
                zerostartWait = false;
                zerostart = !zerostart;
            }
        }
        // Apply new controls
        lastControlState = curControlState;
        curControlState = newControls;
        

        curBmsdata = controls.getBMS()->getBmsBasicData();

        updateLights();
        updateDisplayState();
        updateMotor();
    }
}

void VCU::updateDriveMode(VehicleControls::ControlState& newControls,uint8_t submode){
    changeDriveMode(&driveModes[submode][newControls.mode-1]);
}


void VCU::changeDriveMode(DriveMode* mode){
    curDriveMode = mode;
    // Reset PID, setup limits
    speedPv = 0; speedIv = 0; speedDv = 0;

}

void VCU::fpCb(int finger){
    if(finger < 0){
        // Invalid finger
        // Serial.println("Invalid finger");
        masterMode = false;
        updateDriveMode(curControlState,0);
        return;
    }else if(finger < 10){
        // Master finger
        Serial.println("Master finger");
        if(curControlState.mode != 0 && !locked){ // Do not enter master mode immediately
            masterMode = !masterMode;
        }
    }else{
        // Valid finger
        Serial.println("Valid finger");
        masterMode = false;
    }
    updateDriveMode(curControlState,masterMode ? 1 : 0);
    // Serial.println(finger);
    setLocked(false);
}

void VCU::setLocked(bool locked){
    
    if(this->locked == locked){
        return;
    }
    if(locked){
        // Brake vesc
        controls.currentDisplay.errors |= VCUERR_LOCKED;
        vesc.setBrakeCurrent(lockCurrent);
    }else{
        controls.currentDisplay.errors &= ~VCUERR_LOCKED;
        vesc.setBrakeCurrent(0);
    }
    this->locked = locked;
}

void VCU::setupVesc(){
    VESC_SERIAL.begin(115200);
    vesc.setSerialPort(&VESC_SERIAL);
    // vesc.setDebugPort(&Serial);
    vesc.setCurrent(0);
    if(VESC_EN_MODE == OUTPUT){
        digitalWrite(VESC_EN,1);
    }
    //vesc.setDebugPort(&DBG_SERIAL);

}

float VCU::calcSpeedScale(float wheelDiamMM,int poles){
    return ((wheelDiamMM/10)*0.001885f) / (poles / 2);
}

void VCU::updateMotor(){
    if(locked){
        vesc.setBrakeCurrent(lockCurrent);
        return;
    }
    if(!curControlState.connected || curControlState.mode == 0 || curDriveMode == nullptr || (!zerostart && (curSpeedKmh < minspeed))){
        vesc.setCurrent(0);
        return;
    }
    float current = 0;
    if(curDriveMode->currentMode){
        // Current control mode
        current = curDriveMode->maxCurrent * curControlState.throttle;
        if(curSpeedKmh >= curDriveMode->maxSpeed){
            // Current is only reduced when exceeding limit. TODO Tune this
            speedPv = curDriveMode->maxSpeed-curSpeedKmh;
            double lastSpeedPv = speedPv;
            speedDv = speedPv-lastSpeedPv;
            speedIv += speedPv;
            // current -= (curSpeedKmh - curDriveMode->maxSpeed) * 2;
            speedIv = max(speedIlimneg,min(speedIlim,speedIv)); // Limit
            current = (speedPv * speedP + speedIv * speedI + speedDv * speedD)*speedPIDscale;
        }

        // vesc.setCurrent(current);

    }else{
        // Speed control mode. Target speed set by throttle
        float targetSpeed = curControlState.throttle * curDriveMode->maxSpeed;
        double lastSpeedPv = speedPv;
        speedPv = targetSpeed-curSpeedKmh;
        // speedDv = speedPv-lastSpeedPv; // Derivative of error
        speedDv = lastSpeed-curSpeedKmh; // Alternative based on speed possibly more effective
        speedIv += speedPv;
        speedIv = max(speedIlimneg,min(speedIlim,speedIv)); // Limit

        current = (speedPv * speedP + speedIv * speedI + speedDv * speedD)*speedPIDscale;
        
        if(curSpeedKmh > curDriveMode->maxSpeed){
            // current -= (curSpeedKmh-curDriveMode->maxSpeed) * 2;
            if(speedIv > 0){
                speedIv = speedIv / 2; // Fast decay
            }
        }
    }

    float minLim = idleCurrent;
    if(curControlState.throttle < 0.1){
        minLim = 0;
        speedIv = 0;
        speedDv = 0;
    }        
    if(curControlState.throttle == 0){
            current = 0;
    }else{
        current = max(minLim,min(current,curDriveMode->maxCurrent));
    }

    if(curControlState.brake){
        vesc.setBrakeCurrent(brakeCurrent); // Brake
    }else if(lastControlState.brake && !curControlState.brake){
        vesc.setBrakeCurrent(0); // Do not brake
    }else{ // Not braking. drive
        if(curControlState.throttle == 0.0f && offThrottleBrake && curSpeedKmh > minspeed && curControlState.mode != 0){
            vesc.setBrakeCurrent(offThrottleBrake);
        }else{
            vesc.setCurrent(current);
        }
        
    }
    lastCurrent = current;
    
}

void VCU::updateDisplayState(){
    
    controls.currentDisplay.batterypct = curBmsdata.CapacityRemainPercent;
    controls.currentDisplay.light = curLightState.front;
    controls.currentDisplay.speedkmh = curSpeedKmh; // get from vesc
    controls.currentDisplay.indicators = (curLightState.indl & 0x1) | ((curLightState.indr & 0x1) << 1);
    controls.currentDisplay.warning = masterMode || (zerostart && (curSpeedKmh < minspeed));
}

void VCU::updateLights(){
    // Do effects
    if(!curLightState.overrideLights){
        curLightState.brake = curControlState.brake;
        curLightState.front = curControlState.light & 0x01;
        curLightState.rear = curControlState.light & 0x01;

        if((curLightState.indicatorCounter % curLightState.indicatorPeriod) == 0){
            curLightState.indl = (curControlState.indicators & 0x01) && !curLightState.indl; // left
            curLightState.indr = ((curControlState.indicators & 0x02) >> 1) && !curLightState.indr; // right
        }

        // Flash indicators quickly if moved when locked
        if(locked && (abs(curSpeedKmh) >= minspeedAlarm)){
            if(curLightState.indicatorCounter % curLightState.indicatorPeriod/2 == 0){
                curLightState.indl = !curLightState.indl;
                curLightState.indr = !curLightState.indr;
            }
            
        }
    }


    digitalWrite(LED_FRONT_EN,curLightState.front);
    digitalWrite(LED_BRAKE_EN,curLightState.brake);
    digitalWrite(LED_REAR_EN,curLightState.rear);

    digitalWrite(LED_IND_L_EN,curLightState.indl);
    digitalWrite(LED_IND_R_EN,curLightState.indr);
    digitalWrite(LED_IND_LR_EN,curLightState.indl);
    digitalWrite(LED_IND_RR_EN,curLightState.indr);

    // Set indicator state on display
}


void VCU::taskFuncStatic(void * pvParameters){
    if(!instance) return;
    instance->task(pvParameters);
}
void VCU::run(){
    
    xTaskCreate(
        VCU::taskFuncStatic, // Function that should be called
        "VCU",        // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE+2048,      // Stack size (bytes)
        NULL,      // Parameter to pass
        20,         // Task priority
        VCU::taskHandle       // Task handle
    );


}


VehicleControls::VehicleControls(){
    instance = this;
}

bool VehicleControls::setup(){
    return true;
}

void VehicleControls::taskFuncStatic(void * pvParameters){
    if(!instance) return;
    instance->task(pvParameters);
}

void VehicleControls::run(){
    
    xTaskCreate(
        VehicleControls::taskFuncStatic, // Function that should be called
        "VHCL",        // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE+2048,      // Stack size (bytes)
        NULL,      // Parameter to pass
        17,         // Task priority
        VehicleControls::taskHandle       // Task handle
    );
}

void VehicleControls::task(void * pvParameters){

}


