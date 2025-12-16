#include <Arduino.h>
#include "jbd_bms_data.h"

/**
 * Parser partially based on https://github.com/A-damW/JBD_BMS_BLE_VESC_EXPRESS_BRIDGE
 * Licensed under GPL-3.0
 */

const byte cBasicInfo3 = 3;        // type of packet 3= basic info
const byte cCellInfo4 = 4;         // type of packet 4= individual cell info
packBasicInfoStruct packBasicInfo; // structures for BMS data
packCellInfoStruct packCellInfo;   // structures for BMS data

bool isPacketValid(byte *packet) // check if packet is valid
{
    if (packet == nullptr)
    {
        return false;
    }

    bmsPacketHeaderStruct *pHeader = (bmsPacketHeaderStruct *)packet;
    int checksumLen = pHeader->dataLen + 2; // status + data len + data

    if (pHeader->start != 0xDD)
    {
        return false;
    }

    int offset = 2; // header 0xDD and command type are skipped

    byte checksum = 0;
    for (int i = 0; i < checksumLen; i++)
    {
        checksum += packet[offset + i];
    }

    //   printf("checksum: %x\n", checksum);

    checksum = ((checksum ^ 0xFF) + 1) & 0xFF;
    //   printf("checksum v2: %x\n", checksum);

    byte rxChecksum = packet[offset + checksumLen + 1];

    if (checksum == rxChecksum)
    {
        // printf("Packet is valid\n");
        return true;
    }
    else
    {
        printf("Packet is not valid\n");
        printf("Expected value: %x\n", rxChecksum);

        return false;
    }
}

bool processBasicInfo(packBasicInfoStruct *output, byte *data, unsigned int dataLen)
{

    // Expected data len. Can be longer depending on NTCs
    if (dataLen < 0x1B)
    {
        return false;
    }
    // String myString = String(&data);
    output->NumOfCells = ((byte)data[21]);
    float nominalVoltage = 3.6 * output->NumOfCells;
    output->Volts = ((uint32_t)two_ints_into16(data[0], data[1])) * 10; // Resolution 10 mV -> convert to milivolts   eg 4895 > 48950mV
    output->Amps = ((int32_t)two_ints_into16(data[2], data[3])) * 10;   // Resolution 10 mA -> convert to miliamps

    output->Watts = output->Volts * output->Amps / 1000000; // W
    output->FullCapacity = ((uint16_t)two_ints_into16(data[6], data[7]));
    // output->CapacityRemainAh = ((uint16_t)two_ints_into16(data[4], data[5])) * 10;
    output->CapacityRemainAh = ((uint16_t)output->FullCapacity * output->CapacityRemainPercent / 100);

    output->Cycles = ((uint16_t)two_ints_into16(data[8], data[9]));

    output->CapacityRemainPercent = ((uint8_t)data[19]);

    output->CapacityRemainWh = ((uint32_t)(output->FullCapacity / 100 * (nominalVoltage * 10) * output->CapacityRemainPercent / 100));

    output->Temp1 = (((uint16_t)two_ints_into16(data[23], data[24])) - 2731);
    output->Temp2 = (((uint16_t)two_ints_into16(data[25], data[26])) - 2731);
    output->BalanceCodeLow = (two_ints_into16(data[12], data[13]));
    output->BalanceCodeHigh = (two_ints_into16(data[14], data[15]));
    output->MosfetStatus = ((byte)data[20]);

    return true;
};

bool processCellInfo(packCellInfoStruct *output, byte *data, unsigned int dataLen)
{

    uint16_t _cellSum;
    uint16_t _cellMin = 5000;
    uint16_t _cellMax = 0;
    uint16_t _cellAvg;
    uint16_t _cellDiff;

    output->NumOfCells = dataLen / 2; // Data length * 2 is number of cells !!!!!!

    // go trough individual cells
    for (byte i = 0; i < dataLen / 2; i++)
    {
        output->CellVolt[i] = ((uint16_t)two_ints_into16(data[i * 2], data[i * 2 + 1])); // Resolution 1 mV
        _cellSum += output->CellVolt[i];
        if (output->CellVolt[i] > _cellMax)
        {
            _cellMax = output->CellVolt[i];
        }
        if (output->CellVolt[i] < _cellMin)
        {
            _cellMin = output->CellVolt[i];
        }
    }
    output->CellMin = _cellMin;
    output->CellMax = _cellMax;
    output->CellDiff = _cellMax - _cellMin; // Resolution 10 mV -> convert to volts
    output->CellAvg = _cellSum / output->NumOfCells;

    //----cell median calculation----
    uint16_t n = output->NumOfCells;
    uint16_t i, j;
    uint16_t temp;
    uint16_t x[n];

    for (uint8_t u = 0; u < n; u++)
    {
        x[u] = output->CellVolt[u];
    }

    for (i = 1; i <= n; ++i) // sort data
    {
        for (j = i + 1; j <= n; ++j)
        {
            if (x[i] > x[j])
            {
                temp = x[i];
                x[i] = x[j];
                x[j] = temp;
            }
        }
    }

    if (n % 2 == 0) // compute median
    {
        output->CellMedian = (x[n / 2] + x[n / 2 + 1]) / 2;
    }
    else
    {
        output->CellMedian = x[n / 2 + 1];
    }

    for (uint8_t q = 0; q < output->NumOfCells; q++)
    {
        uint32_t disbal = abs(output->CellMedian - output->CellVolt[q]);
        // output->CellColorDisbalance[q] = getPixelColorHsv(mapHue(disbal, c_cellMaxDisbalance, 0), 255, 255);
    }
    return true;
};

