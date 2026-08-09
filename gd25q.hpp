#pragma once

#include <stdint.h>

/**
 * @brief  GD25Q SPI Flash 驱动 (静态类)
 * @note   模板参数 spi_device 是 HAL::gd32f4::SPI_device<BUS, CS>
 *         所有方法均为 static, 无需实例化
 */
namespace Hardware
{
template <typename spi_device>
struct GD25Q
{
	enum class Command : uint8_t
	{
		WRITE_DATA			 = 0x02,
		WRITE_STATE_REGISTER = 0x01,
		WRITE_ENABLE		 = 0x06,

		READ_DATA			= 0x03,
		READ_STATE_REGISTER = 0x05,
		READ_ID				= 0x9F,

		ERASE_SECTOR = 0x20,
		ERASE_BILK	 = 0xC7,
		// 0x05?
		WIP_FLAG   = 0x01,
		DUMMY_BYTE = 0xA5, // byte for generate clock
	};
	static constexpr uint16_t page_size = 0x100;
	static constexpr uint32_t timeout = 1000;
	static constexpr uint32_t status_poll_limit = 1000000;

private:
	static void write_enable()
	{
		const uint8_t command = static_cast<uint8_t>(Command::WRITE_ENABLE);
		spi_device::transmit(&command, 1, timeout);
	}
	static bool wait_for_write_end()
	{
		// 命令 (0x05) 和状态读循环必须在同一 CS 窗口内,
		// 否则 flash 不会回状态字节, 读到的是 stale 数据
		uint8_t gd25q_status = 0;
		const uint8_t read_status_register = static_cast<uint8_t>(Command::READ_STATE_REGISTER);
		spi_device::select();
		spi_device::transmit_without_ctl_select(&read_status_register, 1, timeout);
		for (uint32_t attempt = 0; attempt < status_poll_limit; ++attempt)
		{
			uint8_t dummy = static_cast<uint8_t>(Command::DUMMY_BYTE);
			spi_device::transfer_without_ctl_select(&dummy, 1, timeout);
			gd25q_status = dummy;
			if ((gd25q_status & static_cast<uint8_t>(Command::WIP_FLAG)) == 0U)
			{
				spi_device::deselect();
				return true;
			}
		}
		spi_device::deselect();
		return false;
	}

public:
	static void init() { spi_device::init(); }
	static uint32_t read_id()
	{
		// transfer() 一次拉低/拉高 CS，命令+地址+数据都在同一 CS 窗口内
		// SPI 全双工：同一缓冲区，发送值会被接收值覆盖
		uint8_t buf[4] = {static_cast<uint8_t>(Command::READ_ID), 0xFF, 0xFF, 0xFF};
		spi_device::transfer(buf, 4, timeout);
		return (static_cast<uint32_t>(buf[1]) << 16U) | (static_cast<uint32_t>(buf[2]) << 8U) | buf[3];
	}
	static bool erase_sector(uint32_t sector_addr)
	{
		write_enable();
		const uint8_t send_buf[4] = {static_cast<uint8_t>(Command::ERASE_SECTOR),
									static_cast<uint8_t>(sector_addr >> 16),
									static_cast<uint8_t>(sector_addr >> 8),
									static_cast<uint8_t>(sector_addr >> 0)};
		spi_device::transmit(send_buf, 4, timeout);
		return wait_for_write_end();
	}
	static bool erase_chip()
	{
		write_enable();
		const uint8_t command = static_cast<uint8_t>(Command::ERASE_BILK);
		spi_device::transmit(&command, 1, timeout);
		return wait_for_write_end();
	}
	static bool write_page(const uint8_t *data, uint32_t address, uint16_t size)
	{
		if (data == nullptr || size == 0U || size > page_size || (address % page_size) + size > page_size) return false;
		write_enable();
		const uint8_t send_buf[4] = {static_cast<uint8_t>(Command::WRITE_DATA),
			static_cast<uint8_t>(address >> 16U), static_cast<uint8_t>(address >> 8U), static_cast<uint8_t>(address)};
		// 命令+地址 和 数据 需要在同一个 CS 窗口内，因此手动控 CS
		spi_device::select();
		spi_device::transmit_without_ctl_select(send_buf, 4, timeout);
		spi_device::transmit_without_ctl_select(data, size, timeout);
		spi_device::deselect();
		return wait_for_write_end();
	}
	static bool write(const uint8_t *data, uint32_t address, uint16_t size)
	{
		if (data == nullptr && size != 0U) return false;
		while (size > 0U)
		{
			const uint16_t chunk = static_cast<uint16_t>(page_size - address % page_size < size
				? page_size - address % page_size : size);
			if (!write_page(data, address, chunk)) return false;
			data += chunk; address += chunk; size -= chunk;
		}
		return true;
	}
	static bool read(uint8_t *data, uint32_t address, uint16_t size)
	{
		if (data == nullptr && size != 0U) return false;
		// 命令+地址 和 数据 需要在同一个 CS 窗口内，因此手动控 CS
		const uint8_t send_buf[4] = {static_cast<uint8_t>(Command::READ_DATA),
			static_cast<uint8_t>(address >> 16U), static_cast<uint8_t>(address >> 8U), static_cast<uint8_t>(address)};
		spi_device::select();
		spi_device::transmit_without_ctl_select(send_buf, 4, timeout);
		spi_device::receive_without_ctl_select(data, size, timeout);
		spi_device::deselect();
		return true;
	}
};
}
