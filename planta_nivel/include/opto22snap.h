//-----------------------------------------------------------------------------
//
// O22SIOMM.h
// Copyright (c) 1999 by Opto 22
//
// Header for the O22SnapIoMemMap C++ class.  
// 
// The O22SnapIoMemMap C++ class is used to communicate from a computer to an
// Opto 22 SNAP Ethernet I/O unit.
//
// While this class was developed on Microsoft Windows 32-bit operating 
// systems, it is intended to be as generic as possible.  For Windows specific
// code, search for "_WIN32" and "_WIN32_WCE".
//-----------------------------------------------------------------------------

#ifndef __O22SIOMM_H_
#define __O22SIOMM_H_


#ifdef _WIN32
#ifdef _WIN32_WCE
#include "wtypes.h"
#include "winsock.h"
#else
#include "winsock2.h"
#endif 
#endif 

#ifdef _LINUX
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>

typedef int SOCKET;
#define INVALID_SOCKET 0
#define SOCKET_ERROR   -1

#endif // _LINUX



#include "opto22snap_2.h"


typedef long                LONG;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef unsigned char       BYTE;
typedef float               FLOAT;


#ifndef O22MAKELONG // makes a long from 4 bytes
#define O22MAKELONG(b0, b1, b2, b3)  ((((DWORD)(b0)) << 24) | (((DWORD)(b1)) << 16) | (((DWORD)(b2)) << 8) | ((DWORD)(b3)))
#endif 


#ifndef O22MAKEFLOAT // makes a float from 4 bytes
#define O22MAKEFLOAT(pf, b0, b1, b2, b3)  ((BYTE*)pf)[0] = b3 ; ((BYTE*)pf)[1] = b2 ; ((BYTE*)pf)[2] = b1 ; ((BYTE*)pf)[3] = b0 ;
#endif


#ifndef O22MAKEWORD // makes a word from 2 bytes
#define O22MAKEWORD(w0, w1)  (((WORD)((BYTE)(w0))) << 8) | (WORD)(((BYTE)(w1)))
#endif 

#ifndef O22MAKEDWORD // makes a dwords from 2 words
#define O22MAKEDWORD(L0, L1) (((DWORD)((WORD)(L0))) << 16) | (DWORD)(((WORD)(L1)))
#endif

#ifndef O22LOWORD // gets the low word from a dword
#define O22LOWORD(l) ((WORD)(l))
#endif

#ifndef O22HIWORD // gets the high word from a dword
#define O22HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#endif

#ifndef O22LOBYTE // gets the low byte from a word
#define O22LOBYTE(w) ((BYTE)(w))
#endif

#ifndef O22HIBYTE // gets the high byte from a word
#define O22HIBYTE(w) ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#endif

#ifndef O22BYTE0 // gets the first byte from a dword
#define O22BYTE0(l) ((BYTE)(((DWORD)(l) >> 24) & 0xFF))
#endif

#ifndef O22BYTE1 // gets the second byte from a dword
#define O22BYTE1(l) ((BYTE)(((DWORD)(l) >> 16) & 0xFF))
#endif

#ifndef O22BYTE2 // gets the third byte from a dword
#define O22BYTE2(l) ((BYTE)(((DWORD)(l) >> 8) & 0xFF))
#endif

#ifndef O22BYTE3 // gets the fourth byte from a dword
#define O22BYTE3(l) ((BYTE)(l))
#endif


// Transaction code used by the I/O unit
#define SIOMM_TCODE_WRITE_QUAD_REQUEST   0
#define SIOMM_TCODE_WRITE_BLOCK_REQUEST  1
#define SIOMM_TCODE_WRITE_RESPONSE       2
#define SIOMM_TCODE_READ_QUAD_REQUEST    4
#define SIOMM_TCODE_READ_BLOCK_REQUEST   5
#define SIOMM_TCODE_READ_QUAD_RESPONSE   6
#define SIOMM_TCODE_READ_BLOCK_RESPONSE  7

// Sizes of the read/write requests and responses
#define SIOMM_SIZE_WRITE_QUAD_REQUEST    16   
#define SIOMM_SIZE_WRITE_BLOCK_REQUEST   16   
#define SIOMM_SIZE_WRITE_RESPONSE        12
#define SIOMM_SIZE_READ_QUAD_REQUEST     12
#define SIOMM_SIZE_READ_QUAD_RESPONSE    16
#define SIOMM_SIZE_READ_BLOCK_REQUEST    16
#define SIOMM_SIZE_READ_BLOCK_RESPONSE   16

