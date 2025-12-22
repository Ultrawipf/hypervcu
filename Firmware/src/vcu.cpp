#include "vcu.h"


TaskHandle_t* VCU::taskHandle;
VCU* VCU::instance = 0;

TaskHandle_t* VehicleControls::taskHandle;
VehicleControls* VehicleControls::instance = 0;

VCU::VCU(VehicleControls& controls,FingerprintReader* fingerprint)
: controls(controls),fingerprint(fingerprint),vesc(25)
{
    instance = this;
    if(fingerprint){
        fingerprint->registerCallback([](int finger){instance->fpCb(finger);});
    }
}

void VCU::task(void * pvParameters){
    controls.setup();
    controls.run();
    setupVesc();
    
    setLocked(true);
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
    

        if(newControls.indicators != curControlState.indicators && curControlState.indicators == 0){
            curLightState.indicatorCounter = 0; // Reset indicator counter
        }else{
            curLightState.indicatorCounter += 1;
        }

        if(newControls.mode != curControlState.mode){
            if(newControls.mode == 0) masterMode = false;
            updateDriveMode(newControls,masterMode ? 1 : 0);
        }
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
        if(curControlState.mode != 0){
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
        vesc.setBrakeCurrent(10);
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

}

float VCU::calcSpeedScale(float wheelDiamMM,int poles){
    return ((wheelDiamMM/10)*0.001885f) / (poles / 2);
}

void VCU::updateMotor(){
    if(locked){
        vesc.setBrakeCurrent(lockCurrent);
        return;
    }
    if(!curControlState.connected || curControlState.mode == 0 || curDriveMode == nullptr){
        vesc.setCurrent(0);
        return;
    }
    float current = 0;
    if(curDriveMode->currentMode){
        // Current control mode
        current = curDriveMode->maxCurrent * curControlState.throttle;
        if(curSpeedKmh >= curDriveMode->maxSpeed){
            // Current is only reduced when exceeding limit
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
        speedDv = speedPv-lastSpeedPv;
        speedIv += speedPv;
        speedIv = max(speedIlimneg,min(speedIlim,speedIv)); // Limit

        current = (speedPv * speedP + speedIv * speedI + speedDv * speedD)*speedPIDscale;
        
        if(curSpeedKmh > curDriveMode->maxSpeed){
            current -= (curSpeedKmh-curDriveMode->maxSpeed) * 2;
        }
    }

    float minLim = 0.1f;
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
        vesc.setCurrent(current);
    }
    
}

void VCU::updateDisplayState(){
    
    controls.currentDisplay.batterypct = curBmsdata.CapacityRemainPercent;
    controls.currentDisplay.light = curLightState.front;
    controls.currentDisplay.speedkmh = curSpeedKmh; // get from vesc
    controls.currentDisplay.indicators = (curLightState.indl & 0x1) | ((curLightState.indr & 0x1) << 1);
    controls.currentDisplay.warning = masterMode;
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


