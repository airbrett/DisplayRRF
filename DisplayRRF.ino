//G10 To turn off -273.15
//M408 fanPercent shows all fans not just adjustable.
//M408 fanPercent shows 0.0 for thermostatic fans set to S0 but is clamped 0.5-1.0

extern "C"
{
#include "json.h"
}

#include "Data.h"
#include "Connecting.h"

#include <U8g2lib.h>
#include <Encoder.h>
#include <PinChangeInterrupt.h>

//Arduino Mini Pro
#define ENC_SW_PIN 8
#define RST_SW_PIN A0
#define ENC_A_PIN 2
#define ENC_B_PIN 3

#define POLL_RATE 1000 //ms
#define CONN_TIMEOUT 8000

U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, U8X8_PIN_NONE);//Mini
Encoder Enc(ENC_A_PIN, ENC_B_PIN);
char SerialBuffer[640];
Data gData;

char* PrintFloat(char* Str, unsigned char precision, bool padminus);
int ReadResponse();
bool ParseM408S1(const char* Buffer, const int BytesRead);
int MakeRequest(PGM_P Req, char* Resp, const int Len);
PGM_P DecodeStatus(const char Code);
void ConnectingScreen();
void MainScreen();
void DrawStrP(PGM_P Str);
void DrawStrP(const int x, const int y, PGM_P Str);
unsigned int StrWidthP(PGM_P Str);
void UpdateMain();
void DrawMenu();

void setup()
{
  pinMode(ENC_SW_PIN, INPUT_PULLUP);
  attachPCINT(digitalPinToPCINT(ENC_SW_PIN), EncSwInt, CHANGE);

  pinMode(RST_SW_PIN, INPUT_PULLUP);
  attachPCINT(digitalPinToPCINT(RST_SW_PIN), ResetSwInt, CHANGE);
  
  Serial.begin(57600);
  Serial.setTimeout(CONN_TIMEOUT);
  DEBUG_PRINT_P("hello world");
  
  u8g2.begin();
  u8g2.setFont(u8g2_font_5x7_tr);
  UpdateConnecting();
}

void EncSwInt()
{
  if (digitalRead(ENC_SW_PIN))
  {
    if (!(gData.Flags & FLAGS_ENC_SW_PREV))
      gData.Flags |= (FLAGS_ENC_SW_PREV | FLAGS_ENC_SW);
  }
  else
  {
    gData.Flags &= ~FLAGS_ENC_SW_PREV;
  }
}

void ResetSwInt()
{
  if (digitalRead(RST_SW_PIN))
  {
    if (!(gData.Flags & FLAGS_RST_SW_PREV))
      gData.Flags |= (FLAGS_RST_SW_PREV | FLAGS_RST_SW);
  }
  else
  {
    gData.Flags &= ~FLAGS_RST_SW_PREV;
  }
}

void loop()
{
  switch (gData.CurrentPage)
  {
  case PG_MAIN:
    UpdateMain();
    break;
  case PG_CONNECTING:
    UpdateConnecting();
    break;
  case PG_MENU1:
    DrawMenu();
    break;
  case PG_RUNMACRO:
    DrawMacrosMenu();
    break;
  }
}

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

void DrawMenu()
{
  char EncPos = Enc.read()/4;

  if (EncPos < 0)
  {
    EncPos = 0;
    Enc.write(0);
  }
  
  if (gData.Flags & FLAGS_ENC_SW)
  {
    gData.Flags &= ~FLAGS_ENC_SW;

    switch (EncPos)
    {
      case 0:
        gData.CurrentPage = PG_MAIN;
        break;
      case 2:
        gData.CurrentPage = PG_RUNMACRO;
        break;
    }
  }
  
  u8g2.firstPage();
  do
  {
    DrawStrP(5,7,PSTR("Back"));
    DrawStrP(5,14,PSTR("Control"));
    DrawStrP(5,21,PSTR("Run Macro"));
    DrawStrP(5,28,PSTR("Preheat"));

    DrawStrP(0,7*EncPos+7,PSTR(">"));
  }
  while (u8g2.nextPage());
}

void DrawMacrosMenu()
{
  char EncPos = Enc.read()/4;

  if (EncPos < 0)
  {
    EncPos = 0;
    Enc.write(0);
  }
  
  if (gData.Flags & FLAGS_ENC_SW)
  {
    gData.Flags &= ~FLAGS_ENC_SW;
    
    if (EncPos == 0)
      gData.CurrentPage = PG_MENU1;
  }

  PGM_P Text = PSTR("Run Macro");
  u8g2.firstPage();
  do
  {
    DrawStrP(u8g2.getDisplayWidth()/2-StrWidthP(Text)/2, 7, Text);
    DrawStrP(5,14,PSTR("Back"));

    DrawStrP(0,7*EncPos+14,PSTR(">"));
  }
  while (u8g2.nextPage());
}

int MakeRequest(PGM_P Req, char* Resp, const int Len)
{
  static int Timer = 0;
  const int Now = millis();
  uint8_t val;

  //limit the rate which messages are sent
  if (Now - Timer < POLL_RATE)
    delay(POLL_RATE - (Now - Timer));

  Timer = millis();

#ifndef DEBUG_NO_DATA
  //Clear out anything that came in between requests. I don't know if this is necessary
  //while (Serial.available())
  //  Serial.read();
  
  while (true)
  {
    val = pgm_read_byte(Req);
    
    if (!val)
      break;
      
    Serial.write(val);
    Req++;
  }
  Serial.write('\n');
  
  const size_t BytesRead = Serial.readBytesUntil('\n', Resp, Len-1);

  if (BytesRead < 1)
    return 0;//Timeout or some other mess

  Resp[BytesRead] = 0;
  return BytesRead+1;
#else
  return 0;
#endif
}

