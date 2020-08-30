//G10 To turn off -273.15
//M408 fanPercent shows all fans not just adjustable.
//M408 fanPercent shows 0.0 for thermostatic fans set to S0 but is clamped 0.5-1.0

#include <U8g2lib.h>

//#define ENABLE_DEBUG_PRINT

#ifdef ENABLE_DEBUG_PRINT
#define DEBUG_PRINT(str) Serial.println(str)
#define DEBUG_PRINT_P(str) Serial.println(F(str))
#else
#define DEBUG_PRINT(str) ((void)0)
#define DEBUG_PRINT_P(str) ((void)0)
#endif

//U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, 23, 17, 16, /* reset=*/ U8X8_PIN_NONE);//RAMPS
U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);//Mini


char SerialBuffer[640];
//int SerialBufferLen = 0;
bool Connected = false;

//Printer info
char PrinterName[32];
char* StatusStr;
char* PosStr;

struct
{
  char* X;
  char* Y;
  char* Z;
} Pos;

#define HEATERS_MAX 4
struct
{
  char* Current;
  char* Active;
  char* Standby;
  unsigned char Status;
} Heaters[HEATERS_MAX];

unsigned char HeaterCount;
unsigned char NumTools;
unsigned char Tool;
char* FractionPrinted = NULL;

//static const char M408_S0[] PROGMEM = "{\"status\":\"I\",\"heaters\":[21.2,20.7],\"active\":[0.0,0.0],\"standby\":[0.0,0.0],\"hstat\":[0,0],\"pos\":[0.000,0.000,301.534],\"machine\":[0.000,0.000,302.384],\"sfactor\":100.0,\"efactor\":[100.0,100.0],\"babystep\":0.850,\"tool\":-1,\"probe\":\"1000\",\"fanPercent\":[0.0,0,100,0],\"fanRPM\":[-1,-1,-1],\"homed\":[0,0,0],\"msgBox.mode\":-1}";
//static const char M408_S1[] PROGMEM = "{\"status\":\"I\",\"heaters\":[20.9,20.7],\"active\":[0.0,0.0],\"standby\":[0.0,0.0],\"hstat\":[0,0],\"pos\":[0.000,0.000,301.534],\"machine\":[0.000,0.000,302.384],\"sfactor\":100.0,\"efactor\":[100.0,100.0],\"babystep\":0.850,\"tool\":-1,\"probe\":\"1000\",\"fanPercent\":[0.0,0,100,0],\"fanRPM\":[-1,-1,-1],\"homed\":[0,0,0],\"msgBox.mode\":-1,\"geometry\":\"delta\",\"axes\":3,\"totalAxes\":3,\"axisNames\":\"XYZ\",\"volumes\":2,\"numTools\":2,\"myName\":\"Deltabot\",\"firmwareName\":\"RepRapFirmware for LPC176x based Boards\"}";

void GetValStr(const char* Haystack, PGM_P Needle, char** Out, unsigned char* OutLen);
void GetValArray(const char* Haystack, PGM_P Needle, char** Out, unsigned char* OutLen);
void GetValNumber(const char* Haystack, PGM_P Needle, char** Out, unsigned char* OutLen);
char* PrintFloat(char* Str, unsigned char precision);
int ReadResponse();
bool ParseM408S0(const char* Buffer, const int BytesRead);
bool ParseM408S1(const char* Buffer, const int BytesRead);
int MakeRequest(PGM_P Req, char* Resp, const int Len);
PGM_P DecodeStatus(const char* Code);

void setup()
{
  Serial.begin(57600);
  Serial.setTimeout(8000);
  DEBUG_PRINT_P("hello world");
  
  u8g2.begin();
  u8g2.setFont(u8g2_font_5x7_tr);
  DrawConnecting();
}

enum
{
  STATE_SEND,
  STATE_RECV,
  STATE_WAIT
};

