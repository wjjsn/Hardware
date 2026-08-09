#pragma once

#include <array>
#include <cstddef>
#include <stdint.h>

// PN532 NFC controller over UART. Frames use the normal (non-extended) PN532 format.
namespace Hardware
{
template <typename uart>
struct Pn532UartTransport
{
	static void init() { uart::init(); }
	static void transmit(const uint8_t *data, uint16_t size, uint32_t timeout)
	{
		uart::transmit(data, size, timeout);
	}
};

template <typename transport, typename ring_buffer>
class Pn532
{
public:
	enum class CardType : uint8_t
	{
		Mifare = 0x00,
		FeliCa = 0x01,
		Jewel = 0x04,
		Iso14443A = 0x00,
		Iso14443B = 0x03
	};

	struct FirmwareVersion
	{
		uint8_t ic = 0;
		uint8_t version = 0;
		uint8_t revision = 0;
		uint8_t support = 0;
		bool valid = false;
	};

	static void init()
	{
		transport::init();
		last_command_ = 0;
		last_response_length_ = 0;
		firmware_ = {};
	}

	static void wake_up()
	{
		transport::transmit(wakeup_frame.data(), static_cast<uint16_t>(wakeup_frame.size()), timeout);
		last_command_ = command_sam_configuration;
	}

	static void request_firmware_version()
	{
		send_command(command_get_firmware_version, nullptr, 0);
	}

	static bool scan_card(CardType type, uint8_t max_targets = 1)
	{
		if (max_targets == 0U) return false;
		std::array<uint8_t, 3> data{max_targets, static_cast<uint8_t>(type), 0};
		const uint8_t size = type == CardType::Iso14443B ? 3U : 2U;
		send_command(command_in_list_passive_target, data.data(), size);
		return true;
	}

	static bool receive()
	{
		consume_ack();
		if (ring_buffer::get_used() < frame_header_size) return false;

		std::array<uint8_t, frame_header_size> header{};
		ring_buffer::peek(header.data(), header.size());
		if (header[0] != preamble || header[1] != start_code_1 || header[2] != start_code_2)
		{
			ring_buffer::drop(1);
			return false;
		}
		if (static_cast<uint8_t>(header[3] + header[4]) != 0U || header[3] < 2U)
		{
			ring_buffer::drop(frame_header_size);
			return false;
		}

		const uint16_t frame_data_length = header[3];
		// header[5] is already the first LEN byte (TFI), so only LEN - 1 bytes remain in the ring buffer.
		const std::size_t total_size = frame_header_size + (frame_data_length - 1U) + 2U;
		if (frame_data_length > receive_buffer_.size() || ring_buffer::get_used() < total_size) return false;

		receive_buffer_[0] = header[5];
		ring_buffer::drop(frame_header_size);
		ring_buffer::read(receive_buffer_.data() + 1U, frame_data_length - 1U);
		uint8_t trailer[2]{};
		ring_buffer::read(trailer, 2);
		if (trailer[1] != postamble || receive_buffer_[0] != device_to_host) return false;

		uint8_t checksum = trailer[0];
		for (uint16_t i = 0; i < frame_data_length; ++i) checksum = static_cast<uint8_t>(checksum + receive_buffer_[i]);
		if (checksum != 0U) return false;

		const uint8_t response_command = receive_buffer_[1];
		if (response_command != static_cast<uint8_t>(last_command_ + 1U)) return false;
		last_response_length_ = static_cast<uint8_t>(frame_data_length - 2U);
		parse_response(response_command, receive_buffer_.data() + 2U, last_response_length_);
		send_ack();
		return true;
	}

	static const FirmwareVersion &firmware_version() { return firmware_; }
	static const uint8_t *response_data() { return receive_buffer_.data() + 2U; }
	static uint8_t response_length() { return last_response_length_; }

private:
	static constexpr uint8_t preamble = 0x00;
	static constexpr uint8_t start_code_1 = 0x00;
	static constexpr uint8_t start_code_2 = 0xFF;
	static constexpr uint8_t host_to_device = 0xD4;
	static constexpr uint8_t device_to_host = 0xD5;
	static constexpr uint8_t postamble = 0x00;
	static constexpr uint8_t command_get_firmware_version = 0x02;
	static constexpr uint8_t command_sam_configuration = 0x14;
	static constexpr uint8_t command_in_list_passive_target = 0x4A;
	static constexpr uint32_t timeout = 1000;
	static constexpr std::size_t frame_header_size = 6;
	inline static std::array<uint8_t, 255> receive_buffer_{};
	inline static uint8_t last_command_ = 0;
	inline static uint8_t last_response_length_ = 0;
	inline static FirmwareVersion firmware_{};

	static constexpr std::array<uint8_t, 6> ack_frame{0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
	static constexpr std::array<uint8_t, 24> wakeup_frame{
		0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0xFF, 0x03, 0xFD, 0xD4, 0x14, 0x01, 0x17, 0x00};

	static void send_command(uint8_t command, const uint8_t *data, uint8_t size)
	{
		if (size > 0U && data == nullptr) return;
		const std::array<uint8_t, 7> header{
			preamble, start_code_1, start_code_2, static_cast<uint8_t>(size + 2U),
			static_cast<uint8_t>(0U - static_cast<uint8_t>(size + 2U)), host_to_device, command};
		transport::transmit(header.data(), header.size(), timeout);
		if (size > 0U && data != nullptr) transport::transmit(data, size, timeout);
		uint8_t checksum = static_cast<uint8_t>(host_to_device + command);
		for (uint8_t i = 0; i < size; ++i) checksum = static_cast<uint8_t>(checksum + data[i]);
		const uint8_t trailer[2]{static_cast<uint8_t>(0U - checksum), postamble};
		transport::transmit(trailer, 2, timeout);
		last_command_ = command;
	}

	static void send_ack() { transport::transmit(ack_frame.data(), ack_frame.size(), timeout); }

	static void consume_ack()
	{
		if (ring_buffer::get_used() < ack_frame.size()) return;
		std::array<uint8_t, ack_frame.size()> data{};
		ring_buffer::peek(data.data(), data.size());
		if (data == ack_frame) ring_buffer::drop(ack_frame.size());
	}

	static void parse_response(uint8_t command, const uint8_t *data, uint8_t size)
	{
		if (command == command_get_firmware_version + 1U && size == 4U)
		{
			firmware_ = FirmwareVersion{data[0], data[1], data[2], data[3], true};
		}
	}
};
}