char strcmpJP(PGM_P str1, char* str2, const int str2len)
{
  char val;
  const char* str2end = str2 + str2len;
  
  while (true)
  {
    val = pgm_read_byte(str1);

    if (val != *str2)
    {
      if (val < *str2)
        return -1;
      else
        return 1;
    }
    
    str1++;
    str2++;

    if (str2 == str2end)
      return 0;
  }
}

void strcpyJ(char* dest, char* src, int srclen)
{
  while (srclen > 0)
  {
    *dest = *src;

    dest++;
    src++;
    srclen--;
  }

  *dest = NULL;
}

bool ParseM408S1(const char* Buffer, const int BytesRead)
{
  char* jstate;
  unsigned char v1type;
  char* v1begin;
  int v1len;
  unsigned char v2type;
  char* v2begin;
  int v2len;
  char result = json_obj(&jstate, Buffer, &v1type, &v1begin, &v1len, &v2type, &v2begin, &v2len);
  char* astate;
  
  unsigned char Index;
  char** PosIter;

  gData.P.MS.FractionPrinted = NULL;

  while (result > 0)
  {
    if (strcmpJP(PSTR("myName"), v1begin, v1len) == 0)
    {
      strcpyJ(gData.PrinterName, v2begin, min(31,v2len));
    }
    else if (strcmpJP(PSTR("numTools"), v1begin, v1len) == 0)
    {
      gData.NumTools = v2begin[0] - '0';
    }
    else if (strcmpJP(PSTR("status"), v1begin, v1len) == 0)
    {
      gData.P.MS.StatusStr = v2begin[0];
    }
    else if (strcmpJP(PSTR("pos"), v1begin, v1len) == 0)
    {
      PosIter = &gData.P.MS.Pos.X;
      result = json_arr(&astate, v2begin, &v1type, &v1begin, &v1len);
      
      while (result > 0)
      {
        *PosIter = v1begin;
        PosIter++;

        if (PosIter > &gData.P.MS.Pos.Z)
          break;
        
        result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
      }
    }
    else if (strcmpJP(PSTR("heaters"), v1begin, v1len) == 0)
    {
      gData.P.MS.HeaterCount = 0;
      
      result = json_arr(&astate, v2begin, &v1type, &v1begin, &v1len);
      
      while (result > 0)
      {
        gData.P.MS.Heaters[gData.P.MS.HeaterCount].Current = v1begin;
        gData.P.MS.HeaterCount++;

        if (gData.P.MS.HeaterCount == HEATERS_MAX)
          break;
        
        result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
      }
    }
    else if (strcmpJP(PSTR("active"), v1begin, v1len) == 0)
    {
      Index = 0;

      result = json_arr(&astate, v2begin, &v1type, &v1begin, &v1len);
      
      while (result > 0)
      {
        gData.P.MS.Heaters[Index].Active = v1begin;
        Index++;

        if (Index == HEATERS_MAX)
          break;
        
        result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
      }
    }
    else if (strcmpJP(PSTR("standby"), v1begin, v1len) == 0)
    {
      Index = 0;

      result = json_arr(&astate, v2begin, &v1type, &v1begin, &v1len);
      
      while (result > 0)
      {
        gData.P.MS.Heaters[Index].Standby = v1begin;
        Index++;

        if (Index == HEATERS_MAX)
          break;
        
        result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
      }
    }
    else if (strcmpJP(PSTR("hstat"), v1begin, v1len) == 0)
    {
      Index = 0;

      result = json_arr(&astate, v2begin, &v1type, &v1begin, &v1len);
      
      while (result > 0)
      {
        gData.P.MS.Heaters[Index].Status = v1begin[0] - '0';
        Index++;

        if (Index == HEATERS_MAX)
          break;
        
        result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
      }
    }
    else if (strcmpJP(PSTR("tool"), v1begin, v1len) == 0)
    {
      if (isdigit(v2begin[0]))
        gData.P.MS.Tool = v2begin[0] - '0';
      else
        gData.P.MS.Tool = -1;
    }
    else if (strcmpJP(PSTR("fraction_printed"), v1begin, v1len) == 0)
    {
      gData.P.MS.FractionPrinted = v2begin;
    }
    
    result = json_obj(&jstate, NULL, &v1type, &v1begin, &v1len, &v2type, &v2begin, &v2len);
  }

  return true;
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

char* PrintFloat(char* Str, unsigned char precision, bool padminus)
{
  if (*Str == '-')
  {
    u8g2.print(*Str);
    Str++;
  }
  else if (padminus)
  {
    u8g2.print(' ');
  }
  
  while (isdigit(*Str))
  {
    u8g2.print(*Str);
    Str++;
  }

  if (*Str == '.')
  {
    u8g2.print(*Str);
    Str++;

    while (isdigit(*Str) && precision > 0)
    {
      u8g2.print(*Str);
      Str++;
      precision--;
    }
  }

  return Str;
}


void DrawStrP(PGM_P Str)
{
  char val;
  while (true)
  {
    val = pgm_read_byte(Str);
    
    if (!val)
      break;
      
    u8g2.print(val);
    Str++;
  }
}

void DrawStrP(const int x, const int y, PGM_P Str)
{
  u8g2.setCursor(x, y);
  DrawStrP(Str);
}

unsigned int StrWidthP(PGM_P Str)
{
  char Buf[2];
  unsigned int Width = 0;

  Buf[1] = 0;

  while (true)
  {
    Buf[0] = pgm_read_byte(Str);
    
    if (!Buf[0])
      break;

    if (Width > 0)
      Width += 1;
      
    Width += u8g2.getStrWidth(Buf);
    Str++;
  }

  return Width;
}