void loop()
{
  static int Timer = -9999;
  const int Now = millis();
  bool Redraw = false;

  if (Now - Timer > 1000)
  {
    Timer = Now;

    if (!Connected)
    {
      const int BytesRead = MakeRequest(PSTR("M408 S1"), SerialBuffer, sizeof(SerialBuffer));
      
      if (BytesRead)
      {
        if (ParseM408S1(SerialBuffer, BytesRead))
        {
          Connected = true;
          Redraw = true;
        }
        else
        {
          DEBUG_PRINT_P("Bad parse");
        }
      }
    }
    else
    {
      const int BytesRead = MakeRequest(PSTR("M408 S0"), SerialBuffer, sizeof(SerialBuffer));
      
      if (BytesRead)
      {
        if (ParseM408S0(SerialBuffer, BytesRead))
        {
          Redraw = true;
        }
        else
        {
          DEBUG_PRINT_P("Bad parse");
        }
      }
      else
      {
        Connected = false;
        Redraw = true;
      }
    }
  }

  if (Connected)
    DrawMain();
  else
    DrawConnecting();
    
}
/*
void loop2(void)
{
  static int Timer;
  static int State = STATE_SEND;
  bool ParseResult = false;
  const int Now = millis();
  int BytesRead;

  switch (State)
  {
    case STATE_SEND:
      if (Connected)
        Serial.println(F("M408 S0"));
      else
        Serial.println(F("M408 S1"));

      Timer = Now;
      State = STATE_RECV;
      break;
    case STATE_RECV:
      BytesRead = ReadResponse();
      
      if (BytesRead)
      {
        DEBUG_PRINT_P("Got stuff");
        
        if (Connected)
          ParseResult = ParseM408S0(SerialBuffer, BytesRead-1);
        else
          ParseResult = ParseM408S1(SerialBuffer, BytesRead-1);

        if (ParseResult)
        {
          State = STATE_WAIT;
          Timer = Now;
          Connected = true;
        }
        else
        {
          DEBUG_PRINT_P("Bad parse");
          Connected = false;
          State = STATE_SEND;
        }
      }
      else if (Timer - Now > 8000)
      {
        DEBUG_PRINT_P("Timeout");
        Connected = false;
        State = STATE_SEND;
      }
      break;
    case STATE_WAIT:
      if (Timer - Now > 1000)
        State = STATE_SEND;
      break;
  }

  if (ParseResult)
  {
    u8g2.firstPage();
    do
    {
      if (Connected)
        MainScreen();
      else
        ConnectingScreen();
    }
    while ( u8g2.nextPage() );
  }
}
*/
void DrawConnecting()
{
  u8g2.firstPage();
  do
  {
    ConnectingScreen();
  }
  while (u8g2.nextPage());
}

void DrawMain()
{
  u8g2.firstPage();
  do
  {
    MainScreen();
  }
  while (u8g2.nextPage());
}

int MakeRequest(PGM_P Req, char* Resp, const int Len)
{
  uint8_t val;

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

  Resp[BytesRead+1] = 0;
  return BytesRead;
}
/*
int ReadResponse()
{
  int BytesRead = 0;
  char Byte;
  
  while (Serial.available())
  {
     Byte = Serial.read();

    SerialBuffer[SerialBufferLen] = Byte;
    SerialBufferLen++;

    if (SerialBuffer[SerialBufferLen-1] == '\n')
    {
      SerialBuffer[SerialBufferLen-1] = 0;
      BytesRead = SerialBufferLen;
      SerialBufferLen = 0;

      DEBUG_PRINT(SerialBuffer);
      break;
    }
    else if (SerialBufferLen == sizeof(SerialBuffer))
    {
      DEBUG_PRINT_P("Buffer overflow");
      SerialBufferLen = 0;
    }
  }
  
  return BytesRead;
}
*/
//"validate"
bool IsValidJSON(const char* JSON, const int Len)
{
  if (JSON[0] == '{' && JSON[Len-1] == '}')
    return true;
  else
    return false;
}

bool ParseM408S1(const char* Buffer, const int BytesRead)
{
  char* NameStr;
  unsigned char NameStrLen;
  char* NumToolsStr;
  unsigned char NumToolsStrLen;
  
  if (IsValidJSON(Buffer, BytesRead))
  {
    GetValStr(Buffer, PSTR("\"myName\""), &NameStr, &NameStrLen);
    GetValNumber(Buffer, PSTR("\"numTools\""), &NumToolsStr, &NumToolsStrLen);

    if (NameStr == NULL || NumToolsStr == NULL)
      return false;
  
    ParseM408S0(Buffer, BytesRead);
    
    *(NameStr + NameStrLen) = 0;
    *(NumToolsStr + NumToolsStrLen) = 0;
    
    strncpy(PrinterName, NameStr, 32);
    NumTools = NumToolsStr[0] - '0';

    return true;
  }

  return false;
}

