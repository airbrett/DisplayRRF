#pragma once

enum
{
  PG_CONNECTING,
  PG_MAIN,
  PG_MENU1,
  PG_RUNMACRO
};

#define FLAGS_ENC_SW 0x1
#define FLAGS_ENC_SW_PREV 0x2
#define FLAGS_RST_SW 0x4
#define FLAGS_RST_SW_PREV 0x8

#define HEATERS_MAX 4

struct Data
{
	unsigned char CurrentPage;
	unsigned char Flags;

	char PrinterName[32];
	unsigned char NumTools;
	
	union
	{
		struct
		{
			char StatusStr;
			char* PosStr;

			struct
			{
			  char* X;
			  char* Y;
			  char* Z;
			} Pos;

			struct
			{
			  char* Current;
			  char* Active;
			  char* Standby;
			  unsigned char Status;
			} Heaters[HEATERS_MAX];

			unsigned char HeaterCount;
			unsigned char Tool;
			char* FractionPrinted;
		} MS;
		
		struct
		{
			char* File[20];
		} RM;
	} P;
};