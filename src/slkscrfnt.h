#ifndef _SLKSCR_FNT_H_
#define _SLKSCR_FNT_H_

/* slkscr.ttf */

#define FONT_ATLAS_WIDTH 769
#define FONT_ATLAS_HEIGHT 14
#define FONT_ASCENDER 16
#define FONT_DESCENDER -4
#define FONT_HEIGHT 20

struct glyph {
    char c;
    int width;
    int x, y;
    int w, h;
    int ox, oy;
} font_map[] = {
     {' ',  8,   0, 12,  1,  0, 0,  0},
     {'!',  6,   1,  2,  3, 10, 2, 10},
     {'"', 10,   4,  2,  7,  4, 2, 10},
     {'#', 14,  11,  2, 11, 10, 2, 10},
     {'$', 12,  22,  0,  9, 14, 2, 12},
     {'%', 14,  31,  2, 11, 10, 2, 10},
     {'&', 12,  42,  0,  9, 14, 2, 12},
     {'\'', 6,  51,  2,  3,  4, 2, 10},
     {'(',  8,  54,  2,  5, 10, 2, 10},
     {')',  8,  59,  2,  5, 10, 2, 10},
     {'*', 14,  64,  2, 11, 10, 2, 10},
     {'+', 14,  75,  2, 11, 10, 2, 10},
     {',',  8,  86, 10,  5,  4, 2,  2},
     {'-', 10,  91,  6,  7,  2, 2,  6},
     {'.',  6,  98, 10,  3,  2, 2,  2},
     {'/', 10, 101,  2,  7, 10, 2, 10},
     {'0', 12, 108,  2,  9, 10, 2, 10},
     {'1', 10, 117,  2,  7, 10, 2, 10},
     {'2', 12, 124,  2,  9, 10, 2, 10},
     {'3', 12, 133,  2,  9, 10, 2, 10},
     {'4', 12, 142,  2,  9, 10, 2, 10},
     {'5', 12, 151,  2,  9, 10, 2, 10},
     {'6', 12, 160,  2,  9, 10, 2, 10},
     {'7', 12, 169,  2,  9, 10, 2, 10},
     {'8', 12, 178,  2,  9, 10, 2, 10},
     {'9', 12, 187,  2,  9, 10, 2, 10},
     {':',  6, 196,  4,  3,  6, 2,  8},
     {';',  8, 199,  4,  5,  8, 2,  8},
     {'<', 10, 204,  2,  7, 10, 2, 10},
     {'=', 10, 211,  4,  7,  6, 2,  8},
     {'>', 10, 218,  2,  7, 10, 2, 10},
     {'?', 12, 225,  2,  9, 10, 2, 10},
     {'@', 14, 234,  2, 11, 10, 2, 10},
     {'A', 12, 245,  2,  9, 10, 2, 10},
     {'B', 12, 254,  2,  9, 10, 2, 10},
     {'C', 12, 263,  2,  9, 10, 2, 10},
     {'D', 12, 272,  2,  9, 10, 2, 10},
     {'E', 10, 281,  2,  7, 10, 2, 10},
     {'F', 10, 288,  2,  7, 10, 2, 10},
     {'G', 12, 295,  2,  9, 10, 2, 10},
     {'H', 12, 304,  2,  9, 10, 2, 10},
     {'I',  6, 313,  2,  3, 10, 2, 10},
     {'J', 12, 316,  2,  9, 10, 2, 10},
     {'K', 12, 325,  2,  9, 10, 2, 10},
     {'L', 10, 334,  2,  7, 10, 2, 10},
     {'M', 14, 341,  2, 11, 10, 2, 10},
     {'N', 14, 352,  2, 11, 10, 2, 10},
     {'O', 12, 363,  2,  9, 10, 2, 10},
     {'P', 12, 372,  2,  9, 10, 2, 10},
     {'Q', 12, 381,  2,  9, 12, 2, 10},
     {'R', 12, 390,  2,  9, 10, 2, 10},
     {'S', 12, 399,  2,  9, 10, 2, 10},
     {'T', 10, 408,  2,  7, 10, 2, 10},
     {'U', 12, 415,  2,  9, 10, 2, 10},
     {'V', 14, 424,  2, 11, 10, 2, 10},
     {'W', 14, 435,  2, 11, 10, 2, 10},
     {'X', 14, 446,  2, 11, 10, 2, 10},
     {'Y', 14, 457,  2, 11, 10, 2, 10},
     {'Z', 10, 468,  2,  7, 10, 2, 10},
     {'[',  8, 475,  2,  5, 10, 2, 10},
     {'\\',10, 480,  2,  7, 10, 2, 10},
     {']',  8, 487,  2,  5, 10, 2, 10},
     {'^', 10, 492,  0,  7,  4, 2, 12},
     {'_', 12, 499, 12,  9,  2, 2,  0},
     {'`',  8, 508,  2,  5,  4, 2, 10},
     {'a', 12, 513,  2,  9, 10, 2, 10},
     {'b', 12, 522,  2,  9, 10, 2, 10},
     {'c', 12, 531,  2,  9, 10, 2, 10},
     {'d', 12, 540,  2,  9, 10, 2, 10},
     {'e', 10, 549,  2,  7, 10, 2, 10},
     {'f', 10, 556,  2,  7, 10, 2, 10},
     {'g', 12, 563,  2,  9, 10, 2, 10},
     {'h', 12, 572,  2,  9, 10, 2, 10},
     {'i',  6, 581,  2,  3, 10, 2, 10},
     {'j', 12, 584,  2,  9, 10, 2, 10},
     {'k', 12, 593,  2,  9, 10, 2, 10},
     {'l', 10, 602,  2,  7, 10, 2, 10},
     {'m', 14, 609,  2, 11, 10, 2, 10},
     {'n', 14, 620,  2, 11, 10, 2, 10},
     {'o', 12, 631,  2,  9, 10, 2, 10},
     {'p', 12, 640,  2,  9, 10, 2, 10},
     {'q', 12, 649,  2,  9, 12, 2, 10},
     {'r', 12, 658,  2,  9, 10, 2, 10},
     {'s', 12, 667,  2,  9, 10, 2, 10},
     {'t', 10, 676,  2,  7, 10, 2, 10},
     {'u', 12, 683,  2,  9, 10, 2, 10},
     {'v', 14, 692,  2, 11, 10, 2, 10},
     {'w', 14, 703,  2, 11, 10, 2, 10},
     {'x', 14, 714,  2, 11, 10, 2, 10},
     {'y', 14, 725,  2, 11, 10, 2, 10},
     {'z', 10, 736,  2,  7, 10, 2, 10},
     {'{', 10, 743,  2,  7, 10, 2, 10},
     {'|',  6, 750,  0,  3, 14, 2, 12},
     {'}', 10, 753,  2,  7, 10, 2, 10},
     {'~', 12, 760,  2,  9,  4, 2, 10},
};

