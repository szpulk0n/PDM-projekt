#include "tsCommon.h"
#include "tsTransportStream.h"
#include <cstdio>

int main(int argc, char *argv[ ], char *envp[ ])
{
  FILE* pFile = fopen("example_new.ts", "rb");
  if (!pFile)
  {
    printf("Nie mozna otworzyc pliku\n");
    return EXIT_FAILURE;
  }

  // Otwarcie pliku wyjściowego dla fonii (Krok 4a)
  FILE* pFileOut = fopen("PID136.mp2", "wb"); // plik otwarty w trybie binarnym 

  xTS_PacketHeader    TS_PacketHeader;
  xTS_AdaptationField TS_PacketAdaptationField; 
  uint8_t             TS_PacketBuffer[xTS::TS_PacketLength];
  int32_t             TS_PacketId = 0;

  xPES_Assembler PES_Assembler;
  PES_Assembler.Init(136); 

  while (fread(TS_PacketBuffer, 1, xTS::TS_PacketLength, pFile) == xTS::TS_PacketLength)
  {
    TS_PacketHeader.Reset();
    TS_PacketAdaptationField.Reset(); 
    
    //Done read from file    
    TS_PacketHeader.Parse(TS_PacketBuffer);

    if (TS_PacketHeader.getPID() == 136)
    {
      if (TS_PacketHeader.hasAdaptationField())
      {
        TS_PacketAdaptationField.Parse(TS_PacketBuffer, TS_PacketHeader.getAdaptationFieldControl());
      }

      printf("%010d ", TS_PacketId);
      TS_PacketHeader.Print();
      if (TS_PacketHeader.hasAdaptationField()) { TS_PacketAdaptationField.Print(); }

      xPES_Assembler::eResult Result = PES_Assembler.AbsorbPacket(TS_PacketBuffer, &TS_PacketHeader, &TS_PacketAdaptationField);

      switch (Result)
      {
        case xPES_Assembler::eResult::StreamPackedLost : printf(" PcktLost"); break;
        case xPES_Assembler::eResult::AssemblingStarted: printf(" Started"); PES_Assembler.PrintPESH(); break;
        case xPES_Assembler::eResult::AssemblingContinue: printf(" Continue"); break;
        case xPES_Assembler::eResult::AssemblingFinished: 
{
    // Pobieramy parametry ukończonego właśnie pakietu
        uint32_t headLen = PES_Assembler.getLastHeaderLen();
        uint32_t dataLen = PES_Assembler.getLastPacketSize() - headLen;

        printf(" Finished PES: Len=%d", PES_Assembler.getLastPacketSize()); 

          // Zapisujemy czyste dane ES do pliku
           if(pFileOut && dataLen > 0) {
            fwrite(PES_Assembler.getPacket() + headLen, 1, dataLen, pFileOut);
          }
        }
        break;
        default: break;
      }
      printf("\n");
    }

    if (TS_PacketHeader.getSyncByte() != 0x47) {
      printf("BŁĄD SYNCHRONIZACJI %d!\n", TS_PacketId);
    }
    
    TS_PacketId++;
  }

  // Zamknięcie plików
  if(pFileOut) fclose(pFileOut); // 
  //Done close file
  fclose(pFile);

  return EXIT_SUCCESS;
}