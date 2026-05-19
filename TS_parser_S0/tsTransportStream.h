#pragma once
#include "tsCommon.h"
#include <string>
#include <cstdint>

/*
MPEG-TS packet:
`        3                   2                   1                   0  `
`      1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0  `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
`   0 |                             Header                            | `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
`   4 |                  Adaptation field + Payload                   | `
`     |                                                               | `
` 184 |                                                               | `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `


MPEG-TS packet header:
`        3                   2                   1                   0  `
`      1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0  `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
`   0 |       SB      |E|S|T|           PID           |TSC|AFC|   CC  | `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
*/

//=============================================================================================================================================================================

// Główne stałe z dokumentacji MPEG-TS
class xTS
{
public:
  static constexpr uint32_t TS_PacketLength  = 188; // Podstawa - rozmiar jednego pakietu TS
  static constexpr uint32_t TS_HeaderLength  = 4;

  static constexpr uint32_t PES_HeaderLength = 6;

  static constexpr uint32_t BaseClockFrequency_Hz         =    90000; //Hz
  static constexpr uint32_t ExtendedClockFrequency_Hz     = 27000000; //Hz
  static constexpr uint32_t BaseClockFrequency_kHz        =       90; //kHz
  static constexpr uint32_t ExtendedClockFrequency_kHz    =    27000; //kHz
  static constexpr uint32_t BaseToExtendedClockMultiplier =      300;
};

//=============================================================================================================================================================================

// Dekoder nagłówka TS (pierwsze 4 bajty z każdych 188)
class xTS_PacketHeader
{
public:
  // Predefiniowane PID-y tablic systemowych. 
  // Strumienie audio/wideo dostają dynamiczne PID-y (u nas audio ma 136).
  enum class ePID : uint16_t
  {
    PAT  = 0x0000,
    CAT  = 0x0001,
    TSDT = 0x0002,
    IPMT = 0x0003,
    NIT  = 0x0010, 
    SDT  = 0x0011, 
    NuLL = 0x1FFF,
  };

protected:
  // Poszczególne pola nagłówka wyciągane bitowo (maskami) z pierwszych 4 bajtów
  uint8_t  m_SB;  // Sync Byte (zawsze 0x47)
  uint8_t  m_E;   // Transport Error Indicator
  uint8_t  m_S;   // PUSI - jeśli 1, to w tym pakiecie zaczyna się nowa ramka PES
  uint8_t  m_T;   // Transport Priority
  uint16_t m_PID; // Packet Identifier - po tym filtrujemy to, co nas interesuje (np. 136)
  uint8_t  m_TSC; // Transport Scrambling Control
  uint8_t  m_AFC; // Adaptation Field Control - określa co jest dalej w pakiecie
  uint8_t  m_CC;  // Continuity Counter (0-15) - sprawdzamy czy nie zgubiliśmy pakietu

public:
  void     Reset();
  int32_t  Parse(const uint8_t* Input);
  void     Print() const;

public:
  // Gettery
  uint8_t  getSyncByte() const { return m_SB; }
  uint8_t  getTransportErrorIndicator() const { return m_E; }
  uint8_t  getPayloadUnitStartIndicator() const { return m_S; }
  uint8_t  getTransportPriority() const { return m_T; }
  uint16_t getPID() const { return m_PID; }
  uint8_t  getTransportScramblingControl() const { return m_TSC; }
  uint8_t  getAdaptationFieldControl() const { return m_AFC; }
  uint8_t  getContinuityCounter() const { return m_CC; }

public:
  // Helpery do wygodnego sprawdzania stanu AFC
  bool     hasAdaptationField() const { return (m_AFC == 2 || m_AFC == 3); }
  bool     hasPayload()         const { return (m_AFC == 1 || m_AFC == 3); }
};

//=============================================================================================================================================================================

// Opcjonalne pole adaptacyjne. Służy głownie do upychania (stuffing), gdy pakiet ma mniej 
// użytecznych danych niż wynosi stała wielkość 188B.
class xTS_AdaptationField{
  protected:
    uint8_t m_AdaptationFieldControl;

    // Długość AF - potrzebujemy tego żeby wiedzieć, o ile bajtów przeskoczyć w przód do danych użytecznych
    uint8_t m_AdaptationFieldLength;
    