bool ParseM408S0(const char* Buffer, const int BytesRead)
{
  unsigned char StatusStrLen;
  unsigned char PosStrLen;
  char* HeatersStr;
  unsigned char HeatersStrLen;
  char* ActiveStr;
  unsigned char ActiveStrLen;
  char* StandbyStr;
  unsigned char StandbyStrLen;
  char* HStatStr;
  unsigned char HStatStrLen;
  char* ToolStr;
  unsigned char ToolStrLen;
  unsigned char FractionPrintedLen;

  if (IsValidJSON(Buffer, BytesRead))
  {
    GetValStr(Buffer, PSTR("\"status\""), &StatusStr, &StatusStrLen);
    GetValArray(Buffer, PSTR("\"pos\""), &PosStr, &PosStrLen);
    GetValArray(Buffer, PSTR("\"heaters\""), &HeatersStr, &HeatersStrLen);
    GetValArray(Buffer, PSTR("\"active\""), &ActiveStr, &ActiveStrLen);
    GetValArray(Buffer, PSTR("\"standby\""), &StandbyStr, &StandbyStrLen);
    GetValArray(Buffer, PSTR("\"hstat\""), &HStatStr, &HStatStrLen);
    GetValNumber(Buffer, PSTR("\"tool\""), &ToolStr, &ToolStrLen);
    GetValNumber(Buffer, PSTR("\"fraction_printed\""), &FractionPrinted, &FractionPrintedLen);
    

    if (StatusStr == NULL || PosStr == NULL ||
        HeatersStr == NULL || ActiveStr == NULL ||
        StandbyStr == NULL || HStatStr == NULL ||
        ToolStr == NULL)
      return false;
  
    *(StatusStr + StatusStrLen) = 0;
    *(PosStr + PosStrLen) = 0;
    *(HeatersStr + HeatersStrLen) = 0;
    *(ActiveStr + ActiveStrLen) = 0;
    *(StandbyStr + StandbyStrLen) = 0;
    *(HStatStr + HStatStrLen) = 0;
    *(ToolStr + ToolStrLen) = 0;

    if (FractionPrinted != NULL)
      *(FractionPrinted + FractionPrintedLen) = 0;
  
    static bool First = true;
    if (First)
    {
      First = false;
      DEBUG_PRINT(StatusStr);
      DEBUG_PRINT(PosStr);
      DEBUG_PRINT(HeatersStr);
      DEBUG_PRINT(ActiveStr);
      DEBUG_PRINT(StandbyStr);
      DEBUG_PRINT(HStatStr);
      DEBUG_PRINT(ToolStr);
    }
  
    Pos.X = strtok(PosStr, ",");
    Pos.Y = strtok(NULL, ",");
    Pos.Z = strtok(NULL, ",");
  
    for (HeaterCount = 0; HeaterCount < HEATERS_MAX; HeaterCount++)
    {
      Heaters[HeaterCount].Current = strtok(HeaterCount == 0 ? HeatersStr : NULL, ",");
  
      if (Heaters[HeaterCount].Current == NULL)
        break;
    }
  
    for (char i = 0; i < HeaterCount; i++)
      Heaters[i].Active = strtok(i == 0 ? ActiveStr : NULL, ",");
  
    for (char i = 0; i < HeaterCount; i++)
      Heaters[i].Standby = strtok(i == 0 ? StandbyStr : NULL, ",");
  
    for (char i = 0; i < HeaterCount; i++)
    {
      char* Temp = strtok(i == 0 ? HStatStr : NULL, ",");
      Heaters[i].Status = Temp[0] - '0';
    }
  
    if (isdigit(ToolStr[0]))
      Tool = ToolStr[0] - '0';
    else
      Tool = -1;
  }
}

