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
  const uint32_t* HeadPtr = (const uint32_t*)Input;
  uint32_t Header32 = xSwapBytes32(*HeadPtr);
  // Sync Byte 8
  m_SB  = (Header32 >> 24) & 0xFF;
  
  // Transport Error Indicator 1
  m_E   = (Header32 >> 23) & 0x01;
  
  // Payload Unit Start Indicator
  m_S   = (Header32 >> 22) & 0x01;
  
  // Transport Priority
  m_T   = (Header32 >> 21) & 0x01;
  
  // Packet Identifier
  m_PID = (Header32 >> 8) & 0x1FFF;
  
  // Transport Scrambling Control
  m_TSC = (Header32 >> 6) & 0x03;
  
  // Adaptation Field Control
  m_AFC = (Header32 >> 4) & 0x03;
  
  // Continuity Counter
  m_CC  = (Header32 >> 0) & 0x0F;

  return xTS::TS_HeaderLength; // Zwracamy 4 (liczba sparsowanych bajtów)
}

/// @brief Print all TS packet header fields
void xTS_PacketHeader::Print() const
{
  printf("TS: SB:%02x E:%d S:%d T:%d PID:%4d TSC:%d AF:%d CC:%2d", 
         m_SB, m_E, m_S, m_T, m_PID, m_TSC, m_AFC, m_CC);
}

// @brief Reset - reset all TS packet header fields
void xTS_AdaptationField::Reset(){
//reset
}
/**
 * @brief Parse adaptation field
 * @param PacketBuffer is pointer to buffer containing TS packet
 * @param AdaptationFieldControl is value of Adapatation Field Control field of corresponding TS packet header
 * @return Numer of parsed bytes (lengt of AF or -1 on failure)
 */
int32_t xTS_AdapationField::Parse(const uint8_t* PacketBuffer, uint8_t xTS_AdapationFieldControl){
  //parsing
}

//@brief Print all TS packet header fields
void xTS_AdaptationField::Print() const{
  //print print print
}

