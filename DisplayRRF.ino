//G10 To turn off -273.15
//M408 fanPercent shows all fans not just adjustable.
//M408 fanPercent shows 0.0 for thermostatic fans set to S0 but is clamped 0.5-1.0

extern "C"
{
#include "json.h"
}

#include <U8g2lib.h>
#include <Encoder.h>
#include <PinChangeInterrupt.h>

//#define ENABLE_DEBUG_PRINT

#ifdef ENABLE_DEBUG_PRINT
#define DEBUG_PRINT(str) Serial.println(str)
#define DEBUG_PRINT_P(str) Serial.println(F(str))
#else
#define DEBUG_PRINT(str) ((void)0)
#define DEBUG_PRINT_P(str) ((void)0)
#endif

#define ENC_SW_PIN 8
#define RST_SW_PIN A0

//U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, 23, 17, 16, /* reset=*/ U8X8_PIN_NONE);//RAMPS
U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);//Mini
Encoder Enc(2, 3);

char SerialBuffer[640];

enum
{
  PG_CONNECTING,
  PG_MAIN,
  PG_MENU1
};

unsigned char CurrentPage = PG_CONNECTING;

#define FLAGS_ENC_SW 0x1
#define FLAGS_RST_SW 0x2
unsigned char gFlags = 0;

//Printer info
char PrinterName[32];
char StatusStr;
char* PosStr;

struct
{
  char* X;
  char* Y;
  char* Z;
} Pos;

#define HEATERS_MAX 4
struct
{
  char* Current;
  char* Active;
  char* Standby;
  unsigned char Status;
} Heaters[HEATERS_MAX];

unsigned char HeaterCount;
unsigned char NumTools;
unsigned char Tool;
char* FractionPrinted = NULL;

const unsigned char ConnectSequence[] = {80, 83, 81, 82, 0};
unsigned char ConnectIndex = 0;

//static const char M408_S0[] PROGMEM = "{\"status\":\"I\",\"heaters\":[21.2,20.7],\"active\":[0.0,0.0],\"standby\":[0.0,0.0],\"hstat\":[0,0],\"pos\":[0.000,0.000,301.534],\"machine\":[0.000,0.000,302.384],\"sfactor\":100.0,\"efactor\":[100.0,100.0],\"babystep\":0.850,\"tool\":-1,\"probe\":\"1000\",\"fanPercent\":[0.0,0,100,0],\"fanRPM\":[-1,-1,-1],\"homed\":[0,0,0],\"msgBox.mode\":-1}";
//static const char M408_S1[] PROGMEM = "{\"status\":\"I\",\"heaters\":[20.9,20.7],\"active\":[0.0,0.0],\"standby\":[0.0,0.0],\"hstat\":[0,0],\"pos\":[0.000,0.000,301.534],\"machine\":[0.000,0.000,302.384],\"sfactor\":100.0,\"efactor\":[100.0,100.0],\"babystep\":0.850,\"tool\":-1,\"probe\":\"1000\",\"fanPercent\":[0.0,0,100,0],\"fanRPM\":[-1,-1,-1],\"homed\":[0,0,0],\"msgBox.mode\":-1,\"geometry\":\"delta\",\"axes\":3,\"totalAxes\":3,\"axisNames\":\"XYZ\",\"volumes\":2,\"numTools\":2,\"myName\":\"Deltabot\",\"firmwareName\":\"RepRapFirmware for LPC176x based Boards\"}";

char* PrintFloat(char* Str, unsigned char precision);
int ReadResponse();
bool ParseM408S0(const char* Buffer, const int BytesRead);
bool ParseM408S1(const char* Buffer, const int BytesRead);
int MakeRequest(PGM_P Req, char* Resp, const int Len);
PGM_P DecodeStatus(const char Code);
void ConnectingScreen();
void MainScreen();
void DrawStrP(PGM_P Str);
void DrawStrP(const int x, const int y, PGM_P Str);
unsigned int StrWidthP(PGM_P Str);
void DrawConnecting();
void DrawMain();
void DrawMenu();

