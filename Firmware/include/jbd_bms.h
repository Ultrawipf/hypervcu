#ifndef bms_H_
#define bms_H_

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "jbd_bms_data.h"

extern NimBLEAddress bmsClientAddr;
void jbdBmsTask(void * pvParameters);
bool requestBmsState(NimBLEAddress bmsClientAddr, bool requestCellInfo = false);
bool subscribeBms(const NimBLEAddress bmsClientAddr);
void bmsNotifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

void setupBms();


class BMS{
public:
    BMS();
    static packBasicInfoStruct getBmsBasicData(bool update = false);
    static packCellInfoStruct  getBmsCellData(bool update = false);
    static bool getBmsConnected();
};

#endif