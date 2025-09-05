#pragma once
#include <cstdint>
#include <array>
#include <algorithm>
#include <cstdio>
#include <inttypes.h>

template <typename uart>
class PN532_HAL_UART
{
	constexpr static std::uint8_t BUFFER_LENGTH = 32;
	constexpr static std::uint32_t TIMEOUT		= 1000;

public:
	static void write(std::uint8_t command, std::uint8_t *data, std::uint8_t length)
	{
		constexpr std::uint8_t preamble = 0x00;
		constexpr std::uint8_t start_code[2]{0x00, 0xFF};
		std::uint8_t length_check_sum = 0x100 - (length + 2);
		/*
		0xD4:MCU->PN532
		0xD5:PN532->MCU
		*/
		constexpr std::uint8_t frame_identifier = 0xD4;
		constexpr std::uint8_t postamble		= 0x00;
		std::array<std::uint8_t, BUFFER_LENGTH> frame{
			preamble,
			start_code[0],
			start_code[1],
			static_cast<std::uint8_t>(length + 2),
			length_check_sum,
			frame_identifier,
			command,
		};
		auto it = frame.begin() + 7;
		if (data != nullptr && length > 0)
		{
			std::copy_n(data, length, it);
			it += length;
		}
		std::uint8_t data_check_sum = frame_identifier + command;
		for (std::uint8_t i = 0; i < length; ++i)
		{
			data_check_sum += *data++;
		}
		*it++					 = 0x100 - data_check_sum;
		*it++					 = postamble;
		std::uint16_t frame_size = std::distance(frame.begin(), it);

		uart::transmit(frame.data(), frame_size, TIMEOUT);
	}
	static void send_ACK()
	{
		constexpr std::array<std::uint8_t, 6> write_buf{0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
		uart::transmit(write_buf.data(), write_buf.size(), TIMEOUT);
	}
};
template <typename HAL, typename RB>
class PN532
{
public:
	static void get_firmware_version()
	{
		HAL::write(0x02, nullptr, 0);
	}
	static void Rx_Handle()
	{
		if (RB::get_used() >= 6 + 9)
		{
			std::uint8_t read_buf[20];
			RB::peek((void *)read_buf, 6);
			if (
				read_buf[0] == 0x00 &&
				read_buf[1] == 0x00 &&
				read_buf[2] == 0xFF &&
				read_buf[3] == 0x00 &&
				read_buf[4] == 0xFF &&
				read_buf[6] == 0x00)
			{
				RB::drop(6);
				RB::read((void *)read_buf, 6);
				if (read_buf[0] == 0x00 &&
					read_buf[1] == 0x00 &&
					read_buf[2] == 0xFF &&
					read_buf[3] + read_buf[4] == 0x100 &&
					read_buf[5] == 0xD5)
				{
					std::uint8_t length = read_buf[3] - 1;
					RB::read((void *)read_buf, length);
					std::uint8_t command		= read_buf[0];
					std::uint8_t *data			= read_buf + 1;
					std::uint8_t data_check_sum = 0xD5 + command;
					for (std::uint8_t i = 0; i < length - 1; ++i)
					{
						data_check_sum += *data++;
					}
					RB::read((void *)read_buf, 2);
					if (static_cast<std::uint8_t>(data_check_sum + read_buf[0]) == 0x00 &&
						read_buf[1] == 0x00)
					{
						HAL::send_ACK();
						printf("ok!,command:0x%X\n", command);
					}
					else
					{
						printf("error\n");
					}
				}
			}
		}
	}
};