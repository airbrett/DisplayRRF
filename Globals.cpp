#include "Globals.h"

U8G2_ST7920_128X64_1_HW_SPI gLCD(U8G2_R0, U8X8_PIN_NONE);//Mini
Encoder gEnc1(ENC_A_PIN, ENC_B_PIN);
char gSerialBuffer[640];
char gStatusStr;
unsigned char gFlags;
unsigned char gCurrentPage;
char gPrinterName[32];
unsigned char gNumTools;
Data gData;
