#pragma once
#include <cstdint>
#include <array>
#include <algorithm>
#include <cstdio>

template <typename uart>
class PN532_HAL_UART
{
	constexpr static std::uint8_t BUFFER_LENGTH = 32;
	constexpr static std::uint32_t TIMEOUT		= 1000;
	constexpr static std::uint8_t preamble		= 0x00;
	constexpr static std::uint8_t start_code[2]{0x00, 0xFF};
	constexpr static std::uint8_t frame_identifier = 0xD4;
	constexpr static std::uint8_t postamble		   = 0x00;

public:
	static void write(std::uint8_t command, std::uint8_t *data, std::uint8_t length)
	{

		std::uint8_t length_check_sum = 0x100 - (length + 2);
		/*
		0xD4:MCU->PN532
		0xD5:PN532->MCU
		*/
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
	static void transmit(const std::uint8_t *data, std::uint8_t length, std::uint32_t timeout)
	{
		uart::transmit(data, length, timeout);
	}
};
template <typename HAL, typename RB>
class PN532
{
	constexpr static std::uint8_t BUFFER_LENGTH = 32;
	constexpr static std::uint32_t TIMEOUT		= 1000;
	constexpr static std::uint8_t preamble		= 0x00;
	constexpr static std::uint8_t start_code[2]{0x00, 0xFF};
	constexpr static std::uint8_t frame_identifier = 0xD5;
	constexpr static std::uint8_t postamble		   = 0x00;
	constexpr static std::uint8_t wakeup_frame[]{
		0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x03, 0xFD, 0xD4, 0x14, 0x01, 0x17, 0x00};
	enum command : std::uint8_t
	{
		GET_FIRMWARE_VERSION  = 0x02,
		SAM_CONFIGURATION	  = 0x14,
		IN_LIST_PASSIVETARGET = 0x4A,
	};

	std::uint8_t last_send_command_;

	void command_Handld(std::uint8_t command, std::uint8_t *data, std::uint8_t length)
	{
		if (command - 1 == last_send_command_) /*如果收到的命令是（上次发送的命令-1）*/
		{
			switch (last_send_command_)
			{
				case GET_FIRMWARE_VERSION:
					if (length == 4)
					{
						printf("Found chip PN5%02X\n", data[0]);
						printf("Firmware version: %d.%d\n", data[1], data[2]);
						printf("Support: %02X\n", data[3]);
					}
					else
					{
						printf("error at length,command:0x%02X,length:%d\n", last_send_command_, length);
					}
					break;
				case SAM_CONFIGURATION:
					printf("config SAM OK\n");
					break;
				case IN_LIST_PASSIVETARGET:
					switch (length)
					{
						case 0x0A:
							printf("Found %d card(s)\n", data[0]);
							printf("SENS_RES(s): 0x%02X 0x%02X\n", data[2], data[3]);
							printf("SEL_RES(s): 0x%02X\n", data[4]);
							printf("NFCID length: %d\n", data[5]);
							printf("NFCID: 0x%02X 0x%02X 0x%02X 0x%02X\n", data[6], data[7], data[8], data[9]);
							break;
						case 0x14:
							break;
						default:
							break;
					}
					break;
				default:
					printf("unknown command:0x%X,length:%d\n", last_send_command_, length);

					break;
			}
		}
		else
		{
			printf("error,command:0x%X,last_send_command:0x%X\n", command, last_send_command_);
		}
	}

public:
	enum baud_rate : std::uint8_t
	{
		MIFARE_ISO14443A = 0x00,
	};
	void wake_up()
	{
		HAL::transmit(wakeup_frame, sizeof(wakeup_frame), TIMEOUT);
		last_send_command_ = SAM_CONFIGURATION;
	}
	void get_firmware_version()
	{
		HAL::write(GET_FIRMWARE_VERSION, nullptr, 0);
		last_send_command_ = GET_FIRMWARE_VERSION;
	}
	void scan_card(baud_rate card_type, std::uint8_t card_number)
	{
		std::array<std::uint8_t, 2> data{card_number, card_type};
		HAL::write(IN_LIST_PASSIVETARGET, data.data(), data.size());
		last_send_command_ = IN_LIST_PASSIVETARGET;
	}
	void Rx_Handle()
	{
		if (RB::get_used() >= 6)
		{
			std::uint8_t read_buf[BUFFER_LENGTH];
			RB::peek((void *)read_buf, 6);
			if (
				read_buf[0] == 0x00 &&
				read_buf[1] == 0x00 &&
				read_buf[2] == 0xFF &&
				read_buf[3] == 0x00 &&
				read_buf[4] == 0xFF &&
				read_buf[5] == 0x00) /*ACK帧*/
			{
				RB::drop(6);
				printf("ACK OK\n");
			}
		}
		if (RB::get_used() >= 9)
		{
			std::uint8_t read_buf[BUFFER_LENGTH];
			// RB::peek((void *)read_buf, 6);
			// if (
			// 	read_buf[0] == 0x00 &&
			// 	read_buf[1] == 0x00 &&
			// 	read_buf[2] == 0xFF &&
			// 	read_buf[3] == 0x00 &&
			// 	read_buf[4] == 0xFF &&
			// 	read_buf[5] == 0x00) /*ACK帧*/
			{

				RB::read((void *)read_buf, 6);
				if (read_buf[0] == preamble &&
					read_buf[1] == start_code[0] &&
					read_buf[2] == start_code[1] &&
					read_buf[3] + read_buf[4] == 0x100 &&
					read_buf[5] == frame_identifier)
				{
					std::uint8_t length = read_buf[3] - 1;
					RB::read((void *)read_buf, length); /*读出数据段*/
					std::uint8_t command		= read_buf[0];
					std::uint8_t *data			= read_buf + 1;
					std::uint8_t data_check_sum = frame_identifier + command;
					for (std::uint8_t i = 0; i < length - 1; ++i)
					{
						data_check_sum += *data++;
					}
					RB::read((void *)read_buf, 1);
					if (static_cast<std::uint8_t>(data_check_sum + read_buf[0]) == 0x00)
					{
						RB::drop(1);
						HAL::send_ACK();
						command_Handld(command, read_buf + 1, length - 1);
					}
					else
					{
						HAL::send_ACK();
						printf("error,data_check_sum:0x%X,DCS:0x%X,command:0x%X,length:%d\n", data_check_sum, read_buf[0], command, length);
						RB::reset_read();
					}
				}
			}
		}
	}
};