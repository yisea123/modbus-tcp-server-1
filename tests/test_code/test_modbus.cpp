#include "CppUTest/TestHarness.h"
#include <string.h>
#include <stdio.h>

#define NO_OF_INPUT_REGS           10
#define INPUT_REG_START_ADDRESS    0

extern "C"
{
	#include "mbap_conf.h"
	#include "mbap.h"
}

#define QUERY_SIZE_IN_BYTES              (255u)
#define RESPONSE_SIZE_IN_BYTES           (255u)
#define INPUT_REGISTER_START_ADDRESS     (0u)
#define MAX_INPUT_REGISTERS              (15u)
#define HOLDING_REGISTER_START_ADDRESS   (0u)
#define MAX_HOLDING_REGISTERS            (15u)
#define DISCRETE_INPUTS_START_ADDRESS    (0u)
#define MAX_DISCRETE_INPUTS              (3u)   
#define COILS_START_ADDRESS              (0u)
#define MAX_COILS                        (3u)
#define MBT_EXCEPTION_PACKET_LEN         (9u)
#define QUERY_LEN                        (12u)
#define NO_OF_DATA_OFFSET                (10u)
#define DATA_START_ADDRESS_OFFSET        (8u)
#define REGISTER_VALUE_OFFSET            (10u)
//PDU Offset in response
#define MBT_BYTE_COUNT_OFFSET            (8u)
#define MBT_DATA_VALUES_OFFSET           (9u)
#define MBAP_HEADER_LEN                  (7u)

#define DISCRETE_INPUT_BUF_SIZE          (MAX_DISCRETE_INPUTS / 8u + 1u)
#define COILS_BUF_SIZE                   (MAX_COILS / 8u + 1u)

int16_t g_sInputRegsBuf[MAX_INPUT_REGISTERS]                = {1, 2, 3};
int16_t g_sHoldingRegsBuf[MAX_HOLDING_REGISTERS]            = {5, 6, 7};
uint8_t g_ucDiscreteInputsBuf[DISCRETE_INPUT_BUF_SIZE]      = {0xef};
uint8_t g_ucCoilsBuf[COILS_BUF_SIZE]                        = {5};
int16_t g_sHoldingRegsLowerLimitBuf[MAX_HOLDING_REGISTERS]  = {0, 0, 0};
int16_t g_sHoldingRegsHigherLimitBuf[MAX_HOLDING_REGISTERS] = {200, 200, 200};

TEST_GROUP(Module)
{
	uint8_t      *pucQuery           = NULL;
	uint8_t      *pucResponse        = NULL;
	int16_t      *psInputRegisters   = NULL;
	int16_t      *psHoldingRegisters = NULL;
	ModbusData_t *pModbusData        = NULL;
		
    void setup()
    {	
		pucQuery     = (uint8_t*)calloc(QUERY_SIZE_IN_BYTES,  sizeof(uint8_t));
		pucResponse  = (uint8_t*)calloc(RESPONSE_SIZE_IN_BYTES, sizeof(uint8_t));		
		pModbusData  = (ModbusData_t*)calloc(1, sizeof(ModbusData_t));
		
		pModbusData->psInputRegisters              = g_sInputRegsBuf;
	    pModbusData->usInputRegisterStartAddress   = INPUT_REGISTER_START_ADDRESS;
	    pModbusData->usMaxInputRegisters           = MAX_INPUT_REGISTERS;
	    pModbusData->psHoldingRegisters            = g_sHoldingRegsBuf;
	    pModbusData->usHoldingRegisterStartAddress = HOLDING_REGISTER_START_ADDRESS;
	    pModbusData->usMaxHoldingRegisters         = MAX_HOLDING_REGISTERS;		
		pModbusData->psHoldingRegisterLowerLimit   = g_sHoldingRegsLowerLimitBuf;
		pModbusData->psHoldingRegisterHigherLimit  = g_sHoldingRegsHigherLimitBuf;
		pModbusData->pucDiscreteInputs             = g_ucDiscreteInputsBuf;
		pModbusData->usDiscreteInputStartAddress   = DISCRETE_INPUTS_START_ADDRESS;
		pModbusData->usMaxDiscreteInputs           = MAX_DISCRETE_INPUTS;
		pModbusData->pucCoils                      = g_ucCoilsBuf;
		pModbusData->usCoilsStartAddress           = COILS_START_ADDRESS;
		pModbusData->usMaxCoils                    = MAX_COILS;
		
		mbap_DataInit(pModbusData);
    }

    void teardown()
    {
		free(pucQuery);
		free(pucResponse);
		free(pModbusData);
    }
};

