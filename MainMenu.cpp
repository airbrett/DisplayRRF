#include "Data.h"
#include "Utility.h"

extern "C"
{
#include "json.h"
}

extern int MakeRequestP(PGM_P Req, char* Resp, const int Len);
extern int MakeRequest(const char* Req, char* Resp, const int Len);

static bool ParseM20(const char* JSON, const int BytesRead);
static void RunMacro(int Index);
static void SendM98(const char* Macro, int MacroLen);
static void Print(int Index);
static void SendM32(const char* Macro, int MacroLen);

void DrawMenu()
{
  char EncPos = gEnc1.read()/4;

  if (EncPos < 0)
  {
    EncPos = 0;
    gEnc1.write(0);
  }
  
  if (gFlags & FLAGS_ENC_SW)
  {
    gFlags &= ~FLAGS_ENC_SW;

    switch (EncPos)
    {
      case 0:
        gCurrentPage = PG_MAIN;
        break;
      case 1:
        gCurrentPage = PG_PRINT;
        gData.RM.FileArray = NULL;
        break;
      case 2:
        gCurrentPage = PG_RUNMACRO;
        gData.RM.FileArray = NULL;
        break;
    }
  }
  
  gLCD.firstPage();
  do
  {
    DrawStrP(5,7,PSTR("Back"));
    DrawStrP(5,14,PSTR("Print"));
    DrawStrP(5,21,PSTR("Run Macro"));
    DrawStrP(5,28,PSTR("Preheat"));

    DrawStrP(0,7*EncPos+7,PSTR(">"));
  }
  while (gLCD.nextPage());
}

void DrawMacrosMenu()
{
  char EncPos = gEnc1.read()/4;

  if (EncPos < 0)
  {
    EncPos = 0;
    gEnc1.write(0);
  }
  
  if (gFlags & FLAGS_ENC_SW)
  {
    gFlags &= ~FLAGS_ENC_SW;
    
    if (EncPos == 0)
      gCurrentPage = PG_MENU1;
    else
      RunMacro(EncPos - 1);
  }

  if (!gData.RM.FileArray)
  {
    const int BytesRead = MakeRequestP(PSTR("M20 S2 P\"0:/macros\""), gSerialBuffer, sizeof(gSerialBuffer));
    
    if (BytesRead)
    {
      if (!ParseM20(gSerialBuffer, BytesRead))
        DEBUG_PRINT_P("Bad parse");
    }
  }

  PGM_P Text = PSTR("Run Macro");
  gLCD.firstPage();
  do
  {
    DrawStrP(gLCD.getDisplayWidth()/2-StrWidthP(Text)/2, 7, Text);
    gLCD.drawLine(0,8,gLCD.getDisplayWidth(),8);
    DrawStrP(5,16,PSTR("Back"));

    DrawStrP(0,7*EncPos+16,PSTR(">"));

    if (gData.RM.FileArray)
    {
      char* astate;
      unsigned char v1type;
      char* v1begin;
      int v1len;
      unsigned char Y = 23;
      
      char result = json_arr(&astate, gData.RM.FileArray, &v1type, &v1begin, &v1len);
      
      while (result > 0)
      {
        DrawStrJ(5, Y, v1begin, v1len);
        Y += 7;
        
        result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
      }
    }
  }
  while (gLCD.nextPage());
}

void DrawPrintMenu()
{
  char EncPos = gEnc1.read()/4;

  if (EncPos < 0)
  {
    EncPos = 0;
    gEnc1.write(0);
  }
  
  if (gFlags & FLAGS_ENC_SW)
  {
    gFlags &= ~FLAGS_ENC_SW;
    
    if (EncPos == 0)
    {
      gCurrentPage = PG_MENU1;
    }
    else
    {
      Print(EncPos - 1);
      gCurrentPage = PG_MAIN;
    }
  }

  if (!gData.RM.FileArray)
  {
    const int BytesRead = MakeRequestP(PSTR("M20 S2 P\"0:/gcodes\""), gSerialBuffer, sizeof(gSerialBuffer));
    
    if (BytesRead)
    {
      if (!ParseM20(gSerialBuffer, BytesRead))
        DEBUG_PRINT_P("Bad parse");
    }
  }

  PGM_P Text = PSTR("Print");
  gLCD.firstPage();
  do
  {
    DrawStrP(gLCD.getDisplayWidth()/2-StrWidthP(Text)/2, 7, Text);
    gLCD.drawLine(0,8,gLCD.getDisplayWidth(),8);
    DrawStrP(5,16,PSTR("Back"));

    DrawStrP(0,7*EncPos+16,PSTR(">"));

    if (gData.RM.FileArray)
    {
      char* astate;
      unsigned char v1type;
      char* v1begin;
      int v1len;
      unsigned char Y = 23;
      
      char result = json_arr(&astate, gData.RM.FileArray, &v1type, &v1begin, &v1len);
      
      while (result > 0)
      {
        DrawStrJ(5, Y, v1begin, v1len);
        Y += 7;
        
        result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
      }
    }
  }
  while (gLCD.nextPage());
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
      gData.RM.FileArray = v2begin;
    
    result = json_obj(&jstate, NULL, &v1type, &v1begin, &v1len, &v2type, &v2begin, &v2len);
  }

  return true;
}

void Print(int Index)
{
  char* astate;
  unsigned char v1type;
  char* v1begin;
  int v1len;
  char result = json_arr(&astate, gData.RM.FileArray, &v1type, &v1begin, &v1len);
  
  while (result > 0)
  {
    if (Index == 0)
    {
      SendM32(v1begin, v1len);
      break;
    }

    Index--;
    result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
  }
}

void RunMacro(int Index)
{
  char* astate;
  unsigned char v1type;
  char* v1begin;
  int v1len;
  char result = json_arr(&astate, gData.RM.FileArray, &v1type, &v1begin, &v1len);
  
  while (result > 0)
  {
    if (Index == 0)
    {
      SendM98(v1begin, v1len);
      break;
    }

    Index--;
    result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
  }
}

void SendM98(const char* Macro, int MacroLen)
{
  char Cmd[64];
  char* Iter = Cmd;
  PGM_P M98 = PSTR("M98 P\"/macros/");

  while (true)
  {
    *Iter = pgm_read_byte(M98);
    
    if (!*Iter)
      break;
      
    M98++;
    Iter++;
  }

  while(MacroLen > 0)
  {
    *Iter = *Macro;
    Iter++;
    Macro++;
    MacroLen--;
  }

  *Iter = '\"';
  Iter++;
  *Iter = NULL;

  MakeRequest(Cmd, NULL, 0);
}

void SendM32(const char* Macro, int MacroLen)
{
  char Cmd[64];
  char* Iter = Cmd;
  PGM_P M32 = PSTR("M32 \"");

  while (true)
  {
    *Iter = pgm_read_byte(M32);
    
    if (!*Iter)
      break;
      
    M32++;
    Iter++;
  }

  while(MacroLen > 0)
  {
    *Iter = *Macro;
    Iter++;
    Macro++;
    MacroLen--;
  }

  *Iter = '\"';
  Iter++;
  *Iter = NULL;

  MakeRequest(Cmd, NULL, 0);
}
