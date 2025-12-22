#include "t6e.h"


T6E::T6E(BMS& bms) : bms(bms){
    instance = this;
}

bool T6E::setup(){
    DISP_SERIAL.begin(9600);
    Serial.println("Starting T6E");

    return true;
}

void T6E::sendState(){
    char tbuf[20] = {0x02,0x14,0x03,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x64,0x00,0x1E,00, 00, 00, 00, 00, 00,0x00};
    if(currentDisplay.warning) tbuf[4] = 0x20;
    if(currentDisplay.brake) tbuf[5] = 0xA0;
    int32_t kmhScaled = abs(currentDisplay.speedkmh) * 10.0f;
    tbuf[8] = (kmhScaled >> 8) & 0xff;
    tbuf[9] = (kmhScaled) & 0xff;
    tbuf[10] = currentDisplay.batterypct;
    tbuf[13] = currentDisplay.errors;
    tbuf[18] = currentDisplay.indicators;

    tbuf[19] = calcChecksum(tbuf,19);
    memcpy(txbuf,tbuf,20);
    DISP_SERIAL.write(txbuf,sizeof(txbuf));
}

char T6E::calcChecksum(char* buf,size_t len){
    char s = 0x00;
    for(uint8_t i = 0; i<len;i++){
        s = s ^ buf[i];
    }
    return s;
}

/**
 * Parses received buffer into the current control struct
 */
void T6E::parseControls(char* buf){
    ControlState controls;
    controls.connected = true;
    controls.brake = !digitalRead(BRAKE_N);
    controls.throttle = (buf[17] | buf[16] << 8) / 1000.0f;
    controls.indicators = buf[18];
    controls.light = buf[5] & 0x20 ? 1 : 0;
    switch(buf[4]){
        case 5: controls.mode = 1; break;
        case 10: controls.mode = 2; break;
        case 15: controls.mode = 3; break;
        default: controls.mode = 0;
    }
    if(buf[5] & 0x01){
        controls.specialmode=1;
    } //+ walk mode
    currentControls = controls;
    // Serial.printf("Mode: %d, Ind: %d, Light: %d, Walk: %d, Brake: %d, Throttle: %f\n",controls.mode,controls.indicators,controls.light,controls.specialmode,controls.brake,controls.throttle);
}

void T6E::updateControls(){
    
    if(DISP_SERIAL.available() >= 20){
        // Align buffer
        while((rxbuf[0] = DISP_SERIAL.read()) != 0x01){delay(1);}
        while((rxbuf[1] = DISP_SERIAL.read()) != 0x14){delay(1);}
        while(DISP_SERIAL.available() < 18){delay(10);} // Wait if we went too far
        // int pos = DISP_SERIAL.readBytesUntil(0x01,this->rxbuf,40); // Find start
        
        DISP_SERIAL.readBytes(rxbuf+2,18);
        if(calcChecksum(rxbuf,19) == rxbuf[19]){
            parseControls(rxbuf);
            timeoutcount = 0;
        }else{
            Serial.println("Checksum mismatch");
            char printbuf[100] = {0};
            int p = 0;
            for(uint8_t i = 0;i<20;i++){
                p+=snprintf(printbuf+p,100,"%02x",rxbuf[i]);
            }
            Serial.println(printbuf);
        }
    }
    if(timeoutcount > 20){ // 2s
        if(currentControls.connected)
            currentControls = ControlState(); // Safe default
    }else{
        timeoutcount += 1;
    }
}

void T6E::task(void * pvParameters){
    while(1){
        delay(50);
        updateControls();
        sendState();
    }
}


VehicleControls::ControlState T6E::getControls(){
    
    return currentControls;
}

BMS* T6E::getBMS(){
    return &bms;
}