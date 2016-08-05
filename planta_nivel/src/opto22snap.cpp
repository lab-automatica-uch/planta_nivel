//-----------------------------------------------------------------------------
//
// O22SIOMM.cpp
// Copyright (c) 1999 by Opto 22
//
// Source for the O22SnapIoMemMap C++ class.  
// 
// The O22SnapIoMemMap C++ class is used to communicate from a computer to an
// Opto 22 SNAP Ethernet I/O unit.
//
// While this class was developed on Microsoft Windows 32-bit operating 
// systems, it is intended to be as generic as possible.  For Windows specific
// code, search for "_WIN32" and "_WIN32_WCE".
//-----------------------------------------------------------------------------


#include "opto22snap.h"


#ifdef _WIN32
#define WINSOCK_VERSION_REQUIRED_MAJ 2
#define WINSOCK_VERSION_REQUIRED_MIN 0
#endif

O22SnapIoMemMap::O22SnapIoMemMap()
//-------------------------------------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------------------------------------
{
  // Set defaults
  m_Socket = INVALID_SOCKET;
  m_byTransactionLabel = 0;
  m_nRetries = 0;
  m_nOpenTime = 0;
  m_nOpenTimeOutMS = 0;
  m_nTimeOutMS = 1000;
  m_tvTimeOut.tv_sec  = m_nTimeOutMS / 1000;
  m_tvTimeOut.tv_usec = m_nTimeOutMS % 1000;
}
 

O22SnapIoMemMap::~O22SnapIoMemMap()
//-------------------------------------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------------------------------------
{
  CloseSockets();

#ifdef _WIN32
  WSACleanup();
#endif
}


LONG O22SnapIoMemMap::OpenEnet(char * pchIpAddressArg, long nPort, long nOpenTimeOutMS, long nAutoPUC)
//-------------------------------------------------------------------------------------------------
// Open a connection to a SNAP Ethernet I/O unit
//-------------------------------------------------------------------------------------------------
{
  m_nAutoPUCFlag = nAutoPUC;

  return OpenSockets(pchIpAddressArg, nPort, nOpenTimeOutMS);
}


LONG O22SnapIoMemMap::OpenSockets(char * pchIpAddressArg, long nPort, long nOpenTimeOutMS)
//-------------------------------------------------------------------------------------------------
// Use sockets to open a connection to the SNAP I/O unit
//-------------------------------------------------------------------------------------------------
{
  int       nResult; // for checking the return values of functions

#ifdef _WIN32
  // Initialize WinSock.dll
  WSADATA   wsaData; // for checking WinSock

  nResult = WSAStartup(  O22MAKEWORD( WINSOCK_VERSION_REQUIRED_MIN, 
                                      WINSOCK_VERSION_REQUIRED_MAJ ), &wsaData );
  if ( nResult != 0 ) 
  {
    // We couldn't find a socket interface. 
    return SIOMM_ERROR_NO_SOCKETS;  
  } 

  // Confirm that the WinSock DLL supports WINSOCK_VERSION 
  if (( O22LOBYTE( wsaData.wVersion ) != WINSOCK_VERSION_REQUIRED_MAJ) || 
      ( O22HIBYTE( wsaData.wVersion ) != WINSOCK_VERSION_REQUIRED_MIN)) 
  {
    // We couldn't find an acceptable socket interface. 
    WSACleanup( );
    return SIOMM_ERROR_NO_SOCKETS; 
  } 
#endif


  // If a socket is open, close it now.
  CloseSockets();

  m_nOpenTimeOutMS = nOpenTimeOutMS;

  // Create the socket
  m_Socket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_Socket == INVALID_SOCKET)
  {
    // Couldn't create the socket
#ifdef _WIN32
    WSACleanup( );
#endif

    return SIOMM_ERROR_CREATING_SOCKET;
  }

  // Make the socket non-blocking
#ifdef _WIN32
  // Windows uses ioctlsocket() to set the socket as non-blocking.
  // Other systems may use ioctl() or fcntl()
  unsigned long nNonBlocking = 1;
  if (SOCKET_ERROR == ioctlsocket(m_Socket, FIONBIO, &nNonBlocking))
#endif
#ifdef _LINUX
  if (-1 == fcntl(m_Socket,F_SETFL,O_NONBLOCK))
#endif
  {
    CloseSockets();

    return SIOMM_ERROR_CREATING_SOCKET;
  }

  // Setup the socket address structure
  m_SocketAddress.sin_addr.s_addr = inet_addr(pchIpAddressArg);
  m_SocketAddress.sin_family      = AF_INET;
  m_SocketAddress.sin_port        = htons((WORD)nPort);

  // attempt connection
  connect(m_Socket, (sockaddr*) &m_SocketAddress, sizeof(m_SocketAddress));

  // Get the time for the timeout logic in IsOpenDone()
#ifdef _WIN32
  m_nOpenTime = GetTickCount(); // GetTickCount returns the number of milliseconds that have
                                // elapsed since the system was started.
#endif
#ifdef _LINUX
  tms DummyTime;
  m_nOpenTime = times(&DummyTime); // times() returns the number of milliseconds that have
                                   // elapsed since the system was started.
#endif

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::CloseSockets()
//-------------------------------------------------------------------------------------------------
// Close the sockets connection
//-------------------------------------------------------------------------------------------------
{
  // Close up everything
  if (m_Socket != INVALID_SOCKET)
  { 
#ifdef _WIN32
    closesocket(m_Socket);
#endif
#ifdef _LINUX
    close(m_Socket);
#endif
  }
  
  // Reset our data members
  memset(&m_SocketAddress, 0, sizeof(m_SocketAddress));
  m_Socket = INVALID_SOCKET;
  m_nOpenTimeOutMS = 0;
  m_nOpenTime = 0;
  m_nRetries = 0;


  return SIOMM_OK;
}

LONG O22SnapIoMemMap::IsOpenDone()
//-------------------------------------------------------------------------------------------------
// Called after an OpenEnet() function to determine if the open process is completed yet.
//-------------------------------------------------------------------------------------------------
{
  fd_set  fds;
  timeval tvTimeOut;
  DWORD   nOpenTimeOutTemp;
  DWORD   dwPUCFlag;   // a flag for checking the status of PowerUp Clear on the I/O unit
  long    nResult;     // for checking the return values of functions

  // Check the open timeout
#ifdef _WIN32
  nOpenTimeOutTemp = GetTickCount(); // GetTickCount returns the number of milliseconds that have
                                     // elapsed since the system was started. 
#endif
#ifdef _LINUX
  tms DummyTime;
  nOpenTimeOutTemp = times(&DummyTime); // times() returns the number of milliseconds that have
                                        // elapsed since the system was started. 
#endif
  
  // Check for overflow of the system timer
  if (m_nOpenTime > nOpenTimeOutTemp) 
    m_nOpenTime = 0;

  // Check for timeout
  if (m_nOpenTimeOutMS < (nOpenTimeOutTemp - m_nOpenTime)) 
  {
    // Timeout has occured.
    CloseSockets();
    return SIOMM_TIME_OUT;
  }

  FD_ZERO(&fds);
  FD_SET(m_Socket, &fds);

  // We want the select to return immediately, so set the timeout to zero
  tvTimeOut.tv_sec  = 0;
  tvTimeOut.tv_usec = 100;

  // Use select() to check if the socket is connect and ready
  if (0 == select(m_Socket + 1, NULL, &fds, NULL, &tvTimeOut))
  {
    // we're not connected yet!
    return SIOMM_ERROR_NOT_CONNECTED_YET;
  }

  // Okay, we must be connected if we get past the select() above.
  
  if (m_nAutoPUCFlag)
  {
    // Now, check the PowerUp Clear flag of the brain.  It must be cleared 
    // before can do anything with it (other than read the status area.)

    // Read PowerUp Clear flag
    nResult = ReadQuad(SIOMM_STATUS_READ_PUC_FLAG, &dwPUCFlag);
  
    // Check for good result from the ReadQuad()
    if (SIOMM_OK == nResult)
    {
      // the PUC flag will be TRUE if PUC is needed
      if (dwPUCFlag) 
      {
        // Send PUC
        nResult = WriteQuad(SIOMM_STATUS_WRITE_OPERATION, 1);

        return nResult;
      }
      else
      {
        // Everything must be okay
        return SIOMM_OK;
      }
    }
    else // the ReadQuad() had an error
    {
      return SIOMM_ERROR;
    }
  }
  else
  {
    // Everything must be okay
    return SIOMM_OK;
  }
}


