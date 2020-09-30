#include "Globals.h"
#include "Utility.h"

extern int MakeRequestP(PGM_P Req, char* Resp, const int Len);
extern bool ParseM408S1(const char* Buffer, const int BytesRead);


static void MainScreen();
static PGM_P DecodeStatus(const char Code);
static void BedHeater(const int16_t x, const int16_t y, const bool On);
static void HotendHeater(const int16_t x, const int16_t y, const char Num, const bool On);
static void Heater(const int16_t x, const int16_t y, const char* current, const char* active, const char* standby);

void UpdateMain()
{
  const int BytesRead = MakeRequestP(PSTR("M408 S0"), gSerialBuffer, sizeof(gSerialBuffer));
  bool Redraw = false;
  
  if (BytesRead)
  {
    if (ParseM408S1(gSerialBuffer, BytesRead))
    {
      Redraw = true;
    }
    else
    {
      DEBUG_PRINT_P("Bad parse");
    }
  }
  else
  {
#ifndef DEBUG_NO_DATA
    gCurrentPage = PG_CONNECTING;
#endif
    Redraw = true;
  }
  
  if (gFlags & FLAGS_ENC_SW)
  {
    gFlags &= ~FLAGS_ENC_SW;
    gCurrentPage = PG_MENU1;
    gEnc1.write(0);
  }

  if (Redraw)
  {
    gLCD.firstPage();
    do
    {
      MainScreen();
    }
    while (gLCD.nextPage());
  }
}

void MainScreen()
{
  gLCD.setFont(u8g2_font_5x7_tr);
  gLCD.drawStr(0, 7, gPrinterName);

  PGM_P DecodedStatus = DecodeStatus(gStatusStr);
  DrawStrP(128 - StrWidthP(DecodedStatus), 7, DecodedStatus);
  
  gLCD.drawLine(0, 8, 128, 8);

  BedHeater(12, 10, gData.MS.Heaters[0].Status == 2);// PanelDue only checks for 2

  DrawStrP(0, 30, PSTR("C"));
  DrawStrP(0, 37, PSTR("A"));
  DrawStrP(0, 44, PSTR("S"));

  for (unsigned char i = 0; i < gData.MS.HeaterCount; i++)
    Heater(7 + 30 * i, 30, gData.MS.Heaters[i].Current, gData.MS.Heaters[i].Active, gData.MS.Heaters[i].Standby);

  for (int i = 0; i < gNumTools; i++)
    HotendHeater(40 + i * 28, 10, '1' + i, gData.MS.Tool == i);

  if (gData.MS.FractionPrinted != NULL)
  {
    gLCD.setCursor(0,51);
    
    if (gData.MS.FractionPrinted[0] != '0')
      gLCD.print(gData.MS.FractionPrinted[0]);

    if (gData.MS.FractionPrinted[2] != '0')
      gLCD.print(gData.MS.FractionPrinted[2]);

    if (isdigit(gData.MS.FractionPrinted[3]))
      gLCD.print(gData.MS.FractionPrinted[3]);
    else
      gLCD.print('0');
      
      
    gLCD.print('%');
  }

  gLCD.drawLine(0, 56, 128, 56);

  //pos
  DrawStrP(0, 64, PSTR("X"));
  PrintFloat(gData.MS.Pos.X, 2, true);

  DrawStrP(45, 64, PSTR("Y"));
  PrintFloat(gData.MS.Pos.Y, 2, true);

  DrawStrP(90, 64, PSTR("Z"));
  PrintFloat(gData.MS.Pos.Z, 2, true);
}

PGM_P DecodeStatus(const char Code)
{
  switch (Code)
  {
    case 'I':
      return PSTR("Idle");
      break;
    case 'P':
      return PSTR("Printing");
      break;
    case 'S':
      return PSTR("Stopped");
      break;
    case 'C':
      return PSTR("Booting");
      break;
    case 'A':
      return PSTR("Paused");
      break;
    case 'R':
      return PSTR("Resuming");
      break;
    case 'B':
      return PSTR("Busy");
      break;
    case 'F':
      return PSTR("Updating");
      break;
    default:
      return PSTR("Unknown");
      break;
  }
}

void BedHeater(const int16_t x, const int16_t y, const bool On)
{
  if (On)
  {
    gLCD.setFont(u8g2_font_unifont_t_76);
    gLCD.drawGlyph(2 + x, 11 + y, 9832);

    gLCD.setFont(u8g2_font_5x7_tr);
  }

  gLCD.drawBox(0 + x, 11 + y, 12, 2);
}

void HotendHeater(const int16_t x, const int16_t y, const char Num, const bool On)
{
  //9929
  if (On)
  {
    gLCD.setFont(u8g2_font_unifont_t_77);
    gLCD.drawGlyph(0 + x, 13 + y, 9930);

    gLCD.setFont(u8g2_font_5x7_tr);
    gLCD.setDrawColor(0);
    gLCD.setCursor(5 + x, 8 + y);
    gLCD.print(Num);
    //gLCD.drawStr(5 + x, 8 + y, Num);

    gLCD.setDrawColor(1);
  }
  else
  {
    gLCD.setFont(u8g2_font_unifont_t_77);
    gLCD.drawGlyph(0 + x, 13 + y, 9929);

    gLCD.setFont(u8g2_font_5x7_tr);
    //gLCD.drawStr(5 + x, 8 + y, Num);
    gLCD.setCursor(5 + x, 8 + y);
    gLCD.print(Num);
  }
}

void Heater(const int16_t x, const int16_t y, const char* current, const char* active, const char* standby)
{
  gLCD.setFont(u8g2_font_5x7_tr);

  gLCD.setCursor(x, y);
  PrintFloat(current, 1, false);
  gLCD.setCursor(x, y + 7);
  PrintFloat(active, 1, false);
  gLCD.setCursor(x, y + 14);
  PrintFloat(standby, 1, false);
}
