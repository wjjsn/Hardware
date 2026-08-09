#pragma once

#include <stdint.h>

namespace Hardware
{
struct ChineseGlyph
{
	char index[5];
	uint8_t data[32];
};

extern const uint8_t font_ascii_8x16[][16];
extern const ChineseGlyph font_chinese_16x16[];
}