LONG O22SnapIoMemMap::SetCommOptions(LONG nTimeOutMS, LONG nReserved)
//-------------------------------------------------------------------------------------------------
// Set communication options
//-------------------------------------------------------------------------------------------------
{
  // m_nRetries   = nReserved;
  m_nTimeOutMS = nTimeOutMS;
  
  // Set the timeout that sockets will use
  m_tvTimeOut.tv_sec  = m_nTimeOutMS / 1000;
  m_tvTimeOut.tv_usec = m_nTimeOutMS % 1000;

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::BuildReadBlockRequest(BYTE * pbyReadBlockRequest,
                                            BYTE   byTransactionLabel, 
                                            DWORD  dwDestinationOffset,
                                            WORD   wDataLength)
//-------------------------------------------------------------------------------------------------
// Build a read block request packet
//-------------------------------------------------------------------------------------------------
{
  // Destination Id
  pbyReadBlockRequest[0]  = 0x00;
  pbyReadBlockRequest[1]  = 0x00;

  // Transaction Label
  pbyReadBlockRequest[2]  = byTransactionLabel << 2;

  // Transaction Code
  pbyReadBlockRequest[3]  = SIOMM_TCODE_READ_BLOCK_REQUEST << 4;

  // Source Id
  pbyReadBlockRequest[4]  = 0x00;
  pbyReadBlockRequest[5]  = 0x00;

  // Destination Offset
  pbyReadBlockRequest[6]  = 0xFF;
  pbyReadBlockRequest[7]  = 0xFF;
  pbyReadBlockRequest[8]  = O22BYTE0(dwDestinationOffset);
  pbyReadBlockRequest[9]  = O22BYTE1(dwDestinationOffset);
  pbyReadBlockRequest[10] = O22BYTE2(dwDestinationOffset);
  pbyReadBlockRequest[11] = O22BYTE3(dwDestinationOffset);

  // Data length
  pbyReadBlockRequest[12] = O22HIBYTE(wDataLength);
  pbyReadBlockRequest[13] = O22LOBYTE(wDataLength);

  // Extended Transaction Code
  pbyReadBlockRequest[14] = 0x00;
  pbyReadBlockRequest[15] = 0x00;

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::BuildReadQuadletRequest(BYTE * pbyReadQuadletRequest,
                                              BYTE byTransactionLabel, 
                                              DWORD dwDestinationOffset)
//-------------------------------------------------------------------------------------------------
// Build a read quadlet request packet
//-------------------------------------------------------------------------------------------------
{
  pbyReadQuadletRequest[0]  = 0x00;
  pbyReadQuadletRequest[1]  = 0x00;
  pbyReadQuadletRequest[2]  = byTransactionLabel << 2;
  pbyReadQuadletRequest[3]  = SIOMM_TCODE_READ_QUAD_REQUEST << 4;
  pbyReadQuadletRequest[4]  = 0x00;
  pbyReadQuadletRequest[5]  = 0x00;
  pbyReadQuadletRequest[6]  = 0xFF;
  pbyReadQuadletRequest[7]  = 0xFF;
  pbyReadQuadletRequest[8]  = O22BYTE0(dwDestinationOffset);
  pbyReadQuadletRequest[9]  = O22BYTE1(dwDestinationOffset);
  pbyReadQuadletRequest[10] = O22BYTE2(dwDestinationOffset);
  pbyReadQuadletRequest[11] = O22BYTE3(dwDestinationOffset);

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::BuildWriteBlockRequest(BYTE *  pbyWriteBlockRequest,
                                             BYTE    byTransactionLabel,
                                             DWORD   dwDestinationOffset,
                                             WORD    wDataLength,
                                             BYTE  * pbyBlockData)
//-------------------------------------------------------------------------------------------------
// Build a write block request packet
//-------------------------------------------------------------------------------------------------
{
  // Destination Id
  pbyWriteBlockRequest[0]  = 0x00;
  pbyWriteBlockRequest[1]  = 0x00;

  // Transaction Label
  pbyWriteBlockRequest[2]  = byTransactionLabel << 2;

  // Transaction Code
  pbyWriteBlockRequest[3]  = SIOMM_TCODE_WRITE_BLOCK_REQUEST << 4;

  // Source Id
  pbyWriteBlockRequest[4]  = 0x00;
  pbyWriteBlockRequest[5]  = 0x00;

  // Destination Offset
  pbyWriteBlockRequest[6]  = 0xFF;
  pbyWriteBlockRequest[7]  = 0xFF;
  pbyWriteBlockRequest[8]  = O22BYTE0(dwDestinationOffset);
  pbyWriteBlockRequest[9]  = O22BYTE1(dwDestinationOffset);
  pbyWriteBlockRequest[10] = O22BYTE2(dwDestinationOffset);
  pbyWriteBlockRequest[11] = O22BYTE3(dwDestinationOffset);

  // Data length
  pbyWriteBlockRequest[12] = O22HIBYTE(wDataLength);
  pbyWriteBlockRequest[13] = O22LOBYTE(wDataLength);
  
  // Extended Transaction Code
  pbyWriteBlockRequest[14] = 0x00;
  pbyWriteBlockRequest[15] = 0x00;

  // Block data to write
  memcpy(&(pbyWriteBlockRequest[16]), pbyBlockData, wDataLength);


  return SIOMM_OK;
}



LONG O22SnapIoMemMap::BuildWriteQuadletRequest(BYTE * pbyWriteQuadletRequest,
                                               BYTE byTransactionLabel, 
                                               WORD wSourceId,
                                               DWORD dwDestinationOffset,
                                               DWORD dwQuadletData)
//-------------------------------------------------------------------------------------------------
// Build a write quadlet request packet
//-------------------------------------------------------------------------------------------------
{
  pbyWriteQuadletRequest[0]  = 0x00;
  pbyWriteQuadletRequest[1]  = 0x00;
  pbyWriteQuadletRequest[2]  = byTransactionLabel << 2;
  pbyWriteQuadletRequest[3]  = SIOMM_TCODE_WRITE_QUAD_REQUEST << 4;
  pbyWriteQuadletRequest[4]  = O22HIBYTE(wSourceId);
  pbyWriteQuadletRequest[5]  = O22LOBYTE(wSourceId);
  pbyWriteQuadletRequest[6]  = 0xFF;
  pbyWriteQuadletRequest[7]  = 0xFF;
  pbyWriteQuadletRequest[8]  = O22BYTE0(dwDestinationOffset);
  pbyWriteQuadletRequest[9]  = O22BYTE1(dwDestinationOffset);
  pbyWriteQuadletRequest[10] = O22BYTE2(dwDestinationOffset);
  pbyWriteQuadletRequest[11] = O22BYTE3(dwDestinationOffset);
  pbyWriteQuadletRequest[12] = O22BYTE0(dwQuadletData);
  pbyWriteQuadletRequest[13] = O22BYTE1(dwQuadletData);
  pbyWriteQuadletRequest[14] = O22BYTE2(dwQuadletData);
  pbyWriteQuadletRequest[15] = O22BYTE3(dwQuadletData);

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::UnpackReadQuadletResponse(BYTE  * pbyReadQuadletResponse,
                                                BYTE  * pbyTransactionLabel, 
                                                BYTE  * pbyResponseCode,
                                                DWORD * pdwQuadletData)
//-------------------------------------------------------------------------------------------------
// Unpack a read quadlet response from the brain
//-------------------------------------------------------------------------------------------------
{
  // Check for correct Transaction Code
  BYTE byTransactionCode = pbyReadQuadletResponse[3] >> 4;
  if (SIOMM_TCODE_READ_QUAD_RESPONSE != byTransactionCode)
  {
    return SIOMM_ERROR;
  }

  *pbyTransactionLabel = pbyReadQuadletResponse[2];
  *pbyTransactionLabel >>= 2;

  *pbyResponseCode = pbyReadQuadletResponse[6];
  *pbyResponseCode >>= 4;

  *pdwQuadletData = O22MAKELONG(pbyReadQuadletResponse[12], pbyReadQuadletResponse[13],
                                pbyReadQuadletResponse[14], pbyReadQuadletResponse[15]);

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::UnpackReadBlockResponse(BYTE  * pbyReadBlockResponse,
                                              BYTE  * pbyTransactionLabel, 
                                              BYTE  * pbyResponseCode,
                                              WORD  * pwDataLength,
                                              BYTE  * pbyBlockData)
//-------------------------------------------------------------------------------------------------
// Unpack a read block response from the brain
//-------------------------------------------------------------------------------------------------
{
  // Check for correct Transaction Code
  BYTE byTransactionCode = pbyReadBlockResponse[3] >> 4;
  if (SIOMM_TCODE_READ_BLOCK_RESPONSE != byTransactionCode)
  {
    return SIOMM_ERROR;
  }

  *pbyTransactionLabel = pbyReadBlockResponse[2];
  *pbyTransactionLabel >>= 2;

  *pbyResponseCode = pbyReadBlockResponse[6];
  *pbyResponseCode >>= 4;

  *pwDataLength = O22MAKEWORD(pbyReadBlockResponse[12], pbyReadBlockResponse[13]);

  memcpy(pbyBlockData, &(pbyReadBlockResponse[16]), *pwDataLength);

  return SIOMM_OK;
}



LONG O22SnapIoMemMap::UnpackWriteResponse(BYTE  * pbyWriteResponse,
                                          BYTE  * pbyTransactionLabel, 
                                          BYTE  * pbyResponseCode)
//-------------------------------------------------------------------------------------------------
// Unpack a write response from the brain
//-------------------------------------------------------------------------------------------------
{
  // Check for correct Transaction Code
  BYTE byTransactionCode = pbyWriteResponse[3] >> 4;
  if (SIOMM_TCODE_WRITE_RESPONSE != byTransactionCode)
  {
    return SIOMM_ERROR;
  }

  // Unpack Transaction Label
  *pbyTransactionLabel = pbyWriteResponse[2];
  *pbyTransactionLabel >>= 2;

  // Unpack Response Code
  *pbyResponseCode = pbyWriteResponse[6];
  *pbyResponseCode >>= 4;

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::ReadFloat(DWORD dwDestOffset, float * pfValue)
//-------------------------------------------------------------------------------------------------
// Read a float value from a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  DWORD dwQuadlet; // A temp for getting the read value
  LONG  nResult;   // for checking the return values of functions
  
  nResult = ReadQuad(dwDestOffset, &dwQuadlet);

  // Check the result
  if (nResult == SIOMM_OK)
  {
    // If the ReadQuad was OK, copy the data
    memcpy(pfValue, &dwQuadlet, 4);
  }

  return nResult;
}


LONG O22SnapIoMemMap::WriteFloat(DWORD dwDestOffset, float fValue)
//-------------------------------------------------------------------------------------------------
// Write a float value to a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  DWORD dwQuadlet; // A temp for setting the write value

  // Copy the float into a DWORD for easy manipulation
  memcpy(&dwQuadlet, &fValue, 4);
  
  return WriteQuad(dwDestOffset, dwQuadlet);
}


LONG O22SnapIoMemMap::ReadBlock(DWORD dwDestOffset, WORD wDataLength, BYTE * pbyData)
//-------------------------------------------------------------------------------------------------
// Read a block of data from a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  BYTE  byReadBlockRequest[SIOMM_SIZE_READ_BLOCK_REQUEST];
  BYTE *pbyReadBlockResponse;
  BYTE  byTransactionLabel;
  BYTE  byResponseCode;
  WORD  wDataLengthTemp;
  BYTE *pbyDataTemp;
  LONG  nResult;
  fd_set fds;


  // Check that we have a valid socket
  if (INVALID_SOCKET == m_Socket)
  {
    return SIOMM_ERROR_NOT_CONNECTED;
  }

  // Increment the transaction label
  UpdateTransactionLabel();

  // The response will be padded to land on a quadlet boundary;
  wDataLengthTemp = wDataLength;
  while ((wDataLengthTemp%4) != 0)
    wDataLengthTemp++;

  // Allocate the response buffer
  pbyReadBlockResponse = new BYTE[wDataLengthTemp + SIOMM_SIZE_READ_BLOCK_RESPONSE];
  if (pbyReadBlockResponse == NULL)
    return SIOMM_ERROR_OUT_OF_MEMORY; // Couldn't allocate memory!

  FD_ZERO(&fds);
  FD_SET(m_Socket, &fds);

  // Build the request packet
  BuildReadBlockRequest(byReadBlockRequest, m_byTransactionLabel, dwDestOffset, wDataLength);

  // Send the packet to the Snap I/O unit
  nResult = send(m_Socket, (char*)byReadBlockRequest, SIOMM_SIZE_READ_BLOCK_REQUEST, 0 /*??*/ );
  if (SOCKET_ERROR == nResult)
  {
#ifdef _WIN32
    nResult = WSAGetLastError();  // for checking the specific error
#endif
    return SIOMM_ERROR; // This probably means we're not connected.
  }

#ifdef _LINUX
  // In the call to select, Linux modifies m_tvTimeOut.  It needs to be reset every time.
  m_tvTimeOut.tv_sec  = m_nTimeOutMS / 1000;
  m_tvTimeOut.tv_usec = m_nTimeOutMS % 1000;
#endif

  // Is the recv ready?
  if (0 == select(m_Socket + 1, &fds, NULL, NULL, &m_tvTimeOut)) // ?? Param#1 is ignored by Windows.  What should it be in other systems?
  {
    // we timed-out
    return SIOMM_TIME_OUT;
  }


  // The response is ready, so recv it.
  nResult = recv(m_Socket, (char*)pbyReadBlockResponse, 
                 wDataLengthTemp + SIOMM_SIZE_READ_BLOCK_RESPONSE, 0 /*??*/);

  if ((wDataLengthTemp + SIOMM_SIZE_READ_BLOCK_RESPONSE) != nResult)
  {
    // we got the wrong number of bytes back!
    return SIOMM_ERROR_RESPONSE_BAD;
  }

  // Allocate a temporary data buffer
  pbyDataTemp = new BYTE[wDataLength];
  if (pbyDataTemp == NULL)
    return SIOMM_ERROR_OUT_OF_MEMORY; // Couldn't allocate memory!

  // Unpack the response.
  nResult = UnpackReadBlockResponse(pbyReadBlockResponse,
                                    &byTransactionLabel, &byResponseCode, 
                                    &wDataLengthTemp, pbyDataTemp);

  // Clean up memory
  delete pbyReadBlockResponse;

  // Check that the response was okay
  if ((SIOMM_OK == nResult) && 
      (SIOMM_RESPONSE_CODE_ACK == byResponseCode) && 
      (m_byTransactionLabel == byTransactionLabel))
  {
    // The response was good, so copy the data   
    memcpy(pbyData, pbyDataTemp, wDataLengthTemp);
    delete pbyDataTemp;
    return SIOMM_OK;
  }
  else if ((SIOMM_OK == nResult) && (SIOMM_RESPONSE_CODE_NAK == byResponseCode))
  {
    // Clean up memory
    delete pbyDataTemp;

    // If a bad response from the brain, get its last error code
    long nErrorCode;
    nResult = GetStatusLastError(&nErrorCode);
    if (SIOMM_OK == nResult)
    {
      return nErrorCode;
    }
    else
    {
      return nResult;
    }
  }
  else
  {
    delete pbyDataTemp;
    return SIOMM_ERROR_RESPONSE_BAD;
  }
}


    
LONG O22SnapIoMemMap::ReadQuad(DWORD dwDestOffset, DWORD * pdwQuadlet)
//-------------------------------------------------------------------------------------------------
// Read a quadlet of data from a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  BYTE  byReadQuadletRequest[SIOMM_SIZE_READ_QUAD_REQUEST];
  BYTE  byReadQuadletResponse[SIOMM_SIZE_READ_QUAD_RESPONSE];
  BYTE  byTransactionLabel;
  BYTE  byResponseCode;
  DWORD dwQuadletTemp;
  LONG  nResult;
  fd_set fds;


  // Check that we have a valid socket
  if (INVALID_SOCKET == m_Socket)
  {
    return SIOMM_ERROR_NOT_CONNECTED;
  }

  // Increment the transaction label
  UpdateTransactionLabel();

  FD_ZERO(&fds);
  FD_SET(m_Socket, &fds);

  // Build the request packet
  BuildReadQuadletRequest(byReadQuadletRequest, m_byTransactionLabel, dwDestOffset);

  // Send the packet to the Snap I/O unit
  nResult = send(m_Socket, (char*)byReadQuadletRequest, SIOMM_SIZE_READ_QUAD_REQUEST, 0 /*??*/ );
  if (SOCKET_ERROR == nResult)
  {
#ifdef _WIN32
    nResult = WSAGetLastError();  // for checking the specific error
#endif
    return SIOMM_ERROR; // This probably means we're not connected.
  }

#ifdef _LINUX
  // In the call to select, Linux modifies m_tvTimeOut.  It needs to be reset every time.
  m_tvTimeOut.tv_sec  = m_nTimeOutMS / 1000;
  m_tvTimeOut.tv_usec = m_nTimeOutMS % 1000;
#endif

  // Is the recv ready?
  if (0 == select(m_Socket + 1, &fds, NULL, NULL, &m_tvTimeOut)) // ?? Param#1 is ignored by Windows.  What should it be in other systems?
  {
    // we timed-out
    return SIOMM_TIME_OUT;
  }

  // The response is ready, so recv it.
  nResult = recv(m_Socket, (char*)byReadQuadletResponse, SIOMM_SIZE_READ_QUAD_RESPONSE, 0 /*??*/);
  if (SIOMM_SIZE_READ_QUAD_RESPONSE != nResult)
  {
    // we got the wrong number of bytes back!
    return SIOMM_ERROR_RESPONSE_BAD;
  }


  // Unpack the response.
  nResult = UnpackReadQuadletResponse(byReadQuadletResponse,
                                      &byTransactionLabel, 
                                      &byResponseCode, &dwQuadletTemp);

  // Check that the response was okay
  if ((SIOMM_OK == nResult) && 
      (SIOMM_RESPONSE_CODE_ACK == byResponseCode) && 
      (m_byTransactionLabel == byTransactionLabel))
  {
    // The response was good, so copy the quadlet
    *pdwQuadlet = dwQuadletTemp;
    return SIOMM_OK;
  }
  else if ((SIOMM_OK == nResult) && (SIOMM_RESPONSE_CODE_NAK == byResponseCode))
  {
    // If a bad response from the brain, get its last error code
    long nErrorCode;
    nResult = GetStatusLastError(&nErrorCode);
    if (SIOMM_OK == nResult)
    {
      return nErrorCode;
    }
    else
    {
      return nResult;
    }
  }
  else
  {
    return SIOMM_ERROR_RESPONSE_BAD;
  }
}


LONG O22SnapIoMemMap::WriteBlock(DWORD dwDestOffset, WORD wDataLength, BYTE * pbyData)
//-------------------------------------------------------------------------------------------------
// Write a block of data to a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  BYTE *pbyWriteBlockRequest;
  BYTE  byWriteBlockResponse[SIOMM_SIZE_WRITE_RESPONSE];
  BYTE  byTransactionLabel;
  BYTE  byResponseCode;
  LONG  nResult;
  fd_set fds;

    // Check that we have a valid socket
  if (INVALID_SOCKET == m_Socket)
  {
    return SIOMM_ERROR_NOT_CONNECTED;
  }

  // Increment the transaction label
  UpdateTransactionLabel();

  FD_ZERO(&fds);
  FD_SET(m_Socket, &fds);

  pbyWriteBlockRequest = new BYTE[SIOMM_SIZE_WRITE_BLOCK_REQUEST + wDataLength];
  if (pbyWriteBlockRequest == NULL)
    return SIOMM_ERROR_OUT_OF_MEMORY; // Couldn't allocate memory!

  // Build the write request packet
  BuildWriteBlockRequest(pbyWriteBlockRequest, m_byTransactionLabel, 
                         dwDestOffset, wDataLength, pbyData);

  // Send the packet to the Snap I/O unit
  nResult = send(m_Socket, (char*)pbyWriteBlockRequest, SIOMM_SIZE_WRITE_BLOCK_REQUEST + wDataLength, 0 /*??*/ );

  // Cleanup memory
  delete pbyWriteBlockRequest;

  // Check the result from send()
  if (SOCKET_ERROR == nResult)
  {
    return SIOMM_ERROR; // oops!
  }


#ifdef _LINUX
  // In the call to select, Linux modifies m_tvTimeOut.  It needs to be reset every time.
  m_tvTimeOut.tv_sec  = m_nTimeOutMS / 1000;
  m_tvTimeOut.tv_usec = m_nTimeOutMS % 1000;
#endif

  // Is the recv ready?
  if (0 == select(m_Socket + 1, &fds, NULL, NULL, &m_tvTimeOut)) // ?? Param#1 is ignored by Windows.  What should it be in other systems?
  {
    // we timed-out
    return SIOMM_TIME_OUT;
  }

  // The response is ready, so recv it.
  nResult = recv(m_Socket, (char*)byWriteBlockResponse, SIOMM_SIZE_WRITE_RESPONSE, 0 /*??*/);
  if (SIOMM_SIZE_WRITE_RESPONSE != nResult)
  {
    // we got the wrong number of bytes back!
    return SIOMM_ERROR_RESPONSE_BAD;
  }
 
  // Unpack the response
  nResult = UnpackWriteResponse(byWriteBlockResponse, &byTransactionLabel, 
                                &byResponseCode);

  // Check that the response was okay
  if ((SIOMM_OK == nResult) && 
      (SIOMM_RESPONSE_CODE_ACK == byResponseCode) && 
      (m_byTransactionLabel == byTransactionLabel))
  {
    // The response was good!
    return SIOMM_OK;
  }
  else if ((SIOMM_OK == nResult) && (SIOMM_RESPONSE_CODE_NAK == byResponseCode))
  {
    // If a bad response from the brain, get its last error code
    long nErrorCode;
    nResult = GetStatusLastError(&nErrorCode);
    if (SIOMM_OK == nResult)
    {
      return nErrorCode;
    }
    else
    {
      return nResult;
    }
  }
  else
  {
    return SIOMM_ERROR_RESPONSE_BAD;
  }
}


LONG O22SnapIoMemMap::WriteQuad(DWORD dwDestOffset, DWORD dwQuadlet)
//-------------------------------------------------------------------------------------------------
// Write a quadlet of data to a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  BYTE  byWriteQuadletRequest[SIOMM_SIZE_WRITE_QUAD_REQUEST];
  BYTE  byWriteQuadletResponse[SIOMM_SIZE_WRITE_RESPONSE];
  BYTE  byTransactionLabel;
  BYTE  byResponseCode;
  LONG  nResult;
  fd_set fds;

  // Check that we have a valid socket
  if (INVALID_SOCKET == m_Socket)
  {
    return SIOMM_ERROR_NOT_CONNECTED;
  }

  // Increment the transaction label
  UpdateTransactionLabel();

  FD_ZERO(&fds);
  FD_SET(m_Socket, &fds);

  // Build the write request packet
  BuildWriteQuadletRequest(byWriteQuadletRequest, m_byTransactionLabel, 1, 
                           dwDestOffset, dwQuadlet);

  // Send the packet to the Snap I/O unit
  nResult = send(m_Socket, (char*)byWriteQuadletRequest, SIOMM_SIZE_WRITE_QUAD_REQUEST, 0 /*??*/ );
  if (SOCKET_ERROR == nResult)
  {
    return SIOMM_ERROR; // oops!
  }

#ifdef _LINUX
  // In the call to select, Linux modifies m_tvTimeOut.  It needs to be reset every time.
  m_tvTimeOut.tv_sec  = m_nTimeOutMS / 1000;
  m_tvTimeOut.tv_usec = m_nTimeOutMS % 1000;
#endif

  // Is the recv ready?
  if (0 == select(m_Socket + 1, &fds, NULL, NULL, &m_tvTimeOut)) // ?? Param#1 is ignored by Windows.  What should it be in other systems?
  {
    // we timed-out
    return SIOMM_TIME_OUT;
  }

  // The response is ready, so recv it.
  nResult = recv(m_Socket, (char*)byWriteQuadletResponse, SIOMM_SIZE_WRITE_RESPONSE, 0 /*??*/);
  if (SIOMM_SIZE_WRITE_RESPONSE != nResult)
  {
    // we got the wrong number of bytes back!
    return SIOMM_ERROR_RESPONSE_BAD;
  }
 
  // Unpack the response
  nResult = UnpackWriteResponse(byWriteQuadletResponse, &byTransactionLabel, &byResponseCode);

  // Check that the response was okay
  if ((SIOMM_OK == nResult) && 
      (SIOMM_RESPONSE_CODE_ACK == byResponseCode) && 
      (m_byTransactionLabel == byTransactionLabel))
  {
    // The response was good!
    return SIOMM_OK;
  }
  else if ((SIOMM_OK == nResult) && (SIOMM_RESPONSE_CODE_NAK == byResponseCode))
  {
    // If a bad response from the brain, get its last error code
    long nErrorCode;
    nResult = GetStatusLastError(&nErrorCode);
    if (SIOMM_OK == nResult)
    {
      return nErrorCode;
    }
    else
    {
      return nResult;
    }
  }
  else
  {
    return SIOMM_ERROR_RESPONSE_BAD;
  }
}