void setup()
{
  pinMode(ENC_SW_PIN, INPUT_PULLUP);
  attachPCINT(digitalPinToPCINT(ENC_SW_PIN), EncSwInt, CHANGE);

  pinMode(RST_SW_PIN, INPUT_PULLUP);
  attachPCINT(digitalPinToPCINT(RST_SW_PIN), ResetSwInt, CHANGE);
  
  Serial.begin(57600);
  Serial.setTimeout(8000);
  DEBUG_PRINT_P("hello world");
  
  u8g2.begin();
  u8g2.setFont(u8g2_font_5x7_tr);
  DrawConnecting();
}

void EncSwInt()
{
  if (!(gFlags & ENC_SW_PIN) && digitalRead(ENC_SW_PIN))
    gFlags |= ENC_SW_PIN;
}

void ResetSwInt()
{
  if (digitalRead(RST_SW_PIN))
    gFlags |= RST_SW_PIN;
}

void loop()
{
  static int Timer = -9999;
  const int Now = millis();
  bool Redraw = false;

  if (Now - Timer > 1000)
  {
    Timer = Now;

    if (CurrentPage == PG_CONNECTING)
    {
      const int BytesRead = MakeRequest(PSTR("M408 S1"), SerialBuffer, sizeof(SerialBuffer));
      
      if (BytesRead)
      {
        if (ParseM408S1(SerialBuffer, BytesRead))
        {
          CurrentPage = PG_MAIN;
          Redraw = true;
        }
        else
        {
          DEBUG_PRINT_P("Bad parse");
        }
      }
    }
    else
    {
      const int BytesRead = MakeRequest(PSTR("M408 S0"), SerialBuffer, sizeof(SerialBuffer));
      
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
        CurrentPage = PG_CONNECTING;
        Redraw = true;
      }
    }
  }

  if (Redraw)
  {
    switch (CurrentPage)
    {
    case PG_MAIN:
      DrawMain();
      break;
    case PG_CONNECTING:
      DrawConnecting();
      break;
    case PG_MENU1:
      DrawMenu();
      break;
    }
  }
}

void DrawConnecting()
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
}

void DrawMain()
{
  if (gFlags & ENC_SW_PIN)
  {
    gFlags &= ~ENC_SW_PIN;
    CurrentPage = PG_MENU1;
  }
  
  u8g2.firstPage();
  do
  {
    MainScreen();
  }
  while (u8g2.nextPage());
}

void DrawMenu()
{
  if (gFlags & ENC_SW_PIN)
  {
    gFlags &= ~ENC_SW_PIN;
    CurrentPage = PG_MAIN;
  }
  
  u8g2.firstPage();
  do
  {
    DrawStrP(0,7,PSTR("Back\n"));
    DrawStrP(0,14,PSTR("Control\n"));
    DrawStrP(0,21,PSTR("Run Macro\n"));
    DrawStrP(0,28,PSTR("Preheat\n"));
  }
  while (u8g2.nextPage());
}

int MakeRequest(PGM_P Req, char* Resp, const int Len)
{
  uint8_t val;

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

  Resp[BytesRead+1] = 0;
  return BytesRead;
}

