#pragma once

#define MENU_BEGIN(text)\
PGM_P Text = PSTR(text);\
unsigned char ItemNum;\
unsigned char Row = 0;\
const unsigned char PrevPage = gCurrentPage;\
\
gLCD.firstPage();\
do\
{\
  ItemNum = 0;\
  Row = 0;\
	DrawStrP(gLCD.getDisplayWidth()/2-StrWidthP(Text)/2, 7, Text);\
	gLCD.drawLine(0,8,gLCD.getDisplayWidth(),8);
	
#define MENU_ITEM_P(text, sel_code)\
  if (ItemNum >= gData.RM.First && ItemNum < gData.RM.Last)\
  {\
	  DrawStrP(5, 16+Row*7, text);\
    Row++;\
\
  	if (gData.RM.Enc && gData.RM.EncPos == ItemNum)\
  	{\
      gData.RM.Enc = 0;\
  		sel_code;\
  	}\
  }\
\
	ItemNum++;

#define MENU_ITEM_J(text, len, sel_code)\
  if (ItemNum >= gData.RM.First && ItemNum < gData.RM.Last)\
  {\
    DrawStrJ(5, 16+Row*7, text, len);\
    Row++;\
\
    if (gData.RM.Enc && gData.RM.EncPos == ItemNum)\
    {\
      gData.RM.Enc = 0;\
      sel_code;\
    }\
  }\
\
  ItemNum++;

#define MENU_END()\
	DrawStrP(0,7*(gData.RM.EncPos - gData.RM.First)+16,PSTR(">"));\
}\
while (gLCD.nextPage()); \
\
if (gCurrentPage == PrevPage)\
{\
  char EncPos = gEnc1.read()/4;\
  \
  if (EncPos < 0)\
  {\
    EncPos = 0;\
    gEnc1.write(EncPos);\
  }\
  else if (EncPos >= ItemNum)\
  {\
    EncPos = ItemNum-1;\
    gEnc1.write(EncPos*4);\
  }\
  \
  gData.RM.EncPos = EncPos;\
  \
  if (EncPos < gData.RM.First)\
  {\
    gData.RM.First = EncPos;\
    gData.RM.Last = min(gData.RM.First + 7, ItemNum - 1);\
  }\
  else if (EncPos >= gData.RM.Last)\
  {\
    gData.RM.Last = EncPos + 1;\
    gData.RM.First = max(gData.RM.Last - 7, 0);\
  }\
  \
  if (gFlags & FLAGS_ENC_SW)\
  {\
    gFlags &= ~FLAGS_ENC_SW;\
    gData.RM.Enc = 1;\
  }\
}\
else\
{\
  MENU_RESET();\
}


#define MENU_RESET()\
do\
{\
  gEnc1.write(0);\
  gData.RM.EncPos = 0;\
  gData.RM.First = 0;\
  gData.RM.Last = 7;\
  gData.RM.Enc = 0;\
}\
while(false)
