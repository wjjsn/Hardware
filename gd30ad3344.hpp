#pragma once

#include <stdint.h>

// GD30AD3344 16-bit SPI ADC, configuration and conversion words are big-endian.
namespace Hardware
{
template <typename spi_device>
class GD30AD3344
{
public:
	enum class Mux : uint8_t
	{
		In0In1, In0In3, In1In3, In2In3, In0Gnd, In1Gnd, In2Gnd, In3Gnd
	};
	enum class Pga : uint8_t { V6_144, V4_096, V2_048, V1_024, V0_512, V0_256, V0_064 };
	enum class WorkMode : uint8_t { Continuous, Single };
	enum class DataRate : uint8_t { Sps6_25, Sps12_5, Sps25, Sps50, Sps100, Sps250, Sps500, Sps1000 };
	enum class MisoPullup : uint8_t { Enabled, Disabled };

	void init(Mux mux = Mux::In1Gnd, Pga pga = Pga::V4_096,
			  WorkMode mode = WorkMode::Single, DataRate rate = DataRate::Sps100,
			  MisoPullup pullup = MisoPullup::Enabled)
	{
		spi_device::init();
		config_register_ = 0;
		set_mux(mux);
		set_pga(pga);
		set_work_mode(mode);
		set_data_rate(rate);
		set_miso_pullup(pullup);
		write_config_register();
	}

	void start_single_conversion()
	{
		config_register_ |= static_cast<uint16_t>(1U << 15U);
		write_config_register();
	}

	int16_t read_conversion_data() const
	{
		uint8_t data[2]{};
		spi_device::receive(data, 2, timeout);
		return static_cast<int16_t>((static_cast<uint16_t>(data[0]) << 8U) | data[1]);
	}

	void set_mux(Mux value) { set_field(12, 0x7U, static_cast<uint16_t>(value)); }
	void set_pga(Pga value) { set_field(9, 0x7U, static_cast<uint16_t>(value)); }
	void set_work_mode(WorkMode value) { set_field(8, 0x1U, static_cast<uint16_t>(value)); }
	void set_data_rate(DataRate value) { set_field(5, 0x7U, static_cast<uint16_t>(value)); }
	void set_miso_pullup(MisoPullup value) { set_field(3, 0x1U, value == MisoPullup::Enabled ? 1U : 0U); }

	uint16_t read_config_register()
	{
		uint8_t data[4]{};
		spi_device::receive(data, 4, timeout);
		config_register_ = static_cast<uint16_t>((static_cast<uint16_t>(data[2]) << 8U) | data[3]);
		return config_register_;
	}

	void write_config_register()
	{
		config_register_ |= static_cast<uint16_t>(1U << 1U);
		config_register_ &= static_cast<uint16_t>(~(1U << 2U));
		const uint8_t data[2]{static_cast<uint8_t>(config_register_ >> 8U), static_cast<uint8_t>(config_register_)};
		spi_device::transmit(data, 2, timeout);
	}

private:
	static constexpr uint32_t timeout = 1000;
	uint16_t config_register_ = 0;

	void set_field(uint8_t shift, uint16_t mask, uint16_t value)
	{
		config_register_ = static_cast<uint16_t>((config_register_ & ~(mask << shift)) | ((value & mask) << shift));
	}
};
}
