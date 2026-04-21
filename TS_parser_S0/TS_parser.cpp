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

  xTS_PacketHeader TS_PacketHeader;
  uint8_t          TS_PacketBuffer[xTS::TS_PacketLength];
  int32_t          TS_PacketId = 0;


  while (fread(TS_PacketBuffer, 1, xTS::TS_PacketLength, pFile) == xTS::TS_PacketLength)
  {
    TS_PacketHeader.Reset();
    //Done read from file    
    TS_PacketHeader.Parse(TS_PacketBuffer);

    printf("%010d ", TS_PacketId);
    TS_PacketHeader.Print(); // Obecnie pusta implementacja
    printf("\n");
    if (TS_PacketHeader.getSyncByte() != 0x47) {
    printf("BŁĄD SYNCHRONIZACJI %d!\n", TS_PacketId);
}
    
    TS_PacketId++;
  }
  //Done close file
  fclose(pFile);

  return EXIT_SUCCESS;
}