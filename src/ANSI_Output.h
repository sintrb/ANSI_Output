#ifndef _ANSI_Output_H
#define _ANSI_Output_H
#include <Arduino.h>
#include <Print.h>
#ifndef uint8_t
typedef unsigned char uint8_t;
#endif
#ifndef size_t
typedef unsigned int size_t;
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
typedef enum
{
    BLACK,   // 0
    RED,     // 1
    GREEN,   // 2
    YELLOW,  // 3
    BLUE,    // 4
    MAGENTA, // 5
    CYAN,    // 6
    WHITE    // 7
} ansi_color_t;

typedef struct // __attribute__((packed))
{
    char c;
    ansi_color_t fg;
    ansi_color_t bg;
} ansi_char_t;

typedef enum
{
    TERMINAL_STATE_NORMAL,
    TERMINAL_STATE_ESC,
    TERMINAL_STATE_CSI,
    TERMINAL_STATE_CSI_PARAM,
    TERMINAL_STATE_CSI_PARAM_DONE,
    TERMINAL_STATE_CSI_PARAM_DONE_COLON,
    TERMINAL_STATE_CSI_PARAM_DONE_COLON_DONE,
    TERMINAL_STATE_CSI_PARAM_DONE_COL
} TERMINAL_STATE;

class ANSI_Output
{
public:
    virtual void clear();                                // clearn screen buffer
    virtual void display() {};                           // flush data to screen
    virtual void drawText(int x, int y, ansi_char_t *c); // draw text at x,y with text/bg color
    virtual int cols();                                  // return screen columns
    virtual int rows();                                  // return screen rows
};

class ANSI_Print : public Print
{
    // 参考 https://www.bilibili.com/read/cv21498657/  https://zhuanlan.zhihu.com/p/648409627
private:
    ANSI_Output *output;
    int cols, rows;
    int curx, cury; // 光标位置（文本模式）
    int charw = 6;  // 每个字符的宽度
    int charh = 10; // 每个字符的高度

public:
    ansi_char_t *buffer = NULL;
    boolean locked = false;

    ansi_color_t fg = ansi_color_t::WHITE;
    ansi_color_t bg = ansi_color_t::BLACK;

    // 输出控制状态机
    TERMINAL_STATE state;
    uint8_t csi_param[64];
    uint8_t csi_param_len;
    bool begin(ANSI_Output *output)
    {
        this->output = output;
        this->cols = output->cols();
        this->rows = output->rows();
        this->curx = this->cury = 0;
        size_t size = cols * rows * sizeof(ansi_char_t);
        this->buffer = (ansi_char_t *)malloc(size);
        if (!this->buffer)
        {
            // ESP_LOGE("TERMINAL", "malloc buffer failed");
            return false;
        }
        this->state = TERMINAL_STATE_NORMAL;
        this->clear();
        this->reset();
        this->locked = false;
        return true;
    }
    inline void reset_attr()
    {
        this->fg = WHITE;
        this->bg = BLACK;
    }
    inline void reset()
    {
        this->reset_attr();
        this->curx = 0;
        this->cury = 0;
    }
    void clear()
    {
        // 清屏
        memset(buffer, 0, cols * rows * sizeof(ansi_char_t));
        this->state = TERMINAL_STATE_NORMAL;
        this->reset();
        this->output->clear();
        this->display();
    }
    void inline move_up()
    {
        // 向上滚动一行
        for (int i = 0; i < rows - 1; i++)
        {
            memcpy(&buffer[i * cols], &buffer[(i + 1) * cols], cols * sizeof(ansi_char_t));
        }
        --cury; // = rows - 1;
        if (cury < rows)
        {
            // 清空最后一行
            memset(&buffer[cury * cols], 0, cols * sizeof(ansi_char_t));

            // 更新一遍显示屏
            // this->lockDisplay();
            this->locked = true;
            this->output->clear();
            for (size_t r = 0; r < rows - 1; r++)
            {
                for (size_t c = 0; c < cols; c++)
                {
                    write_char(c, r, &buffer[r * cols + c]);
                }
            }
            this->locked = false;
        }
        // this->unlockDisplay();
    }

