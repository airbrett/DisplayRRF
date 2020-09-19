//G10 To turn off -273.15
//M408 fanPercent shows all fans not just adjustable.
//M408 fanPercent shows 0.0 for thermostatic fans set to S0 but is clamped 0.5-1.0

extern "C"
{
#include "json.h"
}

#include "Data.h"
#include "Connecting.h"
#include "Utility.h"
#include "MainScreen.h"
#include "MainMenu.h"

#include <PinChangeInterrupt.h>



#define POLL_RATE 1000 //ms
#define CONN_TIMEOUT 8000

U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, U8X8_PIN_NONE);//Mini
Encoder Enc(ENC_A_PIN, ENC_B_PIN);
char SerialBuffer[640];
Data gData;

int ReadResponse();
bool ParseM408S1(const char* Buffer, const int BytesRead);
int MakeRequestP(PGM_P Req, char* Resp, const int Len);
PGM_P DecodeStatus(const char Code);
void ConnectingScreen();
void MainScreen();
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
  case PG_PRINT:
    DrawPrintMenu();
    break;  
  }
}



void RateLimit()
{
  static int Timer = 0;
  const int Now = millis();
  
  if (Now - Timer < POLL_RATE)
    delay(POLL_RATE - (Now - Timer));

  Timer = millis();
}

int MakeRequestP(PGM_P Req, char* Resp, const int Len)
{
  uint8_t val;

  RateLimit();

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

int MakeRequest(const char* Req, char* Resp, const int Len)
{
  RateLimit();

  Serial.println(Req);

  if (Resp)
  {
    const size_t BytesRead = Serial.readBytesUntil('\n', Resp, Len-1);
  
    if (BytesRead < 1)
      return 0;//Timeout or some other mess
  
    Resp[BytesRead] = 0;
    return BytesRead+1;
  }

  return 0;
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
