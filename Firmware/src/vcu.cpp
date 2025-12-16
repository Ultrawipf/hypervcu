#include "vcu.h"


VCU::VCU(VehicleControls& controls)
: controls(controls)
{

}

void VCU::task(void * pvParameters){
    running = true;
    
    while(running){
        vTaskDelay(interval);
    }
}


T6E::T6E(){


}
void T6E::sendState(){

}
VehicleControls::ControlState T6E::getControls(){
    return ControlState();
}