TEST(Module, read_input_registers)
{
	uint8_t  ucQueryBuf[QUERY_LEN] = {0, 0, 0, 0, 0, 6, 1, 4, 0, 5, 0, 3};
    uint8_t  ucResponseLen         = 0;
	uint8_t  ucByteCount           = 0;
	uint16_t usNumOfData           = 0;
	uint16_t usStartAddress        = 0;
	
    usStartAddress  = (uint16_t)(ucQueryBuf[DATA_START_ADDRESS_OFFSET] << 8);
    usStartAddress |= (uint16_t)(ucQueryBuf[DATA_START_ADDRESS_OFFSET + 1]);	
	usNumOfData     = (uint16_t)(ucQueryBuf[NO_OF_DATA_OFFSET] << 8);
    usNumOfData    |= (uint16_t)(ucQueryBuf[NO_OF_DATA_OFFSET + 1]);
	ucResponseLen   = MBAP_HEADER_LEN  + (usNumOfData * 2) + 2;
	ucByteCount     = usNumOfData * 2;
	
	memcpy(pucQuery, ucQueryBuf, QUERY_LEN);
	
	CHECK_EQUAL(ucResponseLen, mbap_ProcessRequest(pucQuery, QUERY_LEN, pucResponse));
	CHECK_EQUAL(ucByteCount, pucResponse[MBT_BYTE_COUNT_OFFSET]);
	
	for (uint8_t ucCount = 0; ucCount < usNumOfData; ucCount++)
	{
		int16_t sReceivedValue = 0;
		
		sReceivedValue  = (int16_t)(pucResponse[MBT_DATA_VALUES_OFFSET + (ucCount * 2)] << 8);
		sReceivedValue |= (int16_t)(pucResponse[MBT_DATA_VALUES_OFFSET + (ucCount * 2) + 1]);
		
	    CHECK_EQUAL(g_sInputRegsBuf[ucCount + usStartAddress], sReceivedValue);	
	}   
}

TEST(Module, illegal_input_registers_address)
{
	uint8_t ucQueryBuf[QUERY_LEN] = {0, 0, 0, 0, 0, 6, 1, 4, 0, 0, 0, 16};
    uint8_t ucResponseLen      = 0;
	
	ucResponseLen = MBT_EXCEPTION_PACKET_LEN;
	
	memcpy(pucQuery, ucQueryBuf, QUERY_LEN);
	
	CHECK_EQUAL(ucResponseLen, mbap_ProcessRequest(pucQuery, QUERY_LEN, pucResponse));
	CHECK_EQUAL(ILLEGAL_DATA_ADDRESS, pucResponse[MBT_BYTE_COUNT_OFFSET] );	
}

TEST(Module, read_holding_registers)
{
	uint8_t  ucQueryBuf[QUERY_LEN] = {0, 0, 0, 0, 0, 6, 1, 3, 0, 5, 0, 3};
    uint8_t  ucResponseLen      = 0;
	uint8_t  ucByteCount        = 0;
	uint16_t usNumOfData        = 0;
	uint16_t usStartAddress     = 0;
	
    usStartAddress  = (uint16_t)(ucQueryBuf[DATA_START_ADDRESS_OFFSET] << 8);
    usStartAddress |= (uint16_t)(ucQueryBuf[DATA_START_ADDRESS_OFFSET + 1]);	
	usNumOfData     = (uint16_t)(ucQueryBuf[NO_OF_DATA_OFFSET] << 8);
    usNumOfData    |= (uint16_t)(ucQueryBuf[NO_OF_DATA_OFFSET + 1]);
	ucResponseLen   = MBAP_HEADER_LEN  + (usNumOfData * 2) + 2;
	ucByteCount     = usNumOfData * 2;
	
	memcpy(pucQuery, ucQueryBuf, QUERY_LEN);
	
	CHECK_EQUAL(ucResponseLen, mbap_ProcessRequest(pucQuery, QUERY_LEN, pucResponse));
	CHECK_EQUAL(ucByteCount, pucResponse[MBT_BYTE_COUNT_OFFSET]);
	
	for (uint8_t ucCount = 0; ucCount < usNumOfData; ucCount++)
	{
		int16_t sReceivedValue = 0;
		
		sReceivedValue  = (int16_t)(pucResponse[MBT_DATA_VALUES_OFFSET + (ucCount * 2)] << 8);
		sReceivedValue |= (int16_t)(pucResponse[MBT_DATA_VALUES_OFFSET + (ucCount * 2) + 1]);
		
	    CHECK_EQUAL(g_sHoldingRegsBuf[ucCount + usStartAddress], sReceivedValue);	
	}   	
}

