#pragma once

#include <stdint.h>

#include "bits_operation.hpp"

// AS5600 magnetic angle sensor, I2C address 0x36, 8-bit registers, little-endian register pairs.
namespace Hardware
{
template <typename i2c_device>
class AS5600
{
public:
	enum class PowerMode : uint8_t { Normal, LowPower1, LowPower2, LowPower3 };
	enum class Hysteresis : uint8_t { Disabled, Lsb1, Lsb2, Lsb3 };
	enum class OutputMode : uint8_t { AnalogFull, AnalogReduced, DigitalPwm };
	enum class PwmFrequency : uint8_t { Hz115, Hz230, Hz460, Hz920 };
	enum class SlowFilter : uint8_t { X16, X8, X4, X2 };
	enum class FastFilterThreshold : uint8_t { SlowOnly, Lsb6, Lsb7, Lsb9, Lsb18, Lsb21, Lsb24, Lsb10 };

	static void init()
	{
		i2c_device::init();
		config_register_ = 0;
	}

	static uint16_t read_cordic_magnitude()
	{
		return read_u16(Register::MagnitudeHigh);
	}

	static uint8_t read_agc()
	{
		uint8_t value = 0;
		i2c_device::mem_read(static_cast<uint16_t>(Register::Agc), &value, 1, timeout);
		return value;
	}

	static uint16_t read_raw_angle() { return read_u16(Register::RawAngleHigh) & 0x0FFFU; }
	static uint16_t read_angle() { return read_u16(Register::AngleHigh) & 0x0FFFU; }

	static bool read_magnet_status()
	{
		uint8_t status = 0;
		i2c_device::mem_read(static_cast<uint16_t>(Register::Status), &status, 1, timeout);
		return !BIT::READ(status, 3) && !BIT::READ(status, 4) && BIT::READ(status, 5);
	}

	static bool set_range(uint16_t start_angle = 0, uint16_t stop_angle = 0x0FFF,
					  uint16_t max_angle = 0x0FFF)
	{
		if (start_angle > 0x0FFFU || stop_angle > 0x0FFFU || max_angle > 0x0FFFU) return false;
		write_u16(Register::ZposHigh, start_angle);
		write_u16(Register::MposHigh, stop_angle);
		write_u16(Register::MangHigh, max_angle);
		return true;
	}

	static void set_config(PowerMode power = PowerMode::Normal,
					   Hysteresis hysteresis = Hysteresis::Disabled,
					   OutputMode output = OutputMode::AnalogFull,
					   PwmFrequency pwm_frequency = PwmFrequency::Hz115,
					   SlowFilter slow_filter = SlowFilter::X16,
					   FastFilterThreshold fast_filter = FastFilterThreshold::SlowOnly,
					   bool watchdog = false)
	{
		config_register_ = static_cast<uint16_t>(power)
			| (static_cast<uint16_t>(hysteresis) << 2U)
			| (static_cast<uint16_t>(output) << 4U)
			| (static_cast<uint16_t>(pwm_frequency) << 6U)
			| (static_cast<uint16_t>(slow_filter) << 8U)
			| (static_cast<uint16_t>(fast_filter) << 10U);
		if (watchdog) BIT::SET(config_register_, 13);
		write_u16(Register::ConfHigh, config_register_);
	}

private:
	enum class Register : uint8_t
	{
		ZposHigh = 0x01, MposHigh = 0x03, MangHigh = 0x05, ConfHigh = 0x07,
		Status = 0x0B, RawAngleHigh = 0x0C, AngleHigh = 0x0E, Agc = 0x1A, MagnitudeHigh = 0x1B
	};
	static constexpr uint32_t timeout = 1000;
	inline static uint16_t config_register_ = 0;

	static uint16_t read_u16(Register reg)
	{
		uint8_t data[2]{};
		i2c_device::mem_read(static_cast<uint16_t>(reg), data, 2, timeout);
		return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8U) | data[1]);
	}

	static void write_u16(Register reg, uint16_t value)
	{
		const uint8_t data[2]{static_cast<uint8_t>(value >> 8U), static_cast<uint8_t>(value)};
		i2c_device::mem_write(static_cast<uint16_t>(reg), data, 2, timeout);
	}
};
}