// Response codes from the I/O unit
#define SIOMM_RESPONSE_CODE_ACK          0
#define SIOMM_RESPONSE_CODE_NAK          7

// Memory Map values

// Status read area of the memory map
#define SIOMM_STATUS_READ_BASE                 0xF0300000
#define SIOMM_STATUS_READ_PUC_FLAG             0xF0300004
#define SIOMM_STATUS_READ_LAST_ERROR           0xF030000C
#define SIOMM_STATUS_READ_BOOTP_FLAG           0xF0300048
#define SIOMM_STATUS_READ_DEGREES_FLAG         0xF030004C

// Status write area of the memory map
#define SIOMM_STATUS_WRITE_OPERATION           0xF0380000
#define SIOMM_STATUS_WRITE_BOOTP               0xF0380004
#define SIOMM_STATUS_WRITE_TEMP_DEGREES        0xF0380008
#define SIOMM_STATUS_WRITE_WATCHDOG_TIME       0xF0380010

// Digital bank read area of the memory map
#define SIOMM_DBANK_READ_AREA_BASE             0xF0400000
#define SIOMM_DBANK_READ_POINT_STATES          0xF0400000
#define SIOMM_DBANK_READ_ON_LATCH_STATES       0xF0400008
#define SIOMM_DBANK_READ_OFF_LATCH_STATES      0xF0400010
#define SIOMM_DBANK_READ_ACTIVE_COUNTERS       0xF0400018
#define SIOMM_DBANK_READ_COUNTER_DATA_BASE     0xF0400100
#define SIOMM_DBANK_READ_COUNTER_DATA_BOUNDARY 0x00000004

// Digital bank write area of the memory map
#define SIOMM_DBANK_WRITE_AREA_BASE            0xF0500000
#define SIOMM_DBANK_WRITE_TURN_ON_MASK         0xF0500000
#define SIOMM_DBANK_WRITE_TURN_OFF_MASK        0xF0500008
#define SIOMM_DBANK_WRITE_ACT_COUNTERS_MASK    0xF0500010
#define SIOMM_DBANK_WRITE_DEACT_COUNTERS_MASK  0xF0500018

// Analog bank read area of the memory map
#define SIOMM_ABANK_READ_AREA_BASE             0xF0600000
#define SIOMM_ABANK_READ_POINT_VALUES          0xF0600000
#define SIOMM_ABANK_READ_POINT_COUNTS          0xF0600100
#define SIOMM_ABANK_READ_POINT_MIN_VALUES      0xF0600200
#define SIOMM_ABANK_READ_POINT_MAX_VALUES      0xF0600300

// Analog bank write area of the memory map
#define SIOMM_ABANK_WRITE_AREA_BASE            0xF0700000
#define SIOMM_ABANK_WRITE_POINT_VALUES         0xF0700000
#define SIOMM_ABANK_WRITE_POINT_COUNTS         0xF0700100

// Digital point read area of the memory map
#define SIOMM_DPOINT_READ_BOUNDARY             0x00000040
#define SIOMM_DPOINT_READ_AREA_BASE            0xF0800000
#define SIOMM_DPOINT_READ_STATE                0xF0800000
#define SIOMM_DPOINT_READ_ONLATCH_STATE        0xF0800004
#define SIOMM_DPOINT_READ_OFFLATCH_STATE       0xF0800008
#define SIOMM_DPOINT_READ_ACTIVE_COUNTER       0xF080000C
#define SIOMM_DPOINT_READ_COUNTER_DATA         0xF0800010

// Digital point write area of the memory map
#define SIOMM_DPOINT_WRITE_BOUNDARY            0x00000040
#define SIOMM_DPOINT_WRITE_TURN_ON_BASE        0xF0900000
#define SIOMM_DPOINT_WRITE_TURN_OFF_BASE       0xF0900004
#define SIOMM_DPOINT_WRITE_ACTIVATE_COUNTER    0xF0900008
#define SIOMM_DPOINT_WRITE_DEACTIVATE_COUNTER  0xF090000C

// Analog point read area of the memory map
#define SIOMM_APOINT_READ_BOUNDARY             0x00000040
#define SIOMM_APOINT_READ_AREA_BASE            0xF0A00000
#define SIOMM_APOINT_READ_VALUE_BASE           0xF0A00000
#define SIOMM_APOINT_READ_COUNTS_BASE          0xF0A00004
#define SIOMM_APOINT_READ_MIN_VALUE_BASE       0xF0A00008
#define SIOMM_APOINT_READ_MAX_VALUE_BASE       0xF0A0000C

