#include <Arduino.h>
#include <NimBLEDevice.h>

extern NimBLEAddress bmsClientAddr;
void jbdBmsTask(void * pvParameters);
bool requestBmsState(NimBLEAddress bmsClientAddr, bool requestCellInfo = false);
bool subscribeBms(const NimBLEAddress bmsClientAddr);
void bmsNotifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

void setupBms();