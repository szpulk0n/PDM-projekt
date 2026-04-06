#include "tsCommon.h"
#include "tsTransportStream.h"
#include <cstdio>

int main(int argc, char *argv[ ], char *envp[ ])
{
  // 1. Otwarcie pliku (na razie nazwa "example_signal.ts")
  FILE* pFile = fopen("example_new.ts", "rb");
  if (!pFile)
  {
    printf("Blad: Nie mozna otworzyc pliku example_signal.ts\n");
    return EXIT_FAILURE;
  }

  xTS_PacketHeader TS_PacketHeader;
  uint8_t          TS_PacketBuffer[xTS::TS_PacketLength];
  int32_t          TS_PacketId = 0;

  // 2. Pętla odczytu - czytaj po 188 bajtów
  while (fread(TS_PacketBuffer, 1, xTS::TS_PacketLength, pFile) == xTS::TS_PacketLength)
  {
    TS_PacketHeader.Reset();
    
    // Na razie przekazujemy bufor, ale Parse() jeszcze nic nie robi
    TS_PacketHeader.Parse(TS_PacketBuffer);

    printf("%010d ", TS_PacketId);
    TS_PacketHeader.Print(); // Obecnie pusta implementacja
    printf("\n");
    if (TS_PacketHeader.getSyncByte() != 0x47) {
    printf("BŁĄD SYNCHRONIZACJI w pakiecie %d!\n", TS_PacketId);
}
    

    TS_PacketId++;
  }
  
  // 3. Zamknięcie pliku
  fclose(pFile);

  return EXIT_SUCCESS;
}