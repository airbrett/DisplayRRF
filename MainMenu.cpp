#include "Data.h"
#include "Utility.h"

extern "C"
{
#include "json.h"
}

extern int MakeRequest(PGM_P Req, char* Resp, const int Len);

static bool ParseM20(const char* JSON, const int BytesRead);

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
        gData.P.RM.FileArray = NULL;
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

  if (!gData.P.RM.FileArray)
  {
    const int BytesRead = MakeRequest(PSTR("M20 S2 P\"0:/macros\""), SerialBuffer, sizeof(SerialBuffer));
    
    if (BytesRead)
    {
      if (!ParseM20(SerialBuffer, BytesRead))
        DEBUG_PRINT_P("Bad parse");
    }
  }

  PGM_P Text = PSTR("Run Macro");
  u8g2.firstPage();
  do
  {
    DrawStrP(u8g2.getDisplayWidth()/2-StrWidthP(Text)/2, 7, Text);
    u8g2.drawLine(0,8,u8g2.getDisplayWidth(),8);
    DrawStrP(5,16,PSTR("Back"));

    DrawStrP(0,7*EncPos+16,PSTR(">"));

    if (gData.P.RM.FileArray)
    {
      char* astate;
      unsigned char v1type;
      char* v1begin;
      int v1len;
      unsigned char Y = 23;
      
      char result = json_arr(&astate, gData.P.RM.FileArray, &v1type, &v1begin, &v1len);
      
      while (result > 0)
      {
        DrawStrJ(5, Y, v1begin, v1len);
        Y += 7;
        
        result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
      }
    }
  }
  while (u8g2.nextPage());
}

bool ParseM20(const char* JSON, const int BytesRead)
{
  char* jstate;
  unsigned char v1type;
  char* v1begin;
  int v1len;
  unsigned char v2type;
  char* v2begin;
  int v2len;
  char result = json_obj(&jstate, JSON, &v1type, &v1begin, &v1len, &v2type, &v2begin, &v2len);

  while (result > 0)
  {
    if (strcmpJP(PSTR("files"), v1begin, v1len) == 0)
      gData.P.RM.FileArray = v2begin;
    
    result = json_obj(&jstate, NULL, &v1type, &v1begin, &v1len, &v2type, &v2begin, &v2len);
  }

  return true;
}