LONG O22SnapIoMemMap::Close()
//-------------------------------------------------------------------------------------------------
// Close the connection to the I/O unit
//-------------------------------------------------------------------------------------------------
{
  return CloseSockets();
}


LONG O22SnapIoMemMap::GetDigPtState(long nPoint, long *pnState)
//-------------------------------------------------------------------------------------------------
// Get the state of the specified digital point.
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_STATE + (SIOMM_DPOINT_READ_BOUNDARY * nPoint), 
                   (DWORD*)pnState);
}


LONG O22SnapIoMemMap::GetDigPtOnLatch(long nPoint, long *pnState)
//-------------------------------------------------------------------------------------------------
// Get the on-latch state of the specified digital point.
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_ONLATCH_STATE + (SIOMM_DPOINT_READ_BOUNDARY * nPoint), 
                   (DWORD*)pnState);

}

LONG O22SnapIoMemMap::GetDigPtOffLatch(long nPoint, long *pnState)
//-------------------------------------------------------------------------------------------------
// Get the off-latch state of the specified digital point.
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_OFFLATCH_STATE + (SIOMM_DPOINT_READ_BOUNDARY * nPoint), 
                   (DWORD*)pnState);

}

LONG O22SnapIoMemMap::GetDigPtCounterState(long nPoint, long *pnState)
//-------------------------------------------------------------------------------------------------
// Get the active counter state of the specified digital point.
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_ACTIVE_COUNTER + (SIOMM_DPOINT_READ_BOUNDARY * nPoint), 
                   (DWORD*)pnState);

}

