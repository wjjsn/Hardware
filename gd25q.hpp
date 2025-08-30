#pragma once

#include <cstdint>
template <typename spi_device>
class GD25Q
{
	enum commands
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
		PAGE_SIZE  = 0x100
	};
	void write_enable()
	{
		const std::uint8_t write_enable = WRITE_ENABLE;
		spi_device::transmit(&write_enable, 1);
	}
	void wait_for_write_end()
	{
		uint8_t gd25q_status					= 0;
		const std::uint8_t read_statee_register = READ_STATE_REGISTER;
		spi_device::transmit(&read_statee_register, 1);
		do {
			spi_device::receive(&gd25q_status, 1);
		} while (1 == (gd25q_status & WIP_FLAG));
	}

public:
	std::uint32_t read_id()
	{
		std::uint8_t send_buf[4] = {READ_ID, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE},
					 rec_buf[4];
		spi_device::transmit_receive(send_buf, rec_buf, 4);
		return rec_buf[1] << 16 | rec_buf[2] << 8 | rec_buf[3] << 0;
	}
	void sector_erase(std::uint32_t sector_addr)
	{
		write_enable();
		std::uint8_t send_buf[4] = {ERASE_SECTOR, sector_addr >> 16, sector_addr >> 8, sector_addr >> 0};
		spi_device::transmit(send_buf, 4);
		wait_for_write_end();
	}
	void chip_erase()
	{
		write_enable();
		const std::uint8_t chip_erase = ERASE_BILK;
		spi_device::transmit(&chip_erase, 1);
		wait_for_write_end();
	}
	void page_write(std::uint8_t *pbuffer, std::uint32_t write_addr, std::uint16_t num_byte_to_write)
	{
		write_enable();
		std::uint8_t send_buf[4] = {WRITE_DATA, write_addr >> 16, write_addr >> 8, write_addr};
		spi_device::transmit(send_buf, 2);
		spi_device::transmit(pbuffer, num_byte_to_write);
		wait_for_write_end();
	}
	void block_write(std::uint8_t *pbuffer, std::uint32_t write_addr, std::uint16_t num_byte_to_write)
	{
		std::uint8_t num_of_page = 0, num_of_single = 0, addr = 0, count = 0, temp = 0;

		addr		  = write_addr % PAGE_SIZE;
		count		  = PAGE_SIZE - addr;
		num_of_page	  = num_byte_to_write / PAGE_SIZE;
		num_of_single = num_byte_to_write % PAGE_SIZE;

		/* write_addr is PAGE_SIZE aligned */
		if (0 == addr)
		{
			/* num_byte_to_write < PAGE_SIZE */
			if (0 == num_of_page)
			{
				page_write(pbuffer, write_addr, num_byte_to_write);
			}
			else
			{
				/* num_byte_to_write >= PAGE_SIZE */
				while (num_of_page--)
				{
					page_write(pbuffer, write_addr, PAGE_SIZE);
					write_addr += PAGE_SIZE;
					pbuffer += PAGE_SIZE;
				}
				page_write(pbuffer, write_addr, num_of_single);
			}
		}
		else
		{
			/* write_addr is not PAGE_SIZE aligned */
			if (0 == num_of_page)
			{
				/* (num_byte_to_write + write_addr) > PAGE_SIZE */
				if (num_of_single > count)
				{
					temp = num_of_single - count;
					page_write(pbuffer, write_addr, count);
					write_addr += count;
					pbuffer += count;
					page_write(pbuffer, write_addr, temp);
				}
				else
				{
					page_write(pbuffer, write_addr, num_byte_to_write);
				}
			}
			else
			{
				num_byte_to_write -= count;
				num_of_page	  = num_byte_to_write / PAGE_SIZE;
				num_of_single = num_byte_to_write % PAGE_SIZE;

				page_write(pbuffer, write_addr, count);
				write_addr += count;
				pbuffer += count;

				while (num_of_page--)
				{
					page_write(pbuffer, write_addr, PAGE_SIZE);
					write_addr += PAGE_SIZE;
					pbuffer += PAGE_SIZE;
				}
				if (0 != num_of_single)
				{
					page_write(pbuffer, write_addr, num_of_single);
				}
			}
		}
	}
	void gd25q_buffer_read(uint8_t *pbuffer, uint32_t read_addr, uint16_t num_byte_to_read)
	{
		std::uint8_t send_buf[4] = {WRITE_DATA, read_addr >> 16, read_addr >> 8, read_addr};
		spi_device::transmit(send_buf, 4); // todo:这个地方要改，这个片选不能打断
		spi_device::receive(pbuffer, num_byte_to_read);
	}
};