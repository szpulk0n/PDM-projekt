#include "tsTransportStream.h"
#include <cstring>

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
  if (Input == nullptr) return -1;

  //i`m not empty :>>>
  uint32_t Header32 = (Input[0] << 24) | (Input[1] << 16) | (Input[2] << 8) | Input[3];
  
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

  return xTS::TS_HeaderLength; 
}

/// @brief Print all TS packet header fields
void xTS_PacketHeader::Print() const
{
  printf("TS: SB:0x%02x E:%d S:%d T:%d PID:%4d TSC:%d AF:%d CC:%2d", 
         m_SB, m_E, m_S, m_T, m_PID, m_TSC, m_AFC, m_CC);
}

// @brief Reset - reset all TS packet header fields
void xTS_AdaptationField::Reset(){
  m_AdaptationFieldLength = 0;
  m_DC = m_RA = m_SP = m_PR = m_OR = m_SF = m_TP = m_EX = 0;
}
/**
 * @brief Parse adaptation field
 * @param PacketBuffer is pointer to buffer containing TS packet
 * @param AdaptationFieldControl is value of Adapatation Field Control field of corresponding TS packet header
 * @return Numer of parsed bytes (lengt of AF or -1 on failure)
 */
int32_t xTS_AdaptationField::Parse(const uint8_t* PacketBuffer, uint8_t AdaptationFieldControl){
  if (AdaptationFieldControl != 2 && AdaptationFieldControl != 3) return 0;

  m_AdaptationFieldLength = PacketBuffer[4];
  
  if (m_AdaptationFieldLength > 0)
  {
    uint8_t flags = PacketBuffer[5];
    m_DC = (flags >> 7) & 0x01;
    m_RA = (flags >> 6) & 0x01;
    m_SP = (flags >> 5) & 0x01;
    m_PR = (flags >> 4) & 0x01;
    m_OR = (flags >> 3) & 0x01;
    m_SF = (flags >> 2) & 0x01;
    m_TP = (flags >> 1) & 0x01;
    m_EX = (flags >> 0) & 0x01;
  }

  return 1 + m_AdaptationFieldLength;
}

//@brief Print all TS packet header fields
void xTS_AdaptationField::Print() const{
  //print print print
  printf(" AF: L=%3d DC=%d RA=%d SP=%d PR=%d OR=%d SF=%d TP=%d EX=%d", 
         m_AdaptationFieldLength, m_DC, m_RA, m_SP, m_PR, m_OR, m_SF, m_TP, m_EX);
}

// ===========================================================================
// xPES_PacketHeader
// ===========================================================================

void xPES_PacketHeader::Reset()
{
    m_PacketStartCodePrefix = 0;
    m_StreamId = 0;
    m_PacketLength = 0;
}

int32_t xPES_PacketHeader::Parse(const uint8_t* Input)
{
    m_PacketStartCodePrefix = (Input[0] << 16) | (Input[1] << 8) | Input[2];
    m_StreamId = Input[3];
    m_PacketLength = (Input[4] << 8) | Input[5];
    return 6; 
}

void xPES_PacketHeader::Print() const
{
    printf(" PES: PSCP=%d SID=%d L=%d", m_PacketStartCodePrefix, m_StreamId, m_PacketLength);
}

// ===========================================================================
// xPES_Assembler
// ===========================================================================

xPES_Assembler::xPES_Assembler() 
    : m_PID(-1), m_Buffer(nullptr), m_BufferSize(0), m_DataOffset(0), 
      m_LastContinuityCounter(-1), m_Started(false), m_LastPacketSize(0)
{
}

xPES_Assembler::~xPES_Assembler()
{
    if(m_Buffer) { delete[] m_Buffer; }
}

void xPES_Assembler::Init(int32_t PID)
{
    m_PID = PID;
    xBufferReset();
}

void xPES_Assembler::xBufferReset()
{
    m_DataOffset = 0;
    m_Started = false;
    m_LastContinuityCounter = -1;
}

void xPES_Assembler::xBufferAppend(const uint8_t* Data, int32_t Size)
{
    if (m_DataOffset + Size > m_BufferSize)
    {
        uint32_t NewSize = m_BufferSize + Size + 65536; 
        uint8_t* NewBuffer = new uint8_t[NewSize];
        if (m_Buffer)
        {
            memcpy(NewBuffer, m_Buffer, m_DataOffset);
            delete[] m_Buffer;
        }
        m_Buffer = NewBuffer;
        m_BufferSize = NewSize;
    }
    memcpy(m_Buffer + m_DataOffset, Data, Size);
    m_DataOffset += Size;
}

xPES_Assembler::eResult xPES_Assembler::AbsorbPacket(const uint8_t* TransportStreamPacket, const xTS_PacketHeader* PacketHeader, const xTS_AdaptationField* AdaptationField)
{
    if (PacketHeader->getPID() != m_PID) return eResult::UnexpectedPID;

    if (PacketHeader->getPayloadUnitStartIndicator())
    {
        uint32_t finishedSize = m_DataOffset;
        bool wasStarted = m_Started;

        uint32_t offset = 4;
        if (PacketHeader->hasAdaptationField()) {
            offset += (1 + AdaptationField->getAdaptationFieldLength());
        }
        
        uint32_t payloadSize = 188 - offset;

        xBufferReset();
        m_PESH.Parse(TransportStreamPacket + offset);
        xBufferAppend(TransportStreamPacket + offset, payloadSize);
        
        m_Started = true;
        m_LastContinuityCounter = PacketHeader->getContinuityCounter();

        if (wasStarted) {
            m_LastPacketSize = finishedSize;
            return eResult::AssemblingFinished;
        }
        return eResult::AssemblingStarted;
    }

    if (m_Started)
    {
        if (((m_LastContinuityCounter + 1) & 0x0F) != PacketHeader->getContinuityCounter())
        {
            xBufferReset();
            return eResult::StreamPackedLost;
        }

        uint32_t offset = 4;
        if (PacketHeader->hasAdaptationField()) {
            offset += (1 + AdaptationField->getAdaptationFieldLength());
        }
        
        uint32_t payloadSize = 188 - offset;
        if (payloadSize > 0) {
            xBufferAppend(TransportStreamPacket + offset, payloadSize);
        }

        m_LastContinuityCounter = PacketHeader->getContinuityCounter();
        return eResult::AssemblingContinue;
    }

    return eResult::UnexpectedPID;
}