LONG O22SnapIoMemMap::GetDigPtCounts(long nPoint, long *pnValue)
//-------------------------------------------------------------------------------------------------
// Get the counters of the specified digital point.
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_COUNTER_DATA + (SIOMM_DPOINT_READ_BOUNDARY * nPoint), 
                   (DWORD*)pnValue);

}

LONG O22SnapIoMemMap::GetDigPtReadAreaEx(long nPoint, SIOMM_DigPointReadArea * pData)
//-------------------------------------------------------------------------------------------------
// Get the read area for the specified digital point
//-------------------------------------------------------------------------------------------------
{

  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[20]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_DPOINT_READ_AREA_BASE + (SIOMM_DPOINT_READ_BOUNDARY * nPoint), 20, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pData->nState        = O22MAKELONG(arrbyData[0],  arrbyData[1],
                                       arrbyData[2],  arrbyData[3]);
    pData->nOnLatch      = O22MAKELONG(arrbyData[4],  arrbyData[5],
                                       arrbyData[6],  arrbyData[7]);
    pData->nOffLatch     = O22MAKELONG(arrbyData[8],  arrbyData[9],
                                       arrbyData[10], arrbyData[11]);
    pData->nCounterState = O22MAKELONG(arrbyData[12], arrbyData[13],
                                       arrbyData[14], arrbyData[15]);
    pData->nCounts       = O22MAKELONG(arrbyData[16], arrbyData[17],
                                       arrbyData[18], arrbyData[19]);

  }

  return nResult;


}