    // Pojedyncze flagi (odczytywane przsunieciami bitowymi)
    uint8_t m_DC, m_RA, m_SP, m_PR, m_OR, m_SF, m_TP, m_EX;

  public:
    void Reset();
    int32_t Parse(const uint8_t* PacketBuffer, uint8_t AdaptationFieldControl);
    void Print() const;
  public:
  uint8_t getAdaptationFieldLength () const {
    return m_AdaptationFieldLength;
  }
};

// ===========================================================================
// xPES_PacketHeader
// ===========================================================================

// Nagłówek paczek PES. W przeciwieństwie do TS, ten nagłówek ma zmienną długość.
class xPES_PacketHeader
{
protected:
    uint32_t m_PacketStartCodePrefix; // Zawsze 0x000001
    uint8_t  m_StreamId;              // Typ strumienia (np. audio)
    uint16_t m_PacketLength;          // Deklarowany rozmiar danych (0 dla wideo)
    uint8_t  m_PES_header_data_length; // Długość opcjonalnej sekcji nagłówka PES
    uint32_t m_TotalHeaderLength;      // Całkowita długość, żeby wiedzieć od którego bajtu zacząć czytać właściwe audio/wideo

public:
    void     Reset();
    int32_t  Parse(const uint8_t* Input);
    void     Print() const;

public:
    uint32_t getPacketStartCodePrefix() const { return m_PacketStartCodePrefix; }
    uint8_t  getStreamId()              const { return m_StreamId; }
    uint16_t getPacketLength()          const { return m_PacketLength; }
    uint32_t getTotalHeaderLength()     const { return m_TotalHeaderLength; }
};

// ===========================================================================
// xPES_Assembler
// ===========================================================================

// Skleja payloady wyciągnięte z małych pakietów TS (188B) w całe, ciągłe ramki PES.
class xPES_Assembler
{
public:
    // Status zwracany po każdym przerobionym pakiecie TS. 
    // Mówi głównej pętli w main() na jakim etapie klejenia ramki jesteśmy.
    enum class eResult : int32_t {
        UnexpectedPID      = -1, // Pakiet odrzucony (inny PID niż chcemy)
        StreamPackedLost   = -2, // Błąd licznika CC (zgubiliśmy dane po drodze)
        AssemblingStarted  =  1, // Znaleziono start nowej ramki
        AssemblingContinue =  2, // Środek ramki - dopisano dane do bufora
        AssemblingFinished =  3, // Koniec ramki - można zapisywać plik na dysk
    };

protected:
    int32_t m_PID;                // PID, który aktualnie wyodrębniamy
    uint8_t* m_Buffer;            // Pamięć, gdzie doklejane są po kolei bajty danych
    uint32_t m_BufferSize;        
    uint32_t m_DataOffset;        // Ile bajtów już jest w buforze
    int8_t   m_LastContinuityCounter; 
    bool     m_Started;           // Flaga, żeby nie zacząć zbierać danych od środka ramki
    xPES_PacketHeader m_PESH;     
    
    // Zmienne do obsługi problemu z nadpisywaniem bufora (zabezpieczenie przed trzaskami w audio)
    uint32_t m_LastPacketSize;
    uint32_t m_LastHeaderLen;

public:
    xPES_Assembler();
    ~xPES_Assembler();

    void    Init(int32_t PID);
    
    // Główna funkcja sklejająca. Dokleja zawartość pakietu do m_Buffer.
    eResult AbsorbPacket(const uint8_t* TransportStreamPacket, const xTS_PacketHeader* PacketHeader, const xTS_AdaptationField* AdaptationField);

    void    PrintPESH() const { m_PESH.Print(); }
    uint8_t* getPacket()      { return m_Buffer; }
    int32_t getNumPacketBytes() const { return m_DataOffset; }
    
    // Bezpieczny odczyt informacji o poprzednim pakiecie dla pętli głównej
    uint32_t getLastPacketSize() const { return m_LastPacketSize; }
    uint32_t getLastHeaderLen()  const { return m_LastHeaderLen; }

protected:
    void xBufferReset();
    void xBufferAppend(const uint8_t* Data, int32_t Size);
};
