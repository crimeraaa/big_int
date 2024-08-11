/**
 * @brief   ANSI/VT100 terminal escape sequences. Useful for coloring/styling
 *          text output.
 *
 * @see     https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
 */
#pragma once

/// local
#include "common.h"

enum AnsiMode {
    ANSI_BOLD = 1,
    ANSI_DIM,
    ANSI_ITALIC,
    ANSI_UNDERLINE,
    ANSI_BLINKING,
    ANSI_REVERSE = 7, // skip 6
    ANSI_INVISIBLE,
    ANSI_STRIKETHROUGH,
};

enum AnsiColor {
    ANSI8_BLACK = 30,
    ANSI8_RED,
    ANSI8_GREEN,
    ANSI8_YELLOW,
    ANSI8_BLUE,
    ANSI8_MAGENTA,
    ANSI8_CYAN,
    ANSI8_WHITE,
    ANSI8_DEFAULT = 39, // Skip 38

    // https://ss64.com/bash/syntax-colors.html
    ANSI256_MEDIUMPURPLE2 = 140,
    ANSI256_MEDIUMPURPLE1,
    ANSI256_GOLD3A,
    ANSI256_DARKKHAKI,
    ANSI256_NAVAJOWHITE3,
    ANSI256_GREY69,
    ANSI256_LIGHTSTEELBLUE3,
    ANSI256_LIGHTSTEELBLUE,
    ANSI256_YELLOW3A,
    ANSI256_DARKOLIVEGREEN3,
    ANSI256_DARKSEAGREEN3,
    ANSI256_DARKSEAGREEN2A,
    ANSI256_LIGHTCYAN3,
    ANSI256_LIGHTSKYBLUE,
    ANSI256_GREENYELLOW,
    ANSI256_DARKOLIVEGREEN2A,
    ANSI256_PALEGREEN1,
    ANSI256_DARKSEAGREEN2B,
    ANSI256_DARKSEAGREEN1A,
    ANSI256_PALETURQUOISE1,
    ANSI256_RED3,
    ANSI256_DEEPPINK3A,
    ANSI256_DEEPPINK3B,
    ANSI256_MAGENTA3A,
    ANSI256_MAGENTA3B,
    ANSI256_MAGENTA2A,
    ANSI256_DARKORANGE3,
    ANSI256_INDIANRED,
    ANSI256_HOTPINK3,
    ANSI256_HOTPINK2,
    ANSI256_ORCHID,
    ANSI256_MEDIUMORCHID1A,
    ANSI256_ORANGE3,
    ANSI256_LIGHTSALMON3,
    ANSI256_LIGHTPINK3,
    ANSI256_PINK3,
    ANSI256_PLUM3,
    ANSI256_VIOLET,
    ANSI256_GOLD3B,
    ANSI256_LIGHTGOLDENROD3,
    ANSI256_TAN,
    ANSI256_MISTYROSE3,
    ANSI256_THISTLE3,
    ANSI256_PLUM2,
    ANSI256_YELLOW3B,
    ANSI256_KHAKI3,
    ANSI256_LIGHTGOLDENROD2,
    ANSI256_LIGHTYELLOW3,
    ANSI256_GREY84,
    ANSI256_LIGHTSTEELBLUE1,
    ANSI256_YELLOW2,
    ANSI256_DARKOLIVEGREEN1,
    ANSI256_DARKOLIVEGREEN2,
    ANSI256_DARKSEAGREEN1B,
    ANSI256_HONEYDEW2,
    ANSI256_LIGHTCYAN1,
    ANSI256_RED1,
    ANSI256_DEEPPINK2,
    ANSI256_DEEPPINK1A,
    ANSI256_DEEPPINK1B,
    ANSI256_MAGENTA2B,
    ANSI256_MAGENTA1,
    ANSI256_ORANGERED1,
    ANSI256_INDIANRED1A,
    ANSI256_INDIANRED1B,
    ANSI256_HOTPINK1A,
    ANSI256_HOTPINK1B,
    ANSI256_MEDIUMORCHID1B,
    ANSI256_DARKORANGE,
    ANSI256_SALMON1,
    ANSI256_LIGHTCORAL,
    ANSI256_PALEVIOLETRED,
    ANSI256_ORCHID2,
    ANSI256_ORCHID1,
    ANSI256_ORANGE1,
    ANSI256_SANDYBROWN,
    ANSI256_LIGHTSALMON1,
    ANSI256_LIGHTPINK1,
    ANSI256_PINK1,
    ANSI256_PLUM1,
    ANSI256_GOLD1,
    ANSI256_LIGHTGOLDENROD2A,
    ANSI256_LIGHTGOLDENROD2B,
    ANSI256_NAVAJOWHITE1,
    ANSI256_MISTYROSE1,
    ANSI256_THISTLE1,
    ANSI256_YELLOW1,
    ANSI256_LIGHTGOLDENROD1,
    ANSI256_KHAKI1,
    ANSI256_WHEAT1,
    ANSI256_CORNSILK1,
};

int ansi_sendcsi_(FILE *stream, int count, ...);
int ansi_resetcsi(FILE *stream);
int ansi_printfg_256color(FILE *stream, enum AnsiColor color, const char *fmt, ...);


// Most: ESC[38;2;{r};{g};{b}m or ESC[48;2:{r};{g};{b};m]
#define ansi_vargc(...) \
    x__xselect6(__VA_ARGS__, 5, 4, 3, 2, 1)

#define ansi_sendcsi(stream, ...) \
    ansi_sendcsi_(stream, ansi_vargc(__VA_ARGS__), __VA_ARGS__)

#define ansi_setfg_256color(stream, id)      ansi_sendcsi(stream, 38, 5, id)
#define ansi_setbg_256color(stream, id)      ansi_sendcsi(stream, 48, 5, id)
#define ansi_setfg_rgbcolor(stream, r, g, b) ansi_sendcsi(stream, 38, 2, r, g, b)
#define ansi_setbg_rgbcolor(stream, r, g, b) ansi_sendcsi(stream, 48, 2, r, g, b)

#ifdef ANSI_INCLUDE_IMPLEMENTATION

int ansi_sendcsi_(FILE *stream, int count, ...)
{
    char    builder[64] = "\x1b[";
    char   *end         = builder + array_countof(builder);
    char   *writer      = &builder[2];
    va_list args;

    va_start(args, count);
    for (int i = 0; i < count; i++) {
        int arg = va_arg(args, int);
        int len;
        // Have more than 1 arg and not at least arg?
        if (1 < count && i < count - 1) 
            len = snprintf(writer, end - writer, "%i;", arg);
        else
            len = snprintf(writer, end - writer, "%i", arg);
        writer += len;
    }
    snprintf(writer++, end - writer, "m");
    *writer = '\0';
    va_end(args);
    return fprintf(stream, "%s", builder);
}

int ansi_resetcsi(FILE *stream)
{
    return ansi_sendcsi_(stream, 0);
}

int ansi_printfg_256color(FILE *stream, enum AnsiColor color, const char *fmt, ...)
{
    va_list args;
    int     out = 0;
    va_start(args, fmt);
    out += ansi_setfg_256color(stream, color);
    out += vfprintf(stream, fmt, args);
    out += ansi_resetcsi(stream);
    va_end(args);
    return out;
}

#endif  // ANSI_INCLUDE_IMPLEMENTATION