LONG O22SnapIoMemMap::SetDigPtState(long nPoint, long nState)
//-------------------------------------------------------------------------------------------------
// Set the state of the specified digital point
//-------------------------------------------------------------------------------------------------
{
  if (nState)
    return  WriteQuad(SIOMM_DPOINT_WRITE_TURN_ON_BASE  + (SIOMM_DPOINT_WRITE_BOUNDARY * nPoint), 1);
  else
    return  WriteQuad(SIOMM_DPOINT_WRITE_TURN_OFF_BASE + (SIOMM_DPOINT_WRITE_BOUNDARY * nPoint), 1);
}


LONG O22SnapIoMemMap::SetDigPtCounterState(long nPoint, long nState)
//-------------------------------------------------------------------------------------------------
// Set the active counter state of the specified digital point
//-------------------------------------------------------------------------------------------------
{
  if (nState)
    return  WriteQuad(SIOMM_DPOINT_WRITE_ACTIVATE_COUNTER   + (SIOMM_DPOINT_WRITE_BOUNDARY * nPoint), 1);
  else
    return  WriteQuad(SIOMM_DPOINT_WRITE_DEACTIVATE_COUNTER + (SIOMM_DPOINT_WRITE_BOUNDARY * nPoint), 1);
}


LONG O22SnapIoMemMap::GetBitmask64(DWORD dwDestOffset, long *pnPts63to32, long *pnPts31to0)
//-------------------------------------------------------------------------------------------------
// Get a 64-bit bitmask from a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[8]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(dwDestOffset, 8, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and make the longs
    *pnPts63to32 = O22MAKELONG(arrbyData[0], arrbyData[1],
                               arrbyData[2], arrbyData[3]);
    *pnPts31to0  = O22MAKELONG(arrbyData[4], arrbyData[5],
                               arrbyData[6], arrbyData[7]);
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetBitmask64(DWORD dwDestOffset, long nPts63to32, long nPts31to0)
//-------------------------------------------------------------------------------------------------
// Set a 64-bit bitmask to a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[8]; // buffer for the data to be read

  // Unpack the bytes
  arrbyData[0] = O22BYTE0(nPts63to32);
  arrbyData[1] = O22BYTE1(nPts63to32);
  arrbyData[2] = O22BYTE2(nPts63to32);
  arrbyData[3] = O22BYTE3(nPts63to32);
  arrbyData[4] = O22BYTE0(nPts31to0);
  arrbyData[5] = O22BYTE1(nPts31to0);
  arrbyData[6] = O22BYTE2(nPts31to0);
  arrbyData[7] = O22BYTE3(nPts31to0);

  // Write the data
  nResult = WriteBlock(dwDestOffset, 8, (BYTE*)arrbyData);

  return nResult;
}


LONG O22SnapIoMemMap::GetDigBankPointStates(long *pnPts63to32, long *pnPts31to0)
//-------------------------------------------------------------------------------------------------
// Get the digital point states in the digital bank
//-------------------------------------------------------------------------------------------------
{
  return GetBitmask64(SIOMM_DBANK_READ_POINT_STATES, pnPts63to32, pnPts31to0); 
}


LONG O22SnapIoMemMap::GetDigBankOnLatchStates(long *pnPts63to32, long *pnPts31to0)
//-------------------------------------------------------------------------------------------------
// Get the digital point on-latch states in the digital bank
//-------------------------------------------------------------------------------------------------
{
  return GetBitmask64(SIOMM_DBANK_READ_ON_LATCH_STATES, pnPts63to32, pnPts31to0); 
}


LONG O22SnapIoMemMap::GetDigBankOffLatchStates(long *pnPts63to32, long *pnPts31to0)
//-------------------------------------------------------------------------------------------------
// Get the digital point off-latch states in the digital bank
//-------------------------------------------------------------------------------------------------
{
  return GetBitmask64(SIOMM_DBANK_READ_OFF_LATCH_STATES, pnPts63to32, pnPts31to0); 
}


LONG O22SnapIoMemMap::GetDigBankActCounterStates(long *pnPts63to32, long *pnPts31to0)
//-------------------------------------------------------------------------------------------------
// Get the digital point active counter states in the digital bank
//-------------------------------------------------------------------------------------------------
{
  return GetBitmask64(SIOMM_DBANK_READ_ACTIVE_COUNTERS, pnPts63to32, pnPts31to0); 
}


LONG O22SnapIoMemMap::SetDigBankPointStates(long nPts63to32, long nPts31to0, 
                                            long nMask63to32, long nMask31to0)
//-------------------------------------------------------------------------------------------------
// Set the digital point states in the digital bank.  Only those points set in the mask parameters
// are set.
//-------------------------------------------------------------------------------------------------
{
  long nOnMask63to32;
  long nOnMask31to0;
  long nOffMask63to32;
  long nOffMask31to0;
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[16]; // buffer for the data to be read

  // Figure out the On Mask
  nOnMask63to32 = nPts63to32 & nMask63to32;
  nOnMask31to0 = nPts31to0 & nMask31to0;

  // Figure out the Off Mask
  nOffMask63to32 = (nPts63to32 ^ 0xFFFFFFFF) & nMask63to32;
  nOffMask31to0  = (nPts31to0  ^ 0xFFFFFFFF) & nMask31to0;

  // Pack the bytes
  arrbyData[0]  = O22BYTE0(nOnMask63to32);
  arrbyData[1]  = O22BYTE1(nOnMask63to32);
  arrbyData[2]  = O22BYTE2(nOnMask63to32);
  arrbyData[3]  = O22BYTE3(nOnMask63to32);
  arrbyData[4]  = O22BYTE0(nOnMask31to0);
  arrbyData[5]  = O22BYTE1(nOnMask31to0);
  arrbyData[6]  = O22BYTE2(nOnMask31to0);
  arrbyData[7]  = O22BYTE3(nOnMask31to0);
  arrbyData[8]  = O22BYTE0(nOffMask63to32);
  arrbyData[9]  = O22BYTE1(nOffMask63to32);
  arrbyData[10] = O22BYTE2(nOffMask63to32);
  arrbyData[11] = O22BYTE3(nOffMask63to32);
  arrbyData[12] = O22BYTE0(nOffMask31to0);
  arrbyData[13] = O22BYTE1(nOffMask31to0);
  arrbyData[14] = O22BYTE2(nOffMask31to0);
  arrbyData[15] = O22BYTE3(nOffMask31to0);

  // Write the data
  nResult = WriteBlock(SIOMM_DBANK_WRITE_TURN_ON_MASK, 16, (BYTE*)arrbyData);

  return nResult;

}

    
LONG O22SnapIoMemMap::SetDigBankOnMask(long nPts63to32, long nPts31to0)
//-------------------------------------------------------------------------------------------------
// Turn on the given digital points
//-------------------------------------------------------------------------------------------------
{
  return SetBitmask64(SIOMM_DBANK_WRITE_TURN_ON_MASK, nPts63to32, nPts31to0); 
}

LONG O22SnapIoMemMap::SetDigBankOffMask(long nPts63to32, long nPts31to0)
//-------------------------------------------------------------------------------------------------
// Turn off the given digital points
//-------------------------------------------------------------------------------------------------
{
  return SetBitmask64(SIOMM_DBANK_WRITE_TURN_OFF_MASK, nPts63to32, nPts31to0); 
}

LONG O22SnapIoMemMap::SetDigBankActCounterMask(long nPts63to32, long nPts31to0)
//-------------------------------------------------------------------------------------------------
// Active counters for the given digital points
//-------------------------------------------------------------------------------------------------
{
  return SetBitmask64(SIOMM_DBANK_WRITE_ACT_COUNTERS_MASK, nPts63to32, nPts31to0); 
}