void ConnectingScreen()
{
  static const int Sequence[] = {9680, 9683, 9681, 9682, 0};
  static unsigned char Index = 0;
  static unsigned long Timer = 0;

  {
    const unsigned long Now = millis();

    if (Now - Timer > 1000)
    {
      Timer = Now;
      Index++;

      if (Sequence[Index] == 0)
        Index = 0;
    }

    u8g2.setFont(u8g2_font_unifont_t_75);
    u8g2.drawGlyph(60, 45, Sequence[Index]);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(40, 30);
    u8g2.print("Connecting");
  }
}

void MainScreen()
{
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, 7, PrinterName);

  PGM_P DecodedStatus = DecodeStatus(StatusStr);
  const char Len = strlen_P(DecodedStatus);
  char* StatusStr = malloc(Len+1);
  strcpy_P(StatusStr, DecodedStatus);
  u8g2_uint_t StatusStrWidth = u8g2.getStrWidth(StatusStr);
  u8g2.drawStr(128 - StatusStrWidth, 7, StatusStr);
  free(StatusStr);
  
  u8g2.drawLine(0, 8, 128, 8);

  //Serial.println(PosStr);
  //
  BedHeater(12, 10, Heaters[0].Status == 2);// PanelDue only checks for 2

  u8g2.drawStr(0, 30, "C");
  u8g2.drawStr(0, 37, "A");
  u8g2.drawStr(0, 44, "S");

  for (unsigned char i = 0; i < HeaterCount; i++)
    Heater(7 + 30 * i, 30, Heaters[i].Current, Heaters[i].Active, Heaters[i].Standby);

  //u8g2.setCursor(0,7);
  //u8g2.print(count);

  for (int i = 0; i < NumTools; i++)
    HotendHeater(40 + i * 28, 10, '1' + i, Tool == i);

  if (FractionPrinted != NULL)
  {
    u8g2.setCursor(0,51);
    
    if (FractionPrinted[0] != '0')
      u8g2.print(FractionPrinted[0]);

    if (FractionPrinted[2] != '0')
      u8g2.print(FractionPrinted[2]);

    if (FractionPrinted[3] == 0)
      u8g2.print('0');
    else
      u8g2.print(FractionPrinted[3]);
      
    u8g2.print('%');
    //u8g2.drawStr(0, 51, FractionPrinted);
  }

  u8g2.drawLine(0, 56, 128, 56);

  //pos
  u8g2.setCursor(0, 64);
  u8g2.print("X ");
  PrintFloat(Pos.X, 2);

  u8g2.setCursor(45, 64);
  u8g2.print("Y ");
  PrintFloat(Pos.Y, 2);

  u8g2.setCursor(90, 64);
  u8g2.print("Z ");
  PrintFloat(Pos.Z, 2);
}

PGM_P DecodeStatus(const char* Code)
{
  switch (Code[0])
  {
    case 'I':
      return PSTR("Idle");
      break;
    case 'P':
      return PSTR("Printing");
      break;
    case 'S':
      return PSTR("Stopped");
      break;
    case 'C':
      return PSTR("Booting");
      break;
    case 'A':
      return PSTR("Paused");
      break;
    case 'R':
      return PSTR("Resuming");
      break;
    case 'B':
      return PSTR("Busy");
      break;
    case 'F':
      return PSTR("Updating");
      break;
    default:
      return PSTR("Unknown");
      break;
  }
}

void BedHeater(const int16_t x, const int16_t y, const bool On)
{
  if (On)
  {
    //u8g2.drawLine(7+x,y, 7+x,5+y);
    //u8g2.drawPixel(7+x,8+y);
    //u8g2.drawStr(7+x,8+y, "HOT");

    //u8g2.setFont(u8g2_font_unifont_t_77);
    //u8g2.drawGlyph(7+x,8+y, 9888);
    u8g2.setFont(u8g2_font_unifont_t_76);
    u8g2.drawGlyph(2 + x, 11 + y, 9832);

    u8g2.setFont(u8g2_font_5x7_tr);
  }

  u8g2.drawBox(0 + x, 11 + y, 12, 2);
}

