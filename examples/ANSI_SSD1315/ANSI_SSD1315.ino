// #include <Arduino.h>
#include <Wire.h>
#include <SSD1315.h>
#include <ANSI_Output.h>
SSD1315 ssd1315;

class MyOutput : public ANSI_Output
{
public:
  void clear()
  {
    ssd1315.clear();
  }
  void display()
  {
    ssd1315.display();
  }
  void drawText(int x, int y, ansi_char_t *ch)
  {
    char c = ch->c;
    bool invert = false;

    if (c < 32)
    {
      c = ' ';
    }
    if (ch->bg)
    {
      invert = true;
    }
    ssd1315.drawChar(x * 6, y * 12, c, 12, invert);
    // Serial.printf("%d,%d, %c [%d]\r\n", x, y, ch->c, ch->c);
  }
  int cols()
  {
    return 12;
  }
  int rows()
  {
    return 3;
  }
};

ANSI_Print ansi;
MyOutput output;

void setup()
{
  Serial.begin(115200);
  Wire.begin(5, 6, 1000000UL);
  ssd1315.begin();

  if (!ansi.begin(&output))
  {
    Serial.println(F("Failed to initialize"));
    while (1)
    {
      delay(1000);
    }
  }
  ansi.clear();
  ansi.println("ANSI_Print demo");
  ansi.display();

  delay(1000);
}
const char *texts[] = ANSI_TEST_TEXTS;

#define TEXT_COUNT (sizeof(texts) / sizeof(texts[0]))

void loop()
{
  static int i = 0;
  ansi.printf(texts[i]);
  ansi.display();
  i = (i + 1) % TEXT_COUNT;
  delay(100);
}