TEST(Module, illegal_holding_registers_address)
{
	uint8_t ucQueryBuf[QUERY_LEN] = {0, 0, 0, 0, 0, 6, 1, 3, 0, 0, 0, 16};
    uint8_t ucResponseLen      = 0;
	
	ucResponseLen = MBT_EXCEPTION_PACKET_LEN;
	
	memcpy(pucQuery, ucQueryBuf, QUERY_LEN);
	
	CHECK_EQUAL(ucResponseLen, mbap_ProcessRequest(pucQuery, QUERY_LEN, pucResponse));
	CHECK_EQUAL(ILLEGAL_DATA_ADDRESS, pucResponse[MBT_BYTE_COUNT_OFFSET] );	
}

TEST(Module, illegal_function_code)
{
	uint8_t ucQueryBuf[QUERY_LEN] = {0, 0, 0, 0, 0, 6, 1, 10, 0, 0, 0, 11};
    uint8_t ucResponseLen      = 0;
	
	ucResponseLen = MBT_EXCEPTION_PACKET_LEN;
	
	memcpy(pucQuery, ucQueryBuf, QUERY_LEN);
	
	CHECK_EQUAL(ucResponseLen, mbap_ProcessRequest(pucQuery, QUERY_LEN, pucResponse));
	CHECK_EQUAL(ILLEGAL_FUNCTION_CODE, pucResponse[MBT_BYTE_COUNT_OFFSET] );	
}

TEST(Module, write_single_holding_register)
{
	uint8_t ucQueryBuf[QUERY_LEN] = {0, 0, 0, 0, 0, 6, 1, 6, 0, 1, 0, 200};
    uint8_t ucResponseLen      = 0;
	int16_t sReceivedValue     = 0;
	int16_t sSentValue         = 0;
	
	ucResponseLen = MBAP_HEADER_LEN + 5;
	
	memcpy(pucQuery, ucQueryBuf, QUERY_LEN);
	
	CHECK_EQUAL(ucResponseLen, mbap_ProcessRequest(pucQuery, QUERY_LEN, pucResponse));
		
	sReceivedValue  = (int16_t)(pucResponse[REGISTER_VALUE_OFFSET] << 8);
	sReceivedValue |= (int16_t)(pucResponse[REGISTER_VALUE_OFFSET + 1]);
    sSentValue	    = (int16_t)(ucQueryBuf[REGISTER_VALUE_OFFSET] << 8);
    sSentValue	   |= (int16_t)(ucQueryBuf[REGISTER_VALUE_OFFSET + 1]);

    CHECK_EQUAL(sSentValue, sReceivedValue);
		
}

TEST(Module, illega_data_value)
{
	uint8_t ucQueryBuf[QUERY_LEN] = {0, 0, 0, 0, 0, 6, 1, 6, 0, 1, 0, 201};
    uint8_t ucResponseLen      = 0;
	
	ucResponseLen = MBT_EXCEPTION_PACKET_LEN;
	
	memcpy(pucQuery, ucQueryBuf, QUERY_LEN);
	
	CHECK_EQUAL(ucResponseLen, mbap_ProcessRequest(pucQuery, QUERY_LEN, pucResponse));
	CHECK_EQUAL(ILLEGAL_DATA_VALUE, pucResponse[MBT_BYTE_COUNT_OFFSET] );
}

TEST(Module, read_discrete_inputs)
{
	uint8_t ucQueryBuf[QUERY_LEN] = {0, 0, 0, 0, 0, 6, 1, 2, 0, 0, 0, 3};
    uint8_t ucResponseLen      = 0;
	uint8_t ucByteCount        = 0;
	uint16_t usNumOfData       = 0;
	
	usNumOfData    = (uint16_t)(ucQueryBuf[NO_OF_DATA_OFFSET] << 8);
    usNumOfData   |= (uint16_t)(ucQueryBuf[NO_OF_DATA_OFFSET + 1]);
	ucResponseLen  = MBAP_HEADER_LEN  + (usNumOfData / 8) + 2;
	ucByteCount    = usNumOfData / 8;	
	
	if (0 != ucQueryBuf[11])
	{
		ucResponseLen = ucResponseLen + 1;
		ucByteCount   = ucByteCount + 1;	
	}
	
	memcpy(pucQuery, ucQueryBuf, QUERY_LEN);
	
    CHECK_EQUAL(ucResponseLen, mbap_ProcessRequest(pucQuery, QUERY_LEN, pucResponse));
	CHECK_EQUAL(ucByteCount, pucResponse[MBT_BYTE_COUNT_OFFSET]);
	
	for (uint8_t ucCount = 0; ucCount < MAX_DISCRETE_INPUTS; ucCount++)
	{
		uint8_t  ucReceivedValue = 0;
		uint16_t usByteOffset    = 0;
		uint16_t usDiscreteBit   = 0;
		
		usByteOffset  = ucCount / 8 ;
        usDiscreteBit = ucCount - usByteOffset * 8;
		
		ucReceivedValue = (uint8_t)(pucResponse[MBT_DATA_VALUES_OFFSET + usByteOffset]);	
		
	    CHECK_EQUAL(g_ucDiscreteInputsBuf[usByteOffset] & (1 << usDiscreteBit) , ucReceivedValue & (1 << usDiscreteBit));	
	}   	 	
}