    size_t inline parse_int_params(const char *param, int *ret, int max)
    {
        int state = 0; // 0:等待 10:10进制 8:8进制 16:16进制
        int index = 0;
        while (*param)
        {
            char c = *param++;
            // printf("H %d %c", state, c);
            if (c == ';')
            {
                // 下一个
                // ret[index] = 0;
                if (index >= (max - 1))
                {
                    break;
                }
                ++index;
                state = 0;
                continue;
            }

            if (c == 'x' || c == 'X')
            {
                // 16进制
                state = 16;
                ret[index] = 0;
            }
            else if (state != 16 && (c == 'b' || c == 'B'))
            {
                // 2进制
                state = 2;
                ret[index] = 0;
            }
            else
            {
                switch (state)
                {
                case 0:
                    if (c >= '0' && c <= '9')
                    {
                        state = c == '0' ? 8 : 10;
                        ret[index] = c - '0';
                    }
                    break;
                case 2:
                case 8:
                case 10:
                    if (c >= '0' && c <= '9')
                    {
                        ret[index] = ret[index] * state + (c - '0');
                    }
                    break;
                case 16:
                    if (c >= '0' && c <= '9')
                    {
                        ret[index] = ret[index] * 16 + (c - '0');
                    }
                    else if (c >= 'a' && c <= 'f')
                    {
                        ret[index] = ret[index] * 16 + (c - 'a' + 10);
                    }
                    else if (c >= 'A' && c <= 'F')
                    {
                        ret[index] = ret[index] * 16 + (c - 'A' + 10);
                    }
                default:
                    break;
                }
            }

            // printf("-> %d %c %d\n", state, c, ret[index]);
        }
        return index + 1;
    }

    void inline handle_state_normal(uint8_t c)
    {
        switch (c)
        {
        case '\n':
        {
            // 换行处理
            curx = 0;
            cury++;
            break;
        }
        case '\r':
        {
            // 回车处理
            curx = 0;
            break;
        }
        case '\a':
        {

            // 响铃处理
            break;
        }
        case '\b':
        {
            // 退格处理
            if (curx > 0)
            {
                curx--;
            }
            else if (cury > 0)
            {
                cury--;
                curx = cols - 1;
            }
            break;
        }
        case '\t':
        {
            // 制表符处理
            if (curx < cols - 4)
                curx += 4;
            else
            {
                curx = 0;
                cury++;
            }
            break;
        }
        case '\033':
        {
            this->state = TERMINAL_STATE_CSI; // 进入控制序列解析
            break;
        }

        default:
            while (cury >= rows)
            {
                // 检测是否需要滚屏幕
                move_up();
            }
            this->putc_char(c, curx, cury);
            curx++;
            if (curx >= cols)
            {
                curx = 0;
                cury++;
            }
            break;
        }

        if ((cury > rows) || (cury == rows && curx > 0))
        {
            // 检测是否需要滚屏幕
            move_up();
        }
    }

    void inline write_char(int x, int y, ansi_char_t *c)
    {
        this->output->drawText(x, y, c);
    }
    void inline putc_char(uint8_t c, int x, int y)
    {
        if (x >= 0 && x <= cols && y >= 0 && y <= rows)
        {
            buffer[y * cols + x].c = c;
            buffer[y * cols + x].bg = bg;
            buffer[y * cols + x].fg = fg;
            write_char(x, y, &buffer[y * cols + x]);
        }
    }
    ansi_color_t int_to_color(int c)
    {
        if (c < BLACK)
        {
            c = BLACK;
        }
        if (c > WHITE)
        {
            c = WHITE;
        }
        return (ansi_color_t)c;
    }
    void inline handle_state_csi_cmd(uint8_t c)
    {
        csi_param[csi_param_len] = '\0';
        int params[8] = {0};
        size_t pcount = parse_int_params((const char *)csi_param, params, sizeof(params) / sizeof(int));
        int p0 = params[0];
        int p1 = params[1];
        switch (c)
        {
        case 'm':
        {
            // 设置颜属性
            if (pcount == 1)
            {
                int m = p0;
                if (m == 0)
                {
                    // 重置所有属性
                    this->reset_attr();
                }
                else if (m == 1)
                {
                    // 加粗文本
                    // TODO
                }
                if (m >= 30 && m <= 37)
                {
                    this->fg = int_to_color(m - 30);
                }
                else if (m >= 40 && m <= 47)
                {
                    this->bg = int_to_color(m - 40);
                }
            }
            break;
        }
        case 'J':
        {
            // 清屏
            if (pcount == 1)
            {

                if (p0 == 2)
                {
                    // 2J 清除屏幕
                    this->clear();
                }
                else if (p0 == 1)
                {
                    // 1J 清除光标上方的内容
                    this->putc_char('\0', this->curx, this->cury - 1);
                }
                else if (p0 == 0)
                {
                    // 0J 清除光标下方的内容
                    this->putc_char('\0', this->curx, this->cury + 1);
                }
            }

            break;
        }
        case 'K':
        {
            // 清除当前行
            if (pcount >= 1)
            {
                if (p0 == 0)
                {
                    // 0K 清除光标右侧的内容
                    this->putc_char('\0', this->curx + 1, this->cury);
                }
                else if (p0 == 1)
                {
                    // 1K 清除光标左侧的内容
                    this->putc_char('\0', this->curx - 1, this->cury);
                }
                else if (p0 == 2)
                {
                    // 2K 清除当前行
                    for (int i = 0; i < this->cols; i++)
                    {
                        this->putc_char('\0', i, this->cury);
                    }
                }
            }
            break;
        }
        case 'H':
        {
            // 设置光标位置 y;xH
            if (pcount >= 2)
            {
                // 移动到指定位置
                this->cury = MIN(this->rows, p0 - 1);
                this->curx = MIN(this->cols, p1 - 1);
            }
            else if (pcount == 1)
            {
                // 移动到指定行
                this->cury = MAX(0, p0 - 1);
            }
            else
            {
                // 移动到左上角
                this->cury = 0;
                this->curx = 0;
            }
            break;
        }
        case 'A':
        {
            // 光标上移一行
            if (this->cury > 0)
                this->cury--;
            break;
        }
        case 'B':
        {
            // 光标下移一行
            if (this->cury < (this->rows - 1))
                this->cury++;
            break;
        }
        case 'C':
        {
            // 光标右移一列
            if (this->curx < (this->cols - 1))
                this->curx++;
            break;
        }
        case 'D':
        {
            // 光标左移一列
            if (this->curx > 0)
                this->curx--;
            break;
        }
        default:
            break;
        }
        this->state = TERMINAL_STATE_NORMAL;
    }

