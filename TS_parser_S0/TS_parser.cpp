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
            printf(" Finished PES: Len=%d", PES_Assembler.getLastPacketSize()); 
            printf("\n%010d ", TS_PacketId);
            TS_PacketHeader.Print();
            if (TS_PacketHeader.hasAdaptationField()) { TS_PacketAdaptationField.Print(); }
            printf(" Started"); PES_Assembler.PrintPESH();
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
  //Done close file
  fclose(pFile);

  return EXIT_SUCCESS;
}