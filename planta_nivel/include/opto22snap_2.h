//-----------------------------------------------------------------------------
//
// O22STRCT.h
// Copyright (c) 1999 by Opto 22
//
// 
// This is a support file for "O22SIOMM.H" and its class, O22SnapIoMemMap.
// It is seperated from "O22SIOMM.H" because this file is also used in
// "OptoSnapIoMemMapX.idl" for the definition of the OptoSnapIoMemMapX ActiveX
// component.  It should not be modified without consideration to the ActiveX
// requirements of an OLE Automation interface.
//-----------------------------------------------------------------------------


#ifndef __O22STRCT_H
#define __O22STRCT_H

  /////////////////////////////////////////////////////////////////////
  // Digital Bank read-only area
  //
  typedef struct SIOMM_DigBankReadArea
  { 
    long nStatePts63to32;
    long nStatePts31to0;
    long nOnLatchStatePts63to32;
    long nOnLatchStatePts31to0;
    long nOffLatchStatePts63to32;
    long nOffLatchStatePts31to0;
    long nActiveCountersPts63to32;
    long nActiveCountersPts31to0;
  } O22_SIOMM_DigBankReadArea; 

  /////////////////////////////////////////////////////////////////////
  // Digital Point read-only area.
  //
  typedef struct SIOMM_DigPointReadArea
  {
    long  nState;          // bool
    long  nOnLatch;        // bool
    long  nOffLatch;       // bool
    long  nCounterState;   // bool
    long  nCounts;         // unsigned int
  } O22_SIOMM_DigPointReadArea;


  /////////////////////////////////////////////////////////////////////
  // Analog Point read-only area.
  //
  typedef struct SIOMM_AnaPointReadArea
  {
    float fValue;
    float fCounts;
    float fMinValue;
    float fMaxValue;
  } O22_SIOMM_AnaPointReadArea;

  /////////////////////////////////////////////////////////////////////
  // Analog Bank area
  //
  typedef struct SIOMM_AnaBank
  {
    float fValue[64];
  } O22_SIOMM_AnaBank;

  /////////////////////////////////////////////////////////////////////
  // Point Configuration read/write area.
  //
  typedef struct SIOMM_PointConfigArea
  {
    long  nModuleType; // Read only.  Not used in SetPointConfiguration()
    long  nPointType;  // Read/Write
    long  nFeature;    // Read/Write.  Only used for digital points
    float fOffset;     // Read/Write.  Only used for analog points
    float fGain;       // Read/Write.  Only used for analog points
    float fHiScale;    // Read/Write.  Only used for analog points
    float fLoScale;    // Read/Write.  Only used for analog points
    float fWatchdogValue;    // Read/Write
    long  nWatchdogEnabled;  // Read/Write
  } O22_SIOMM_PointConfigArea;


  typedef struct SIOMM_StatusVersion
  {
    long          nMapVer;            //  Memory map version
    long          nLoaderVersion;     //  Loader version (1.2.3.4 format)
    long          nKernelVersion;     //  Kernel version (1.2.3.4 format)
  } O22_SIOMM_StatusVersion;


  typedef struct SIOMM_StatusHardware
  {
    long          nIoUnitType;        //  I/O unit type
    unsigned char byHwdVerMonth;      //  hardware version (month)
    unsigned char byHwdVerDay;        //  hardware version (day)
    short         wHwdVerYear;        //  hardware version (4 digit year)
    long          nRamSize;           //  bytes of installed RAM
  } O22_SIOMM_StatusHardware;


  typedef struct SIOMM_StatusNetwork
  {
    short         wMACAddress0;       //  MAC address 
    short         wMACAddress1;       //  MAC address 
    short         wMACAddress2;       //  MAC address 
    long          nTCPIPAddress;      //  IP address
    long          nSubnetMask;        //  subnet mask
    long          nDefGateway;        //  default gateway
  } O22_SIOMM_StatusNetwork;


#endif // __O22STRCT_H