#include "Data.h"

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