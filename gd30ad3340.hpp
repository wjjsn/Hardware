#pragma once

#include <stdint.h>

#include "bits_operation.hpp"

// GD30AD3340 16-bit I2C ADC, 8-bit registers, big-endian register values.
namespace Hardware
{
template <typename i2c_device>
class GD30AD3340
{
public:
	enum class Mux : uint8_t { Ain0Ain1, Ain0Ain3, Ain1Ain3, Ain2Ain3, Ain0Gnd, Ain1Gnd, Ain2Gnd, Ain3Gnd };
	enum class Pga : uint8_t { V6_144, V4_096, V2_048, V1_024, V0_512, V0_256, V0_064 };
	enum class Mode : uint8_t { Continuous, Single };
	enum class DataRate : uint8_t { Sps6_25, Sps12_5, Sps25, Sps50, Sps100, Sps250, Sps500, Sps1000 };
	enum class ComparatorQueue : uint8_t { One, Two, Four, Disabled };

	void init(Mux mux = Mux::Ain0Gnd, Pga pga = Pga::V2_048,
			  Mode mode = Mode::Continuous, DataRate rate = DataRate::Sps100,
			  ComparatorQueue queue = ComparatorQueue::Disabled)
	{
		i2c_device::init();
		config_register_ = 0;
		set_mux(mux); set_pga(pga); set_mode(mode); set_data_rate(rate); set_comparator_queue(queue);
		write_config();
	}

	void set_mux(Mux value) { set_field(12, 0x7U, static_cast<uint16_t>(value)); }
	void set_pga(Pga value) { set_field(9, 0x7U, static_cast<uint16_t>(value)); }
	void set_mode(Mode value) { set_field(8, 0x1U, static_cast<uint16_t>(value)); }
	void set_data_rate(DataRate value) { set_field(5, 0x7U, static_cast<uint16_t>(value)); }
	void set_comparator_queue(ComparatorQueue value) { set_field(0, 0x3U, static_cast<uint16_t>(value)); }

	void set_comparator(bool polarity_high, bool latch)
	{
		polarity_high ? BIT::SET(config_register_, 3) : BIT::CLR(config_register_, 3);
		latch ? BIT::SET(config_register_, 2) : BIT::CLR(config_register_, 2);
	}

	void write_config() const { write_register(Register::Config, config_register_); }

	uint16_t read_config()
	{
		config_register_ = read_register(Register::Config);
		return config_register_;
	}

	int16_t read_raw() const { return static_cast<int16_t>(read_register(Register::Conversion)); }

	bool read_single(int16_t &result)
	{
		BIT::SET(config_register_, 15);
		write_config();
		for (uint32_t attempts = 0; attempts < conversion_poll_limit; ++attempts)
		{
			config_register_ = read_register(Register::Config);
			if (BIT::READ(config_register_, 15))
			{
				result = read_raw();
				return true;
			}
		}
		return false;
	}

	float read_voltage(float full_scale = 2.048F) const
	{
		const int16_t raw = read_raw();
		return static_cast<float>(raw) * full_scale / 32768.0F;
	}

	float read_temperature(float full_scale = 2.048F) const
	{
		const float voltage = read_voltage(full_scale);
		return -6.91F * voltage * voltage + 268.66F * voltage - 279.28F;
	}

private:
	enum class Register : uint8_t { Conversion = 0x00, Config = 0x01 };
	static constexpr uint32_t timeout = 1000;
	static constexpr uint32_t conversion_poll_limit = 100000;
	uint16_t config_register_ = 0;

	void set_field(uint8_t shift, uint16_t mask, uint16_t value)
	{
		config_register_ = static_cast<uint16_t>((config_register_ & ~(mask << shift)) | ((value & mask) << shift));
	}

	static uint16_t read_register(Register reg)
	{
		uint8_t data[2]{};
		i2c_device::mem_read(static_cast<uint16_t>(reg), data, 2, timeout);
		return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8U) | data[1]);
	}

	static void write_register(Register reg, uint16_t value)
	{
		const uint8_t data[2]{static_cast<uint8_t>(value >> 8U), static_cast<uint8_t>(value)};
		i2c_device::mem_write(static_cast<uint16_t>(reg), data, 2, timeout);
	}
};
}
