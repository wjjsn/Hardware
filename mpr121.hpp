#pragma once

#include <stdint.h>

// MPR121 capacitive touch controller, I2C addresses 0x5A-0x5D, 8-bit registers.
namespace Hardware
{
template <typename i2c_device>
class Mpr121
{
public:
	static bool init(uint8_t touch_threshold = 12, uint8_t release_threshold = 6)
	{
		i2c_device::init();
		write_register(Register::SoftReset, 0x63);
		write_register(Register::Ecr, 0x00);
		uint8_t config = 0;
		i2c_device::mem_read(value(Register::Config2), &config, 1, timeout);
		if (config != 0x24U) return false;
		set_thresholds(touch_threshold, release_threshold);
		write_register(Register::Mhdr, 0x01); write_register(Register::Nhdr, 0x01);
		write_register(Register::Nclr, 0x0E); write_register(Register::Fdlr, 0x00);
		write_register(Register::Mhdf, 0x01); write_register(Register::Nhdf, 0x05);
		write_register(Register::Nclf, 0x01); write_register(Register::Fdlf, 0x00);
		write_register(Register::Nhdt, 0x00); write_register(Register::Nclt, 0x00);
		write_register(Register::Fdlt, 0x00); write_register(Register::Debounce, 0x00);
		write_register(Register::Config1, 0x10); write_register(Register::Config2, 0x20);
		write_register(Register::Ecr, 0x8C);
		return true;
	}

	static uint16_t read_touched()
	{
		uint8_t data[2]{};
		i2c_device::mem_read(value(Register::TouchStatusLow), data, 2, timeout);
		return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8U);
	}

private:
	enum class Register : uint8_t
	{
		TouchStatusLow = 0x00, Mhdr = 0x2B, Nhdr = 0x2C, Nclr = 0x2D, Fdlr = 0x2E,
		Mhdf = 0x2F, Nhdf = 0x30, Nclf = 0x31, Fdlf = 0x32, Nhdt = 0x33, Nclt = 0x34,
		Fdlt = 0x35, TouchThreshold0 = 0x41, ReleaseThreshold0 = 0x42, Debounce = 0x5B,
		Config1 = 0x5C, Config2 = 0x5D, Ecr = 0x5E, SoftReset = 0x80
	};
	static constexpr uint32_t timeout = 1000;
	static constexpr uint16_t value(Register reg) { return static_cast<uint16_t>(reg); }

	static void write_register(Register reg, uint8_t data)
	{
		const uint16_t address = value(reg);
		const bool stop_required = reg != Register::Ecr && !(address >= 0x73U && address <= 0x7AU);
		uint8_t ecr = 0;
		if (stop_required)
		{
			i2c_device::mem_read(value(Register::Ecr), &ecr, 1, timeout);
			const uint8_t stop = 0;
			i2c_device::mem_write(value(Register::Ecr), &stop, 1, timeout);
		}
		i2c_device::mem_write(address, &data, 1, timeout);
		if (stop_required) i2c_device::mem_write(value(Register::Ecr), &ecr, 1, timeout);
	}

	static void set_thresholds(uint8_t touch, uint8_t release)
	{
		for (uint8_t electrode = 0; electrode < 12U; ++electrode)
		{
			write_register(static_cast<Register>(value(Register::TouchThreshold0) + 2U * electrode), touch);
			write_register(static_cast<Register>(value(Register::ReleaseThreshold0) + 2U * electrode), release);
		}
	}
};
}
