#pragma once

void DrawStrP(PGM_P Str);
void DrawStrP(const int x, const int y, PGM_P Str);
void DrawStrJ(const char* Str, int Len);
void DrawStrJ(const int x, const int y, const char* Str, int Len);
unsigned int StrWidthP(PGM_P Str);
char* PrintFloat(char* Str, unsigned char precision, bool padminus);
char strcmpJP(PGM_P str1, char* str2, const int str2len);
void strcpyJ(char* dest, char* src, int srclen);
