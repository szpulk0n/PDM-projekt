#include "tsTransportStream.h"

//=============================================================================================================================================================================
// xTS_PacketHeader
//=============================================================================================================================================================================


/// @brief Reset - reset all TS packet header fields
void xTS_PacketHeader::Reset()
{
  m_SB  = 0;
  m_E   = 0;
  m_S   = 0;
  m_T   = 0;
  m_PID = 0;
  m_TSC = 0;
  m_AFC = 0;
  m_CC  = 0;  
}

/**
  @brief Parse all TS packet header fields
  @param Input is pointer to buffer containing TS packet
  @return Number of parsed bytes (4 on success, -1 on failure) 
 */
int32_t xTS_PacketHeader::Parse(const uint8_t* Input)
{
  if (Input == nullptr) return NOT_VALID;

  //i`m not empty :>>>
  // 1. Odczytujemy 4 bajty i zamieniamy kolejność (Big-Endian -> Little-Endian)
  // Używamy castowania na uint32_t*, aby pobrać cały nagłówek naraz
  uint32_t Header32 = xSwapBytes32(*(const uint32_t*)Input);

  // 2. Wyodrębniamy pola za pomocą operacji bitowych zgodnie ze specyfikacją
  // b31-b24: Sync Byte (8 bitów)
  m_SB  = (Header32 >> 24) & 0xFF;
  
  // b23: Transport Error Indicator (1 bit)
  m_E   = (Header32 >> 23) & 0x01;
  
  // b22: Payload Unit Start Indicator (1 bit)
  m_S   = (Header32 >> 22) & 0x01;
  
  // b21: Transport Priority (1 bit)
  m_T   = (Header32 >> 21) & 0x01;
  
  // b20-b8: Packet Identifier (PID) (13 bitów)
  m_PID = (Header32 >> 8) & 0x1FFF;
  
  // b7-b6: Transport Scrambling Control (2 bity)
  m_TSC = (Header32 >> 6) & 0x03;
  
  // b5-b4: Adaptation Field Control (2 bity)
  m_AFC = (Header32 >> 4) & 0x03;
  
  // b3-b0: Continuity Counter (4 bity)
  m_CC  = (Header32 >> 0) & 0x0F;

  return xTS::TS_HeaderLength; // Zwracamy 4 (liczba sparsowanych bajtów)
}

/// @brief Print all TS packet header fields
void xTS_PacketHeader::Print() const
{
  
  // Wyświetlamy wszystkie pola nagłówka w jednej linii
  // %02x dla Sync Byte (hex), %4d dla PID (dziesiętnie)
  printf("TS: SB:%02x E:%d S:%d T:%d PID:%4d TSC:%d AF:%d CC:%2d", 
         m_SB, m_E, m_S, m_T, m_PID, m_TSC, m_AFC, m_CC);
}

//=============================================================================================================================================================================