LONG O22SnapIoMemMap::SetDigBankDeactCounterMask(long nPts63to32, long nPts31to0)
//-------------------------------------------------------------------------------------------------
// Deactive counters for the given digital points
//-------------------------------------------------------------------------------------------------
{
  return SetBitmask64(SIOMM_DBANK_WRITE_DEACT_COUNTERS_MASK, nPts63to32, nPts31to0); 
}


LONG O22SnapIoMemMap::GetDigBankReadAreaEx(SIOMM_DigBankReadArea * pData)
//-------------------------------------------------------------------------------------------------
// Get the read area for the digital bank
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;       // for checking the return values of functions
  BYTE arrbyData[32]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_DBANK_READ_AREA_BASE, 32, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pData->nStatePts63to32          = O22MAKELONG(arrbyData[0],  arrbyData[1],
                                                  arrbyData[2],  arrbyData[3]);
    pData->nStatePts31to0           = O22MAKELONG(arrbyData[4],  arrbyData[5],
                                                  arrbyData[6],  arrbyData[7]);

    pData->nOnLatchStatePts63to32   = O22MAKELONG(arrbyData[8],  arrbyData[9],
                                                  arrbyData[10], arrbyData[11]);
    pData->nOnLatchStatePts31to0    = O22MAKELONG(arrbyData[12], arrbyData[13],
                                                  arrbyData[14], arrbyData[15]);

    pData->nOffLatchStatePts63to32  = O22MAKELONG(arrbyData[16], arrbyData[17], 
                                                  arrbyData[18], arrbyData[19]);
    pData->nOffLatchStatePts31to0   = O22MAKELONG(arrbyData[20], arrbyData[21], 
                                                  arrbyData[22], arrbyData[23]);

    pData->nActiveCountersPts63to32 = O22MAKELONG(arrbyData[24], arrbyData[25],
                                                  arrbyData[26], arrbyData[27]);
    pData->nActiveCountersPts31to0  = O22MAKELONG(arrbyData[28], arrbyData[29],
                                                  arrbyData[30], arrbyData[31]);
  }

  return nResult;
}


LONG O22SnapIoMemMap::ReadClearDigPtCounts(long nPoint, long * pnState)
//-------------------------------------------------------------------------------------------------
// Read and clear counts for the specified digital point
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_CLEAR_COUNTS_BASE + (SIOMM_DPOINT_READ_CLEAR_BOUNDARY * nPoint), 
                   (DWORD*)pnState);
}


LONG O22SnapIoMemMap::ReadClearDigPtOnLatch(long nPoint, long * pnState)
//-------------------------------------------------------------------------------------------------
// Read and clear the on-latch state for the specified digital point
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_CLEAR_ON_LATCH_BASE + (SIOMM_DPOINT_READ_CLEAR_BOUNDARY * nPoint), 
                   (DWORD*)pnState);
}


LONG O22SnapIoMemMap::ReadClearDigPtOffLatch(long nPoint, long * pnState)
//-------------------------------------------------------------------------------------------------
// Read and clear the off-latch state for the specified digital point
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_CLEAR_OFF_LATCH_BASE + (SIOMM_DPOINT_READ_CLEAR_BOUNDARY * nPoint), 
                   (DWORD*)pnState);
}



LONG O22SnapIoMemMap::GetAnaPtValue(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Get the value of the specified analog point.
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_VALUE_BASE + (SIOMM_APOINT_READ_BOUNDARY * nPoint),
                   pfValue);
}


LONG O22SnapIoMemMap::GetAnaPtCounts(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Get the raw counts of the specified analog point.
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_COUNTS_BASE + (SIOMM_APOINT_READ_BOUNDARY * nPoint),
                   pfValue);
}


LONG O22SnapIoMemMap::GetAnaPtMinValue(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Get the minimum value of the specified analog point.
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_MIN_VALUE_BASE + (SIOMM_APOINT_READ_BOUNDARY * nPoint),
                   pfValue);
}


LONG O22SnapIoMemMap::GetAnaPtMaxValue(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Get the maximum value of the specified analog point.
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_MAX_VALUE_BASE + (SIOMM_APOINT_READ_BOUNDARY * nPoint),
                   pfValue);
}


LONG O22SnapIoMemMap::GetAnaPtReadAreaEx(long nPoint, SIOMM_AnaPointReadArea * pData)
//-------------------------------------------------------------------------------------------------
// Get the read area for the specified analog point
//-------------------------------------------------------------------------------------------------
{
  LONG  nResult;       // for checking the return values of functions
  BYTE  arrbyData[16]; // buffer for the data to be read
  DWORD dwTemp;

  // Read the data
  nResult = ReadBlock(SIOMM_APOINT_READ_AREA_BASE + (SIOMM_APOINT_READ_BOUNDARY * nPoint), 
                      16, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    dwTemp = O22MAKELONG(arrbyData[0],  arrbyData[1],  arrbyData[2],  arrbyData[3]);
    memcpy(&(pData->fValue), &dwTemp, 4);

    dwTemp = O22MAKELONG(arrbyData[4],  arrbyData[5],  arrbyData[6],  arrbyData[7]);
    memcpy(&(pData->fCounts), &dwTemp, 4);

    dwTemp = O22MAKELONG(arrbyData[8],  arrbyData[9],  arrbyData[10], arrbyData[11]);
    memcpy(&(pData->fMinValue), &dwTemp, 4);

    dwTemp = O22MAKELONG(arrbyData[12], arrbyData[13], arrbyData[14], arrbyData[15]);
    memcpy(&(pData->fMaxValue), &dwTemp, 4);
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetAnaPtValue(long nPoint, float fValue)
//-------------------------------------------------------------------------------------------------
// Set the value for the specified analog point
//-------------------------------------------------------------------------------------------------
{
  return WriteFloat(SIOMM_APOINT_WRITE_VALUE_BASE + (SIOMM_APOINT_WRITE_BOUNDARY * nPoint), 
                    fValue);
}


LONG O22SnapIoMemMap::SetAnaPtCounts(long nPoint, float fValue)
//-------------------------------------------------------------------------------------------------
// Set the raw counts for the specified analog point
//-------------------------------------------------------------------------------------------------
{
  return WriteFloat(SIOMM_APOINT_WRITE_COUNTS_BASE + (SIOMM_APOINT_WRITE_BOUNDARY * nPoint), 
                    fValue);
}



LONG O22SnapIoMemMap::ReadClearAnaPtMinValue(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Read and clear the minimum value for the specified analog point
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_CLEAR_MIN_VALUE_BASE + (SIOMM_APOINT_READ_CLEAR_BOUNDARY * nPoint), 
                   pfValue);
}


LONG O22SnapIoMemMap::ReadClearAnaPtMaxValue(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Read and clear the maximum value for the specified analog point
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_CLEAR_MAX_VALUE_BASE + (SIOMM_APOINT_READ_CLEAR_BOUNDARY * nPoint), 
                   pfValue);
}


LONG O22SnapIoMemMap::ConfigurePoint(long nPoint, long nPointType)
//-------------------------------------------------------------------------------------------------
// Configure the specified point
//-------------------------------------------------------------------------------------------------
{
  return WriteQuad(SIOMM_POINT_CONFIG_WRITE_TYPE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                   nPointType);
}


LONG O22SnapIoMemMap::GetModuleType(long nPoint, long * pnModuleType)
//-------------------------------------------------------------------------------------------------
// Get the module type at the specified point position
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_POINT_CONFIG_READ_MOD_TYPE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                   (DWORD*)pnModuleType);
}


LONG O22SnapIoMemMap::SetDigPtConfiguration(long nPoint, long nPointType, long nFeature)
//-------------------------------------------------------------------------------------------------
// Configure a digital point
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[8]; // buffer for the data to be read

  // Build the data area
  arrbyData[0] = O22BYTE0(nPointType);
  arrbyData[1] = O22BYTE1(nPointType);
  arrbyData[2] = O22BYTE2(nPointType);
  arrbyData[3] = O22BYTE3(nPointType);
  arrbyData[4] = O22BYTE0(nFeature);
  arrbyData[5] = O22BYTE1(nFeature);
  arrbyData[6] = O22BYTE2(nFeature);
  arrbyData[7] = O22BYTE3(nFeature);

  // Write the data
  nResult = WriteBlock(SIOMM_POINT_CONFIG_WRITE_TYPE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                       8, (BYTE*)arrbyData);

  return nResult;
}


LONG O22SnapIoMemMap::SetAnaPtConfiguration(long nPoint, long nPointType, 
                                            float fOffset, float fGain, 
                                            float fHiScale, float fLoScale)
