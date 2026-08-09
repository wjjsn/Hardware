#pragma once

#include <array>
#include <cstdarg>
#include <cstddef>
#include <stdint.h>
#include <cstdio>
#include <cstring>

#include "../font.hpp"

// SSD1306 OLED over I2C (commonly address 0x3C), 8-bit commands and page-addressed data.
namespace Hardware
{
template <typename i2c_device, std::size_t page_count>
class Ssd1306
{
public:
	static_assert(page_count == 4U || page_count == 8U, "SSD1306 supports 32 or 64 pixel height");
	static constexpr std::size_t width = 128;
	static constexpr std::size_t height = page_count * 8U;

	void init()
	{
		i2c_device::init();
		const uint8_t common_commands[]{
			0xAE, 0xD5, 0x80, 0xA8, static_cast<uint8_t>(height - 1U), 0xD3, 0x00,
			0x40, 0xA1, 0xC8, 0xDA, page_count == 4U ? uint8_t{0x02} : uint8_t{0x12},
			0x81, page_count == 4U ? uint8_t{0xFF} : uint8_t{0xCF}, 0xD9,
			page_count == 4U ? uint8_t{0x1F} : uint8_t{0xF1}, 0xDB, 0x30,
			0xA4, 0xA6, 0x8D, 0x14, 0xAF};
		for (const uint8_t command : common_commands) write_command(command);
		for (auto &page : draw_buffer_) page[0] = 0x40;
		for (auto &page : sent_buffer_) page[0] = 0x40;
		clear();
		update_force();
	}

	void clear()
	{
		for (auto &page : draw_buffer_) page.fill(0);
		for (auto &page : draw_buffer_) page[0] = 0x40;
	}

	bool draw_image(std::size_t page, std::size_t x, std::size_t image_width,
					std::size_t image_pages, const uint8_t *image)
	{
		if (image == nullptr || page >= page_count || x >= width || image_pages == 0U
			|| page + image_pages > page_count || image_width > width - x) return false;
		for (std::size_t row = 0; row < image_pages; ++row)
		{
			for (std::size_t column = 0; column < image_width; ++column)
				draw_buffer_[page + row][x + column + 1U] = image[row * image_width + column];
		}
		return true;
	}

	bool draw_char(std::size_t page, std::size_t x, char character)
	{
		const unsigned char code = static_cast<unsigned char>(character);
		if (code < 0x20U || code > 0x7EU) return false;
		return draw_image(page, x, 8, 2, font_ascii_8x16[code - 0x20U]);
	}

	bool draw_text(std::size_t page, std::size_t x, const char *text)
	{
		if (text == nullptr || page >= page_count || x >= width) return false;
		std::size_t offset = 0;
		for (std::size_t index = 0; text[index] != '\0';)
		{
			const unsigned char lead = static_cast<unsigned char>(text[index]);
			if (lead < 0x80U)
			{
				if (!draw_char(page, x + offset, text[index])) return false;
				++index; offset += 8U;
			}
			else
			{
				const std::size_t length = utf8_length(lead);
				if (length < 2U || length > 4U || x + offset + 16U > width) return false;
				char character[5]{};
				for (std::size_t byte = 0; byte < length; ++byte)
				{
					if (text[index + byte] == '\0') return false;
					character[byte] = text[index + byte];
				}
				const ChineseGlyph *glyph = find_chinese_glyph(character);
				if (!draw_image(page, x + offset, 16, 2, glyph->data)) return false;
				index += length; offset += 16U;
			}
			if (x + offset > width) return false;
		}
		return true;
	}

	bool print(std::size_t page, std::size_t x, const char *format, ...)
	{
		if (format == nullptr) return false;
		std::array<char, 64> buffer{};
		std::va_list args;
		va_start(args, format);
		const int length = std::vsnprintf(buffer.data(), buffer.size(), format, args);
		va_end(args);
		return length >= 0 && static_cast<std::size_t>(length) < buffer.size() && draw_text(page, x, buffer.data());
	}

	void update()
	{
		for (std::size_t page = 0; page < page_count; ++page)
		{
			if (draw_buffer_[page] != sent_buffer_[page]) write_page(page);
		}
	}

	void update_force()
	{
		for (std::size_t page = 0; page < page_count; ++page) write_page(page);
	}

private:
	static constexpr uint32_t timeout = 1000;
	using PageBuffer = std::array<uint8_t, width + 1U>;
	std::array<PageBuffer, page_count> draw_buffer_{};
	std::array<PageBuffer, page_count> sent_buffer_{};

	static void write_command(uint8_t command)
	{
		const uint8_t data[2]{0x00, command};
		i2c_device::transmit(data, 2, timeout);
	}

	static void set_cursor(std::size_t page, std::size_t x)
	{
		if (page >= page_count || x >= width) return;
		write_command(static_cast<uint8_t>(0xB0U | page));
		write_command(static_cast<uint8_t>(0x10U | ((x >> 4U) & 0x0FU)));
		write_command(static_cast<uint8_t>(x & 0x0FU));
	}

	void write_page(std::size_t page)
	{
		set_cursor(page, 0);
		i2c_device::transmit(draw_buffer_[page].data(), draw_buffer_[page].size(), timeout);
		sent_buffer_[page] = draw_buffer_[page];
	}

	static std::size_t utf8_length(unsigned char lead)
	{
		if ((lead & 0xE0U) == 0xC0U) return 2;
		if ((lead & 0xF0U) == 0xE0U) return 3;
		if ((lead & 0xF8U) == 0xF0U) return 4;
		return 0;
	}

	static const ChineseGlyph *find_chinese_glyph(const char *character)
	{
		std::size_t index = 0;
		while (font_chinese_16x16[index].index[0] != '\0')
		{
			if (std::strcmp(font_chinese_16x16[index].index, character) == 0) break;
			++index;
		}
		return &font_chinese_16x16[index];
	}
};
}
