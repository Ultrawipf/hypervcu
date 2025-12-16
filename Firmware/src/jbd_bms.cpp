#include <Arduino.h>
#include <NimBLEDevice.h>
#include "jbd_bms_data.h"
#include "jbd_bms.h"
#include "pins.h"

static BLEUUID serviceUUID_BMS("0000ff00-0000-1000-8000-00805f9b34fb");  //xiaoxiang/JBD bms original module
static BLEUUID charUUID_rx_BMS("0000ff01-0000-1000-8000-00805f9b34fb");  //xiaoxiang/JBD bms original module
static BLEUUID charUUID_tx_BMS("0000ff02-0000-1000-8000-00805f9b34fb");  //xiaoxiang/JBD bms original module


static constexpr uint32_t scanTimeMs = 5 * 1000;

NimBLEAddress bmsClientAddr;
bool bmsSubscribed = false;


void bmsNotifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    // Process bms packet
    bleCollectPacket((char*)pData, length);
}

bool subscribeBms(const NimBLEAddress bmsClientAddr){
    NimBLERemoteService* pSvc = nullptr;
    NimBLERemoteCharacteristic* pChr = nullptr;
    
    NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(bmsClientAddr);
    if(!pClient || !pClient->isConnected()){
        DBG_SERIAL.println("Not connected");
        return false;
    }
    DBG_SERIAL.printf(pClient->toString().c_str());
    // DBG_SERIAL.println("Getting service");
    pSvc = pClient->getService(serviceUUID_BMS);
    if (!pSvc) { /** make sure it's not null */
        DBG_SERIAL.println("Service not found");
        return false;
    }
    pChr = pSvc->getCharacteristic(charUUID_rx_BMS);
    
    if (pChr) { /** make sure it's not null */
        if (pChr->canNotify()) {
            if (!pChr->subscribe(true, bmsNotifyCB)) {
                /** Disconnect if subscribe failed */
                DBG_SERIAL.print("Failed subscribe");
                pClient->disconnect();
                return false;
            }
        } else if (pChr->canIndicate()) {
            /** Send false as first argument to subscribe to indications instead of notifications */
            if (!pChr->subscribe(false, bmsNotifyCB)) {
                /** Disconnect if subscribe failed */
                DBG_SERIAL.print("Failed subscribe");
                pClient->disconnect();
                return false;
            }
        }else{
            DBG_SERIAL.print("## Can NOT notify or indicate?!");
            return false;
        }
    }else{
        DBG_SERIAL.print("## Char missing");
    }
    bmsSubscribed = true;
    return true;
}

bool requestBmsState(NimBLEAddress bmsClientAddr, bool requestCellInfo)
{
    NimBLEClient *pClient = NimBLEDevice::getClientByPeerAddress(bmsClientAddr);
    if (!pClient || !pClient->isConnected())
    {
        DBG_SERIAL.println("pClient is NULL or not connected");
    }
    else
    {
        DBG_SERIAL.println(pClient->getPeerAddress().toString().c_str());

        NimBLERemoteService *pSvc = nullptr;
        NimBLERemoteCharacteristic *pChr = nullptr;

        pSvc = pClient->getService(serviceUUID_BMS);
        if (pSvc)
        { /** make sure it's not null */
            pChr = pSvc->getCharacteristic(charUUID_tx_BMS);
            if (pChr->canWriteNoResponse())
            {
                uint8_t bmsBasicInfoReq[7] = {0xdd, 0xa5, 0x3, 0x0, 0xff, 0xfd, 0x77}; // Basic info
                if (!pChr->writeValue(bmsBasicInfoReq, sizeof(bmsBasicInfoReq), true))
                {
                    DBG_SERIAL.println("pChr->writeValue FAILED");
                }
                if (requestCellInfo)
                {
                    uint8_t bmsCellInfoReq[7] = {0xdd, 0xa5, 0x4, 0x0, 0xff, 0xfc, 0x77}; // Cell info
                    if (!pChr->writeValue(bmsCellInfoReq, sizeof(bmsCellInfoReq), true))
                    {
                        DBG_SERIAL.println("pChr->writeValue FAILED");
                    }
                }
            }
        }
    }
    return false;
}

// ---- NIMBLE COMMON

class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override {
        DBG_SERIAL.printf("Connected to: %s\n", pClient->getPeerAddress().toString().c_str());        
    }

    void onDisconnect(NimBLEClient* pClient, int reason) override {
        if(pClient->getPeerAddress() == bmsClientAddr){
            bmsClientAddr = NimBLEAddress();
            bmsSubscribed = false;
        }
        DBG_SERIAL.printf("%s Disconnected, reason = %d - Starting scan\n", pClient->getPeerAddress().toString().c_str(), reason);
        NimBLEDevice::getScan()->start(scanTimeMs);
    }
} clientCallbacks;

class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        // DBG_SERIAL.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());
        //if (advertisedDevice->haveName() && advertisedDevice->getName() == "NimBLE-Server") {
        if (advertisedDevice->isAdvertisingService(NimBLEUUID(serviceUUID_BMS))) {
            DBG_SERIAL.printf("Found BMS Device\n");

            /** Async connections can be made directly in the scan callbacks */
            auto pClient = NimBLEDevice::getDisconnectedClient();
            if (!pClient) {
                pClient = NimBLEDevice::createClient(advertisedDevice->getAddress());
                if (!pClient) {
                    DBG_SERIAL.printf("Failed to create client\n");
                    return;
                }
            }
            bmsClientAddr = pClient->getPeerAddress();

            pClient->setClientCallbacks(&clientCallbacks, false);
            if (!pClient->connect(true, true, false)) { // delete attributes, async connect, no MTU exchange
                NimBLEDevice::deleteClient(pClient);
                DBG_SERIAL.printf("Failed to connect\n");
                return;
            }
        }
    }

    void onScanEnd(const NimBLEScanResults& results, int reason) override {
        DBG_SERIAL.printf("Scan Ended\n");
        if(bmsClientAddr.isNull()) // Restart
            NimBLEDevice::getScan()->start(scanTimeMs);
    }
} scanCallbacks;


// END NIMBLE COMMON

void jbdBmsTask(void * pvParameters){
    while(1){
        vTaskDelay(1000);

        if(!bmsClientAddr.isNull()){
            // DBG_SERIAL.printf("We are connected\n");
            if(!bmsSubscribed){
                subscribeBms(bmsClientAddr);
            }else{
                requestBmsState(bmsClientAddr,false);
                // printCellInfo();
                printBasicInfo();
            }
        }
    }
}

void setupBms() {
    // DBG_SERIAL.begin(115200);
    DBG_SERIAL.printf("Starting NimBLE Async Client\n");
    NimBLEDevice::init("Async-Client");
    NimBLEDevice::setPower(3); /** +3db */

    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(&scanCallbacks);
    pScan->setInterval(45);
    pScan->setWindow(45);
    pScan->setActiveScan(true);
    pScan->start(scanTimeMs);


        //     auto pClients = NimBLEDevice::getConnectedClients();
        // if (!pClients.size()) {
        //     DBG_SERIAL.printf("No clients. Start scan\n");
        //     NimBLEDevice::getScan()->start(scanTimeMs);
            
        //     continue;
        // }

}

// void loop() {
    
//     // for (auto& pClient : pClients) {
//     //     DBG_SERIAL.printf("%s\n", pClient->toString().c_str());
//     //     NimBLEDevice::deleteClient(pClient);
//     // }

//     // NimBLEDevice::getScan()->start(scanTimeMs);
// }
