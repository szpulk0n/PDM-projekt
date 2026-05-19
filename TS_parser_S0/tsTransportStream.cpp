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
    m_PES_header_data_length = 0;
    m_TotalHeaderLength = 0;
}

int32_t xPES_PacketHeader::Parse(const uint8_t* Input)
{
    // Podstawowe pola (6 bajtów)
    m_PacketStartCodePrefix = (Input[0] << 16) | (Input[1] << 8) | Input[2];
    m_StreamId = Input[3];
    m_PacketLength = (Input[4] << 8) | Input[5];

    // Logika wyznaczania długości nagłówka (MPEG-TS-S4 slajd 5)
    // Sprawdzamy StreamId, aby wiedzieć czy występuje rozszerzenie (3 bajty + PES_header_data_length)
    if (m_StreamId != 0xBC && m_StreamId != 0xBE && m_StreamId != 0xBF &&
        m_StreamId != 0xF0 && m_StreamId != 0xF1 && m_StreamId != 0xFF &&
        m_StreamId != 0xF2 && m_StreamId != 0xF8)
    {
        // PES_header_data_length znajduje się na 9-tym bajcie (indeks 8)
        m_PES_header_data_length = Input[8];
        // Całkowita długość nagłówka: 6B (podstawa) + 3B (flagi i bajt długości) + dane dodatkowe
        m_TotalHeaderLength = 6 + 3 + m_PES_header_data_length;
    }
    else
    {
        m_PES_header_data_length = 0;
        m_TotalHeaderLength = 6;
    }

    return m_TotalHeaderLength;
}

void xPES_PacketHeader::Print() const
{
    // Obliczenia zgodnie ze slajdem 
    uint32_t PcktLen = m_PacketLength + 6;
    uint32_t DataLen = m_PacketLength - (m_TotalHeaderLength - 6);

    printf(" PES: PcktLen=%d HeadLen=%d DataLen=%d", PcktLen, m_TotalHeaderLength, DataLen);
}

// ===========================================================================
// xPES_Assembler
// ===========================================================================

xPES_Assembler::xPES_Assembler() 
    : m_PID(-1), m_Buffer(nullptr), m_BufferSize(0), m_DataOffset(0), 
      m_LastContinuityCounter(-1), m_Started(false), m_LastPacketSize(0), m_LastHeaderLen(0)
{}

xPES_Assembler::~xPES_Assembler() { if(m_Buffer) delete[] m_Buffer; }
void xPES_Assembler::Init(int32_t PID) { m_PID = PID; xBufferReset(); }
void xPES_Assembler::xBufferReset() { m_DataOffset = 0; m_Started = false; m_LastContinuityCounter = -1; }

void xPES_Assembler::xBufferAppend(const uint8_t* Data, int32_t Size) {
    if (m_DataOffset + Size > m_BufferSize) {
        uint32_t NewSize = m_BufferSize + Size + 65536; 
        uint8_t* NewBuffer = new uint8_t[NewSize];
        if (m_Buffer) { memcpy(NewBuffer, m_Buffer, m_DataOffset); delete[] m_Buffer; }
        m_Buffer = NewBuffer; m_BufferSize = NewSize;
    }
    memcpy(m_Buffer + m_DataOffset, Data, Size);
    m_DataOffset += Size;
}

xPES_Assembler::eResult xPES_Assembler::AbsorbPacket(const uint8_t* TS_Packet, const xTS_PacketHeader* PH, const xTS_AdaptationField* AF) {
    if (PH->getPID() != m_PID) return eResult::UnexpectedPID;

    // 1. Obsługa Startu (S=1)
    if (PH->getPayloadUnitStartIndicator()) {
        // Jeśli już coś składaliśmy, a nie skończyliśmy naturalnie (np. wideo L=0),
        // to teraz musimy wymusić koniec przed nadpisaniem bufora.
        if (m_Started) {
            m_LastPacketSize = m_DataOffset;
            m_LastHeaderLen = m_PESH.getTotalHeaderLength();
            // Nie resetujemy jeszcze - zwracamy Finished, a ten pakiet S=1 
            // przetworzymy w następnym wywołaniu 
        }

        uint32_t offset = 4 + (PH->hasAdaptationField() ? 1 + AF->getAdaptationFieldLength() : 0);
        xBufferReset();
        m_PESH.Parse(TS_Packet + offset);
        xBufferAppend(TS_Packet + offset, 188 - offset);
        m_Started = true;
        m_LastContinuityCounter = PH->getContinuityCounter();
        return eResult::AssemblingStarted;
    }

    // 2. Obsługa kontynuacji (S=0)
    if (m_Started) {
        if (((m_LastContinuityCounter + 1) & 0x0F) != PH->getContinuityCounter()) {
            xBufferReset(); return eResult::StreamPackedLost;
        }

        uint32_t offset = 4 + (PH->hasAdaptationField() ? 1 + AF->getAdaptationFieldLength() : 0);
        xBufferAppend(TS_Packet + offset, 188 - offset);
        m_LastContinuityCounter = PH->getContinuityCounter();

        // sprawdzamy czy osiągnęliśmy koniec pakietu na podstawie długości L
        if (m_PESH.getPacketLength() != 0) {
            uint32_t expectedTotal = m_PESH.getPacketLength() + 6;
            if (m_DataOffset >= expectedTotal) {
                m_LastPacketSize = m_DataOffset;
                m_LastHeaderLen = m_PESH.getTotalHeaderLength();
                m_Started = false; // Koniec składania tego pakietu PES
                return eResult::AssemblingFinished;
            }
        }
        return eResult::AssemblingContinue;
    }
    return eResult::UnexpectedPID;
}
