#include "Globals.h"
#include "Utility.h"
#include "Menu.h"

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
	MENU_BEGIN("Main Menu")
	MENU_ITEM_P(PSTR("Back"), gCurrentPage = PG_MAIN;)
  if (gStatusStr == 'I')
  {
	  MENU_ITEM_P(PSTR("Print"), gCurrentPage = PG_PRINT; gData.RM.FileArray = NULL;)
	  MENU_ITEM_P(PSTR("Run Macro"), gCurrentPage = PG_RUNMACRO; gData.RM.FileArray = NULL;)
  }
  else if (gStatusStr == 'P')
  {
    MENU_ITEM_P(PSTR("Pause"), MakeRequestP(PSTR("M25"), NULL, 0); gCurrentPage = PG_MAIN)
  }
  else if (gStatusStr == 'A')
  {
    MENU_ITEM_P(PSTR("Resume"), MakeRequestP(PSTR("M24"), NULL, 0); gCurrentPage = PG_MAIN)
    MENU_ITEM_P(PSTR("Stop"), MakeRequestP(PSTR("M0"), NULL, 0); gCurrentPage = PG_MAIN)
  }
	MENU_ITEM_P(PSTR("Babystep"), gCurrentPage = PG_BABYSTEP;)
	MENU_END()
}

void DrawMacrosMenu()
{
	if (!gData.RM.FileArray)
	{
		const int BytesRead = MakeRequestP(PSTR("M20 S2 P\"0:/macros\""), gSerialBuffer, sizeof(gSerialBuffer));

		if (BytesRead)
		{
			if (!ParseM20(gSerialBuffer, BytesRead))
				DEBUG_PRINT_P("Bad parse");
		}
	}
  
	MENU_BEGIN("Run Macro")
	MENU_ITEM_P(PSTR("Back"), gCurrentPage = PG_MENU1)
	
	if (gData.RM.FileArray)
	{
		char* astate;
		unsigned char v1type;
		char* v1begin;
		int v1len;

		char result = json_arr(&astate, gData.RM.FileArray, &v1type, &v1begin, &v1len);

		while (result > 0)
		{
			MENU_ITEM_J(v1begin, v1len, RunMacro(gData.RM.EncPos - 1))
			result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
		}
	}
	
	MENU_END()
}

void DrawPrintMenu()
{
	if (!gData.RM.FileArray)
	{
		const int BytesRead = MakeRequestP(PSTR("M20 S2 P\"0:/gcodes\""), gSerialBuffer, sizeof(gSerialBuffer));

		if (BytesRead)
		{
			if (!ParseM20(gSerialBuffer, BytesRead))
				DEBUG_PRINT_P("Bad parse");
		}
	}
  
	MENU_BEGIN("Print")
	MENU_ITEM_P(PSTR("Back"), gCurrentPage = PG_MENU1;)
	
	if (gData.RM.FileArray)
	{
		char* astate;
		unsigned char v1type;
		char* v1begin;
		int v1len;

		char result = json_arr(&astate, gData.RM.FileArray, &v1type, &v1begin, &v1len);

		while (result > 0)
		{
			MENU_ITEM_J(v1begin, v1len, Print(gData.RM.EncPos - 1); gCurrentPage = PG_MAIN;)
			result = json_arr(&astate, NULL, &v1type, &v1begin, &v1len);
		}
	}
	
	MENU_END()
}

void DrawBabyStepMenu()
{
  MENU_BEGIN("Babystep")
  MENU_ITEM_P(PSTR("Back"), gCurrentPage = PG_MENU1;)
  MENU_ITEM_P(PSTR("Z +0.02"), MakeRequestP(PSTR("M290 S0.02"), NULL, 0))
  MENU_ITEM_P(PSTR("Z -0.02"), MakeRequestP(PSTR("M290 S-0.02"), NULL, 0))
  MENU_END()
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
