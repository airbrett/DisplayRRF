#pragma once

#define MENU_BEGIN(name) \
char EncPos = gEnc1.read()/4;\
\
if (EncPos < 0)\
{\
	EncPos = 0;\
	gEnc1.write(0);\
}\
\
PGM_P Text = PSTR(name);\
unsigned char ItemNum;\
\
gLCD.firstPage();\
do\
{\
  ItemNum = 0;\
	DrawStrP(gLCD.getDisplayWidth()/2-StrWidthP(Text)/2, 7, Text);\
	gLCD.drawLine(0,8,gLCD.getDisplayWidth(),8);
	
#define MENU_ITEM_P(name, sel_code)\
	DrawStrP(5, 16+ItemNum*7, name);\
\
	if (gFlags & FLAGS_ENC_SW && EncPos == ItemNum)\
	{\
    gFlags &= ~FLAGS_ENC_SW;\
		sel_code;\
	}\
\
	ItemNum++;

#define MENU_ITEM_J(name, len, sel_code)\
	DrawStrJ(5, 16+ItemNum*7, name, len);\
\
	if (gFlags & FLAGS_ENC_SW && EncPos == ItemNum)\
	{\
    gFlags &= ~FLAGS_ENC_SW;\
		sel_code;\
	}\
\
	ItemNum++;

#define MENU_END()\
	if (EncPos >= ItemNum)\
  {\
    EncPos = ItemNum -1;\
		gEnc1.write(EncPos*4);\
  }\
	\
	DrawStrP(0,7*EncPos+16,PSTR(">"));\
}\
while (gLCD.nextPage());
