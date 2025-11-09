#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Fonts/FreeSans12pt7b.h>
#include <SPI.h>
#include <ANSI_Output.h>

#define PIN_LCD_LIGHT_PIN 10
#define PIN_LCD_CS_PIN 14
#define PIN_LCD_DC_PIN 21 // DC/WRX
#define PIN_LCD_RST_PIN 11
#define PIN_LCD_SDA_PIN 12
#define PIN_LCD_CLK_PIN 13 // SCK

#define LCD_WIDTH 240
#define LCD_HEIGHT 320

// 1 is default 6x8, 2 is 12x16, 3 is 18x24
#define FONT_SIZE 1
#define FONT_WIDTH (FONT_SIZE == 1 ? 6 : (FONT_SIZE == 2 ? 12 : 18))
#define FONT_HEIGHT (FONT_SIZE == 1 ? 8 : (FONT_SIZE == 2 ? 16 : 24))
#define COLS (LCD_WIDTH / FONT_WIDTH)
#define ROWS (LCD_HEIGHT / FONT_HEIGHT)

SPIClass spi = SPIClass(1);
Adafruit_ST7789 tft = Adafruit_ST7789(&spi, PIN_LCD_CS_PIN, PIN_LCD_DC_PIN, PIN_LCD_RST_PIN);

uint16_t colorMap[] = {
    ST77XX_BLACK,   // 0
    ST77XX_RED,     // 1
    ST77XX_GREEN,   // 2
    ST77XX_YELLOW,  // 3
    ST77XX_BLUE,    // 4
    ST77XX_MAGENTA, // 5
    ST77XX_CYAN,    // 6
    ST77XX_WHITE    // 7
};

class MyOutput : public ANSI_Output
{
public:
  void clear()
  {
    tft.fillScreen(ST77XX_BLACK);
  }
  void display()
  {
  }
  uint16_t getColor(ansi_color_t ac)
  {
    int c = (int)ac;
    if (c >= 0 and c < (sizeof(colorMap) / sizeof(colorMap[0])))
    {
      return colorMap[c];
    }
    return ST77XX_WHITE;
  }
  void drawText(int x, int y, ansi_char_t *ch)
  {
    char c = ch->c;
    bool invert = false;
    if (c < 32)
    {
      c = ' ';
    }
    uint16_t color = getColor(ch->fg);
    if (ch->bg)
    {
      tft.fillRect(x * FONT_WIDTH, y * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, getColor(ch->bg));
    }
    // ssd1315.drawChar(x * 6, y * 12, c, 12, invert);
    tft.setCursor(x * FONT_WIDTH, y * FONT_HEIGHT);
    tft.setTextColor(color);
    tft.print(c);
  }
  int cols()
  {
    return COLS;
  }
  int rows()
  {
    return ROWS;
  }
};

ANSI_Print ansi;
MyOutput output;

void setup(void)
{
  Serial.begin(115200);
  delay(2000);
  Serial.print(F("Hello! ST77xx TFT Test"));

  if (PIN_LCD_LIGHT_PIN >= 0)
  {
    pinMode(PIN_LCD_LIGHT_PIN, OUTPUT);
    digitalWrite(PIN_LCD_LIGHT_PIN, HIGH);
  }

  spi.begin(PIN_LCD_CLK_PIN, 1, PIN_LCD_SDA_PIN, PIN_LCD_CS_PIN);
  tft.init(LCD_WIDTH, LCD_HEIGHT);
  tft.setSPISpeed(1000 * 1000 * 80);
  tft.setTextSize(FONT_SIZE);
  tft.setTextWrap(false);
  Serial.println(F("Initialized"));

  // tft.fillScreen(ST77XX_BLACK);

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