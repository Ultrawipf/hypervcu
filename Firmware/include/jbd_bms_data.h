#ifndef jbd_bms_data_H_
#define jbd_bms_data_H_

/**
 * Based on https://github.com/A-damW/JBD_BMS_BLE_VESC_EXPRESS_BRIDGE/blob/main/JBD_BMS_BLE_VESC_EXPRESS_BRIDGE/mydatatypes.h
 */

typedef struct
{
	byte start;
	byte type;
	byte status;
	byte dataLen;
} bmsPacketHeaderStruct;

typedef struct
{
	uint16_t Volts; // unit 1mV
	int32_t Amps;	// unit 1mA
	int32_t Watts;	// unit 1W
	uint16_t CapacityRemainAh;
	uint16_t FullCapacity;
	uint16_t Cycles;
	uint8_t CapacityRemainPercent; // unit 1%
	uint32_t CapacityRemainWh;	   // unit Wh
	uint16_t Temp1;				   // unit 0.1C
	uint16_t Temp2;				   // unit 0.1C
	uint16_t BalanceCodeLow;
	uint16_t BalanceCodeHigh;
	uint8_t MosfetStatus;
	uint8_t NumOfCells;
	// uint8_t NumNtc;
	// uint8_t NtcVal;

} packBasicInfoStruct;

typedef struct
{
	uint8_t NumOfCells;
	uint16_t CellVolt[20]; // cell 1 has index 0 :-/
	uint16_t CellMax;
	uint16_t CellMin;
	uint16_t CellDiff; // difference between highest and lowest
	uint16_t CellAvg;
	uint16_t CellMedian;
	uint32_t CellColor[15];
	uint32_t CellColorDisbalance[15]; // green cell == median, red/violet cell => median + c_cellMaxDisbalance
} packCellInfoStruct;

struct packEepromStruct
{
	uint16_t POVP;
	uint16_t PUVP;
	uint16_t COVP;
	uint16_t CUVP;
	uint16_t POVPRelease;
	uint16_t PUVPRelease;
	uint16_t COVPRelease;
	uint16_t CUVPRelease;
	uint16_t CHGOC;
	uint16_t DSGOC;
};

#define STRINGBUFFERSIZE 300



extern packBasicInfoStruct packBasicInfo; // structures for BMS data
extern packCellInfoStruct packCellInfo;   // structures for BMS data


// Functions

bool isPacketValid(byte *packet);
bool processBasicInfo(packBasicInfoStruct *output, byte *data, unsigned int dataLen);
bool processCellInfo(packCellInfoStruct *output, byte *data, unsigned int dataLen);
bool bmsProcessPacket(byte *packet);
bool bleCollectPacket(char *data, uint32_t dataSize);
void sendCommand(uint8_t *data, uint32_t dataLen);

// Debug
void printBasicInfo();
void printCellInfo();
void hexDump(const char *data, uint32_t dataSize);
int16_t two_ints_into16(int highbyte, int lowbyte);
void constructBigString();

#endif /* jbd_bms_data_H_ */