//-------------------------------------------------------------------------------------------------
// Configure an analog point
//-------------------------------------------------------------------------------------------------
{
  BYTE arrbyData[24]; // buffer for the data to be read
  DWORD dwTemp; 

  // Build the data area
  arrbyData[0] = O22BYTE0(nPointType);
  arrbyData[1] = O22BYTE1(nPointType);
  arrbyData[2] = O22BYTE2(nPointType);
  arrbyData[3] = O22BYTE3(nPointType);
  arrbyData[4] = 0x00;
  arrbyData[5] = 0x00;
  arrbyData[6] = 0x00;
  arrbyData[7] = 0x00;

  memcpy(&dwTemp, &fOffset, 4);
  arrbyData[8]  = O22BYTE0(dwTemp);
  arrbyData[9]  = O22BYTE1(dwTemp);
  arrbyData[10] = O22BYTE2(dwTemp);
  arrbyData[11] = O22BYTE3(dwTemp);

  memcpy(&dwTemp, &fGain, 4);
  arrbyData[12] = O22BYTE0(dwTemp);
  arrbyData[13] = O22BYTE1(dwTemp);
  arrbyData[14] = O22BYTE2(dwTemp);
  arrbyData[15] = O22BYTE3(dwTemp);

  memcpy(&dwTemp, &fHiScale, 4);
  arrbyData[16] = O22BYTE0(dwTemp);
  arrbyData[17] = O22BYTE1(dwTemp);
  arrbyData[18] = O22BYTE2(dwTemp);
  arrbyData[19] = O22BYTE3(dwTemp);

  memcpy(&dwTemp, &fLoScale, 4);
  arrbyData[20] = O22BYTE0(dwTemp);
  arrbyData[21] = O22BYTE1(dwTemp);
  arrbyData[22] = O22BYTE2(dwTemp);
  arrbyData[23] = O22BYTE3(dwTemp);


  // Write the data
  return WriteBlock(SIOMM_POINT_CONFIG_WRITE_TYPE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                    24, (BYTE*)arrbyData);
}


LONG O22SnapIoMemMap::SetPtConfigurationEx(long nPoint, SIOMM_PointConfigArea PtConfigData)
//-------------------------------------------------------------------------------------------------
// Configure a digital or analog point
//-------------------------------------------------------------------------------------------------
{
  BYTE arrbyData[40]; // buffer for the data to be read
  DWORD dwTemp; 

  // Build the data area
  arrbyData[0] = O22BYTE0(PtConfigData.nPointType);
  arrbyData[1] = O22BYTE1(PtConfigData.nPointType);
  arrbyData[2] = O22BYTE2(PtConfigData.nPointType);
  arrbyData[3] = O22BYTE3(PtConfigData.nPointType);
  
  arrbyData[4] = O22BYTE0(PtConfigData.nFeature);
  arrbyData[5] = O22BYTE1(PtConfigData.nFeature);
  arrbyData[6] = O22BYTE2(PtConfigData.nFeature);
  arrbyData[7] = O22BYTE3(PtConfigData.nFeature);

  memcpy(&dwTemp, &(PtConfigData.fOffset), 4);
  arrbyData[8]  = O22BYTE0(dwTemp);
  arrbyData[9]  = O22BYTE1(dwTemp);
  arrbyData[10] = O22BYTE2(dwTemp);
  arrbyData[11] = O22BYTE3(dwTemp);

  memcpy(&dwTemp, &(PtConfigData.fGain), 4);
  arrbyData[12] = O22BYTE0(dwTemp);
  arrbyData[13] = O22BYTE1(dwTemp);
  arrbyData[14] = O22BYTE2(dwTemp);
  arrbyData[15] = O22BYTE3(dwTemp);

  memcpy(&dwTemp, &(PtConfigData.fHiScale), 4);
  arrbyData[16] = O22BYTE0(dwTemp);
  arrbyData[17] = O22BYTE1(dwTemp);
  arrbyData[18] = O22BYTE2(dwTemp);
  arrbyData[19] = O22BYTE3(dwTemp);

  memcpy(&dwTemp, &(PtConfigData.fLoScale), 4);
  arrbyData[20] = O22BYTE0(dwTemp);
  arrbyData[21] = O22BYTE1(dwTemp);
  arrbyData[22] = O22BYTE2(dwTemp);
  arrbyData[23] = O22BYTE3(dwTemp);

  // Bytes 24-31 are not used at this time. 

  memcpy(&dwTemp, &(PtConfigData.fWatchdogValue), 4);
  arrbyData[32] = O22BYTE0(dwTemp);
  arrbyData[33] = O22BYTE1(dwTemp);
  arrbyData[34] = O22BYTE2(dwTemp);
  arrbyData[35] = O22BYTE3(dwTemp);

  arrbyData[36] = O22BYTE0(PtConfigData.nWatchdogEnabled);
  arrbyData[37] = O22BYTE1(PtConfigData.nWatchdogEnabled);
  arrbyData[38] = O22BYTE2(PtConfigData.nWatchdogEnabled);
  arrbyData[39] = O22BYTE3(PtConfigData.nWatchdogEnabled);


  // Write the data
  return WriteBlock(SIOMM_POINT_CONFIG_WRITE_TYPE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                    40, (BYTE*)arrbyData);
}


LONG O22SnapIoMemMap::GetPtConfigurationEx(long nPoint, SIOMM_PointConfigArea * pPtConfigData)
//-------------------------------------------------------------------------------------------------
// Get the configuration for a point
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[44]; // buffer for the data to be read
  DWORD dwTemp; 

  // Read the data
  nResult = ReadBlock(SIOMM_POINT_CONFIG_READ_MOD_TYPE_BASE + 
                      (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                      44, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pPtConfigData->nModuleType = O22MAKELONG(arrbyData[0],  arrbyData[1],
                                             arrbyData[2],  arrbyData[3]);
    pPtConfigData->nPointType  = O22MAKELONG(arrbyData[4],  arrbyData[5],
                                             arrbyData[6],  arrbyData[7]);
   
    dwTemp = O22MAKELONG(arrbyData[8],  arrbyData[9], arrbyData[10], arrbyData[11]);
    memcpy(&(pPtConfigData->nFeature), &dwTemp, 4);
    
    dwTemp = O22MAKELONG(arrbyData[12], arrbyData[13], arrbyData[14], arrbyData[15]);
    memcpy(&(pPtConfigData->fOffset), &dwTemp, 4);

    dwTemp = O22MAKELONG(arrbyData[16], arrbyData[17], arrbyData[18], arrbyData[19]);
    memcpy(&(pPtConfigData->fGain), &dwTemp, 4);

    dwTemp = O22MAKELONG(arrbyData[20], arrbyData[21], arrbyData[22], arrbyData[23]);
    memcpy(&(pPtConfigData->fHiScale), &dwTemp, 4);

    dwTemp = O22MAKELONG(arrbyData[24], arrbyData[25], arrbyData[26], arrbyData[27]);
    memcpy(&(pPtConfigData->fLoScale), &dwTemp, 4);

    // Bytes 28-35 are not used at this time

    dwTemp = O22MAKELONG(arrbyData[36], arrbyData[37], arrbyData[38], arrbyData[39]);
    memcpy(&(pPtConfigData->fWatchdogValue), &dwTemp, 4);

    pPtConfigData->nWatchdogEnabled = O22MAKELONG(arrbyData[40],  arrbyData[41],
                                                  arrbyData[42],  arrbyData[43]);

  }

  return nResult;
}


LONG O22SnapIoMemMap::SetPtWatchdog(long nPoint, float fValue, long nEnabled)
//-------------------------------------------------------------------------------------------------
// Set the watchdog values for a point.
//-------------------------------------------------------------------------------------------------
{
  BYTE arrbyData[8]; // buffer for the data to be read
  DWORD dwTemp; 

  // Build the data area
  memcpy(&dwTemp, &fValue, 4);
  arrbyData[0] = O22BYTE0(dwTemp);
  arrbyData[1] = O22BYTE1(dwTemp);
  arrbyData[2] = O22BYTE2(dwTemp);
  arrbyData[3] = O22BYTE3(dwTemp);
  
  arrbyData[4] = O22BYTE0(nEnabled);
  arrbyData[5] = O22BYTE1(nEnabled);
  arrbyData[6] = O22BYTE2(nEnabled);
  arrbyData[7] = O22BYTE3(nEnabled);

  // Write the data
  return WriteBlock(SIOMM_POINT_CONFIG_WRITE_WDOG_VALUE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                    8, (BYTE*)arrbyData);
}



LONG O22SnapIoMemMap::CalcSetAnaPtOffset(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Calculate and set an analog point's offset
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_CALC_SET_OFFSET_BASE + 
                   (SIOMM_APOINT_READ_CALC_SET_BOUNDARY * nPoint), 
                   pfValue);
}


LONG O22SnapIoMemMap::CalcSetAnaPtGain(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Calculate and set an analog point's gain
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_CALC_SET_GAIN_BASE + 
                   (SIOMM_APOINT_READ_CALC_SET_BOUNDARY * nPoint), 
                   pfValue);
}


LONG O22SnapIoMemMap::GetStatusPUC(long *pnPUCFlag)
//-------------------------------------------------------------------------------------------------
// Get the Powerup Clear flag
//-------------------------------------------------------------------------------------------------
{
  return ReadQuad(SIOMM_STATUS_READ_PUC_FLAG, (DWORD*)pnPUCFlag);
}