// Analog point write area of the memory map
#define SIOMM_APOINT_WRITE_BOUNDARY            0x00000040
#define SIOMM_APOINT_WRITE_VALUE_BASE          0xF0B00000
#define SIOMM_APOINT_WRITE_COUNTS_BASE         0xF0B00004

// Point configuration area of the memory map
#define SIOMM_POINT_CONFIG_BOUNDARY            0x00000040
#define SIOMM_POINT_CONFIG_READ_MOD_TYPE_BASE  0xF0C00000
#define SIOMM_POINT_CONFIG_WRITE_TYPE_BASE     0xF0C00004
#define SIOMM_POINT_CONFIG_WRITE_FEATURE_BASE  0xF0C00008
#define SIOMM_POINT_CONFIG_WRITE_OFFSET_BASE   0xF0C0000C
#define SIOMM_POINT_CONFIG_WRITE_GAIN_BASE     0xF0C00010
#define SIOMM_POINT_CONFIG_WRITE_HISCALE_BASE  0xF0C00014
#define SIOMM_POINT_CONFIG_WRITE_LOSCALE_BASE  0xF0C00018
#define SIOMM_POINT_CONFIG_WRITE_WDOG_VALUE_BASE  0xF0C00024
#define SIOMM_POINT_CONFIG_WRITE_WDOG_ENABLE_BASE 0xF0C00028

// Digital point read and clear area of the memory map
#define SIOMM_DPOINT_READ_CLEAR_BOUNDARY       0x00000004
#define SIOMM_DPOINT_READ_CLEAR_COUNTS_BASE    0xF0F00000
#define SIOMM_DPOINT_READ_CLEAR_ON_LATCH_BASE  0xF0F00100
#define SIOMM_DPOINT_READ_CLEAR_OFF_LATCH_BASE 0xF0F00200

// Analog point read and clear area of the memory map
#define SIOMM_APOINT_READ_CLEAR_BOUNDARY       0x00000004
#define SIOMM_APOINT_READ_CLEAR_MIN_VALUE_BASE 0xF0F80000
#define SIOMM_APOINT_READ_CLEAR_MAX_VALUE_BASE 0xF0F80100

// Analog point calc and set area of the memory map
#define SIOMM_APOINT_READ_CALC_SET_BOUNDARY    0x00000004
#define SIOMM_APOINT_READ_CALC_SET_OFFSET_BASE 0xF0E00000
#define SIOMM_APOINT_READ_CALC_SET_GAIN_BASE   0xF0E00100

// Error values generated by this class
#define SIOMM_OK                             1
#define SIOMM_ERROR                         -1
#define SIOMM_TIME_OUT                      -2 
#define SIOMM_ERROR_NO_SOCKETS              -3  // Unable to access socket interface
#define SIOMM_ERROR_CREATING_SOCKET         -4
#define SIOMM_ERROR_CONNECTING_SOCKET       -5
#define SIOMM_ERROR_RESPONSE_BAD            -6
#define SIOMM_ERROR_NOT_CONNECTED_YET       -7
#define SIOMM_ERROR_OUT_OF_MEMORY           -8
#define SIOMM_ERROR_NOT_CONNECTED           -9

// Error values generated by the I/O brain itself
#define SIOMM_BRAIN_ERROR_UNDEFINED_CMD      57345
#define SIOMM_BRAIN_ERROR_INVALID_PT_TYPE    57346
#define SIOMM_BRAIN_ERROR_INVALID_FLOAT      57347
#define SIOMM_BRAIN_ERROR_PUC_EXPECTED       57348
#define SIOMM_BRAIN_ERROR_INVALID_ADDRESS    57349
#define SIOMM_BRAIN_ERROR_INVALID_CMD_LENGTH 57350
#define SIOMM_BRAIN_ERROR_RESERVED           57351
#define SIOMM_BRAIN_ERROR_BUSY               57352
#define SIOMM_BRAIN_ERROR_CANT_ERASE_FLASH   57353
#define SIOMM_BRAIN_ERROR_CANT_PROG_FLASH    57354
#define SIOMM_BRAIN_ERROR_IMAGE_TOO_SMALL    57355
#define SIOMM_BRAIN_ERROR_IMAGE_CRC_MISMATCH 57356
#define SIOMM_BRAIN_ERROR_IMAGE_LEN_MISMATCH 57357
  

class O22SnapIoMemMap {

  public:
  // Public data

    // Public Construction/Destruction
    O22SnapIoMemMap();
    ~O22SnapIoMemMap();

  // Public Members

