#include "Data.h"
#include "Utility.h"

#include <U8g2lib.h>

extern int MakeRequestP(PGM_P Req, char* Resp, const int Len);
extern bool ParseM408S1(const char* Buffer, const int BytesRead);

static const unsigned char ConnectSequence[] = {80, 83, 81, 82, 0};
static unsigned char ConnectIndex = 0;

static void ConnectingScreen();

void UpdateConnecting()
{
  ConnectIndex++;

  if (ConnectSequence[ConnectIndex] == 0)
    ConnectIndex = 0;

  gLCD.firstPage();
  do
  {
    ConnectingScreen();
  }
  while (gLCD.nextPage());

  const int BytesRead = MakeRequestP(PSTR("M408 S1"), gSerialBuffer, sizeof(gSerialBuffer));
  
  if (BytesRead)
  {
    if (ParseM408S1(gSerialBuffer, BytesRead))
    {
      gCurrentPage = PG_MAIN;
    }
    else
    {
      DEBUG_PRINT_P("Bad parse");
    }
  }
}

void ConnectingScreen()
{
  gLCD.setFont(u8g2_font_unifont_t_75);
  gLCD.drawGlyph(60, 45, 9600 + ConnectSequence[ConnectIndex]);
  gLCD.setFont(u8g2_font_5x7_tr);
  DrawStrP(40,30,PSTR("Connecting"));
}