    void inline handle_state_csi_param(uint8_t c)
    {
        switch (c)
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case ';':
        {
            // 参数处理
            csi_param[csi_param_len++] = c;
            break;
        }
        default:
            // 处理csi命令
            handle_state_csi_cmd(c);
            break;
        }
    }

    virtual size_t write(uint8_t c)
    {
        switch (state)
        {
        case TERMINAL_STATE_NORMAL:
            handle_state_normal(c);
            break;
        case TERMINAL_STATE_CSI:
        {
            if (c == '[')
            {
                this->state = TERMINAL_STATE_CSI_PARAM;
                this->csi_param_len = 0;
            }
            else
            {
                this->state = TERMINAL_STATE_NORMAL;
            }
            break;
        }
        case TERMINAL_STATE_CSI_PARAM:
            handle_state_csi_param(c);
            break;
        default:
            break;
        }
        // return putc(c, stdout);
        return 1;
    }
    int inline getCursorX()
    {
        return this->curx;
    }
    int inline getCursorY()
    {
        return this->cury;
    }
    void inline setCursorX(int x)
    {
        this->curx = x;
    }
    void inline setCursorY(int y)
    {
        this->cury = y;
    }
    void inline setCursor(int x, int y)
    {
        setCursorX(x);
        setCursorY(y);
    }
    void inline display()
    {
        this->output->display();
    }
    boolean inline lockDisplay()
    {
        if (this->locked)
            return false;
        this->locked = true;
    }
    boolean inline unlockDisplay()
    {
        this->locked = false;
    }
    boolean inline isLocked()
    {
        return this->locked;
    }
};

#define ANSI_TEST_TEXTS {                                                                \
    "\033[31mThis is Red Tx on Blue BG\033[0m\n",                                        \
    "\033[32mThis is Green Tx\033[0m\n",                                                 \
    "Hello!!! Word!!!",                                                                  \
    "\033[41mThis TTT\033[0m\n",                                                         \
    "\033[32mThis is Green Tx\033[0m\n",                                                 \
    "\033[40mThis Tx is BG black.\033[0m\n",                                             \
    "\033[41mThis Tx is BG pink-red.\033[0m\033[46mThis Tx is BG light-green.\033[0m\n", \
    "\033[42mThis Tx is BG dark-green.\033[0m\n",                                        \
    "\033[43mThis Tx is BG yellow-red.\033[0m\n",                                        \
    "\033[44mThis Tx is BG light-blue.\033[0m\n",                                        \
    "\033[45mThis Tx is BG pink.\033[0m\n",                                              \
    "\033[46mThis Tx is BG light-green.\033[0m\n",                                       \
    "\033[47mThis Tx is BG grey color.\033[0m\n",                                        \
}
#endif