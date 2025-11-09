# ANSI_Output
A ANSI Escape Sequences parse and output lib for Arduino.

1、You need to include the ANSI_Output.h file in your project.

2、Define a subclass of ANSI_Output with your own output device(such as TFT, OLED, LCD, etc.).

3、Create an ANSI_Print instance and begin with your subclass instance.

ANSI_Output is a abstract class, you need to implement the following methods:

```C++
class ANSI_Output
{
public:
    virtual void clear();                                // clearn screen buffer
    virtual void display() {};                           // flush data to screen
    virtual void drawText(int x, int y, ansi_char_t *c); // draw text at x,y with text/bg color
    virtual int cols();                                  // return screen columns
    virtual int rows();                                  // return screen rows
};
```

Adafruit_GFX is a good choice for LCD/TFT output. But when scroll, the whole screen will be refreshed. So you may need to use a buffer to store the screen data. You can see example/ANSI_ST7789_Buffer for more details.