TEST(Module, read_coils)
{
	uint8_t ucQueryBuf[QUERY_LEN] = {0, 0, 0, 0, 0, 6, 1, 1, 0, 0, 0, 3};
    uint8_t ucResponseLen      = 0;
	uint8_t ucByteCount        = 0;
	uint16_t usNumOfData        = 0;
	
	usNumOfData    = (uint16_t)(ucQueryBuf[NO_OF_DATA_OFFSET] << 8);
    usNumOfData   |= (uint16_t)(ucQueryBuf[NO_OF_DATA_OFFSET + 1]);
	ucResponseLen  = MBAP_HEADER_LEN  + (usNumOfData / 8) + 2;
	ucByteCount    = usNumOfData / 8;	
	
	if (0 != ucQueryBuf[11])
	{
		ucResponseLen = ucResponseLen + 1;
		ucByteCount   = ucByteCount + 1;		
	}
	
	memcpy(pucQuery, ucQueryBuf, QUERY_LEN);
	
    CHECK_EQUAL(ucResponseLen, mbap_ProcessRequest(pucQuery, QUERY_LEN, pucResponse));
	CHECK_EQUAL(ucByteCount, pucResponse[MBT_BYTE_COUNT_OFFSET]);
	
	for (uint8_t ucCount = 0; ucCount < MAX_COILS; ucCount++)
	{
		uint8_t  ucReceivedValue = 0;
		uint16_t usByteOffset    = 0;
		uint16_t usCoilsBit      = 0;
		
		usByteOffset = ucCount / 8 ;
        usCoilsBit   = ucCount - usByteOffset * 8;
		
		ucReceivedValue = (uint8_t)(pucResponse[MBT_DATA_VALUES_OFFSET + usByteOffset]);	
		
	    CHECK_EQUAL(g_ucCoilsBuf[usByteOffset] & (1 << usCoilsBit) , ucReceivedValue & (1 << usCoilsBit));	
	}   	 	
}

TEST(Module, write_coil)
{
    uint8_t ucQueryBuf[QUERY_LEN] = {0, 0, 0, 0, 0, 6, 1, 1, 0, 0, 0, 3};
    uint8_t ucResponseLen      = 0;
    uint8_t ucByteCount        = 0;
    uint16_t usNumOfData       = 0;

    usNumOfData    = (uint16_t)(ucQueryBuf[NO_OF_DATA_OFFSET] << 8);
    usNumOfData   |= (uint16_t)(ucQueryBuf[NO_OF_DATA_OFFSET + 1]);
    ucResponseLen  = MBAP_HEADER_LEN  + (usNumOfData / 8) + 2;
    ucByteCount    = usNumOfData / 8;

    if (0 != ucQueryBuf[11])
    {
        ucResponseLen = ucResponseLen + 1;
        ucByteCount   = ucByteCount + 1;
    }

    memcpy(pucQuery, ucQueryBuf, QUERY_LEN);

    CHECK_EQUAL(ucResponseLen, mbap_ProcessRequest(pucQuery, QUERY_LEN, pucResponse));
    CHECK_EQUAL(ucByteCount, pucResponse[MBT_BYTE_COUNT_OFFSET]);

    for (uint8_t ucCount = 0; ucCount < MAX_COILS; ucCount++)
    {
        uint8_t  ucReceivedValue = 0;
        uint16_t usByteOffset    = 0;
        uint16_t usCoilsBit      = 0;

        usByteOffset = ucCount / 8 ;
        usCoilsBit   = ucCount - usByteOffset * 8;

        ucReceivedValue = (uint8_t)(pucResponse[MBT_DATA_VALUES_OFFSET + usByteOffset]);

        CHECK_EQUAL(g_ucCoilsBuf[usByteOffset] & (1 << usCoilsBit) , ucReceivedValue & (1 << usCoilsBit));
    }
}