unsigned char font_data_bits[] = {
   0x00, 0x00, 0x00, 0x0c, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00,
   0x00, 0x00, 0x36, 0x63, 0x06, 0xbf, 0x67, 0xf0, 0x1b, 0x1b, 0x30, 0x80,
   0x01, 0x00, 0x00, 0xc6, 0xe3, 0xf1, 0xe3, 0xc7, 0x8c, 0x7f, 0x3c, 0xfe,
   0xf1, 0xe0, 0x01, 0x00, 0x03, 0x0c, 0x7e, 0xf0, 0x83, 0xc7, 0x0f, 0x1e,
   0x3f, 0x7e, 0x3f, 0x7e, 0xc3, 0x06, 0x6c, 0xd8, 0x60, 0x60, 0x03, 0xe3,
   0xf1, 0x83, 0xc7, 0x0f, 0x7e, 0xbf, 0x61, 0x03, 0x1b, 0xd8, 0xc0, 0x06,
   0xf6, 0x7b, 0x83, 0x37, 0x03, 0x30, 0x78, 0xfc, 0xe0, 0xf1, 0xe3, 0xf7,
   0xe3, 0x37, 0x6c, 0xc0, 0x86, 0x0d, 0x06, 0x36, 0x30, 0x1e, 0x3f, 0x78,
   0xfc, 0xe0, 0xf7, 0x1b, 0x36, 0xb0, 0x81, 0x0d, 0x6c, 0x60, 0x3f, 0xde,
   0x1e, 0xcc, 0x00, 0x36, 0x63, 0x06, 0xbf, 0x67, 0xf0, 0x1b, 0x1b, 0x30,
   0x80, 0x01, 0x00, 0x00, 0xc6, 0xe3, 0xf1, 0xe3, 0xc7, 0x8c, 0x7f, 0x3c,
   0xfe, 0xf1, 0xe0, 0x01, 0x00, 0x03, 0x0c, 0x7e, 0xf0, 0x83, 0xc7, 0x0f,
   0x1e, 0x3f, 0x7e, 0x3f, 0x7e, 0xc3, 0x06, 0x6c, 0xd8, 0x60, 0x60, 0x03,
   0xe3, 0xf1, 0x83, 0xc7, 0x0f, 0x7e, 0xbf, 0x61, 0x03, 0x1b, 0xd8, 0xc0,
   0x06, 0xf6, 0x7b, 0x83, 0x37, 0x03, 0x30, 0x78, 0xfc, 0xe0, 0xf1, 0xe3,
   0xf7, 0xe3, 0x37, 0x6c, 0xc0, 0x86, 0x0d, 0x06, 0x36, 0x30, 0x1e, 0x3f,
   0x78, 0xfc, 0xe0, 0xf7, 0x1b, 0x36, 0xb0, 0x81, 0x0d, 0x6c, 0x60, 0x3f,
   0xde, 0x1e, 0xcc, 0x00, 0x36, 0xfb, 0xdf, 0x80, 0x67, 0x0c, 0xd8, 0x60,
   0x33, 0x83, 0x01, 0x00, 0x00, 0x36, 0x8c, 0x01, 0x0c, 0xd8, 0x8c, 0x01,
   0x03, 0x80, 0x0d, 0x1b, 0x36, 0xc6, 0xf8, 0x31, 0x80, 0xcd, 0x6c, 0xd8,
   0xb0, 0x61, 0xc3, 0x06, 0x83, 0x01, 0xc3, 0x06, 0x6c, 0xc6, 0xe0, 0x79,
   0x0f, 0x1b, 0x36, 0x6c, 0xd8, 0xb0, 0x01, 0x8c, 0x61, 0x03, 0x9b, 0x19,
   0x33, 0x98, 0x01, 0x1b, 0x03, 0x06, 0x00, 0xc0, 0x86, 0x0d, 0x1b, 0x36,
   0x6c, 0x30, 0x18, 0x30, 0x6c, 0xc0, 0x66, 0x0c, 0x9e, 0xf7, 0xb0, 0x61,
   0xc3, 0x86, 0x0d, 0x1b, 0xc0, 0x18, 0x36, 0xb0, 0x99, 0x31, 0x83, 0x19,
   0x30, 0xc6, 0x18, 0x33, 0x00, 0x36, 0xfb, 0xdf, 0x80, 0x67, 0x0c, 0xd8,
   0x60, 0x33, 0x83, 0x01, 0x00, 0x00, 0x36, 0x8c, 0x01, 0x0c, 0xd8, 0x8c,
   0x01, 0x03, 0x80, 0x0d, 0x1b, 0x36, 0xc6, 0xf8, 0x31, 0x80, 0xcd, 0x6c,
   0xd8, 0xb0, 0x61, 0xc3, 0x06, 0x83, 0x01, 0xc3, 0x06, 0x6c, 0xc6, 0xe0,
   0x79, 0x0f, 0x1b, 0x36, 0x6c, 0xd8, 0xb0, 0x01, 0x8c, 0x61, 0x03, 0x9b,
   0x19, 0x33, 0x98, 0x01, 0x1b, 0x03, 0x06, 0x00, 0xc0, 0x86, 0x0d, 0x1b,
   0x36, 0x6c, 0x30, 0x18, 0x30, 0x6c, 0xc0, 0x66, 0x0c, 0x9e, 0xf7, 0xb0,
   0x61, 0xc3, 0x86, 0x0d, 0x1b, 0xc0, 0x18, 0x36, 0xb0, 0x99, 0x31, 0x83,
   0x19, 0x30, 0xc6, 0x18, 0x33, 0x00, 0x06, 0x60, 0x06, 0x0f, 0x18, 0xf0,
   0xc0, 0x60, 0xfc, 0xf8, 0x1f, 0xf8, 0x81, 0x31, 0x8c, 0xc1, 0x83, 0xc7,
   0xbf, 0x1f, 0x3f, 0x60, 0xf0, 0xe0, 0x07, 0x30, 0x00, 0xc0, 0x78, 0xcc,
   0xe3, 0xdf, 0xbf, 0x01, 0xc3, 0x7e, 0xbf, 0x79, 0xff, 0x06, 0xec, 0xc1,
   0x60, 0x66, 0x33, 0x1b, 0xf6, 0x63, 0xd8, 0x0f, 0x1e, 0x8c, 0x61, 0xcc,
   0x98, 0x19, 0x0c, 0x60, 0xc0, 0x18, 0x0c, 0x06, 0x00, 0x00, 0xfe, 0xfd,
   0x1b, 0x30, 0xec, 0xf7, 0x9b, 0xf7, 0x6f, 0xc0, 0x1e, 0x0c, 0x66, 0x36,
   0xb3, 0x61, 0x3f, 0x86, 0xfd, 0xe0, 0xc1, 0x18, 0xc6, 0x8c, 0x99, 0xc1,
   0x00, 0x06, 0x8c, 0xc1, 0x60, 0x00, 0x00, 0x06, 0x60, 0x06, 0x0f, 0x18,
   0xf0, 0xc0, 0x60, 0xfc, 0xf8, 0x1f, 0xf8, 0x81, 0x31, 0x8c, 0xc1, 0x83,
   0xc7, 0xbf, 0x1f, 0x3f, 0x60, 0xf0, 0xe0, 0x07, 0x30, 0x00, 0xc0, 0x78,
   0xcc, 0xe3, 0xdf, 0xbf, 0x01, 0xc3, 0x7e, 0xbf, 0x79, 0xff, 0x06, 0xec,
   0xc1, 0x60, 0x66, 0x33, 0x1b, 0xf6, 0x63, 0xd8, 0x0f, 0x1e, 0x8c, 0x61,
   0xcc, 0x98, 0x19, 0x0c, 0x60, 0xc0, 0x18, 0x0c, 0x06, 0x00, 0x00, 0xfe,
   0xfd, 0x1b, 0x30, 0xec, 0xf7, 0x9b, 0xf7, 0x6f, 0xc0, 0x1e, 0x0c, 0x66,
   0x36, 0xb3, 0x61, 0x3f, 0x86, 0xfd, 0xe0, 0xc1, 0x18, 0xc6, 0x8c, 0x99,
   0xc1, 0x00, 0x06, 0x8c, 0xc1, 0x60, 0x00, 0x00, 0x00, 0xf8, 0x1f, 0x30,
   0xe6, 0x0d, 0xc0, 0x60, 0x33, 0x83, 0x01, 0x00, 0x60, 0x30, 0x8c, 0x31,
   0x00, 0x18, 0x0c, 0x60, 0xc3, 0x18, 0x0c, 0x03, 0x36, 0xc6, 0xf8, 0x31,
   0x00, 0x0c, 0x60, 0xd8, 0xb0, 0x61, 0xc3, 0x06, 0x83, 0x61, 0xc3, 0x36,
   0x6c, 0xc6, 0x60, 0x60, 0xc3, 0x1b, 0x36, 0x60, 0xd8, 0x0c, 0x60, 0x8c,
   0x61, 0xcc, 0x98, 0x19, 0x33, 0x60, 0x30, 0x18, 0x30, 0x06, 0x00, 0x00,
   0x86, 0x0d, 0x1b, 0x36, 0x6c, 0x30, 0x18, 0x36, 0x6c, 0xc3, 0x66, 0x0c,
   0x06, 0x36, 0xbc, 0x61, 0x03, 0x86, 0xcd, 0x00, 0xc6, 0x18, 0xc6, 0x8c,
   0x99, 0x31, 0x03, 0x06, 0x03, 0xc6, 0x18, 0x00, 0x00, 0x00, 0xf8, 0x1f,
   0x30, 0xe6, 0x0d, 0xc0, 0x60, 0x33, 0x83, 0x01, 0x00, 0x60, 0x30, 0x8c,
   0x31, 0x00, 0x18, 0x0c, 0x60, 0xc3, 0x18, 0x0c, 0x03, 0x36, 0xc6, 0xf8,
   0x31, 0x00, 0x0c, 0x60, 0xd8, 0xb0, 0x61, 0xc3, 0x06, 0x83, 0x61, 0xc3,
   0x36, 0x6c, 0xc6, 0x60, 0x60, 0xc3, 0x1b, 0x36, 0x60, 0xd8, 0x0c, 0x60,
   0x8c, 0x61, 0xcc, 0x98, 0x19, 0x33, 0x60, 0x30, 0x18, 0x30, 0x06, 0x00,
   0x00, 0x86, 0x0d, 0x1b, 0x36, 0x6c, 0x30, 0x18, 0x36, 0x6c, 0xc3, 0x66,
   0x0c, 0x06, 0x36, 0xbc, 0x61, 0x03, 0x86, 0xcd, 0x00, 0xc6, 0x18, 0xc6,
   0x8c, 0x99, 0x31, 0x03, 0x06, 0x03, 0xc6, 0x18, 0x00, 0x00, 0x06, 0x60,
   0xc6, 0x0f, 0xe6, 0xf1, 0x03, 0x1b, 0x30, 0x80, 0x01, 0x03, 0x6c, 0xc0,
   0xe3, 0xf7, 0xef, 0x07, 0x8c, 0x1f, 0x3c, 0x18, 0xf0, 0xe0, 0x81, 0x01,
   0x03, 0x0c, 0x18, 0xf0, 0x63, 0xd8, 0x0f, 0x1e, 0x3f, 0x7e, 0x03, 0x1e,
   0xc3, 0xc6, 0x63, 0xd8, 0x6f, 0x60, 0x03, 0xe3, 0x31, 0x80, 0xc7, 0xb0,
   0x1f, 0x0c, 0x1e, 0x30, 0x60, 0xc6, 0xc0, 0x60, 0xf0, 0x7b, 0xb0, 0x07,
   0x00, 0x00, 0x86, 0xfd, 0xe0, 0xf1, 0xe3, 0x37, 0xe0, 0x31, 0x6c, 0x3c,
   0x86, 0xfd, 0x06, 0x36, 0x30, 0x1e, 0x03, 0x78, 0x0c, 0xfb, 0xc1, 0xe0,
   0x01, 0x03, 0x66, 0x0c, 0x0c, 0x06, 0x3f, 0xde, 0x1e, 0x00, 0x00, 0x06,
   0x60, 0xc6, 0x0f, 0xe6, 0xf1, 0x03, 0x1b, 0x30, 0x80, 0x01, 0x03, 0x6c,
   0xc0, 0xe3, 0xf7, 0xef, 0x07, 0x8c, 0x1f, 0x3c, 0x18, 0xf0, 0xe0, 0x81,
   0x01, 0x03, 0x0c, 0x18, 0xf0, 0x63, 0xd8, 0x0f, 0x1e, 0x3f, 0x7e, 0x03,
   0x1e, 0xc3, 0xc6, 0x63, 0xd8, 0x6f, 0x60, 0x03, 0xe3, 0x31, 0x80, 0xc7,
   0xb0, 0x1f, 0x0c, 0x1e, 0x30, 0x60, 0xc6, 0xc0, 0x60, 0xf0, 0x7b, 0xb0,
   0x07, 0x00, 0x00, 0x86, 0xfd, 0xe0, 0xf1, 0xe3, 0x37, 0xe0, 0x31, 0x6c,
   0x3c, 0x86, 0xfd, 0x06, 0x36, 0x30, 0x1e, 0x03, 0x78, 0x0c, 0xfb, 0xc1,
   0xe0, 0x01, 0x03, 0x66, 0x0c, 0x0c, 0x06, 0x3f, 0xde, 0x1e, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x03, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xf8, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0xc0,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xf8, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00,
   0x00, 0x00
};

#endif /* _SLKSCR_FNT_H_ */