char strcmpJP(PGM_P str1, char* str2, const int str2len)
{
  char val;
  const char* str2end = str2 + str2len;
  
  while (true)
  {
    val = pgm_read_byte(str1);

    if (val == *str2)
    {
      //if (val == NULL)
        //return 0;
    }
    else
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

  FractionPrinted = NULL;

  while (result > 0)
  {
    if (strcmpJP(PSTR("myName"), v1begin, v1len) == 0)
    {
      strcpyJ(PrinterName, v2begin, min(32,v2len));
    }
    else if (strcmpJP(PSTR("numTools"), v1begin, v1len) == 0)
    {
      NumTools = v2begin[0] - '0';
    }
    else if (strcmpJP(PSTR("status"), v1begin, v1len) == 0)
    {
      StatusStr = v2begin[0];
    }
    else if (strcmpJP(PSTR("pos"), v1begin, v1len) == 0)
    {
      PosIter = &Pos.X;
      result = json_arr(&astate, v2begin, &v1type, &v1begin, &v1len);
      
      while (result > 0)
      {
        *PosIter = v1begin;
        PosIter++;

        if (PosIter > &Pos.Z)
          break;
        
        result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
      }
    }
    else if (strcmpJP(PSTR("heaters"), v1begin, v1len) == 0)
    {
      HeaterCount = 0;
      
      result = json_arr(&astate, v2begin, &v1type, &v1begin, &v1len);
      
      while (result > 0)
      {
        Heaters[HeaterCount].Current = v1begin;
        HeaterCount++;

        if (HeaterCount == HEATERS_MAX)
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
        Heaters[Index].Active = v1begin;
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
        Heaters[Index].Standby = v1begin;
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
        Heaters[Index].Status = v1begin[0] - '0';
        Index++;

        if (Index == HEATERS_MAX)
          break;
        
        result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
      }
    }
    else if (strcmpJP(PSTR("tool"), v1begin, v1len) == 0)
    {
      if (isdigit(v1begin[0]))
        Tool = v1begin[0] - '0';
      else
        Tool = -1;
    }
    else if (strcmpJP(PSTR("fraction_printed"), v1begin, v1len) == 0)
    {
      FractionPrinted = v2begin;
    }
    
    result = json_obj(&jstate, NULL, &v1type, &v1begin, &v1len, &v2type, &v2begin, &v2len);
  }

  return true;
}

void ConnectingScreen()
{
  u8g2.setFont(u8g2_font_unifont_t_75);
  u8g2.drawGlyph(60, 45, 9600 + ConnectSequence[ConnectIndex]);
  u8g2.setFont(u8g2_font_5x7_tr);
  DrawStrP(40,30,PSTR("Connecting"));
}

void MainScreen()
{
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, 7, PrinterName);

  PGM_P DecodedStatus = DecodeStatus(StatusStr);
  DrawStrP(128 - StrWidthP(DecodedStatus), 7, DecodedStatus);
  
  u8g2.drawLine(0, 8, 128, 8);

  BedHeater(12, 10, Heaters[0].Status == 2);// PanelDue only checks for 2

  DrawStrP(0, 30, PSTR("C"));
  DrawStrP(0, 37, PSTR("A"));
  DrawStrP(0, 44, PSTR("S"));

  for (unsigned char i = 0; i < HeaterCount; i++)
    Heater(7 + 30 * i, 30, Heaters[i].Current, Heaters[i].Active, Heaters[i].Standby);

  for (int i = 0; i < NumTools; i++)
    HotendHeater(40 + i * 28, 10, '1' + i, Tool == i);

  if (FractionPrinted != NULL)
  {
    u8g2.setCursor(0,51);
    
    if (FractionPrinted[0] != '0')
      u8g2.print(FractionPrinted[0]);

    if (FractionPrinted[2] != '0')
      u8g2.print(FractionPrinted[2]);

    if (isdigit(FractionPrinted[3]))
      u8g2.print(FractionPrinted[3]);
    else
      u8g2.print('0');
      
      
    u8g2.print('%');
  }

  u8g2.drawLine(0, 56, 128, 56);

  //pos
  DrawStrP(0, 64, PSTR("X "));
  PrintFloat(Pos.X, 2);

  DrawStrP(45, 64, PSTR("Y "));
  PrintFloat(Pos.Y, 2);

  DrawStrP(90, 64, PSTR("Z "));
  PrintFloat(Pos.Z, 2);
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
  //u8g2.drawBox(3+x,2+y,4,5);
  //u8g2.drawTriangle(0+x,7+y, 5+x,13+y, 11+x,7+y);

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
  PrintFloat(current, 1);
  u8g2.setCursor(x, y + 7);
  PrintFloat(active, 1);
  u8g2.setCursor(x, y + 14);
  PrintFloat(standby, 1);
}

void Triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
  u8g2.drawLine(x0, y0, x1, y1);
  u8g2.drawLine(x1, y1, x2, y2);
  u8g2.drawLine(x2, y2, x0, y0);
}

char* PrintFloat(char* Str, unsigned char precision)
{
  while ((*Str >= '0' && *Str <= '9') || *Str == '-')
  {
    u8g2.print(*Str);
    Str++;
  }

  if (*Str == '.')
  {
    u8g2.print(*Str);
    Str++;

    while (*Str >= '0' && *Str <= '9' && precision > 0)
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
