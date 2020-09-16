#include "Data.h"
#include "Utility.h"

#include <U8g2lib.h>

extern int MakeRequest(PGM_P Req, char* Resp, const int Len);
extern bool ParseM408S1(const char* Buffer, const int BytesRead);

static const unsigned char ConnectSequence[] = {80, 83, 81, 82, 0};
static unsigned char ConnectIndex = 0;

static void ConnectingScreen();

void UpdateConnecting()
{
  ConnectIndex++;

  if (ConnectSequence[ConnectIndex] == 0)
    ConnectIndex = 0;

  u8g2.firstPage();
  do
  {
    ConnectingScreen();
  }
  while (u8g2.nextPage());

  const int BytesRead = MakeRequest(PSTR("M408 S1"), SerialBuffer, sizeof(SerialBuffer));
  
  if (BytesRead)
  {
    if (ParseM408S1(SerialBuffer, BytesRead))
    {
      gData.CurrentPage = PG_MAIN;
    }
    else
    {
      DEBUG_PRINT_P("Bad parse");
    }
  }
}

void ConnectingScreen()
{
  u8g2.setFont(u8g2_font_unifont_t_75);
  u8g2.drawGlyph(60, 45, 9600 + ConnectSequence[ConnectIndex]);
  u8g2.setFont(u8g2_font_5x7_tr);
  DrawStrP(40,30,PSTR("Connecting"));
}