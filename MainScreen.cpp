#include "Data.h"
#include "Utility.h"

extern int MakeRequest(PGM_P Req, char* Resp, const int Len);
extern bool ParseM408S1(const char* Buffer, const int BytesRead);


static void MainScreen();
static PGM_P DecodeStatus(const char Code);
static void BedHeater(const int16_t x, const int16_t y, const bool On);
static void HotendHeater(const int16_t x, const int16_t y, const char Num, const bool On);
static void Heater(const int16_t x, const int16_t y, const char* current, const char* active, const char* standby);

void UpdateMain()
{
  const int BytesRead = MakeRequest(PSTR("M408 S0"), SerialBuffer, sizeof(SerialBuffer));
  bool Redraw = false;
  
  if (BytesRead)
  {
    if (ParseM408S1(SerialBuffer, BytesRead))
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
    gData.CurrentPage = PG_CONNECTING;
#endif
    Redraw = true;
  }
  
  if (gData.Flags & FLAGS_ENC_SW)
  {
    gData.Flags &= ~FLAGS_ENC_SW;
    gData.CurrentPage = PG_MENU1;
    Enc.write(0);
  }

  if (Redraw)
  {
    u8g2.firstPage();
    do
    {
      MainScreen();
    }
    while (u8g2.nextPage());
  }
}

void MainScreen()
{
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, 7, gData.PrinterName);

  PGM_P DecodedStatus = DecodeStatus(gData.P.MS.StatusStr);
  DrawStrP(128 - StrWidthP(DecodedStatus), 7, DecodedStatus);
  
  u8g2.drawLine(0, 8, 128, 8);

  BedHeater(12, 10, gData.P.MS.Heaters[0].Status == 2);// PanelDue only checks for 2

  DrawStrP(0, 30, PSTR("C"));
  DrawStrP(0, 37, PSTR("A"));
  DrawStrP(0, 44, PSTR("S"));

  for (unsigned char i = 0; i < gData.P.MS.HeaterCount; i++)
    Heater(7 + 30 * i, 30, gData.P.MS.Heaters[i].Current, gData.P.MS.Heaters[i].Active, gData.P.MS.Heaters[i].Standby);

  for (int i = 0; i < gData.NumTools; i++)
    HotendHeater(40 + i * 28, 10, '1' + i, gData.P.MS.Tool == i);

  if (gData.P.MS.FractionPrinted != NULL)
  {
    u8g2.setCursor(0,51);
    
    if (gData.P.MS.FractionPrinted[0] != '0')
      u8g2.print(gData.P.MS.FractionPrinted[0]);

    if (gData.P.MS.FractionPrinted[2] != '0')
      u8g2.print(gData.P.MS.FractionPrinted[2]);

    if (isdigit(gData.P.MS.FractionPrinted[3]))
      u8g2.print(gData.P.MS.FractionPrinted[3]);
    else
      u8g2.print('0');
      
      
    u8g2.print('%');
  }

  u8g2.drawLine(0, 56, 128, 56);

  //pos
  DrawStrP(0, 64, PSTR("X"));
  PrintFloat(gData.P.MS.Pos.X, 2, true);

  DrawStrP(45, 64, PSTR("Y"));
  PrintFloat(gData.P.MS.Pos.Y, 2, true);

  DrawStrP(90, 64, PSTR("Z"));
  PrintFloat(gData.P.MS.Pos.Z, 2, true);
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
    u8g2.setFont(u8g2_font_unifont_t_76);
    u8g2.drawGlyph(2 + x, 11 + y, 9832);

    u8g2.setFont(u8g2_font_5x7_tr);
  }

  u8g2.drawBox(0 + x, 11 + y, 12, 2);
}

void HotendHeater(const int16_t x, const int16_t y, const char Num, const bool On)
{
  //9929
  if (On)
  {
    u8g2.setFont(u8g2_font_unifont_t_77);
    u8g2.drawGlyph(0 + x, 13 + y, 9930);

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setDrawColor(0);
    u8g2.setCursor(5 + x, 8 + y);
    u8g2.print(Num);
    //u8g2.drawStr(5 + x, 8 + y, Num);

    u8g2.setDrawColor(1);
  }
  else
  {
    u8g2.setFont(u8g2_font_unifont_t_77);
    u8g2.drawGlyph(0 + x, 13 + y, 9929);

    u8g2.setFont(u8g2_font_5x7_tr);
    //u8g2.drawStr(5 + x, 8 + y, Num);
    u8g2.setCursor(5 + x, 8 + y);
    u8g2.print(Num);
  }
}

void Heater(const int16_t x, const int16_t y, const char* current, const char* active, const char* standby)
{
  u8g2.setFont(u8g2_font_5x7_tr);

  u8g2.setCursor(x, y);
  PrintFloat(current, 1, false);
  u8g2.setCursor(x, y + 7);
  PrintFloat(active, 1, false);
  u8g2.setCursor(x, y + 14);
  PrintFloat(standby, 1, false);
}