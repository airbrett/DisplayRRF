#include "Data.h"
#include "Utility.h"

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