    // Connection functions
    LONG OpenEnet(char * pchIpAddressArg, long nPort, long nOpenTimeOutMS, long nAutoPUC);
    LONG IsOpenDone();

    LONG Close();

    LONG SetCommOptions(LONG nTimeOutMS, LONG nReserved);

    // Functions for building and unpacking read/write requests
    LONG BuildWriteBlockRequest(BYTE  * pbyWriteBlockRequest,
                                BYTE    byTransactionLabel,
                                DWORD   dwDestinationOffset,
                                WORD    wDataLength,
                                BYTE  * pbyBlockData);

    LONG BuildWriteQuadletRequest(BYTE * pbyWriteQuadletRequest,
                                  BYTE  byTransactionLabel, 
                                  WORD  wSourceId,
                                  DWORD dwDestinationOffset,
                                  DWORD dwQuadletData);

    LONG UnpackWriteResponse(BYTE * pbyWriteQuadletResponse,
                             BYTE * pbyTransactionLabel, 
                             BYTE * pbyResponseCode);

    LONG BuildReadQuadletRequest(BYTE * pbyReadQuadletRequest,
                                 BYTE   byTransactionLabel, 
                                 DWORD  dwDestinationOffset);

    LONG UnpackReadQuadletResponse(BYTE  * pbyReadQuadletResponse,
                                   BYTE  * pbyTransactionLabel, 
                                   BYTE  * pbyResponseCode,
                                   DWORD * pdwQuadletData);

    LONG BuildReadBlockRequest(BYTE * pbyReadBlockRequest,
                               BYTE   byTransactionLabel, 
                               DWORD  dwDestinationOffset,
                               WORD   wDataLength);

    LONG UnpackReadBlockResponse(BYTE  * pbyReadBlockResponse,
                                 BYTE  * pbyTransactionLabel, 
                                 BYTE  * pbyResponseCode,
                                 WORD  * pwDataLength,
                                 BYTE  * pbyBlockData);

    // Functions for reading and writing quadlets (DWORDs)
    LONG ReadQuad(DWORD dwDestOffset, DWORD * pdwQuadlet);
    LONG WriteQuad(DWORD dwDestOffset, DWORD dwQuadlet);

    // Functions for reading and writing floats
    LONG ReadFloat(DWORD dwDestOffset, float * pfValue);
    LONG WriteFloat(DWORD dwDestOffset, float fValue);

    // Functions for reading and writing blocks of bytes
    LONG ReadBlock(DWORD dwDestOffset, WORD wDataLength, BYTE * pbyData);
    LONG WriteBlock(DWORD dwDestOffset, WORD wDataLength, BYTE * pbyData);

    // Status read
    LONG GetStatusPUC(long *pnPUCFlag);
    LONG GetStatusLastError(long *pnErrorCode);
    LONG GetStatusBootpAlways(long *pnBootpAlways);
    LONG GetStatusDegrees(long *pnDegrees);
    LONG GetStatusVersionEx(SIOMM_StatusVersion *pVersionData);
    LONG GetStatusHardwareEx(SIOMM_StatusHardware *pHardwareData);
    LONG GetStatusNetworkEx(SIOMM_StatusNetwork *pNetworkData);

    // Status write
    LONG SetStatusOperation(long nOpCode);
    LONG SetStatusBootpRequest(long nFlag);
    LONG SetStatusDegrees(long nDegFlag);
    LONG SetStatusWatchdogTime(long nTimeMS);

    // Point configuration
    LONG ConfigurePoint(long nPoint, long nPointType);
    LONG GetModuleType(long nPoint, long * pnModuleType);
    LONG GetPtConfigurationEx(long nPoint, SIOMM_PointConfigArea * pData);
    LONG SetPtConfigurationEx(long nPoint, SIOMM_PointConfigArea Data);
    LONG SetDigPtConfiguration(long nPoint, long nPointType, long nFeature);
    LONG SetAnaPtConfiguration(long nPoint, long nPointType, float fOffset, float fGain, float fHiScale, float fLoScale);
    LONG SetPtWatchdog(long nPoint, float fValue, long nEnabled);

    // Digital point read
    LONG GetDigPtState(long nPoint, long *pnState);
    LONG GetDigPtOnLatch(long nPoint, long *pnState);
    LONG GetDigPtOffLatch(long nPoint, long *pnState);
    LONG GetDigPtCounterState(long nPoint, long *pnState);
    LONG GetDigPtCounts(long nPoint, long *pnValue);
    LONG GetDigPtReadAreaEx(long nPoint, SIOMM_DigPointReadArea * pData);

