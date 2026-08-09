#pragma once

#include <stdint.h>
#include "bits_operation.hpp"
template <typename PWM, const int led_num>
class WS2812
{
	enum compare_value
	{
		code0 = 30,
		code1 = 60,
		reset = 0
	};
	uint16_t rgb_buf_[led_num * 24 + 1] = {0}; // todo:最后一位为reset
	uint16_t step_						 = 0;

public:
	void set_one(uint16_t pos, uint32_t color)
	{
		uint8_t
			R = color >> 16,
			G = color >> 8,
			B = color >> 0;
		for (int i = 0; i < 8; ++i)
		{
			rgb_buf_[pos * 24 + i + 0]	= BIT::READ(G, i) ? code1 : code0;
			rgb_buf_[pos * 24 + i + 8]	= BIT::READ(R, i) ? code1 : code0;
			rgb_buf_[pos * 24 + i + 16] = BIT::READ(B, i) ? code1 : code0;
		}
	}
	void set_multiple(uint16_t start, uint16_t stop, uint32_t color)
	{
		for (int i = start; i < stop; ++i)
		{
			set_one(i, color);
		}
	}
	void update()
	{
		step_ = 0;
	}
	void IRQ_Handler() // 放入800KHz的定时器中断中
	{
		if (step_ <= led_num * 24 + 1) // todo：尝试优化掉这个if
			PWM::set_compare(rgb_buf_[step_++]);
	}
};