void HotendHeater(const int16_t x, const int16_t y, const char Num, const bool On)
{
  //u8g2.drawBox(3+x,2+y,4,5);
  //u8g2.drawTriangle(0+x,7+y, 5+x,13+y, 11+x,7+y);

  //9929
  if (On)
  {
    u8g2.setFont(u8g2_font_unifont_t_77);
    u8g2.drawGlyph(0 + x, 13 + y, 9930);

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setDrawColor(0);
    u8g2.setCursor(5 + x, 8 + y);
    u8g2.print(Num);
    //u8g2.drawStr(5 + x, 8 + y, Num);

    u8g2.setDrawColor(1);
  }
  else
  {
    u8g2.setFont(u8g2_font_unifont_t_77);
    u8g2.drawGlyph(0 + x, 13 + y, 9929);

    u8g2.setFont(u8g2_font_5x7_tr);
    //u8g2.drawStr(5 + x, 8 + y, Num);
    u8g2.setCursor(5 + x, 8 + y);
    u8g2.print(Num);
  }
}

void Heater(const int16_t x, const int16_t y, const char* current, const char* active, const char* standby)
{
  u8g2.setFont(u8g2_font_5x7_tr);
  //u8g2.setCursor(x, y);
  //u8g2.print(current,1);
  u8g2.drawStr(x, y, current);

  //u8g2.setCursor(x, y + 7);
  //dtostrf(active, 5, 1, active);
  //u8g2.print(active);
  u8g2.drawStr(x, y + 7, active);

  //u8g2.setCursor(x, y + 14);
  //u8g2.print(standby,1);
  //dtostrf(standby, 5, 1, standby);
  //u8g2.print(standby);
  u8g2.drawStr(x, y + 14, standby);
}

void Triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
  u8g2.drawLine(x0, y0, x1, y1);
  u8g2.drawLine(x1, y1, x2, y2);
  u8g2.drawLine(x2, y2, x0, y0);
}

void GetValStr(const char* Haystack, PGM_P Needle, char** Out, unsigned char* OutLen)
{
  char* Occurance = strstr_P(Haystack, Needle);

  if (Occurance != NULL)
  {
    Occurance += strlen_P(Needle);

    while (*Occurance != '\"')
      Occurance++;

    Occurance++;

    *Out = Occurance;

    while (*Occurance != '\"')
      Occurance++;

    *OutLen = Occurance - *Out;
  }
  else
  {
    *Out = NULL;
    *OutLen = 0;
  }
}

void GetValArray(const char* Haystack, PGM_P Needle, char** Out, unsigned char* OutLen)
{
  static const char Failure[] = "?";

  char* Occurance = strstr_P(Haystack, Needle);

  if (Occurance != NULL)
  {
    Occurance += strlen_P(Needle);

    while (*Occurance != '[')
      Occurance++;

    Occurance++;

    *Out = Occurance;

    while (*Occurance != ']')
      Occurance++;

    *OutLen = Occurance - *Out;
  }
  else
  {
    *Out = Failure;
    *OutLen = 1;
  }
}

void GetValNumber(const char* Haystack, PGM_P Needle, char** Out, unsigned char* OutLen)
{
  char* Occurance = strstr_P(Haystack, Needle);

  if (Occurance != NULL)
  {
    Occurance += strlen_P(Needle);

    while (!isdigit(*Occurance) && *Occurance != '.' && *Occurance != '-')
      Occurance++;

    *Out = Occurance;

    while (isdigit(*Occurance) || *Occurance == '.' || *Occurance == '-')
      Occurance++;

    *OutLen = Occurance - *Out;
  }
  else
  {
    *Out = NULL;
    *OutLen = 0;
  }
}

char* PrintFloat(char* Str, unsigned char precision)
{
  while (*Str >= '0' && *Str <= '9')
  {
    u8g2.print(*Str);
    Str++;
  }

  if (*Str == '.')
  {
    u8g2.print(*Str);
    Str++;

    while (*Str >= '0' && *Str <= '9' && precision > 0)
    {
      u8g2.print(*Str);
      Str++;
      precision--;
    }
  }

  return Str;
}

void DrawStrP(const int x, const int y, PGM_P Str)
{
  u8g2.setCursor(x, y);

  uint8_t val;
  while (true)
  {
    val = pgm_read_byte(Str);
    
    if (!val)
      break;
      
    u8g2.print(val);
    Str++;
  }
}