    // Digital point write
    LONG SetDigPtState(long nPoint, long nState);
    LONG SetDigPtCounterState(long nPoint, long nState);

    // Digital point read and clear
    LONG ReadClearDigPtCounts(long nPoint, long * pnState);
    LONG ReadClearDigPtOnLatch(long nPoint, long * pnState);
    LONG ReadClearDigPtOffLatch(long nPoint, long * pnState);

    // Digital bank read
    LONG GetDigBankPointStates(long *pnPts63to32, long *pnPts31to0);
    LONG GetDigBankOnLatchStates(long *pnPts63to32, long *pnPts31to0);
    LONG GetDigBankOffLatchStates(long *pnPts63to32, long *pnPts31to0);
    LONG GetDigBankActCounterStates(long *pnPts63to32, long *pnPts31to0);
    LONG GetDigBankReadAreaEx(SIOMM_DigBankReadArea * pData);

    // Digital bank write
    LONG SetDigBankPointStates(long nPts63to32, long nPts31to0, long nMask63to32, long nMask31to0);
    LONG SetDigBankOnMask(long nPts63to32, long nPts31to0);
    LONG SetDigBankOffMask(long nPts63to32, long nPts31to0);
    LONG SetDigBankActCounterMask(long nPts63to32, long nPts31to0);
    LONG SetDigBankDeactCounterMask(long nPts63to32, long nPts31to0);

    // Analog point read
    LONG GetAnaPtValue(long nPoint, float *pfValue);
    LONG GetAnaPtCounts(long nPoint, float *pfValue);
    LONG GetAnaPtMinValue(long nPoint, float *pfValue);
    LONG GetAnaPtMaxValue(long nPoint, float *pfValue);
    LONG GetAnaPtReadAreaEx(long nPoint, SIOMM_AnaPointReadArea * pData);

    // Analog point write
    LONG SetAnaPtValue(long nPoint, float fValue);
    LONG SetAnaPtCounts(long nPoint, float fValue);

    // Analog point read and clear
    LONG ReadClearAnaPtMinValue(long nPoint, float *pfValue);
    LONG ReadClearAnaPtMaxValue(long nPoint, float *pfValue);

    // Analog bank read
    LONG GetAnaBankValuesEx(SIOMM_AnaBank * pBankData);
    LONG GetAnaBankCountsEx(SIOMM_AnaBank * pBankData);
    LONG GetAnaBankMinValuesEx(SIOMM_AnaBank * pBankData);
    LONG GetAnaBankMaxValuesEx(SIOMM_AnaBank * pBankData);

    // Analog bank write
    LONG SetAnaBankValuesEx(SIOMM_AnaBank BankData);
    LONG SetAnaBankCountsEx(SIOMM_AnaBank BankData);

    // Analog point calc and set
    LONG CalcSetAnaPtOffset(long nPoint, float *pfValue);
    LONG CalcSetAnaPtGain(long nPoint, float *pfValue);


  protected:
    // Protected data
    SOCKET m_Socket;
    sockaddr_in m_SocketAddress; // for connecting our socket

    timeval m_tvTimeOut;      // Timeout structure for sockets
    LONG    m_nTimeOutMS;     // For holding the user's timeout
    DWORD   m_nOpenTimeOutMS; // For holding the open timeout
    DWORD   m_nOpenTime;      // For testing the open timeout
    LONG    m_nRetries;       // For holding the user's retries.
    
    LONG    m_nAutoPUCFlag;   // For holding the AutoPUC flag sent in OpenEnet()

    BYTE    m_byTransactionLabel; // The current transaction label

    // Protected Members

    // Open/Close sockets functions
    LONG OpenSockets(char * pchIpAddressArg, long nPort, long nOpenTimeOutMS);
    LONG CloseSockets();

    // Generic functions for getting/setting 64-bit bitmasks
    LONG GetBitmask64(DWORD dwDestOffset, long *pnPts63to32, long *pnPts31to0);
    LONG SetBitmask64(DWORD dwDestOffset, long nPts63to32, long nPts31to0);

    // Generic functions for getting/setting analog banks
    LONG GetAnaBank(DWORD dwDestOffset, SIOMM_AnaBank * pBankData);
    LONG SetAnaBank(DWORD dwDestOffset, SIOMM_AnaBank BankData);

    // Gets the next transaction label
    inline void UpdateTransactionLabel() {m_byTransactionLabel++; \
                                          if (m_byTransactionLabel>=64) m_byTransactionLabel=0;}

  private:
    // Private data

    // Private Members
};



#endif // __O22SIOMM_H_