bool bmsProcessPacket(byte *packet)
{

    bool isValid = isPacketValid(packet);

    if (isValid != true)
    {
        Serial.println("Invalid packet received");
        return false;
    }

    bmsPacketHeaderStruct *pHeader = (bmsPacketHeaderStruct *)packet;
    byte *data = packet + sizeof(bmsPacketHeaderStruct); // TODO Fix this ugly hack
    unsigned int dataLen = pHeader->dataLen;

    bool result = false;

    // |Decision based on packet type (info3 or info4)
    switch (pHeader->type)
    {
    case cBasicInfo3:
    {
        // Process basic info
        result = processBasicInfo(&packBasicInfo, data, dataLen);
        // newPacketReceived = true;
        break;
    }

    case cCellInfo4:
    {
        result = processCellInfo(&packCellInfo, data, dataLen);
        // newPacketReceived = true;
        break;
    }

    default:
        result = false;
        Serial.printf("Unsupported packet type detected. Type: %d", pHeader->type);
    }

    return result;
}

bool bleCollectPacket(char *data, uint32_t dataSize) // reconstruct packet from BLE incomming data, called by notifyCallback function
{

    static uint8_t packetstate = 0;
    static uint8_t packetbuff[60] = {0x0};
    static uint32_t curDataSize = 0;
    static uint32_t headerDataSize = 0;
    bool retVal = false;
    //   hexDump(data,dataSize);
    // Serial.print("bleCollectPacket_1");
    if (data[0] == 0xdd && packetstate == 0) // probably got 1st half of packet
    {
        packetstate = 1;
        curDataSize = dataSize;
        headerDataSize = data[3];
        for (uint8_t i = 0; i < dataSize; i++)
        {
            packetbuff[i] = data[i];
        }
        return false; // Not yet ready
    }
    // Must have returned on first packet already
    if (packetstate == 1) // Packet may consist of more than 2 segments
    {
        // packetstate = 2;
        for (uint8_t i = 0; i < min(sizeof(packetbuff) - curDataSize, dataSize); i++)
        {
            packetbuff[i + curDataSize] = data[i];
        }
        curDataSize += dataSize;

        retVal = false;
    }

    if (curDataSize >= (headerDataSize + 6) || curDataSize >= sizeof(packetbuff))
    { // Full packet received. Length minus header and end
        if (packetbuff[curDataSize - 1] == 0x77)
        { // End marker
            uint8_t packet[curDataSize];
            memcpy(packet, packetbuff, curDataSize);
            bmsProcessPacket(packet); // pass pointer to retrieved packet to processing function

            retVal = true;
        }
        else
        {
            Serial.println("Size exceeded");
            retVal = false;
        }
        packetstate = 0;
    }

    //   Serial.print("Header size: ");
    //   Serial.println(headerDataSize);
    //   Serial.print("Data size: ");
    //   Serial.println(dataSize);
    //   Serial.print("Cur Data size: ");
    //   Serial.println(curDataSize);

    return retVal;
}

void printBasicInfo() // debug all data to uart
{

    Serial.printf("Total voltage: %.2f\n", (float)packBasicInfo.Volts / 1000);
    Serial.printf("Amps: %.2f\n", (float)packBasicInfo.Amps / 1000);
    Serial.printf("CapacityRemain Ah: %.2f\n", (float)packBasicInfo.CapacityRemainAh / 100);
    Serial.printf("CapacityRemain %%: %.2f\n", (float)packBasicInfo.CapacityRemainPercent);
    Serial.printf("CapacityRemain kWh: %.2f\n", (float)packBasicInfo.CapacityRemainWh / 10000);
    Serial.printf("FullCapacity Ah: %.2f\n", (float)packBasicInfo.FullCapacity / 100);
    Serial.printf("Cycles: %d\n", packBasicInfo.Cycles);
    Serial.printf("Temp1: %.2f\n", (float)packBasicInfo.Temp1 / 10);
    Serial.printf("Temp2: %.2f\n", (float)packBasicInfo.Temp2 / 10);
    Serial.printf("Balance Code Low: 0x%x\n", packBasicInfo.BalanceCodeLow);
    Serial.printf("Balance Code High: 0x%x\n", packBasicInfo.BalanceCodeHigh);
    Serial.printf("Mosfet Status: 0x%x\n", packBasicInfo.MosfetStatus);
    Serial.printf("Number of cells: %d\n", packBasicInfo.NumOfCells);
}

void printCellInfo() // debug all data to uart
{

    Serial.printf("Number of cells: %u\n", packCellInfo.NumOfCells);
    for (byte i = 1; i <= packCellInfo.NumOfCells; i++)
    {
        Serial.printf("Cell no. %u", i);
        Serial.printf("   %.3f\n", (float)packCellInfo.CellVolt[i - 1] / 1000);
    }
    Serial.printf("Max cell volt: %.3f\n", (float)packCellInfo.CellMax / 1000);
    Serial.printf("Min cell volt: %.3f\n", (float)packCellInfo.CellMin / 1000);
    Serial.printf("Difference cell volt: %.3f\n", (float)packCellInfo.CellDiff / 1000);
    Serial.printf("Average cell volt: %.3f\n", (float)packCellInfo.CellAvg / 1000);
    Serial.printf("Median cell volt: %.3f\n", (float)packCellInfo.CellMedian / 1000);
    Serial.println();
}

void hexDump(const char *data, uint32_t dataSize) // debug function
{

    Serial.println("HEX data:");

    for (int i = 0; i < dataSize; i++)
    {
        Serial.printf("0x%x, ", data[i]);
    }
    Serial.println("");
}

int16_t two_ints_into16(int highbyte, int lowbyte) // turns two bytes into a single long integer
{

    int16_t result = (highbyte);
    result <<= 8;                // Left shift 8 bits,
    result = (result | lowbyte); // OR operation, merge the two
    return result;
}