LONG O22SnapIoMemMap::GetStatusLastError(long *pnErrorCode)
//-------------------------------------------------------------------------------------------------
// Get the last error
//-------------------------------------------------------------------------------------------------
{
  return ReadQuad(SIOMM_STATUS_READ_LAST_ERROR, (DWORD*)pnErrorCode);
}


LONG O22SnapIoMemMap::GetStatusBootpAlways(long *pnBootpAlways)
//-------------------------------------------------------------------------------------------------
// Get the "bootp always" flag
//-------------------------------------------------------------------------------------------------
{
  return ReadQuad(SIOMM_STATUS_READ_BOOTP_FLAG, (DWORD*)pnBootpAlways);
}


LONG O22SnapIoMemMap::GetStatusDegrees(long *pnDegrees)
//-------------------------------------------------------------------------------------------------
// Get the temperature unit.
//-------------------------------------------------------------------------------------------------
{
  return ReadQuad(SIOMM_STATUS_READ_DEGREES_FLAG, (DWORD*)pnDegrees);
}


LONG O22SnapIoMemMap::GetStatusVersionEx(SIOMM_StatusVersion *pVersionData)
//-------------------------------------------------------------------------------------------------
// Get version information
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[32]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_STATUS_READ_BASE, 32, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pVersionData->nMapVer        = O22MAKELONG(arrbyData[0],  arrbyData[1],
                                               arrbyData[2],  arrbyData[3]);
    pVersionData->nLoaderVersion = O22MAKELONG(arrbyData[24],  arrbyData[25],
                                               arrbyData[26],  arrbyData[27]);
    pVersionData->nKernelVersion = O22MAKELONG(arrbyData[28],  arrbyData[29],
                                               arrbyData[30],  arrbyData[31]);
  }

  return nResult;
}


LONG O22SnapIoMemMap::GetStatusHardwareEx(SIOMM_StatusHardware *pHardwareData)
//-------------------------------------------------------------------------------------------------
// Get hardware information
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[44]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_STATUS_READ_BASE, 44, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pHardwareData->nIoUnitType   = O22MAKELONG(arrbyData[32], arrbyData[33],
                                               arrbyData[34], arrbyData[35]);
    pHardwareData->byHwdVerMonth = arrbyData[36];
    pHardwareData->byHwdVerDay   = arrbyData[37];
    pHardwareData->wHwdVerYear   = O22MAKEWORD(arrbyData[38], arrbyData[39]);
    pHardwareData->nRamSize      = O22MAKELONG(arrbyData[40], arrbyData[41],
                                               arrbyData[42], arrbyData[43]);
  }

  return nResult;
}


LONG O22SnapIoMemMap::GetStatusNetworkEx(SIOMM_StatusNetwork *pNetworkData)
//-------------------------------------------------------------------------------------------------
// Get network information
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[64]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_STATUS_READ_BASE, 64, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pNetworkData->wMACAddress0  = O22MAKEWORD(arrbyData[46], arrbyData[47]);
    pNetworkData->wMACAddress1  = O22MAKEWORD(arrbyData[48], arrbyData[49]);
    pNetworkData->wMACAddress2  = O22MAKEWORD(arrbyData[50], arrbyData[51]);
    pNetworkData->nTCPIPAddress = O22MAKELONG(arrbyData[52], arrbyData[53],
                                              arrbyData[54], arrbyData[55]);
    pNetworkData->nSubnetMask   = O22MAKELONG(arrbyData[56], arrbyData[57],
                                              arrbyData[58], arrbyData[59]);
    pNetworkData->nDefGateway   = O22MAKELONG(arrbyData[60], arrbyData[61],
                                              arrbyData[62], arrbyData[63]);
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetStatusOperation(long nOpCode)
//-------------------------------------------------------------------------------------------------
// Issue an operation code
//-------------------------------------------------------------------------------------------------
{
  return WriteQuad(SIOMM_STATUS_WRITE_OPERATION, nOpCode);
}


LONG O22SnapIoMemMap::SetStatusBootpRequest(long nFlag)
//-------------------------------------------------------------------------------------------------
// Set the "bootp always" flag
//-------------------------------------------------------------------------------------------------
{
  return WriteQuad(SIOMM_STATUS_WRITE_BOOTP, nFlag);
}


LONG O22SnapIoMemMap::SetStatusDegrees(long nDegFlag)
//-------------------------------------------------------------------------------------------------
// Set the temperature unit
//-------------------------------------------------------------------------------------------------
{
  return WriteQuad(SIOMM_STATUS_WRITE_TEMP_DEGREES, nDegFlag);
}


LONG O22SnapIoMemMap::SetStatusWatchdogTime(long nTimeMS)
//-------------------------------------------------------------------------------------------------
// Set the watchdog time
//-------------------------------------------------------------------------------------------------
{
  return WriteQuad(SIOMM_STATUS_WRITE_WATCHDOG_TIME, nTimeMS);
}


LONG O22SnapIoMemMap::GetAnaBank(DWORD dwDestOffset, SIOMM_AnaBank * pBankData)
//-------------------------------------------------------------------------------------------------
// Get an analog bank section from a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;        // for checking the return values of functions
  BYTE arrbyData[256]; // buffer for the data to be read
  DWORD dwQuadlet;     // A temp for getting the read value
  LONG  nArrayOffset;  // for stepping through the array

  // Read the data
  nResult = ReadBlock(dwDestOffset, 256, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // Unpack the data packet
    for (int i = 0 ; i < 64 ; i++)
    {
      nArrayOffset = i*4;
      dwQuadlet = O22MAKELONG(arrbyData[nArrayOffset],     arrbyData[nArrayOffset + 1],
                              arrbyData[nArrayOffset + 2], arrbyData[nArrayOffset + 3]);

      // Copy the data
      memcpy(&(pBankData->fValue[i]), &dwQuadlet, 4);
    }
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetAnaBank(DWORD dwDestOffset, SIOMM_AnaBank BankData)
//-------------------------------------------------------------------------------------------------
// Set an analog bank section to a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;        // for checking the return values of functions
  BYTE arrbyData[256]; // buffer for the data to be read
  DWORD dwQuadlet;     // A temp for getting the read value
  LONG  nArrayOffset;  // for stepping through the array

  // Pack the data packet
  for (int i = 0 ; i < 64 ; i++)
  {
    // Copy the float into a DWORD for easy manipulation
    memcpy(&dwQuadlet, &(BankData.fValue[i]), 4);

    nArrayOffset = i*4;

    arrbyData[nArrayOffset]     = O22BYTE0(dwQuadlet);
    arrbyData[nArrayOffset + 1] = O22BYTE1(dwQuadlet);
    arrbyData[nArrayOffset + 2] = O22BYTE2(dwQuadlet);
    arrbyData[nArrayOffset + 3] = O22BYTE3(dwQuadlet);
  }

  // Read the data
  nResult = WriteBlock(dwDestOffset, 256, (BYTE*)&arrbyData);

  return nResult;
}



LONG O22SnapIoMemMap::GetAnaBankValuesEx(SIOMM_AnaBank * pBankData)
//-------------------------------------------------------------------------------------------------
// Get the analog point values in the analog bank
//-------------------------------------------------------------------------------------------------
{
  return GetAnaBank(SIOMM_ABANK_READ_POINT_VALUES, pBankData); 
}


LONG O22SnapIoMemMap::GetAnaBankCountsEx(SIOMM_AnaBank * pBankData)
//-------------------------------------------------------------------------------------------------
// Get the analog point raw counts in the analog bank
//-------------------------------------------------------------------------------------------------
{
  return GetAnaBank(SIOMM_ABANK_READ_POINT_COUNTS, pBankData); 
}


LONG O22SnapIoMemMap::GetAnaBankMinValuesEx(SIOMM_AnaBank * pBankData)
//-------------------------------------------------------------------------------------------------
// Get the analog point minimum values in the analog bank
//-------------------------------------------------------------------------------------------------
{
  return GetAnaBank(SIOMM_ABANK_READ_POINT_MIN_VALUES, pBankData); 
}

LONG O22SnapIoMemMap::GetAnaBankMaxValuesEx(SIOMM_AnaBank * pBankData)
//-------------------------------------------------------------------------------------------------
// Get the analog point maximum values in the analog bank
//-------------------------------------------------------------------------------------------------
{
  return GetAnaBank(SIOMM_ABANK_READ_POINT_MAX_VALUES, pBankData); 
}


LONG O22SnapIoMemMap::SetAnaBankValuesEx(SIOMM_AnaBank BankData)
//-------------------------------------------------------------------------------------------------
// Set the analog point values in the analog bank
//-------------------------------------------------------------------------------------------------
{
  return SetAnaBank(SIOMM_ABANK_WRITE_POINT_VALUES, BankData); 
}


LONG O22SnapIoMemMap::SetAnaBankCountsEx(SIOMM_AnaBank BankData)
//-------------------------------------------------------------------------------------------------
// Set the analog point raw counts in the analog bank
//-------------------------------------------------------------------------------------------------
{
  return SetAnaBank(SIOMM_ABANK_WRITE_POINT_COUNTS, BankData); 
}


